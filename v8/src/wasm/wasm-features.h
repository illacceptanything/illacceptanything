// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_WASM_FEATURES_H_
#define V8_WASM_WASM_FEATURES_H_

// The feature flags are declared in their own header.
#include "src/base/enum-set.h"
#include "src/base/macros.h"
#include "src/wasm/wasm-feature-flags.h"

// All features, including features that do not have flags.
#define FOREACH_WASM_FEATURE FOREACH_WASM_FEATURE_FLAG

namespace v8 {
namespace internal {

class Context;
template <typename T>
class Handle;
class Isolate;

namespace wasm {

enum WasmFeature {
#define DECL_FEATURE_ENUM(feat, ...) kFeature_##feat,
  FOREACH_WASM_FEATURE(DECL_FEATURE_ENUM)
#undef DECL_FEATURE_ENUM
};

// Enabled or detected features.
class WasmFeatures : public base::EnumSet<WasmFeature> {
 public:
  constexpr WasmFeatures() = default;
  explicit constexpr WasmFeatures(std::initializer_list<WasmFeature> features)
      : EnumSet(features) {}

  // Simplified getters. Use {has_foo()} instead of {contains(kFeature_foo)}.
#define DECL_FEATURE_GETTER(feat, ...) \
  constexpr bool has_##feat() const { return contains(kFeature_##feat); }
  FOREACH_WASM_FEATURE(DECL_FEATURE_GETTER)
#undef DECL_FEATURE_GETTER

  static constexpr const char* name_for_feature(WasmFeature feature) {
    switch (feature) {
#define NAME(feat, ...)              \
  case WasmFeature::kFeature_##feat: \
    return #feat;
      FOREACH_WASM_FEATURE(NAME)
    }
#undef NAME
  }
  static inline constexpr WasmFeatures All();
  static inline constexpr WasmFeatures None();
  static inline constexpr WasmFeatures ForAsmjs();
  static WasmFeatures FromFlags();
  static V8_EXPORT_PRIVATE WasmFeatures FromIsolate(Isolate*);
  static V8_EXPORT_PRIVATE WasmFeatures FromContext(Isolate*,
                                                    Handle<Context> context);
};

// static
constexpr WasmFeatures WasmFeatures::All() {
#define LIST_FEATURE(feat, ...) kFeature_##feat,
  return WasmFeatures({FOREACH_WASM_FEATURE(LIST_FEATURE)});
#undef LIST_FEATURE
}

// static
constexpr WasmFeatures WasmFeatures::None() { return {}; }

// static
constexpr WasmFeatures WasmFeatures::ForAsmjs() { return {}; }

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_WASM_FEATURES_H_
