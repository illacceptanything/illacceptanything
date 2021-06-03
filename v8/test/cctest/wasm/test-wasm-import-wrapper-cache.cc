// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/compiler/wasm-compiler.h"
#include "src/wasm/function-compiler.h"
#include "src/wasm/module-compiler.h"
#include "src/wasm/wasm-code-manager.h"
#include "src/wasm/wasm-engine.h"
#include "src/wasm/wasm-import-wrapper-cache.h"
#include "src/wasm/wasm-module.h"

#include "test/cctest/cctest.h"
#include "test/common/wasm/test-signatures.h"

namespace v8 {
namespace internal {
namespace wasm {
namespace test_wasm_import_wrapper_cache {

std::shared_ptr<NativeModule> NewModule(Isolate* isolate) {
  std::shared_ptr<WasmModule> module(new WasmModule);
  constexpr size_t kCodeSizeEstimate = 16384;
  auto native_module = isolate->wasm_engine()->NewNativeModule(
      isolate, WasmFeatures::All(), std::move(module), kCodeSizeEstimate);
  native_module->SetWireBytes({});
  return native_module;
}

TEST(CacheHit) {
  Isolate* isolate = CcTest::InitIsolateOnce();
  auto module = NewModule(isolate);
  TestSignatures sigs;
  WasmCodeRefScope wasm_code_ref_scope;
  WasmImportWrapperCache::ModificationScope cache_scope(
      module->import_wrapper_cache());

  auto kind = compiler::WasmImportCallKind::kJSFunctionArityMatch;
  auto sig = sigs.i_i();
  int expected_arity = static_cast<int>(sig->parameter_count());

  WasmCode* c1 = CompileImportWrapper(isolate->wasm_engine(), module.get(),
                                      isolate->counters(), kind, sig,
                                      expected_arity, &cache_scope);

  CHECK_NOT_NULL(c1);
  CHECK_EQ(WasmCode::Kind::kWasmToJsWrapper, c1->kind());

  WasmCode* c2 = cache_scope[{kind, sig, expected_arity}];

  CHECK_NOT_NULL(c2);
  CHECK_EQ(c1, c2);
}

TEST(CacheMissSig) {
  Isolate* isolate = CcTest::InitIsolateOnce();
  auto module = NewModule(isolate);
  TestSignatures sigs;
  WasmCodeRefScope wasm_code_ref_scope;
  WasmImportWrapperCache::ModificationScope cache_scope(
      module->import_wrapper_cache());

  auto kind = compiler::WasmImportCallKind::kJSFunctionArityMatch;
  auto sig1 = sigs.i_i();
  int expected_arity1 = static_cast<int>(sig1->parameter_count());
  auto sig2 = sigs.i_ii();
  int expected_arity2 = static_cast<int>(sig2->parameter_count());

  WasmCode* c1 = CompileImportWrapper(isolate->wasm_engine(), module.get(),
                                      isolate->counters(), kind, sig1,
                                      expected_arity1, &cache_scope);

  CHECK_NOT_NULL(c1);
  CHECK_EQ(WasmCode::Kind::kWasmToJsWrapper, c1->kind());

  WasmCode* c2 = cache_scope[{kind, sig2, expected_arity2}];

  CHECK_NULL(c2);
}

TEST(CacheMissKind) {
  Isolate* isolate = CcTest::InitIsolateOnce();
  auto module = NewModule(isolate);
  TestSignatures sigs;
  WasmCodeRefScope wasm_code_ref_scope;
  WasmImportWrapperCache::ModificationScope cache_scope(
      module->import_wrapper_cache());

  auto kind1 = compiler::WasmImportCallKind::kJSFunctionArityMatch;
  auto kind2 = compiler::WasmImportCallKind::kJSFunctionArityMismatch;
  auto sig = sigs.i_i();
  int expected_arity = static_cast<int>(sig->parameter_count());

  WasmCode* c1 = CompileImportWrapper(isolate->wasm_engine(), module.get(),
                                      isolate->counters(), kind1, sig,
                                      expected_arity, &cache_scope);

  CHECK_NOT_NULL(c1);
  CHECK_EQ(WasmCode::Kind::kWasmToJsWrapper, c1->kind());

  WasmCode* c2 = cache_scope[{kind2, sig, expected_arity}];

  CHECK_NULL(c2);
}

TEST(CacheHitMissSig) {
  Isolate* isolate = CcTest::InitIsolateOnce();
  auto module = NewModule(isolate);
  TestSignatures sigs;
  WasmCodeRefScope wasm_code_ref_scope;
  WasmImportWrapperCache::ModificationScope cache_scope(
      module->import_wrapper_cache());

  auto kind = compiler::WasmImportCallKind::kJSFunctionArityMatch;
  auto sig1 = sigs.i_i();
  int expected_arity1 = static_cast<int>(sig1->parameter_count());
  auto sig2 = sigs.i_ii();
  int expected_arity2 = static_cast<int>(sig2->parameter_count());

  WasmCode* c1 = CompileImportWrapper(isolate->wasm_engine(), module.get(),
                                      isolate->counters(), kind, sig1,
                                      expected_arity1, &cache_scope);

  CHECK_NOT_NULL(c1);
  CHECK_EQ(WasmCode::Kind::kWasmToJsWrapper, c1->kind());

  WasmCode* c2 = cache_scope[{kind, sig2, expected_arity2}];

  CHECK_NULL(c2);

  c2 = CompileImportWrapper(isolate->wasm_engine(), module.get(),
                            isolate->counters(), kind, sig2, expected_arity2,
                            &cache_scope);

  CHECK_NE(c1, c2);

  WasmCode* c3 = cache_scope[{kind, sig1, expected_arity1}];

  CHECK_NOT_NULL(c3);
  CHECK_EQ(c1, c3);

  WasmCode* c4 = cache_scope[{kind, sig2, expected_arity2}];

  CHECK_NOT_NULL(c4);
  CHECK_EQ(c2, c4);
}

}  // namespace test_wasm_import_wrapper_cache
}  // namespace wasm
}  // namespace internal
}  // namespace v8
