// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test/fuzzer/wasm-fuzzer-common.h"

#include <ctime>

#include "include/v8.h"
#include "src/execution/isolate.h"
#include "src/objects/objects-inl.h"
#include "src/utils/ostreams.h"
#include "src/wasm/wasm-engine.h"
#include "src/wasm/wasm-feature-flags.h"
#include "src/wasm/wasm-module-builder.h"
#include "src/wasm/wasm-module.h"
#include "src/wasm/wasm-objects-inl.h"
#include "src/zone/accounting-allocator.h"
#include "src/zone/zone.h"
#include "test/common/wasm/flag-utils.h"
#include "test/common/wasm/wasm-module-runner.h"
#include "test/fuzzer/fuzzer-support.h"

namespace v8 {
namespace internal {
namespace wasm {
namespace fuzzer {

void InterpretAndExecuteModule(i::Isolate* isolate,
                               Handle<WasmModuleObject> module_object) {
  // We do not instantiate the module if there is a start function, because a
  // start function can contain an infinite loop which we cannot handle.
  if (module_object->module()->start_function_index >= 0) return;

  HandleScope handle_scope(isolate);  // Avoid leaking handles.
  Handle<WasmInstanceObject> instance;

  // Try to instantiate, return if it fails.
  {
    ErrorThrower thrower(isolate, "WebAssembly Instantiation");
    if (!isolate->wasm_engine()
             ->SyncInstantiate(isolate, &thrower, module_object, {},
                               {})  // no imports & memory
             .ToHandle(&instance)) {
      isolate->clear_pending_exception();
      thrower.Reset();  // Ignore errors.
      return;
    }
  }

  // Get the "main" exported function. Do nothing if it does not exist.
  Handle<WasmExportedFunction> main_function;
  if (!testing::GetExportedFunction(isolate, instance, "main")
           .ToHandle(&main_function)) {
    return;
  }

  OwnedVector<WasmValue> arguments =
      testing::MakeDefaultInterpreterArguments(isolate, main_function->sig());

  // Now interpret.
  testing::WasmInterpretationResult interpreter_result =
      testing::InterpretWasmModule(isolate, instance,
                                   main_function->function_index(),
                                   arguments.begin());
  if (interpreter_result.failed()) return;

  // The WebAssembly spec allows the sign bit of NaN to be non-deterministic.
  // This sign bit can make the difference between an infinite loop and
  // terminating code. With possible non-determinism we cannot guarantee that
  // the generated code will not go into an infinite loop and cause a timeout in
  // Clusterfuzz. Therefore we do not execute the generated code if the result
  // may be non-deterministic.
  if (interpreter_result.possible_nondeterminism()) return;

  // Try to instantiate and execute the module_object.
  {
    ErrorThrower thrower(isolate, "Second Instantiation");
    // We instantiated before, so the second instantiation must also succeed:
    CHECK(isolate->wasm_engine()
              ->SyncInstantiate(isolate, &thrower, module_object, {},
                                {})  // no imports & memory
              .ToHandle(&instance));
  }

  OwnedVector<Handle<Object>> compiled_args =
      testing::MakeDefaultArguments(isolate, main_function->sig());

  bool exception = false;
  int32_t result_compiled = testing::CallWasmFunctionForTesting(
      isolate, instance, "main", static_cast<int>(compiled_args.size()),
      compiled_args.begin(), &exception);
  if (interpreter_result.trapped() != exception) {
    const char* exception_text[] = {"no exception", "exception"};
    FATAL("interpreter: %s; compiled: %s",
          exception_text[interpreter_result.trapped()],
          exception_text[exception]);
  }

  if (interpreter_result.finished()) {
    CHECK_EQ(interpreter_result.result(), result_compiled);
  }
}

namespace {
struct PrintSig {
  const size_t num;
  const std::function<ValueType(size_t)> getter;
};
PrintSig PrintParameters(const FunctionSig* sig) {
  return {sig->parameter_count(), [=](size_t i) { return sig->GetParam(i); }};
}
PrintSig PrintReturns(const FunctionSig* sig) {
  return {sig->return_count(), [=](size_t i) { return sig->GetReturn(i); }};
}
const char* ValueTypeToConstantName(ValueType type) {
  switch (type.kind()) {
    case kI32:
      return "kWasmI32";
    case kI64:
      return "kWasmI64";
    case kF32:
      return "kWasmF32";
    case kF64:
      return "kWasmF64";
    case kS128:
      return "kWasmS128";
    case kOptRef:
      switch (type.heap_representation()) {
        case HeapType::kExtern:
          return "kWasmExternRef";
        case HeapType::kFunc:
          return "kWasmFuncRef";
        case HeapType::kAny:
        case HeapType::kI31:
        case HeapType::kBottom:
        default:
          // TODO(7748): Implement these if fuzzing for them is enabled.
          UNREACHABLE();
      }
    default:
      UNREACHABLE();
  }
}
std::ostream& operator<<(std::ostream& os, const PrintSig& print) {
  os << "[";
  for (size_t i = 0; i < print.num; ++i) {
    os << (i == 0 ? "" : ", ") << ValueTypeToConstantName(print.getter(i));
  }
  return os << "]";
}

struct PrintName {
  WasmName name;
  PrintName(ModuleWireBytes wire_bytes, WireBytesRef ref)
      : name(wire_bytes.GetNameOrNull(ref)) {}
};
std::ostream& operator<<(std::ostream& os, const PrintName& name) {
  return os.write(name.name.begin(), name.name.size());
}

std::ostream& operator<<(std::ostream& os, const WasmInitExpr& expr) {
  os << "WasmInitExpr.";
  switch (expr.kind()) {
    case WasmInitExpr::kNone:
      UNREACHABLE();
    case WasmInitExpr::kS128Const:
    case WasmInitExpr::kRttCanon:
    case WasmInitExpr::kRttSub:
    case WasmInitExpr::kRefNullConst:
      // TODO(manoskouk): Implement these.
      UNIMPLEMENTED();
    case WasmInitExpr::kGlobalGet:
      os << "GlobalGet(" << expr.immediate().index;
      break;
    case WasmInitExpr::kI32Const:
      os << "I32Const(" << expr.immediate().i32_const;
      break;
    case WasmInitExpr::kI64Const:
      os << "I64Const(" << expr.immediate().i64_const;
      break;
    case WasmInitExpr::kF32Const:
      os << "F32Const(" << expr.immediate().f32_const;
      break;
    case WasmInitExpr::kF64Const:
      os << "F64Const(" << expr.immediate().f64_const;
      break;
    case WasmInitExpr::kRefFuncConst:
      os << "RefFunc(" << expr.immediate().index;
      break;
  }
  return os << ")";
}
}  // namespace

void GenerateTestCase(Isolate* isolate, ModuleWireBytes wire_bytes,
                      bool compiles) {
  constexpr bool kVerifyFunctions = false;
  auto enabled_features = i::wasm::WasmFeatures::FromIsolate(isolate);
  ModuleResult module_res = DecodeWasmModule(
      enabled_features, wire_bytes.start(), wire_bytes.end(), kVerifyFunctions,
      ModuleOrigin::kWasmOrigin, isolate->counters(),
      isolate->metrics_recorder(), v8::metrics::Recorder::ContextId::Empty(),
      DecodingMethod::kSync, isolate->wasm_engine()->allocator());
  CHECK(module_res.ok());
  WasmModule* module = module_res.value().get();
  CHECK_NOT_NULL(module);

  StdoutStream os;

  tzset();
  time_t current_time = time(nullptr);
  struct tm current_localtime;
#ifdef V8_OS_WIN
  localtime_s(&current_localtime, &current_time);
#else
  localtime_r(&current_time, &current_localtime);
#endif
  int year = 1900 + current_localtime.tm_year;

  os << "// Copyright " << year
     << " the V8 project authors. All rights reserved.\n"
        "// Use of this source code is governed by a BSD-style license that "
        "can be\n"
        "// found in the LICENSE file.\n"
        "\n"
        "// Flags: --wasm-staging\n"
        "\n"
        "load('test/mjsunit/wasm/wasm-module-builder.js');\n"
        "\n"
        "const builder = new WasmModuleBuilder();\n";

  if (module->has_memory) {
    os << "builder.addMemory(" << module->initial_pages;
    if (module->has_maximum_pages) {
      os << ", " << module->maximum_pages;
    } else {
      os << ", undefined";
    }
    os << ", " << (module->mem_export ? "true" : "false");
    if (module->has_shared_memory) {
      os << ", true";
    }
    os << ");\n";
  }

  for (WasmGlobal& glob : module->globals) {
    os << "builder.addGlobal(" << ValueTypeToConstantName(glob.type) << ", "
       << glob.mutability << ", " << glob.init << ");\n";
  }

  // TODO(7748): Support array/struct types.
#if DEBUG
  for (uint8_t kind : module->type_kinds) {
    DCHECK_EQ(kWasmFunctionTypeCode, kind);
  }
#endif
  for (TypeDefinition type : module->types) {
    const FunctionSig* sig = type.function_sig;
    os << "builder.addType(makeSig(" << PrintParameters(sig) << ", "
       << PrintReturns(sig) << "));\n";
  }

  Zone tmp_zone(isolate->allocator(), ZONE_NAME);

  // There currently cannot be more than one table.
  // TODO(manoskouk): Add support for more tables.
  // TODO(9495): Add support for talbes with explicit initializers.
  DCHECK_GE(1, module->tables.size());
  for (const WasmTable& table : module->tables) {
    os << "builder.setTableBounds(" << table.initial_size << ", ";
    if (table.has_maximum_size) {
      os << table.maximum_size << ");\n";
    } else {
      os << "undefined);\n";
    }
  }
  for (const WasmElemSegment& elem_segment : module->elem_segments) {
    const char* status_str =
        elem_segment.status == WasmElemSegment::kStatusActive
            ? "Active"
            : elem_segment.status == WasmElemSegment::kStatusPassive
                  ? "Passive"
                  : "Declarative";
    os << "builder.add" << status_str << "ElementSegment(";
    if (elem_segment.status == WasmElemSegment::kStatusActive) {
      os << elem_segment.table_index << ", " << elem_segment.offset << ", ";
    }
    os << "[";
    for (uint32_t i = 0; i < elem_segment.entries.size(); i++) {
      os << elem_segment.entries[i];
      if (i < elem_segment.entries.size() - 1) os << ", ";
    }
    os << "], " << ValueTypeToConstantName(elem_segment.type) << ");\n";
  }

  for (const WasmFunction& func : module->functions) {
    Vector<const uint8_t> func_code = wire_bytes.GetFunctionBytes(&func);
    os << "// Generate function " << (func.func_index + 1) << " (out of "
       << module->functions.size() << ").\n";

    // Add function.
    os << "builder.addFunction(undefined, " << func.sig_index
       << " /* sig */)\n";

    // Add locals.
    BodyLocalDecls decls(&tmp_zone);
    DecodeLocalDecls(enabled_features, &decls, module, func_code.begin(),
                     func_code.end());
    if (!decls.type_list.empty()) {
      os << "  ";
      for (size_t pos = 0, count = 1, locals = decls.type_list.size();
           pos < locals; pos += count, count = 1) {
        ValueType type = decls.type_list[pos];
        while (pos + count < locals && decls.type_list[pos + count] == type) {
          ++count;
        }
        os << ".addLocals(" << ValueTypeToConstantName(type) << ", " << count
           << ")";
      }
      os << "\n";
    }

    // Add body.
    os << "  .addBodyWithEnd([\n";

    FunctionBody func_body(func.sig, func.code.offset(), func_code.begin(),
                           func_code.end());
    PrintRawWasmCode(isolate->allocator(), func_body, module, kOmitLocals);
    os << "]);\n";
  }

  for (WasmExport& exp : module->export_table) {
    if (exp.kind != kExternalFunction) continue;
    os << "builder.addExport('" << PrintName(wire_bytes, exp.name) << "', "
       << exp.index << ");\n";
  }

  if (compiles) {
    os << "const instance = builder.instantiate();\n"
          "print(instance.exports.main(1, 2, 3));\n";
  } else {
    os << "assertThrows(function() { builder.instantiate(); }, "
          "WebAssembly.CompileError);\n";
  }
}

void OneTimeEnableStagedWasmFeatures(v8::Isolate* isolate) {
  struct EnableStagedWasmFeatures {
    explicit EnableStagedWasmFeatures(v8::Isolate* isolate) {
#define ENABLE_STAGED_FEATURES(feat, desc, val) \
  FLAG_experimental_wasm_##feat = true;
      FOREACH_WASM_STAGING_FEATURE_FLAG(ENABLE_STAGED_FEATURES)
#undef ENABLE_STAGED_FEATURES
      isolate->InstallConditionalFeatures(isolate->GetCurrentContext());
    }
  };
  // The compiler will properly synchronize the constructor call.
  static EnableStagedWasmFeatures one_time_enable_staged_features(isolate);
}

void WasmExecutionFuzzer::FuzzWasmModule(Vector<const uint8_t> data,
                                         bool require_valid) {
  v8_fuzzer::FuzzerSupport* support = v8_fuzzer::FuzzerSupport::Get();
  v8::Isolate* isolate = support->GetIsolate();

  // Strictly enforce the input size limit. Note that setting "max_len" on the
  // fuzzer target is not enough, since different fuzzers are used and not all
  // respect that limit.
  if (data.size() > max_input_size()) return;

  i::Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  // Clear any pending exceptions from a prior run.
  i_isolate->clear_pending_exception();

  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(support->GetContext());

  // We explicitly enable staged WebAssembly features here to increase fuzzer
  // coverage. For libfuzzer fuzzers it is not possible that the fuzzer enables
  // the flag by itself.
  OneTimeEnableStagedWasmFeatures(isolate);

  v8::TryCatch try_catch(isolate);
  HandleScope scope(i_isolate);

  AccountingAllocator allocator;
  Zone zone(&allocator, ZONE_NAME);

  ZoneBuffer buffer(&zone);
  int32_t num_args = 0;
  std::unique_ptr<WasmValue[]> interpreter_args;
  std::unique_ptr<Handle<Object>[]> compiler_args;
  // The first byte builds the bitmask to control which function will be
  // compiled with Turbofan and which one with Liftoff.
  uint8_t tier_mask = data.empty() ? 0 : data[0];
  if (!data.empty()) data += 1;
  uint8_t debug_mask = data.empty() ? 0 : data[0];
  if (!data.empty()) data += 1;
  if (!GenerateModule(i_isolate, &zone, data, &buffer, &num_args,
                      &interpreter_args, &compiler_args)) {
    return;
  }

  testing::SetupIsolateForWasmModule(i_isolate);

  ErrorThrower interpreter_thrower(i_isolate, "Interpreter");
  ModuleWireBytes wire_bytes(buffer.begin(), buffer.end());

  auto enabled_features = i::wasm::WasmFeatures::FromIsolate(i_isolate);
  MaybeHandle<WasmModuleObject> compiled_module;
  {
    // Explicitly enable Liftoff, disable tiering and set the tier_mask. This
    // way, we deterministically test a combination of Liftoff and Turbofan.
    FlagScope<bool> liftoff(&FLAG_liftoff, true);
    FlagScope<bool> no_tier_up(&FLAG_wasm_tier_up, false);
    FlagScope<int> tier_mask_scope(&FLAG_wasm_tier_mask_for_testing, tier_mask);
    FlagScope<int> debug_mask_scope(&FLAG_wasm_debug_mask_for_testing,
                                    debug_mask);
    compiled_module = i_isolate->wasm_engine()->SyncCompile(
        i_isolate, enabled_features, &interpreter_thrower, wire_bytes);
  }
  bool compiles = !compiled_module.is_null();

  if (FLAG_wasm_fuzzer_gen_test) {
    GenerateTestCase(i_isolate, wire_bytes, compiles);
  }

  bool validates = i_isolate->wasm_engine()->SyncValidate(
      i_isolate, enabled_features, wire_bytes);

  CHECK_EQ(compiles, validates);
  CHECK_IMPLIES(require_valid, validates);

  if (!compiles) return;

  InterpretAndExecuteModule(i_isolate, compiled_module.ToHandleChecked());
}

}  // namespace fuzzer
}  // namespace wasm
}  // namespace internal
}  // namespace v8
