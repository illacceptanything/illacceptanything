// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/api/api-inl.h"
#include "src/init/v8.h"

#include "src/wasm/streaming-decoder.h"
#include "src/wasm/wasm-code-manager.h"
#include "src/wasm/wasm-engine.h"
#include "src/wasm/wasm-module-builder.h"

#include "test/cctest/cctest.h"

#include "test/common/wasm/test-signatures.h"
#include "test/common/wasm/wasm-macro-gen.h"

namespace v8 {
namespace internal {
namespace wasm {

namespace {

class TestResolver : public CompilationResultResolver {
 public:
  explicit TestResolver(std::atomic<int>* pending)
      : native_module_(nullptr), pending_(pending) {}

  void OnCompilationSucceeded(i::Handle<i::WasmModuleObject> module) override {
    if (!module.is_null()) {
      native_module_ = module->shared_native_module();
      pending_->fetch_sub(1);
    }
  }

  void OnCompilationFailed(i::Handle<i::Object> error_reason) override {
    CHECK(false);
  }

  std::shared_ptr<NativeModule> native_module() { return native_module_; }

 private:
  std::shared_ptr<NativeModule> native_module_;
  std::atomic<int>* pending_;
};

class StreamTester {
 public:
  explicit StreamTester(std::shared_ptr<TestResolver> test_resolver)
      : internal_scope_(CcTest::i_isolate()), test_resolver_(test_resolver) {
    i::Isolate* i_isolate = CcTest::i_isolate();

    Handle<Context> context = i_isolate->native_context();

    stream_ = i_isolate->wasm_engine()->StartStreamingCompilation(
        i_isolate, WasmFeatures::All(), context,
        "WebAssembly.compileStreaming()", test_resolver_);
  }

  void OnBytesReceived(const uint8_t* start, size_t length) {
    stream_->OnBytesReceived(Vector<const uint8_t>(start, length));
  }

  void FinishStream() { stream_->Finish(); }

  void SetCompiledModuleBytes(const uint8_t* start, size_t length) {
    stream_->SetCompiledModuleBytes(Vector<const uint8_t>(start, length));
  }

