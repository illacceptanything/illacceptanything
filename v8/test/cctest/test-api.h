// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_TEST_CCTEST_TEST_API_H_
#define V8_TEST_CCTEST_TEST_API_H_

#include "src/init/v8.h"

#include "src/api/api.h"
#include "src/execution/isolate.h"
#include "src/execution/vm-state.h"
#include "test/cctest/cctest.h"

template <typename T>
static void CheckReturnValue(const T& t, i::Address callback) {
  v8::ReturnValue<v8::Value> rv = t.GetReturnValue();
  i::FullObjectSlot o(*reinterpret_cast<i::Address*>(&rv));
  CHECK_EQ(CcTest::isolate(), t.GetIsolate());
  i::Isolate* isolate = reinterpret_cast<i::Isolate*>(t.GetIsolate());
  CHECK_EQ(t.GetIsolate(), rv.GetIsolate());
  CHECK((*o).IsTheHole(isolate) || (*o).IsUndefined(isolate));
  // Verify reset
  bool is_runtime = (*o).IsTheHole(isolate);
  if (is_runtime) {
    CHECK(rv.Get()->IsUndefined());
  } else {
    i::Handle<i::Object> v = v8::Utils::OpenHandle(*rv.Get());
    CHECK_EQ(*v, *o);
  }
  rv.Set(true);
  CHECK(!(*o).IsTheHole(isolate) && !(*o).IsUndefined(isolate));
  rv.Set(v8::Local<v8::Object>());
  CHECK((*o).IsTheHole(isolate) || (*o).IsUndefined(isolate));
  CHECK_EQ(is_runtime, (*o).IsTheHole(isolate));
  // If CPU profiler is active check that when API callback is invoked
  // VMState is set to EXTERNAL.
  if (isolate->is_profiling()) {
    CHECK_EQ(v8::EXTERNAL, isolate->current_vm_state());
    CHECK(isolate->external_callback_scope());
    CHECK_EQ(callback, isolate->external_callback_scope()->callback());
  }
}

template <typename T>
static void CheckInternalFieldsAreZero(v8::Local<T> value) {
  CHECK_EQ(T::kInternalFieldCount, value->InternalFieldCount());
  for (int i = 0; i < value->InternalFieldCount(); i++) {
    CHECK_EQ(0, value->GetInternalField(i)
                    ->Int32Value(CcTest::isolate()->GetCurrentContext())
                    .FromJust());
  }
}

template <typename T>
struct ConvertJSValue {
  static v8::Maybe<T> Get(v8::Local<v8::Value> value,
                          v8::Local<v8::Context> context);
};

template <>
struct ConvertJSValue<int32_t> {
  static v8::Maybe<int32_t> Get(v8::Local<v8::Value> value,
                                v8::Local<v8::Context> context) {
    return value->Int32Value(context);
  }
};

template <>
struct ConvertJSValue<uint32_t> {
  static v8::Maybe<uint32_t> Get(v8::Local<v8::Value> value,
                                 v8::Local<v8::Context> context) {
    return value->Uint32Value(context);
  }
};

