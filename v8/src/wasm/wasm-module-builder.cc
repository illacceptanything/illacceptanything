// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/wasm/wasm-module-builder.h"

#include "src/base/memory.h"
#include "src/codegen/signature.h"
#include "src/handles/handles.h"
#include "src/init/v8.h"
#include "src/objects/objects-inl.h"
#include "src/wasm/function-body-decoder.h"
#include "src/wasm/leb-helper.h"
#include "src/wasm/wasm-constants.h"
#include "src/wasm/wasm-module.h"
#include "src/zone/zone-containers.h"

namespace v8 {
namespace internal {
namespace wasm {

namespace {

// Emit a section code and the size as a padded varint that can be patched
// later.
size_t EmitSection(SectionCode code, ZoneBuffer* buffer) {
  // Emit the section code.
  buffer->write_u8(code);

  // Emit a placeholder for the length.
  return buffer->reserve_u32v();
}

// Patch the size of a section after it's finished.
void FixupSection(ZoneBuffer* buffer, size_t start) {
  buffer->patch_u32v(start, static_cast<uint32_t>(buffer->offset() - start -
                                                  kPaddedVarInt32Size));
}

}  // namespace

WasmFunctionBuilder::WasmFunctionBuilder(WasmModuleBuilder* builder)
    : builder_(builder),
      locals_(builder->zone()),
      signature_index_(0),
      func_index_(static_cast<uint32_t>(builder->functions_.size())),
      body_(builder->zone(), 256),
      i32_temps_(builder->zone()),
      i64_temps_(builder->zone()),
      f32_temps_(builder->zone()),
      f64_temps_(builder->zone()),
      direct_calls_(builder->zone()),
      asm_offsets_(builder->zone(), 8) {}

void WasmFunctionBuilder::EmitByte(byte val) { body_.write_u8(val); }

void WasmFunctionBuilder::EmitI32V(int32_t val) { body_.write_i32v(val); }

void WasmFunctionBuilder::EmitU32V(uint32_t val) { body_.write_u32v(val); }

void WasmFunctionBuilder::SetSignature(FunctionSig* sig) {
  DCHECK(!locals_.has_sig());
  locals_.set_sig(sig);
  signature_index_ = builder_->AddSignature(sig);
}

uint32_t WasmFunctionBuilder::AddLocal(ValueType type) {
  DCHECK(locals_.has_sig());
  return locals_.AddLocals(1, type);
}

void WasmFunctionBuilder::EmitGetLocal(uint32_t local_index) {
  EmitWithU32V(kExprLocalGet, local_index);
}

void WasmFunctionBuilder::EmitSetLocal(uint32_t local_index) {
  EmitWithU32V(kExprLocalSet, local_index);
}

void WasmFunctionBuilder::EmitTeeLocal(uint32_t local_index) {
  EmitWithU32V(kExprLocalTee, local_index);
}

void WasmFunctionBuilder::EmitCode(const byte* code, uint32_t code_size) {
  body_.write(code, code_size);
}

void WasmFunctionBuilder::Emit(WasmOpcode opcode) { body_.write_u8(opcode); }

void WasmFunctionBuilder::EmitWithPrefix(WasmOpcode opcode) {
  DCHECK_NE(0, opcode & 0xff00);
  body_.write_u8(opcode >> 8);
  if ((opcode >> 8) == WasmOpcode::kSimdPrefix) {
    // SIMD opcodes are LEB encoded
    body_.write_u32v(opcode & 0xff);
  } else {
    body_.write_u8(opcode);
  }
}

void WasmFunctionBuilder::EmitWithU8(WasmOpcode opcode, const byte immediate) {
  body_.write_u8(opcode);
  body_.write_u8(immediate);
}

void WasmFunctionBuilder::EmitWithU8U8(WasmOpcode opcode, const byte imm1,
                                       const byte imm2) {
  body_.write_u8(opcode);
  body_.write_u8(imm1);
  body_.write_u8(imm2);
}

void WasmFunctionBuilder::EmitWithI32V(WasmOpcode opcode, int32_t immediate) {
  body_.write_u8(opcode);
  body_.write_i32v(immediate);
}

void WasmFunctionBuilder::EmitWithU32V(WasmOpcode opcode, uint32_t immediate) {
  body_.write_u8(opcode);
  body_.write_u32v(immediate);
}

void WasmFunctionBuilder::EmitI32Const(int32_t value) {
  EmitWithI32V(kExprI32Const, value);
}

void WasmFunctionBuilder::EmitI64Const(int64_t value) {
  body_.write_u8(kExprI64Const);
  body_.write_i64v(value);
}

void WasmFunctionBuilder::EmitF32Const(float value) {
  body_.write_u8(kExprF32Const);
  body_.write_f32(value);
}

void WasmFunctionBuilder::EmitF64Const(double value) {
  body_.write_u8(kExprF64Const);
  body_.write_f64(value);
}

void WasmFunctionBuilder::EmitDirectCallIndex(uint32_t index) {
  DirectCallIndex call;
  call.offset = body_.size();
  call.direct_index = index;
  direct_calls_.push_back(call);
  byte placeholder_bytes[kMaxVarInt32Size] = {0};
  EmitCode(placeholder_bytes, arraysize(placeholder_bytes));
}

void WasmFunctionBuilder::SetName(Vector<const char> name) { name_ = name; }

void WasmFunctionBuilder::AddAsmWasmOffset(size_t call_position,
                                           size_t to_number_position) {
  // We only want to emit one mapping per byte offset.
  DCHECK(asm_offsets_.size() == 0 || body_.size() > last_asm_byte_offset_);

  DCHECK_LE(body_.size(), kMaxUInt32);
  uint32_t byte_offset = static_cast<uint32_t>(body_.size());
  asm_offsets_.write_u32v(byte_offset - last_asm_byte_offset_);
  last_asm_byte_offset_ = byte_offset;

  DCHECK_GE(std::numeric_limits<uint32_t>::max(), call_position);
  uint32_t call_position_u32 = static_cast<uint32_t>(call_position);
  asm_offsets_.write_i32v(call_position_u32 - last_asm_source_position_);

  DCHECK_GE(std::numeric_limits<uint32_t>::max(), to_number_position);
  uint32_t to_number_position_u32 = static_cast<uint32_t>(to_number_position);
  asm_offsets_.write_i32v(to_number_position_u32 - call_position_u32);
  last_asm_source_position_ = to_number_position_u32;
}

void WasmFunctionBuilder::SetAsmFunctionStartPosition(
    size_t function_position) {
  DCHECK_EQ(0, asm_func_start_source_position_);
  DCHECK_GE(std::numeric_limits<uint32_t>::max(), function_position);
  uint32_t function_position_u32 = static_cast<uint32_t>(function_position);
  // Must be called before emitting any asm.js source position.
  DCHECK_EQ(0, asm_offsets_.size());
  asm_func_start_source_position_ = function_position_u32;
  last_asm_source_position_ = function_position_u32;
}

void WasmFunctionBuilder::SetCompilationHint(
    WasmCompilationHintStrategy strategy, WasmCompilationHintTier baseline,
    WasmCompilationHintTier top_tier) {
  uint8_t hint_byte = static_cast<uint8_t>(strategy) |
                      static_cast<uint8_t>(baseline) << 2 |
                      static_cast<uint8_t>(top_tier) << 4;
  DCHECK_NE(hint_byte, kNoCompilationHint);
  hint_ = hint_byte;
}

void WasmFunctionBuilder::DeleteCodeAfter(size_t position) {
  DCHECK_LE(position, body_.size());
  body_.Truncate(position);
}

void WasmFunctionBuilder::WriteSignature(ZoneBuffer* buffer) const {
  buffer->write_u32v(signature_index_);
}

void WasmFunctionBuilder::WriteBody(ZoneBuffer* buffer) const {
  size_t locals_size = locals_.Size();
  buffer->write_size(locals_size + body_.size());
  buffer->EnsureSpace(locals_size);
  byte** ptr = buffer->pos_ptr();
  locals_.Emit(*ptr);
  (*ptr) += locals_size;  // UGLY: manual bump of position pointer
  if (body_.size() > 0) {
    size_t base = buffer->offset();
    buffer->write(body_.begin(), body_.size());
    for (DirectCallIndex call : direct_calls_) {
      buffer->patch_u32v(
          base + call.offset,
          call.direct_index +
              static_cast<uint32_t>(builder_->function_imports_.size()));
    }
  }
}

void WasmFunctionBuilder::WriteAsmWasmOffsetTable(ZoneBuffer* buffer) const {
  if (asm_func_start_source_position_ == 0 && asm_offsets_.size() == 0) {
    buffer->write_size(0);
    return;
  }
  size_t locals_enc_size = LEBHelper::sizeof_u32v(locals_.Size());
  size_t func_start_size =
      LEBHelper::sizeof_u32v(asm_func_start_source_position_);
  buffer->write_size(asm_offsets_.size() + locals_enc_size + func_start_size);
  // Offset of the recorded byte offsets.
  DCHECK_GE(kMaxUInt32, locals_.Size());
  buffer->write_u32v(static_cast<uint32_t>(locals_.Size()));
  // Start position of the function.
  buffer->write_u32v(asm_func_start_source_position_);
  buffer->write(asm_offsets_.begin(), asm_offsets_.size());
}

WasmModuleBuilder::WasmModuleBuilder(Zone* zone)
    : zone_(zone),
      types_(zone),
      function_imports_(zone),
      global_imports_(zone),
      exports_(zone),
      functions_(zone),
      tables_(zone),
      data_segments_(zone),
      indirect_functions_(zone),
      globals_(zone),
      exceptions_(zone),
      signature_map_(zone),
      start_function_index_(-1),
      min_memory_size_(16),
      max_memory_size_(0),
      has_max_memory_size_(false),
      has_shared_memory_(false) {}

WasmFunctionBuilder* WasmModuleBuilder::AddFunction(FunctionSig* sig) {
  functions_.push_back(zone_->New<WasmFunctionBuilder>(this));
  // Add the signature if one was provided here.
  if (sig) functions_.back()->SetSignature(sig);
  return functions_.back();
}

void WasmModuleBuilder::AddDataSegment(const byte* data, uint32_t size,
                                       uint32_t dest) {
  data_segments_.push_back({ZoneVector<byte>(zone()), dest});
  ZoneVector<byte>& vec = data_segments_.back().data;
  for (uint32_t i = 0; i < size; i++) {
    vec.push_back(data[i]);
  }
}

uint32_t WasmModuleBuilder::AddSignature(FunctionSig* sig) {
  auto sig_entry = signature_map_.find(*sig);
  if (sig_entry != signature_map_.end()) return sig_entry->second;
  uint32_t index = static_cast<uint32_t>(types_.size());
  signature_map_.emplace(*sig, index);
  types_.push_back(Type(sig));
  return index;
}

uint32_t WasmModuleBuilder::AddException(FunctionSig* type) {
  DCHECK_EQ(0, type->return_count());
  int type_index = AddSignature(type);
  uint32_t except_index = static_cast<uint32_t>(exceptions_.size());
  exceptions_.push_back(type_index);
  return except_index;
}

uint32_t WasmModuleBuilder::AddStructType(StructType* type) {
  uint32_t index = static_cast<uint32_t>(types_.size());
  types_.push_back(Type(type));
  return index;
}

uint32_t WasmModuleBuilder::AddArrayType(ArrayType* type) {
  uint32_t index = static_cast<uint32_t>(types_.size());
  types_.push_back(Type(type));
  return index;
}

// static
const uint32_t WasmModuleBuilder::kNullIndex =
    std::numeric_limits<uint32_t>::max();

// TODO(9495): Add support for typed function tables and more init. expressions.
uint32_t WasmModuleBuilder::AllocateIndirectFunctions(uint32_t count) {
  DCHECK(allocating_indirect_functions_allowed_);
  uint32_t index = static_cast<uint32_t>(indirect_functions_.size());
  DCHECK_GE(FLAG_wasm_max_table_size, index);
  if (count > FLAG_wasm_max_table_size - index) {
    return std::numeric_limits<uint32_t>::max();
  }
  uint32_t new_size = static_cast<uint32_t>(indirect_functions_.size()) + count;
  DCHECK(max_table_size_ == 0 || new_size <= max_table_size_);
  indirect_functions_.resize(new_size, kNullIndex);
  uint32_t max = max_table_size_ > 0 ? max_table_size_ : new_size;
  if (tables_.empty()) {
    // This cannot use {AddTable} because that would flip the
    // {allocating_indirect_functions_allowed_} flag.
    tables_.push_back({kWasmFuncRef, new_size, max, true, {}});
  } else {
    // There can only be the indirect function table so far, otherwise the
    // {allocating_indirect_functions_allowed_} flag would have been false.
    DCHECK_EQ(1u, tables_.size());
    DCHECK_EQ(kWasmFuncRef, tables_[0].type);
    DCHECK(tables_[0].has_maximum);
    tables_[0].min_size = new_size;
    tables_[0].max_size = max;
  }
  return index;
}

void WasmModuleBuilder::SetIndirectFunction(uint32_t indirect,
                                            uint32_t direct) {
  indirect_functions_[indirect] = direct;
}

void WasmModuleBuilder::SetMaxTableSize(uint32_t max) {
  DCHECK_GE(FLAG_wasm_max_table_size, max);
  DCHECK_GE(max, indirect_functions_.size());
  max_table_size_ = max;
  DCHECK(allocating_indirect_functions_allowed_);
  if (!tables_.empty()) {
    tables_[0].max_size = max;
  }
}

uint32_t WasmModuleBuilder::AddTable(ValueType type, uint32_t min_size) {
#if DEBUG
  allocating_indirect_functions_allowed_ = false;
#endif
  tables_.push_back({type, min_size, 0, false, {}});
  return static_cast<uint32_t>(tables_.size() - 1);
}

uint32_t WasmModuleBuilder::AddTable(ValueType type, uint32_t min_size,
                                     uint32_t max_size) {
#if DEBUG
  allocating_indirect_functions_allowed_ = false;
#endif
  tables_.push_back({type, min_size, max_size, true, {}});
  return static_cast<uint32_t>(tables_.size() - 1);
}

uint32_t WasmModuleBuilder::AddTable(ValueType type, uint32_t min_size,
                                     uint32_t max_size, WasmInitExpr init) {
#if DEBUG
  allocating_indirect_functions_allowed_ = false;
#endif
  tables_.push_back({type, min_size, max_size, true, std::move(init)});
  return static_cast<uint32_t>(tables_.size() - 1);
}

uint32_t WasmModuleBuilder::AddImport(Vector<const char> name, FunctionSig* sig,
                                      Vector<const char> module) {
  DCHECK(adding_imports_allowed_);
  function_imports_.push_back({module, name, AddSignature(sig)});
  return static_cast<uint32_t>(function_imports_.size() - 1);
}

uint32_t WasmModuleBuilder::AddGlobalImport(Vector<const char> name,
                                            ValueType type, bool mutability,
                                            Vector<const char> module) {
  global_imports_.push_back({module, name, type.value_type_code(), mutability});
  return static_cast<uint32_t>(global_imports_.size() - 1);
}

void WasmModuleBuilder::MarkStartFunction(WasmFunctionBuilder* function) {
  start_function_index_ = function->func_index();
}

void WasmModuleBuilder::AddExport(Vector<const char> name,
                                  ImportExportKindCode kind, uint32_t index) {
  DCHECK_LE(index, std::numeric_limits<int>::max());
  exports_.push_back({name, kind, static_cast<int>(index)});
}

uint32_t WasmModuleBuilder::AddExportedGlobal(ValueType type, bool mutability,
                                              WasmInitExpr init,
                                              Vector<const char> name) {
  uint32_t index = AddGlobal(type, mutability, std::move(init));
  AddExport(name, kExternalGlobal, index);
  return index;
}

void WasmModuleBuilder::ExportImportedFunction(Vector<const char> name,
                                               int import_index) {
#if DEBUG
  // The size of function_imports_ must not change any more.
  adding_imports_allowed_ = false;
#endif
  exports_.push_back(
      {name, kExternalFunction,
       import_index - static_cast<int>(function_imports_.size())});
}

uint32_t WasmModuleBuilder::AddGlobal(ValueType type, bool mutability,
                                      WasmInitExpr init) {
  globals_.push_back({type, mutability, std::move(init)});
  return static_cast<uint32_t>(globals_.size() - 1);
}

void WasmModuleBuilder::SetMinMemorySize(uint32_t value) {
  min_memory_size_ = value;
}

void WasmModuleBuilder::SetMaxMemorySize(uint32_t value) {
  has_max_memory_size_ = true;
  max_memory_size_ = value;
}

void WasmModuleBuilder::SetHasSharedMemory() { has_shared_memory_ = true; }

namespace {
void WriteValueType(ZoneBuffer* buffer, const ValueType& type) {
  buffer->write_u8(type.value_type_code());
  if (type.encoding_needs_heap_type()) {
    buffer->write_i32v(type.heap_type().code());
  }
  if (type.is_rtt()) {
    if (type.has_depth()) buffer->write_u32v(type.depth());
    buffer->write_u32v(type.ref_index());
  }
}

void WriteInitializerExpression(ZoneBuffer* buffer, const WasmInitExpr& init,
                                ValueType type) {
  switch (init.kind()) {
    case WasmInitExpr::kI32Const:
      buffer->write_u8(kExprI32Const);
      buffer->write_i32v(init.immediate().i32_const);
      break;
    case WasmInitExpr::kI64Const:
      buffer->write_u8(kExprI64Const);
      buffer->write_i64v(init.immediate().i64_const);
      break;
    case WasmInitExpr::kF32Const:
      buffer->write_u8(kExprF32Const);
      buffer->write_f32(init.immediate().f32_const);
      break;
    case WasmInitExpr::kF64Const:
      buffer->write_u8(kExprF64Const);
      buffer->write_f64(init.immediate().f64_const);
      break;
    case WasmInitExpr::kS128Const:
      buffer->write_u8(kSimdPrefix);
      buffer->write_u8(kExprS128Const & 0xFF);
      buffer->write(init.immediate().s128_const.data(), kSimd128Size);
      break;
    case WasmInitExpr::kGlobalGet:
      buffer->write_u8(kExprGlobalGet);
      buffer->write_u32v(init.immediate().index);
      break;
    case WasmInitExpr::kRefNullConst:
      buffer->write_u8(kExprRefNull);
      buffer->write_i32v(HeapType(init.immediate().heap_type).code());
      break;
    case WasmInitExpr::kRefFuncConst:
      buffer->write_u8(kExprRefFunc);
      buffer->write_u32v(init.immediate().index);
      break;
    case WasmInitExpr::kNone: {
      // No initializer, emit a default value.
      switch (type.kind()) {
        case kI32:
          buffer->write_u8(kExprI32Const);
          // LEB encoding of 0.
          buffer->write_u8(0);
          break;
        case kI64:
          buffer->write_u8(kExprI64Const);
          // LEB encoding of 0.
          buffer->write_u8(0);
          break;
        case kF32:
          buffer->write_u8(kExprF32Const);
          buffer->write_f32(0.f);
          break;
        case kF64:
          buffer->write_u8(kExprF64Const);
          buffer->write_f64(0.);
          break;
        case kOptRef:
          buffer->write_u8(kExprRefNull);
          break;
        case kI8:
        case kI16:
        case kVoid:
        case kS128:
        case kBottom:
        case kRef:
        case kRtt:
        case kRttWithDepth:
          UNREACHABLE();
      }
      break;
    }
    case WasmInitExpr::kRttCanon:
      STATIC_ASSERT((kExprRttCanon >> 8) == kGCPrefix);
      buffer->write_u8(kGCPrefix);
      buffer->write_u8(static_cast<uint8_t>(kExprRttCanon));
      buffer->write_i32v(static_cast<int32_t>(init.immediate().index));
      break;
    case WasmInitExpr::kRttSub:
      // The operand to rtt.sub must be emitted first.
      WriteInitializerExpression(buffer, *init.operand(), kWasmBottom);
      STATIC_ASSERT((kExprRttSub >> 8) == kGCPrefix);
      buffer->write_u8(kGCPrefix);
      buffer->write_u8(static_cast<uint8_t>(kExprRttSub));
      buffer->write_i32v(static_cast<int32_t>(init.immediate().index));
      break;
  }
}

}  // namespace

void WasmModuleBuilder::WriteTo(ZoneBuffer* buffer) const {
  // == Emit magic =============================================================
  buffer->write_u32(kWasmMagic);
  buffer->write_u32(kWasmVersion);

  // == Emit types =============================================================
  if (types_.size() > 0) {
    size_t start = EmitSection(kTypeSectionCode, buffer);
    buffer->write_size(types_.size());

    for (const Type& type : types_) {
      switch (type.kind) {
        case Type::kFunctionSig: {
          FunctionSig* sig = type.sig;
          buffer->write_u8(kWasmFunctionTypeCode);
          buffer->write_size(sig->parameter_count());
          for (auto param : sig->parameters()) {
            WriteValueType(buffer, param);
          }
          buffer->write_size(sig->return_count());
          for (auto ret : sig->returns()) {
            WriteValueType(buffer, ret);
          }
          break;
        }
        case Type::kStructType: {
          StructType* struct_type = type.struct_type;
          buffer->write_u8(kWasmStructTypeCode);
          buffer->write_size(struct_type->field_count());
          for (uint32_t i = 0; i < struct_type->field_count(); i++) {
            WriteValueType(buffer, struct_type->field(i));
            buffer->write_u8(struct_type->mutability(i) ? 1 : 0);
          }
          break;
        }
        case Type::kArrayType: {
          ArrayType* array_type = type.array_type;
          buffer->write_u8(kWasmArrayTypeCode);
          WriteValueType(buffer, array_type->element_type());
          buffer->write_u8(array_type->mutability() ? 1 : 0);
          break;
        }
      }
    }
    FixupSection(buffer, start);
  }

  // == Emit imports ===========================================================
  if (global_imports_.size() + function_imports_.size() > 0) {
    size_t start = EmitSection(kImportSectionCode, buffer);
    buffer->write_size(global_imports_.size() + function_imports_.size());
    for (auto import : global_imports_) {
      buffer->write_string(import.module);  // module name
      buffer->write_string(import.name);    // field name
      buffer->write_u8(kExternalGlobal);
      buffer->write_u8(import.type_code);
      buffer->write_u8(import.mutability ? 1 : 0);
    }
    for (auto import : function_imports_) {
      buffer->write_string(import.module);  // module name
      buffer->write_string(import.name);    // field name
      buffer->write_u8(kExternalFunction);
      buffer->write_u32v(import.sig_index);
    }
    FixupSection(buffer, start);
  }

  // == Emit function signatures ===============================================
  uint32_t num_function_names = 0;
  if (functions_.size() > 0) {
    size_t start = EmitSection(kFunctionSectionCode, buffer);
    buffer->write_size(functions_.size());
    for (auto* function : functions_) {
      function->WriteSignature(buffer);
      if (!function->name_.empty()) ++num_function_names;
    }
    FixupSection(buffer, start);
  }

  // == Emit tables ============================================================
  if (tables_.size() > 0) {
    size_t start = EmitSection(kTableSectionCode, buffer);
    buffer->write_size(tables_.size());
    for (const WasmTable& table : tables_) {
      WriteValueType(buffer, table.type);
      buffer->write_u8(table.has_maximum ? kWithMaximum : kNoMaximum);
      buffer->write_size(table.min_size);
      if (table.has_maximum) buffer->write_size(table.max_size);
      if (table.init.kind() != WasmInitExpr::kNone) {
        WriteInitializerExpression(buffer, table.init, table.type);
      }
    }
    FixupSection(buffer, start);
  }

  // == Emit memory declaration ================================================
  {
    size_t start = EmitSection(kMemorySectionCode, buffer);
    buffer->write_u8(1);  // memory count
    if (has_shared_memory_) {
      buffer->write_u8(has_max_memory_size_ ? kSharedWithMaximum
                                            : kSharedNoMaximum);
    } else {
      buffer->write_u8(has_max_memory_size_ ? kWithMaximum : kNoMaximum);
    }
    buffer->write_u32v(min_memory_size_);
    if (has_max_memory_size_) {
      buffer->write_u32v(max_memory_size_);
    }
    FixupSection(buffer, start);
  }

  // Emit event section.
  if (exceptions_.size() > 0) {
    size_t start = EmitSection(kExceptionSectionCode, buffer);
    buffer->write_size(exceptions_.size());
    for (int type : exceptions_) {
      buffer->write_u32v(kExceptionAttribute);
      buffer->write_u32v(type);
    }
    FixupSection(buffer, start);
  }

  // == Emit globals ===========================================================
  if (globals_.size() > 0) {
    size_t start = EmitSection(kGlobalSectionCode, buffer);
    buffer->write_size(globals_.size());

    for (const WasmGlobal& global : globals_) {
      WriteValueType(buffer, global.type);
      buffer->write_u8(global.mutability ? 1 : 0);
      WriteInitializerExpression(buffer, global.init, global.type);
      buffer->write_u8(kExprEnd);
    }
    FixupSection(buffer, start);
  }

  // == emit exports ===========================================================
  if (exports_.size() > 0) {
    size_t start = EmitSection(kExportSectionCode, buffer);
    buffer->write_size(exports_.size());
    for (auto ex : exports_) {
      buffer->write_string(ex.name);
      buffer->write_u8(ex.kind);
      switch (ex.kind) {
        case kExternalFunction:
          buffer->write_size(ex.index + function_imports_.size());
          break;
        case kExternalGlobal:
          buffer->write_size(ex.index + global_imports_.size());
          break;
        case kExternalMemory:
        case kExternalTable:
          // The WasmModuleBuilder doesn't support importing tables or memories
          // yet, so there is no index offset to add.
          buffer->write_size(ex.index);
          break;
        case kExternalException:
          UNREACHABLE();
      }
    }
    FixupSection(buffer, start);
  }

  // == emit start function index ==============================================
  if (start_function_index_ >= 0) {
    size_t start = EmitSection(kStartSectionCode, buffer);
    buffer->write_size(start_function_index_ + function_imports_.size());
    FixupSection(buffer, start);
  }

  // == emit function table elements ===========================================
  if (indirect_functions_.size() > 0) {
    size_t start = EmitSection(kElementSectionCode, buffer);
    buffer->write_u8(1);              // count of entries
    buffer->write_u8(0);              // table index
    uint32_t first_element = 0;
    while (first_element < indirect_functions_.size() &&
           indirect_functions_[first_element] == kNullIndex) {
      first_element++;
    }
    uint32_t last_element =
        static_cast<uint32_t>(indirect_functions_.size() - 1);
    while (last_element >= first_element &&
           indirect_functions_[last_element] == kNullIndex) {
      last_element--;
    }
    buffer->write_u8(kExprI32Const);  // offset
    buffer->write_u32v(first_element);
    buffer->write_u8(kExprEnd);
    uint32_t element_count = last_element - first_element + 1;
    buffer->write_size(element_count);
    for (uint32_t i = first_element; i <= last_element; i++) {
      buffer->write_size(indirect_functions_[i] + function_imports_.size());
    }

    FixupSection(buffer, start);
  }

  // == emit compilation hints section =========================================
  bool emit_compilation_hints = false;
  for (auto* fn : functions_) {
    if (fn->hint_ != kNoCompilationHint) {
      emit_compilation_hints = true;
      break;
    }
  }
  if (emit_compilation_hints) {
    // Emit the section code.
    buffer->write_u8(kUnknownSectionCode);
    // Emit a placeholder for section length.
    size_t start = buffer->reserve_u32v();
    // Emit custom section name.
    buffer->write_string(CStrVector("compilationHints"));
    // Emit hint count.
    buffer->write_size(functions_.size());
    // Emit hint bytes.
    for (auto* fn : functions_) {
      uint8_t hint_byte =
          fn->hint_ != kNoCompilationHint ? fn->hint_ : kDefaultCompilationHint;
      buffer->write_u8(hint_byte);
    }
    FixupSection(buffer, start);
  }

  // == emit code ==============================================================
  if (functions_.size() > 0) {
    size_t start = EmitSection(kCodeSectionCode, buffer);
    buffer->write_size(functions_.size());
    for (auto* function : functions_) {
      function->WriteBody(buffer);
    }
    FixupSection(buffer, start);
  }

  // == emit data segments =====================================================
  if (data_segments_.size() > 0) {
    size_t start = EmitSection(kDataSectionCode, buffer);
    buffer->write_size(data_segments_.size());

    for (auto segment : data_segments_) {
      buffer->write_u8(0);              // linear memory segment
      buffer->write_u8(kExprI32Const);  // initializer expression for dest
      buffer->write_u32v(segment.dest);
      buffer->write_u8(kExprEnd);
      buffer->write_u32v(static_cast<uint32_t>(segment.data.size()));
      buffer->write(&segment.data[0], segment.data.size());
    }
    FixupSection(buffer, start);
  }

  // == Emit names =============================================================
  if (num_function_names > 0 || !function_imports_.empty()) {
    // Emit the section code.
    buffer->write_u8(kUnknownSectionCode);
    // Emit a placeholder for the length.
    size_t start = buffer->reserve_u32v();
    // Emit the section string.
    buffer->write_string(CStrVector("name"));
    // Emit a subsection for the function names.
    buffer->write_u8(NameSectionKindCode::kFunction);
    // Emit a placeholder for the subsection length.
    size_t functions_start = buffer->reserve_u32v();
    // Emit the function names.
    // Imports are always named.
    uint32_t num_imports = static_cast<uint32_t>(function_imports_.size());
    buffer->write_size(num_imports + num_function_names);
    uint32_t function_index = 0;
    for (; function_index < num_imports; ++function_index) {
      const WasmFunctionImport* import = &function_imports_[function_index];
      DCHECK(!import->name.empty());
      buffer->write_u32v(function_index);
      buffer->write_string(import->name);
    }
    if (num_function_names > 0) {
      for (auto* function : functions_) {
        DCHECK_EQ(function_index,
                  function->func_index() + function_imports_.size());
        if (!function->name_.empty()) {
          buffer->write_u32v(function_index);
          buffer->write_string(function->name_);
        }
        ++function_index;
      }
    }
    FixupSection(buffer, functions_start);
    FixupSection(buffer, start);
  }
}

void WasmModuleBuilder::WriteAsmJsOffsetTable(ZoneBuffer* buffer) const {
  // == Emit asm.js offset table ===============================================
  buffer->write_size(functions_.size());
  // Emit the offset table per function.
  for (auto* function : functions_) {
    function->WriteAsmWasmOffsetTable(buffer);
  }
}
}  // namespace wasm
}  // namespace internal
}  // namespace v8