 private:
  i::HandleScope internal_scope_;
  std::shared_ptr<StreamingDecoder> stream_;
  std::shared_ptr<TestResolver> test_resolver_;
};

// Create a valid module such that the bytes depend on {n}.
ZoneBuffer GetValidModuleBytes(Zone* zone, int n) {
  ZoneBuffer buffer(zone);
  TestSignatures sigs;
  WasmModuleBuilder builder(zone);
  {
    WasmFunctionBuilder* f = builder.AddFunction(sigs.v_v());
    uint8_t code[] = {kExprI32Const, n, kExprDrop, kExprEnd};
    f->EmitCode(code, arraysize(code));
  }
  builder.WriteTo(&buffer);
  return buffer;
}

std::shared_ptr<NativeModule> SyncCompile(Vector<const uint8_t> bytes) {
  ErrorThrower thrower(CcTest::i_isolate(), "Test");
  auto enabled_features = WasmFeatures::FromIsolate(CcTest::i_isolate());
  auto wire_bytes = ModuleWireBytes(bytes.begin(), bytes.end());
  Handle<WasmModuleObject> module =
      CcTest::i_isolate()
          ->wasm_engine()
          ->SyncCompile(CcTest::i_isolate(), enabled_features, &thrower,
                        wire_bytes)
          .ToHandleChecked();
  return module->shared_native_module();
}

// Shared prefix.
constexpr uint8_t kPrefix[] = {
    WASM_MODULE_HEADER,                // module header
    kTypeSectionCode,                  // section code
    U32V_1(1 + SIZEOF_SIG_ENTRY_v_v),  // section size
    U32V_1(1),                         // type count
    SIG_ENTRY_v_v,                     // signature entry
    kFunctionSectionCode,              // section code
    U32V_1(2),                         // section size
    U32V_1(1),                         // functions count
    0,                                 // signature index
    kCodeSectionCode,                  // section code
    U32V_1(7),                         // section size
    U32V_1(1),                         // functions count
    5,                                 // body size
};

constexpr uint8_t kFunctionA[] = {
    U32V_1(0), kExprI32Const, U32V_1(0), kExprDrop, kExprEnd,
};
constexpr uint8_t kFunctionB[] = {
    U32V_1(0), kExprI32Const, U32V_1(1), kExprDrop, kExprEnd,
};

constexpr size_t kPrefixSize = arraysize(kPrefix);
constexpr size_t kFunctionSize = arraysize(kFunctionA);

}  // namespace

TEST(TestAsyncCache) {
  CcTest::InitializeVM();
  i::HandleScope internal_scope(CcTest::i_isolate());
  AccountingAllocator allocator;
  Zone zone(&allocator, "CompilationCacheTester");

  auto bufferA = GetValidModuleBytes(&zone, 0);
  auto bufferB = GetValidModuleBytes(&zone, 1);

  std::atomic<int> pending(3);
  auto resolverA1 = std::make_shared<TestResolver>(&pending);
  auto resolverA2 = std::make_shared<TestResolver>(&pending);
  auto resolverB = std::make_shared<TestResolver>(&pending);

  CcTest::i_isolate()->wasm_engine()->AsyncCompile(
      CcTest::i_isolate(), WasmFeatures::All(), resolverA1,
      ModuleWireBytes(bufferA.begin(), bufferA.end()), true,
      "WebAssembly.compile");
  CcTest::i_isolate()->wasm_engine()->AsyncCompile(
      CcTest::i_isolate(), WasmFeatures::All(), resolverA2,
      ModuleWireBytes(bufferA.begin(), bufferA.end()), true,
      "WebAssembly.compile");
  CcTest::i_isolate()->wasm_engine()->AsyncCompile(
      CcTest::i_isolate(), WasmFeatures::All(), resolverB,
      ModuleWireBytes(bufferB.begin(), bufferB.end()), true,
      "WebAssembly.compile");

  while (pending > 0) {
    v8::platform::PumpMessageLoop(i::V8::GetCurrentPlatform(),
                                  CcTest::isolate());
  }

  CHECK_EQ(resolverA1->native_module(), resolverA2->native_module());
  CHECK_NE(resolverA1->native_module(), resolverB->native_module());
}

TEST(TestStreamingCache) {
  CcTest::InitializeVM();

  std::atomic<int> pending(3);
  auto resolverA1 = std::make_shared<TestResolver>(&pending);
  auto resolverA2 = std::make_shared<TestResolver>(&pending);
  auto resolverB = std::make_shared<TestResolver>(&pending);

  StreamTester testerA1(resolverA1);
  StreamTester testerA2(resolverA2);
  StreamTester testerB(resolverB);

  // Start receiving kPrefix bytes.
  testerA1.OnBytesReceived(kPrefix, kPrefixSize);
  testerA2.OnBytesReceived(kPrefix, kPrefixSize);
  testerB.OnBytesReceived(kPrefix, kPrefixSize);

  // Receive function bytes and start streaming compilation.
  testerA1.OnBytesReceived(kFunctionA, kFunctionSize);
  testerA1.FinishStream();
  testerA2.OnBytesReceived(kFunctionA, kFunctionSize);
  testerA2.FinishStream();
  testerB.OnBytesReceived(kFunctionB, kFunctionSize);
  testerB.FinishStream();

  while (pending > 0) {
    v8::platform::PumpMessageLoop(i::V8::GetCurrentPlatform(),
                                  CcTest::isolate());
  }

  std::shared_ptr<NativeModule> native_module_A1 = resolverA1->native_module();
  std::shared_ptr<NativeModule> native_module_A2 = resolverA2->native_module();
  std::shared_ptr<NativeModule> native_module_B = resolverB->native_module();
  CHECK_EQ(native_module_A1, native_module_A2);
  CHECK_NE(native_module_A1, native_module_B);
}

TEST(TestStreamingAndSyncCache) {
  CcTest::InitializeVM();

  std::atomic<int> pending(1);
  auto resolver = std::make_shared<TestResolver>(&pending);
  StreamTester tester(resolver);

  tester.OnBytesReceived(kPrefix, kPrefixSize);

  // Compile the same module synchronously to make sure we don't deadlock
  // waiting for streaming compilation to finish.
  auto full_bytes = OwnedVector<uint8_t>::New(kPrefixSize + kFunctionSize);
  memcpy(full_bytes.begin(), kPrefix, kPrefixSize);
  memcpy(full_bytes.begin() + kPrefixSize, kFunctionA, kFunctionSize);
  auto native_module_sync = SyncCompile(full_bytes.as_vector());

  // Streaming compilation should just discard its native module now and use the
  // one inserted in the cache by sync compilation.
  tester.OnBytesReceived(kFunctionA, kFunctionSize);
  tester.FinishStream();

  while (pending > 0) {
    v8::platform::PumpMessageLoop(i::V8::GetCurrentPlatform(),
                                  CcTest::isolate());
  }

  std::shared_ptr<NativeModule> native_module_streaming =
      resolver->native_module();
  CHECK_EQ(native_module_streaming, native_module_sync);
}

}  // namespace wasm
}  // namespace internal
}  // namespace v8