// NaNs and +/-Infinity should be 0, otherwise (modulo 2^64) - 2^63.
// Step 8 - 12 of https://heycam.github.io/webidl/#abstract-opdef-converttoint
// The int64_t and uint64_t implementations below are copied from Blink:
// https://source.chromium.org/chromium/chromium/src/+/master:third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h;l=249?q=doubletointeger&sq=&ss=chromium%2Fchromium%2Fsrc
template <>
struct ConvertJSValue<int64_t> {
  static v8::Maybe<int64_t> Get(v8::Local<v8::Value> value,
                                v8::Local<v8::Context> context) {
    v8::Maybe<double> double_value = value->NumberValue(context);
    if (!double_value.IsJust()) {
      return v8::Nothing<int64_t>();
    }
    double result = double_value.ToChecked();
    if (std::isinf(result) || std::isnan(result)) {
      return v8::Just(int64_t(0));
    }
    result = trunc(result);

    constexpr uint64_t kMaxULL = std::numeric_limits<uint64_t>::max();

    // -2^{64} < fmod_value < 2^{64}.
    double fmod_value = fmod(result, kMaxULL + 1.0);
    if (fmod_value >= 0) {
      if (fmod_value < pow(2, 63)) {
        // 0 <= fmod_value < 2^{63}.
        // 0 <= value < 2^{63}. This cast causes no loss.
        return v8::Just(static_cast<int64_t>(fmod_value));
      } else {
        // 2^{63} <= fmod_value < 2^{64}.
        // 2^{63} <= value < 2^{64}. This cast causes no loss.
        return v8::Just(static_cast<int64_t>(fmod_value - pow(2, 64)));
      }
    }
    // -2^{64} < fmod_value < 0.
    // 0 < fmod_value_uint64 < 2^{64}. This cast causes no loss.
    uint64_t fmod_value_uint64 = static_cast<uint64_t>(-fmod_value);
    // -1 < (kMaxULL - fmod_value_uint64) < 2^{64} - 1.
    // 0 < value < 2^{64}.
    return v8::Just(static_cast<int64_t>(kMaxULL - fmod_value_uint64 + 1));
  }
};

template <>
struct ConvertJSValue<uint64_t> {
  static v8::Maybe<uint64_t> Get(v8::Local<v8::Value> value,
                                 v8::Local<v8::Context> context) {
    v8::Maybe<double> double_value = value->NumberValue(context);
    if (!double_value.IsJust()) {
      return v8::Nothing<uint64_t>();
    }
    double result = double_value.ToChecked();
    if (std::isinf(result) || std::isnan(result)) {
      return v8::Just(uint64_t(0));
    }
    result = trunc(result);

    constexpr uint64_t kMaxULL = std::numeric_limits<uint64_t>::max();

    // -2^{64} < fmod_value < 2^{64}.
    double fmod_value = fmod(result, kMaxULL + 1.0);
    if (fmod_value >= 0) {
      return v8::Just(static_cast<uint64_t>(fmod_value));
    }
    // -2^{64} < fmod_value < 0.
    // 0 < fmod_value_uint64 < 2^{64}. This cast causes no loss.
    uint64_t fmod_value_uint64 = static_cast<uint64_t>(-fmod_value);
    // -1 < (kMaxULL - fmod_value_uint64) < 2^{64} - 1.
    // 0 < value < 2^{64}.
    return v8::Just(static_cast<uint64_t>(kMaxULL - fmod_value_uint64 + 1));
  }
};

template <>
struct ConvertJSValue<v8::BigInt> {
  static v8::Maybe<v8::Local<v8::BigInt>> Get(v8::Local<v8::Value> value,
                                              v8::Local<v8::Context> context) {
    if (value->IsBigInt()) {
      return v8::Just(value.As<v8::BigInt>());
    }
    return v8::Nothing<v8::Local<v8::BigInt>>();
  }
};

template <>
struct ConvertJSValue<float> {
  static v8::Maybe<float> Get(v8::Local<v8::Value> value,
                              v8::Local<v8::Context> context) {
    v8::Maybe<double> val = value->NumberValue(context);
    if (val.IsNothing()) return v8::Nothing<float>();
    return v8::Just(static_cast<float>(val.ToChecked()));
  }
};

template <>
struct ConvertJSValue<double> {
  static v8::Maybe<double> Get(v8::Local<v8::Value> value,
                               v8::Local<v8::Context> context) {
    return value->NumberValue(context);
  }
};

template <>
struct ConvertJSValue<bool> {
  static v8::Maybe<bool> Get(v8::Local<v8::Value> value,
                             v8::Local<v8::Context> context) {
    return v8::Just<bool>(value->BooleanValue(CcTest::isolate()));
  }
};

#endif  // V8_TEST_CCTEST_TEST_API_H_
