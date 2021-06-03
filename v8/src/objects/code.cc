// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/objects/code.h"

#include <iomanip>

#include "src/codegen/assembler-inl.h"
#include "src/codegen/cpu-features.h"
#include "src/codegen/reloc-info.h"
#include "src/codegen/safepoint-table.h"
#include "src/codegen/source-position.h"
#include "src/deoptimizer/deoptimizer.h"
#include "src/execution/isolate-utils.h"
#include "src/interpreter/bytecode-array-iterator.h"
#include "src/interpreter/bytecode-decoder.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/allocation-site-inl.h"
#include "src/objects/code-kind.h"
#include "src/roots/roots-inl.h"
#include "src/snapshot/embedded/embedded-data.h"
#include "src/utils/ostreams.h"

#ifdef ENABLE_DISASSEMBLER
#include "src/codegen/code-comments.h"
#include "src/diagnostics/disasm.h"
#include "src/diagnostics/disassembler.h"
#include "src/diagnostics/eh-frame.h"
#endif

namespace v8 {
namespace internal {

Address Code::SafepointTableAddress() const {
  return MetadataStart() + safepoint_table_offset();
}

int Code::safepoint_table_size() const {
  DCHECK_GE(handler_table_offset() - safepoint_table_offset(), 0);
  return handler_table_offset() - safepoint_table_offset();
}

bool Code::has_safepoint_table() const { return safepoint_table_size() > 0; }

Address Code::HandlerTableAddress() const {
  return MetadataStart() + handler_table_offset();
}

int Code::handler_table_size() const {
  DCHECK_GE(constant_pool_offset() - handler_table_offset(), 0);
  return constant_pool_offset() - handler_table_offset();
}

bool Code::has_handler_table() const { return handler_table_size() > 0; }

int Code::constant_pool_size() const {
  const int size = code_comments_offset() - constant_pool_offset();
  DCHECK_IMPLIES(!FLAG_enable_embedded_constant_pool, size == 0);
  DCHECK_GE(size, 0);
  return size;
}

bool Code::has_constant_pool() const { return constant_pool_size() > 0; }

int Code::code_comments_size() const {
  DCHECK_GE(unwinding_info_offset() - code_comments_offset(), 0);
  return unwinding_info_offset() - code_comments_offset();
}

bool Code::has_code_comments() const { return code_comments_size() > 0; }

void Code::ClearEmbeddedObjects(Heap* heap) {
  HeapObject undefined = ReadOnlyRoots(heap).undefined_value();
  int mode_mask = RelocInfo::EmbeddedObjectModeMask();
  for (RelocIterator it(*this, mode_mask); !it.done(); it.next()) {
    DCHECK(RelocInfo::IsEmbeddedObjectMode(it.rinfo()->rmode()));
    it.rinfo()->set_target_object(heap, undefined, SKIP_WRITE_BARRIER);
  }
  set_embedded_objects_cleared(true);
}

void Code::Relocate(intptr_t delta) {
  for (RelocIterator it(*this, RelocInfo::kApplyMask); !it.done(); it.next()) {
    it.rinfo()->apply(delta);
  }
  FlushICache();
}

void Code::FlushICache() const {
  FlushInstructionCache(raw_instruction_start(), raw_instruction_size());
}

void Code::CopyFromNoFlush(Heap* heap, const CodeDesc& desc) {
  // Copy code.
  STATIC_ASSERT(kOnHeapBodyIsContiguous);
  CopyBytes(reinterpret_cast<byte*>(raw_instruction_start()), desc.buffer,
            static_cast<size_t>(desc.instr_size));
  // TODO(jgruber,v8:11036): Merge with the above.
  CopyBytes(reinterpret_cast<byte*>(raw_instruction_start() + desc.instr_size),
            desc.unwinding_info, static_cast<size_t>(desc.unwinding_info_size));

  // Copy reloc info.
  CopyRelocInfoToByteArray(unchecked_relocation_info(), desc);

  // Unbox handles and relocate.
  Assembler* origin = desc.origin;
  const int mode_mask = RelocInfo::PostCodegenRelocationMask();
  for (RelocIterator it(*this, mode_mask); !it.done(); it.next()) {
    RelocInfo::Mode mode = it.rinfo()->rmode();
    if (RelocInfo::IsEmbeddedObjectMode(mode)) {
      Handle<HeapObject> p = it.rinfo()->target_object_handle(origin);
      it.rinfo()->set_target_object(heap, *p, UPDATE_WRITE_BARRIER,
                                    SKIP_ICACHE_FLUSH);
    } else if (RelocInfo::IsCodeTargetMode(mode)) {
      // Rewrite code handles to direct pointers to the first instruction in the
      // code object.
      Handle<Object> p = it.rinfo()->target_object_handle(origin);
      Code code = Code::cast(*p);
      it.rinfo()->set_target_address(code.raw_instruction_start(),
                                     UPDATE_WRITE_BARRIER, SKIP_ICACHE_FLUSH);
    } else if (RelocInfo::IsRuntimeEntry(mode)) {
      Address p = it.rinfo()->target_runtime_entry(origin);
      it.rinfo()->set_target_runtime_entry(p, UPDATE_WRITE_BARRIER,
                                           SKIP_ICACHE_FLUSH);
    } else {
      intptr_t delta =
          raw_instruction_start() - reinterpret_cast<Address>(desc.buffer);
      it.rinfo()->apply(delta);
    }
  }
}

SafepointEntry Code::GetSafepointEntry(Isolate* isolate, Address pc) {
  SafepointTable table(isolate, pc, *this);
  return table.FindEntry(pc);
}

int Code::OffHeapInstructionSize() const {
  DCHECK(is_off_heap_trampoline());
  if (Isolate::CurrentEmbeddedBlobCode() == nullptr) {
    return raw_instruction_size();
  }
  EmbeddedData d = EmbeddedData::FromBlob();
  return d.InstructionSizeOfBuiltin(builtin_index());
}

namespace {

// Helper function for getting an EmbeddedData that can handle un-embedded
// builtins when short builtin calls are enabled.
inline EmbeddedData EmbeddedDataWithMaybeRemappedEmbeddedBuiltins(Code code) {
#if defined(V8_COMPRESS_POINTERS_IN_ISOLATE_CAGE)
  // GetIsolateFromWritableObject(*this) works for both read-only and writable
  // objects when pointer compression is enabled with a per-Isolate cage.
  return EmbeddedData::FromBlob(GetIsolateFromWritableObject(code));
#elif defined(V8_COMPRESS_POINTERS_IN_SHARED_CAGE)
  // When pointer compression is enabled with a shared cage, there is also a
  // shared CodeRange. When short builtin calls are enabled, there is a single
  // copy of the re-embedded builtins in the shared CodeRange, so use that if
  // it's present.
  if (FLAG_jitless) return EmbeddedData::FromBlob();
  CodeRange* code_range = CodeRange::GetProcessWideCodeRange().get();
  return (code_range && code_range->embedded_blob_code_copy() != nullptr)
             ? EmbeddedData::FromBlob(code_range)
             : EmbeddedData::FromBlob();
#else
  // Otherwise there is a single copy of the blob across all Isolates, use the
  // global atomic variables.
  return EmbeddedData::FromBlob();
#endif
}

}  // namespace

Address Code::OffHeapInstructionStart() const {
  DCHECK(is_off_heap_trampoline());
  if (Isolate::CurrentEmbeddedBlobCode() == nullptr) {
    return raw_instruction_size();
  }

  // TODO(11527): pass Isolate as an argument for getting the EmbeddedData.
  EmbeddedData d = EmbeddedDataWithMaybeRemappedEmbeddedBuiltins(*this);
  return d.InstructionStartOfBuiltin(builtin_index());
}

Address Code::OffHeapInstructionEnd() const {
  DCHECK(is_off_heap_trampoline());
  if (Isolate::CurrentEmbeddedBlobCode() == nullptr) {
    return raw_instruction_size();
  }

  // TODO(11527): pass Isolate as an argument for getting the EmbeddedData.
  EmbeddedData d = EmbeddedDataWithMaybeRemappedEmbeddedBuiltins(*this);
  return d.InstructionStartOfBuiltin(builtin_index()) +
         d.InstructionSizeOfBuiltin(builtin_index());
}

Address Code::OffHeapInstructionStart(Isolate* isolate, Address pc) const {
  DCHECK(is_off_heap_trampoline());
  EmbeddedData d = EmbeddedData::GetEmbeddedDataForPC(isolate, pc);
  return d.InstructionStartOfBuiltin(builtin_index());
}

Address Code::OffHeapInstructionEnd(Isolate* isolate, Address pc) const {
  DCHECK(is_off_heap_trampoline());
  EmbeddedData d = EmbeddedData::GetEmbeddedDataForPC(isolate, pc);
  return d.InstructionStartOfBuiltin(builtin_index()) +
         d.InstructionSizeOfBuiltin(builtin_index());
}

int Code::OffHeapMetadataSize() const {
  DCHECK(is_off_heap_trampoline());
  if (Isolate::CurrentEmbeddedBlobCode() == nullptr) {
    return raw_instruction_size();
  }
  EmbeddedData d = EmbeddedData::FromBlob();
  return d.MetadataSizeOfBuiltin(builtin_index());
}

Address Code::OffHeapMetadataStart() const {
  DCHECK(is_off_heap_trampoline());
  if (Isolate::CurrentEmbeddedBlobCode() == nullptr) {
    return raw_instruction_size();
  }
  EmbeddedData d = EmbeddedData::FromBlob();
  return d.MetadataStartOfBuiltin(builtin_index());
}

Address Code::OffHeapMetadataEnd() const {
  DCHECK(is_off_heap_trampoline());
  if (Isolate::CurrentEmbeddedBlobCode() == nullptr) {
    return raw_instruction_size();
  }
  EmbeddedData d = EmbeddedData::FromBlob();
  return d.MetadataStartOfBuiltin(builtin_index()) +
         d.MetadataSizeOfBuiltin(builtin_index());
}

// TODO(cbruni): Move to BytecodeArray
int AbstractCode::SourcePosition(int offset) {
  CHECK_NE(kind(), CodeKind::BASELINE);
  Object maybe_table = SourcePositionTableInternal();
  if (maybe_table.IsException()) return kNoSourcePosition;

  ByteArray source_position_table = ByteArray::cast(maybe_table);
  // Subtract one because the current PC is one instruction after the call site.
  if (IsCode()) offset--;
  int position = 0;
  for (SourcePositionTableIterator iterator(
           source_position_table, SourcePositionTableIterator::kJavaScriptOnly,
           SourcePositionTableIterator::kDontSkipFunctionEntry);
       !iterator.done() && iterator.code_offset() <= offset;
       iterator.Advance()) {
    position = iterator.source_position().ScriptOffset();
  }
  return position;
}

// TODO(cbruni): Move to BytecodeArray
int AbstractCode::SourceStatementPosition(int offset) {
  CHECK_NE(kind(), CodeKind::BASELINE);
  // First find the closest position.
  int position = SourcePosition(offset);
  // Now find the closest statement position before the position.
  int statement_position = 0;
  for (SourcePositionTableIterator it(SourcePositionTableInternal());
       !it.done(); it.Advance()) {
    if (it.is_statement()) {
      int p = it.source_position().ScriptOffset();
      if (statement_position < p && p <= position) {
        statement_position = p;
      }
    }
  }
  return statement_position;
}

bool Code::CanDeoptAt(Isolate* isolate, Address pc) {
  DeoptimizationData deopt_data =
      DeoptimizationData::cast(deoptimization_data());
  Address code_start_address = InstructionStart(isolate, pc);
  for (int i = 0; i < deopt_data.DeoptCount(); i++) {
    if (deopt_data.Pc(i).value() == -1) continue;
    Address address = code_start_address + deopt_data.Pc(i).value();
    if (address == pc &&
        deopt_data.GetBytecodeOffset(i) != BytecodeOffset::None()) {
      return true;
    }
  }
  return false;
}

bool Code::IsIsolateIndependent(Isolate* isolate) {
  static constexpr int kModeMask =
      RelocInfo::AllRealModesMask() &
      ~RelocInfo::ModeMask(RelocInfo::CONST_POOL) &
      ~RelocInfo::ModeMask(RelocInfo::OFF_HEAP_TARGET) &
      ~RelocInfo::ModeMask(RelocInfo::VENEER_POOL);
  STATIC_ASSERT(kModeMask ==
                (RelocInfo::ModeMask(RelocInfo::CODE_TARGET) |
                 RelocInfo::ModeMask(RelocInfo::RELATIVE_CODE_TARGET) |
                 RelocInfo::ModeMask(RelocInfo::COMPRESSED_EMBEDDED_OBJECT) |
                 RelocInfo::ModeMask(RelocInfo::FULL_EMBEDDED_OBJECT) |
                 RelocInfo::ModeMask(RelocInfo::DATA_EMBEDDED_OBJECT) |
                 RelocInfo::ModeMask(RelocInfo::EXTERNAL_REFERENCE) |
                 RelocInfo::ModeMask(RelocInfo::INTERNAL_REFERENCE) |
                 RelocInfo::ModeMask(RelocInfo::INTERNAL_REFERENCE_ENCODED) |
                 RelocInfo::ModeMask(RelocInfo::RUNTIME_ENTRY) |
                 RelocInfo::ModeMask(RelocInfo::WASM_CALL) |
                 RelocInfo::ModeMask(RelocInfo::WASM_STUB_CALL)));

#if defined(V8_TARGET_ARCH_PPC) || defined(V8_TARGET_ARCH_PPC64) || \
    defined(V8_TARGET_ARCH_MIPS64)
  return RelocIterator(*this, kModeMask).done();
#elif defined(V8_TARGET_ARCH_X64) || defined(V8_TARGET_ARCH_ARM64) || \
    defined(V8_TARGET_ARCH_ARM) || defined(V8_TARGET_ARCH_MIPS) ||    \
    defined(V8_TARGET_ARCH_S390) || defined(V8_TARGET_ARCH_IA32) ||   \
    defined(V8_TARGET_ARCH_RISCV64)
  for (RelocIterator it(*this, kModeMask); !it.done(); it.next()) {
    // On these platforms we emit relative builtin-to-builtin
    // jumps for isolate independent builtins in the snapshot. They are later
    // rewritten as pc-relative jumps to the off-heap instruction stream and are
    // thus process-independent. See also: FinalizeEmbeddedCodeTargets.
    if (RelocInfo::IsCodeTargetMode(it.rinfo()->rmode())) {
      Address target_address = it.rinfo()->target_address();
      if (InstructionStream::PcIsOffHeap(isolate, target_address)) continue;

      Code target = Code::GetCodeFromTargetAddress(target_address);
      CHECK(target.IsCode());
      if (Builtins::IsIsolateIndependentBuiltin(target)) continue;
    }
    return false;
  }
#else
#error Unsupported architecture.
#endif
  return true;
}

bool Code::Inlines(SharedFunctionInfo sfi) {
  // We can only check for inlining for optimized code.
  DCHECK(is_optimized_code());
  DisallowGarbageCollection no_gc;
  DeoptimizationData const data =
      DeoptimizationData::cast(deoptimization_data());
  if (data.length() == 0) return false;
  if (data.SharedFunctionInfo() == sfi) return true;
  FixedArray const literals = data.LiteralArray();
  int const inlined_count = data.InlinedFunctionCount().value();
  for (int i = 0; i < inlined_count; ++i) {
    if (SharedFunctionInfo::cast(literals.get(i)) == sfi) return true;
  }
  return false;
}

Code::OptimizedCodeIterator::OptimizedCodeIterator(Isolate* isolate) {
  isolate_ = isolate;
  Object list = isolate->heap()->native_contexts_list();
  next_context_ =
      list.IsUndefined(isolate_) ? NativeContext() : NativeContext::cast(list);
}

Code Code::OptimizedCodeIterator::Next() {
  do {
    Object next;
    if (!current_code_.is_null()) {
      // Get next code in the linked list.
      next = current_code_.next_code_link();
    } else if (!next_context_.is_null()) {
      // Linked list of code exhausted. Get list of next context.
      next = next_context_.OptimizedCodeListHead();
      Object next_context = next_context_.next_context_link();
      next_context_ = next_context.IsUndefined(isolate_)
                          ? NativeContext()
                          : NativeContext::cast(next_context);
    } else {
      // Exhausted contexts.
      return Code();
    }
    current_code_ = next.IsUndefined(isolate_) ? Code() : Code::cast(next);
  } while (current_code_.is_null());
  DCHECK(CodeKindCanDeoptimize(current_code_.kind()));
  return current_code_;
}

Handle<DeoptimizationData> DeoptimizationData::New(Isolate* isolate,
                                                   int deopt_entry_count,
                                                   AllocationType allocation) {
  return Handle<DeoptimizationData>::cast(isolate->factory()->NewFixedArray(
      LengthFor(deopt_entry_count), allocation));
}

Handle<DeoptimizationData> DeoptimizationData::Empty(Isolate* isolate) {
  return Handle<DeoptimizationData>::cast(
      isolate->factory()->empty_fixed_array());
}

SharedFunctionInfo DeoptimizationData::GetInlinedFunction(int index) {
  if (index == -1) {
    return SharedFunctionInfo::cast(SharedFunctionInfo());
  } else {
    return SharedFunctionInfo::cast(LiteralArray().get(index));
  }
}

#ifdef ENABLE_DISASSEMBLER

const char* Code::GetName(Isolate* isolate) const {
  if (kind() == CodeKind::BYTECODE_HANDLER) {
    return isolate->interpreter()->LookupNameOfBytecodeHandler(*this);
  } else {
    // There are some handlers and ICs that we can also find names for with
    // Builtins::Lookup.
    return isolate->builtins()->Lookup(raw_instruction_start());
  }
}

namespace {
void print_pc(std::ostream& os, int pc) {
  if (pc == -1) {
    os << "NA";
  } else {
    os << std::hex << pc << std::dec;
  }
}
}  // anonymous namespace

void DeoptimizationData::DeoptimizationDataPrint(std::ostream& os) {
  if (length() == 0) {
    os << "Deoptimization Input Data invalidated by lazy deoptimization\n";
    return;
  }

  int const inlined_function_count = InlinedFunctionCount().value();
  os << "Inlined functions (count = " << inlined_function_count << ")\n";
  for (int id = 0; id < inlined_function_count; ++id) {
    Object info = LiteralArray().get(id);
    os << " " << Brief(SharedFunctionInfo::cast(info)) << "\n";
  }
  os << "\n";
  int deopt_count = DeoptCount();
  os << "Deoptimization Input Data (deopt points = " << deopt_count << ")\n";
  if (0 != deopt_count) {
    os << " index  bytecode-offset    pc";
    if (FLAG_print_code_verbose) os << "  commands";
    os << "\n";
  }
  for (int i = 0; i < deopt_count; i++) {
    os << std::setw(6) << i << "  " << std::setw(15)
       << GetBytecodeOffset(i).ToInt() << "  " << std::setw(4);
    print_pc(os, Pc(i).value());
    os << std::setw(2);

    if (!FLAG_print_code_verbose) {
      os << "\n";
      continue;
    }

    TranslationArrayPrintSingleFrame(os, TranslationByteArray(),
                                     TranslationIndex(i).value(),
                                     LiteralArray());
  }
}

namespace {

inline void DisassembleCodeRange(Isolate* isolate, std::ostream& os, Code code,
                                 Address begin, size_t size,
                                 Address current_pc) {
  Address end = begin + size;
  AllowHandleAllocation allow_handles;
  DisallowGarbageCollection no_gc;
  HandleScope handle_scope(isolate);
  Disassembler::Decode(isolate, &os, reinterpret_cast<byte*>(begin),
                       reinterpret_cast<byte*>(end),
                       CodeReference(handle(code, isolate)), current_pc);
}

}  // namespace

void Code::Disassemble(const char* name, std::ostream& os, Isolate* isolate,
                       Address current_pc) {
  os << "kind = " << CodeKindToString(kind()) << "\n";
  if (name == nullptr) {
    name = GetName(isolate);
  }
  if ((name != nullptr) && (name[0] != '\0')) {
    os << "name = " << name << "\n";
  }
  if (CodeKindIsOptimizedJSFunction(kind()) && kind() != CodeKind::BASELINE) {
    os << "stack_slots = " << stack_slots() << "\n";
  }
  os << "compiler = "
     << (is_turbofanned()
             ? "turbofan"
             : kind() == CodeKind::BASELINE ? "baseline" : "unknown")
     << "\n";
  os << "address = " << reinterpret_cast<void*>(ptr()) << "\n\n";

  if (is_off_heap_trampoline()) {
    int trampoline_size = raw_instruction_size();
    os << "Trampoline (size = " << trampoline_size << ")\n";
    DisassembleCodeRange(isolate, os, *this, raw_instruction_start(),
                         trampoline_size, current_pc);
    os << "\n";
  }

  {
    int code_size = InstructionSize();
    os << "Instructions (size = " << code_size << ")\n";
    DisassembleCodeRange(isolate, os, *this, InstructionStart(), code_size,
                         current_pc);

    if (int pool_size = constant_pool_size()) {
      DCHECK_EQ(pool_size & kPointerAlignmentMask, 0);
      os << "\nConstant Pool (size = " << pool_size << ")\n";
      Vector<char> buf = Vector<char>::New(50);
      intptr_t* ptr =
          reinterpret_cast<intptr_t*>(MetadataStart() + constant_pool_offset());
      for (int i = 0; i < pool_size; i += kSystemPointerSize, ptr++) {
        SNPrintF(buf, "%4d %08" V8PRIxPTR, i, *ptr);
        os << static_cast<const void*>(ptr) << "  " << buf.begin() << "\n";
      }
    }
  }
  os << "\n";

  // TODO(cbruni): add support for baseline code.
  if (kind() != CodeKind::BASELINE) {
    {
      SourcePositionTableIterator it(
          source_position_table(),
          SourcePositionTableIterator::kJavaScriptOnly);
      if (!it.done()) {
        os << "Source positions:\n pc offset  position\n";
        for (; !it.done(); it.Advance()) {
          os << std::setw(10) << std::hex << it.code_offset() << std::dec
             << std::setw(10) << it.source_position().ScriptOffset()
             << (it.is_statement() ? "  statement" : "") << "\n";
        }
        os << "\n";
      }
    }

    {
      SourcePositionTableIterator it(
          source_position_table(), SourcePositionTableIterator::kExternalOnly);
      if (!it.done()) {
        os << "External Source positions:\n pc offset  fileid  line\n";
        for (; !it.done(); it.Advance()) {
          DCHECK(it.source_position().IsExternal());
          os << std::setw(10) << std::hex << it.code_offset() << std::dec
             << std::setw(10) << it.source_position().ExternalFileId()
             << std::setw(10) << it.source_position().ExternalLine() << "\n";
        }
        os << "\n";
      }
    }
  }

  if (CodeKindCanDeoptimize(kind())) {
    DeoptimizationData data =
        DeoptimizationData::cast(this->deoptimization_data());
    data.DeoptimizationDataPrint(os);
  }
  os << "\n";

  if (has_safepoint_info()) {
    SafepointTable table(isolate, current_pc, *this);
    os << "Safepoints (size = " << table.size() << ")\n";
    for (unsigned i = 0; i < table.length(); i++) {
      unsigned pc_offset = table.GetPcOffset(i);
      os << reinterpret_cast<const void*>(InstructionStart() + pc_offset)
         << "  ";
      os << std::setw(6) << std::hex << pc_offset << "  " << std::setw(4);
      int trampoline_pc = table.GetTrampolinePcOffset(i);
      print_pc(os, trampoline_pc);
      os << std::dec << "  ";
      table.PrintEntry(i, os);
      os << " (sp -> fp)  ";
      SafepointEntry entry = table.GetEntry(i);
      if (entry.has_deoptimization_index()) {
        os << std::setw(6) << entry.deoptimization_index();
      } else {
        os << "<none>";
      }
      os << "\n";
    }
    os << "\n";
  }

  if (has_handler_table()) {
    HandlerTable table(*this);
    os << "Handler Table (size = " << table.NumberOfReturnEntries() << ")\n";
    if (CodeKindIsOptimizedJSFunction(kind()))
      table.HandlerTableReturnPrint(os);
    os << "\n";
  }

  os << "RelocInfo (size = " << relocation_size() << ")\n";
  for (RelocIterator it(*this); !it.done(); it.next()) {
    it.rinfo()->Print(isolate, os);
  }
  os << "\n";

  if (has_unwinding_info()) {
    os << "UnwindingInfo (size = " << unwinding_info_size() << ")\n";
    EhFrameDisassembler eh_frame_disassembler(
        reinterpret_cast<byte*>(unwinding_info_start()),
        reinterpret_cast<byte*>(unwinding_info_end()));
    eh_frame_disassembler.DisassembleToStream(os);
    os << "\n";
  }
}
#endif  // ENABLE_DISASSEMBLER

void BytecodeArray::Disassemble(std::ostream& os) {
  DisallowGarbageCollection no_gc;

  os << "Parameter count " << parameter_count() << "\n";
  os << "Register count " << register_count() << "\n";
  os << "Frame size " << frame_size() << "\n";
  os << "OSR nesting level: " << osr_loop_nesting_level() << "\n";
  os << "Bytecode Age: " << bytecode_age() << "\n";

  Address base_address = GetFirstBytecodeAddress();
  SourcePositionTableIterator source_positions(SourcePositionTable());

  // Storage for backing the handle passed to the iterator. This handle won't be
  // updated by the gc, but that's ok because we've disallowed GCs anyway.
  BytecodeArray handle_storage = *this;
  Handle<BytecodeArray> handle(reinterpret_cast<Address*>(&handle_storage));
  interpreter::BytecodeArrayIterator iterator(handle);
  while (!iterator.done()) {
    if (!source_positions.done() &&
        iterator.current_offset() == source_positions.code_offset()) {
      os << std::setw(5) << source_positions.source_position().ScriptOffset();
      os << (source_positions.is_statement() ? " S> " : " E> ");
      source_positions.Advance();
    } else {
      os << "         ";
    }
    Address current_address = base_address + iterator.current_offset();
    os << reinterpret_cast<const void*>(current_address) << " @ "
       << std::setw(4) << iterator.current_offset() << " : ";
    interpreter::BytecodeDecoder::Decode(
        os, reinterpret_cast<byte*>(current_address),
        static_cast<int>(parameter_count()));
    if (interpreter::Bytecodes::IsJump(iterator.current_bytecode())) {
      Address jump_target = base_address + iterator.GetJumpTargetOffset();
      os << " (" << reinterpret_cast<void*>(jump_target) << " @ "
         << iterator.GetJumpTargetOffset() << ")";
    }
    if (interpreter::Bytecodes::IsSwitch(iterator.current_bytecode())) {
      os << " {";
      bool first_entry = true;
      for (const auto& entry : iterator.GetJumpTableTargetOffsets()) {
        if (first_entry) {
          first_entry = false;
        } else {
          os << ",";
        }
        os << " " << entry.case_value << ": @" << entry.target_offset;
      }
      os << " }";
    }
    os << std::endl;
    iterator.Advance();
  }

  os << "Constant pool (size = " << constant_pool().length() << ")\n";
#ifdef OBJECT_PRINT
  if (constant_pool().length() > 0) {
    constant_pool().Print(os);
  }
#endif

  os << "Handler Table (size = " << handler_table().length() << ")\n";
#ifdef ENABLE_DISASSEMBLER
  if (handler_table().length() > 0) {
    HandlerTable table(*this);
    table.HandlerTableRangePrint(os);
  }
#endif

  ByteArray source_position_table = SourcePositionTable();
  os << "Source Position Table (size = " << source_position_table.length()
     << ")\n";
#ifdef OBJECT_PRINT
  if (source_position_table.length() > 0) {
    os << Brief(source_position_table) << std::endl;
  }
#endif
}

void BytecodeArray::CopyBytecodesTo(BytecodeArray to) {
  BytecodeArray from = *this;
  DCHECK_EQ(from.length(), to.length());
  CopyBytes(reinterpret_cast<byte*>(to.GetFirstBytecodeAddress()),
            reinterpret_cast<byte*>(from.GetFirstBytecodeAddress()),
            from.length());
}

void BytecodeArray::MakeOlder() {
  // BytecodeArray is aged in concurrent marker.
  // The word must be completely within the byte code array.
  Address age_addr = address() + kBytecodeAgeOffset;
  DCHECK_LE(RoundDown(age_addr, kTaggedSize) + kTaggedSize, address() + Size());
  Age age = bytecode_age();
  if (age < kLastBytecodeAge) {
    base::AsAtomic8::Relaxed_CompareAndSwap(
        reinterpret_cast<base::Atomic8*>(age_addr), age, age + 1);
  }

  DCHECK_GE(bytecode_age(), kFirstBytecodeAge);
  DCHECK_LE(bytecode_age(), kLastBytecodeAge);
}

bool BytecodeArray::IsOld() const {
  return bytecode_age() >= kIsOldBytecodeAge;
}

DependentCode DependentCode::GetDependentCode(Handle<HeapObject> object) {
  if (object->IsMap()) {
    return Handle<Map>::cast(object)->dependent_code();
  } else if (object->IsPropertyCell()) {
    return Handle<PropertyCell>::cast(object)->dependent_code();
  } else if (object->IsAllocationSite()) {
    return Handle<AllocationSite>::cast(object)->dependent_code();
  }
  UNREACHABLE();
}

void DependentCode::SetDependentCode(Handle<HeapObject> object,
                                     Handle<DependentCode> dep) {
  if (object->IsMap()) {
    Handle<Map>::cast(object)->set_dependent_code(*dep);
  } else if (object->IsPropertyCell()) {
    Handle<PropertyCell>::cast(object)->set_dependent_code(*dep);
  } else if (object->IsAllocationSite()) {
    Handle<AllocationSite>::cast(object)->set_dependent_code(*dep);
  } else {
    UNREACHABLE();
  }
}

void DependentCode::InstallDependency(Isolate* isolate,
                                      const MaybeObjectHandle& code,
                                      Handle<HeapObject> object,
                                      DependencyGroup group) {
  if (V8_UNLIKELY(FLAG_trace_code_dependencies)) {
    StdoutStream{} << "Installing dependency of [" << code->GetHeapObject()
                   << "] on [" << object << "] in group ["
                   << DependencyGroupName(group) << "]\n";
  }
  Handle<DependentCode> old_deps(DependentCode::GetDependentCode(object),
                                 isolate);
  Handle<DependentCode> new_deps =
      InsertWeakCode(isolate, old_deps, group, code);
  // Update the list head if necessary.
  if (!new_deps.is_identical_to(old_deps))
    DependentCode::SetDependentCode(object, new_deps);
}

Handle<DependentCode> DependentCode::InsertWeakCode(
    Isolate* isolate, Handle<DependentCode> entries, DependencyGroup group,
    const MaybeObjectHandle& code) {
  if (entries->length() == 0 || entries->group() > group) {
    // There is no such group.
    return DependentCode::New(isolate, group, code, entries);
  }
  if (entries->group() < group) {
    // The group comes later in the list.
    Handle<DependentCode> old_next(entries->next_link(), isolate);
    Handle<DependentCode> new_next =
        InsertWeakCode(isolate, old_next, group, code);
    if (!old_next.is_identical_to(new_next)) {
      entries->set_next_link(*new_next);
    }
    return entries;
  }
  DCHECK_EQ(group, entries->group());
  int count = entries->count();
  // Check for existing entry to avoid duplicates.
  for (int i = 0; i < count; i++) {
    if (entries->object_at(i) == *code) return entries;
  }
  if (entries->length() < kCodesStartIndex + count + 1) {
    entries = EnsureSpace(isolate, entries);
    // Count could have changed, reload it.
    count = entries->count();
  }
  entries->set_object_at(count, *code);
  entries->set_count(count + 1);
  return entries;
}

Handle<DependentCode> DependentCode::New(Isolate* isolate,
                                         DependencyGroup group,
                                         const MaybeObjectHandle& object,
                                         Handle<DependentCode> next) {
  Handle<DependentCode> result =
      Handle<DependentCode>::cast(isolate->factory()->NewWeakFixedArray(
          kCodesStartIndex + 1, AllocationType::kOld));
  result->set_next_link(*next);
  result->set_flags(GroupField::encode(group) | CountField::encode(1));
  result->set_object_at(0, *object);
  return result;
}

Handle<DependentCode> DependentCode::EnsureSpace(
    Isolate* isolate, Handle<DependentCode> entries) {
  if (entries->Compact()) return entries;
  int capacity = kCodesStartIndex + DependentCode::Grow(entries->count());
  int grow_by = capacity - entries->length();
  return Handle<DependentCode>::cast(
      isolate->factory()->CopyWeakFixedArrayAndGrow(entries, grow_by));
}

bool DependentCode::Compact() {
  int old_count = count();
  int new_count = 0;
  for (int i = 0; i < old_count; i++) {
    MaybeObject obj = object_at(i);
    if (!obj->IsCleared()) {
      if (i != new_count) {
        copy(i, new_count);
      }
      new_count++;
    }
  }
  set_count(new_count);
  for (int i = new_count; i < old_count; i++) {
    clear_at(i);
  }
  return new_count < old_count;
}

bool DependentCode::MarkCodeForDeoptimization(
    DependentCode::DependencyGroup group) {
  if (this->length() == 0 || this->group() > group) {
    // There is no such group.
    return false;
  }
  if (this->group() < group) {
    // The group comes later in the list.
    return next_link().MarkCodeForDeoptimization(group);
  }
  DCHECK_EQ(group, this->group());
  DisallowGarbageCollection no_gc_scope;
  // Mark all the code that needs to be deoptimized.
  bool marked = false;
  int count = this->count();
  for (int i = 0; i < count; i++) {
    MaybeObject obj = object_at(i);
    if (obj->IsCleared()) continue;
    Code code = Code::cast(obj->GetHeapObjectAssumeWeak());
    if (!code.marked_for_deoptimization()) {
      code.SetMarkedForDeoptimization(DependencyGroupName(group));
      marked = true;
    }
  }
  for (int i = 0; i < count; i++) {
    clear_at(i);
  }
  set_count(0);
  return marked;
}

void DependentCode::DeoptimizeDependentCodeGroup(
    DependentCode::DependencyGroup group) {
  DisallowGarbageCollection no_gc_scope;
  bool marked = MarkCodeForDeoptimization(group);
  if (marked) {
    DCHECK(AllowCodeDependencyChange::IsAllowed());
    // TODO(11527): pass Isolate as an argument.
    Deoptimizer::DeoptimizeMarkedCode(GetIsolateFromWritableObject(*this));
  }
}

void Code::SetMarkedForDeoptimization(const char* reason) {
  set_marked_for_deoptimization(true);
  Deoptimizer::TraceMarkForDeoptimization(*this, reason);
}

const char* DependentCode::DependencyGroupName(DependencyGroup group) {
  switch (group) {
    case kTransitionGroup:
      return "transition";
    case kPrototypeCheckGroup:
      return "prototype-check";
    case kPropertyCellChangedGroup:
      return "property-cell-changed";
    case kFieldConstGroup:
      return "field-const";
    case kFieldTypeGroup:
      return "field-type";
    case kFieldRepresentationGroup:
      return "field-representation";
    case kInitialMapChangedGroup:
      return "initial-map-changed";
    case kAllocationSiteTenuringChangedGroup:
      return "allocation-site-tenuring-changed";
    case kAllocationSiteTransitionChangedGroup:
      return "allocation-site-transition-changed";
  }
  UNREACHABLE();
}

}  // namespace internal
}  // namespace v8
