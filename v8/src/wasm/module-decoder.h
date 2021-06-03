// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_MODULE_DECODER_H_
#define V8_WASM_MODULE_DECODER_H_

#include <memory>

#include "src/common/globals.h"
#include "src/logging/metrics.h"
#include "src/wasm/function-body-decoder.h"
#include "src/wasm/wasm-constants.h"
#include "src/wasm/wasm-features.h"
#include "src/wasm/wasm-module.h"
#include "src/wasm/wasm-result.h"

namespace v8 {
namespace internal {

class Counters;

namespace wasm {

struct CompilationEnv;

inline bool IsValidSectionCode(uint8_t byte) {
  return kTypeSectionCode <= byte && byte <= kLastKnownModuleSection;
}

const char* SectionName(SectionCode code);

using ModuleResult = Result<std::shared_ptr<WasmModule>>;
using FunctionResult = Result<std::unique_ptr<WasmFunction>>;
using FunctionOffsets = std::vector<std::pair<int, int>>;
using FunctionOffsetsResult = Result<FunctionOffsets>;

struct AsmJsOffsetEntry {
  int byte_offset;
  int source_position_call;
  int source_position_number_conversion;
};
struct AsmJsOffsetFunctionEntries {
  int start_offset;
  int end_offset;
  std::vector<AsmJsOffsetEntry> entries;
};
struct AsmJsOffsets {
  std::vector<AsmJsOffsetFunctionEntries> functions;
};
using AsmJsOffsetsResult = Result<AsmJsOffsets>;

// The class names "NameAssoc", "NameMap", and "IndirectNameMap" match
// the terms used by the spec:
// https://webassembly.github.io/spec/core/bikeshed/index.html#name-section%E2%91%A0
class NameAssoc {
 public:
  NameAssoc(int index, WireBytesRef name) : index_(index), name_(name) {}

  int index() const { return index_; }
  WireBytesRef name() const { return name_; }

  struct IndexLess {
    bool operator()(const NameAssoc& a, const NameAssoc& b) const {
      return a.index() < b.index();
    }
  };

 private:
  int index_;
  WireBytesRef name_;
};

class NameMap {
 public:
  // For performance reasons, {NameMap} should not be copied.
  MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(NameMap);

  explicit NameMap(std::vector<NameAssoc> names) : names_(std::move(names)) {
    DCHECK(
        std::is_sorted(names_.begin(), names_.end(), NameAssoc::IndexLess{}));
  }

  WireBytesRef GetName(int index) {
    auto it = std::lower_bound(names_.begin(), names_.end(),
                               NameAssoc{index, {}}, NameAssoc::IndexLess{});
    if (it == names_.end()) return {};
    if (it->index() != index) return {};
    return it->name();
  }

 private:
  std::vector<NameAssoc> names_;
};

class IndirectNameMapEntry : public NameMap {
 public:
  // For performance reasons, {IndirectNameMapEntry} should not be copied.
  MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(IndirectNameMapEntry);

  IndirectNameMapEntry(int index, std::vector<NameAssoc> names)
      : NameMap(std::move(names)), index_(index) {}

  int index() const { return index_; }

  struct IndexLess {
    bool operator()(const IndirectNameMapEntry& a,
                    const IndirectNameMapEntry& b) const {
      return a.index() < b.index();
    }
  };

 private:
  int index_;
};

class IndirectNameMap {
 public:
  // For performance reasons, {IndirectNameMap} should not be copied.
  MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(IndirectNameMap);

  explicit IndirectNameMap(std::vector<IndirectNameMapEntry> functions)
      : functions_(std::move(functions)) {
    DCHECK(std::is_sorted(functions_.begin(), functions_.end(),
                          IndirectNameMapEntry::IndexLess{}));
  }

  WireBytesRef GetName(int function_index, int local_index) {
    auto it = std::lower_bound(functions_.begin(), functions_.end(),
                               IndirectNameMapEntry{function_index, {}},
                               IndirectNameMapEntry::IndexLess{});
    if (it == functions_.end()) return {};
    if (it->index() != function_index) return {};
    return it->GetName(local_index);
  }

 private:
  std::vector<IndirectNameMapEntry> functions_;
};

enum class DecodingMethod {
  kSync,
  kAsync,
  kSyncStream,
  kAsyncStream,
  kDeserialize
};

// Decodes the bytes of a wasm module between {module_start} and {module_end}.
V8_EXPORT_PRIVATE ModuleResult DecodeWasmModule(
    const WasmFeatures& enabled, const byte* module_start,
    const byte* module_end, bool verify_functions, ModuleOrigin origin,
    Counters* counters, std::shared_ptr<metrics::Recorder> metrics_recorder,
    v8::metrics::Recorder::ContextId context_id, DecodingMethod decoding_method,
    AccountingAllocator* allocator);

// Exposed for testing. Decodes a single function signature, allocating it
// in the given zone. Returns {nullptr} upon failure.
V8_EXPORT_PRIVATE const FunctionSig* DecodeWasmSignatureForTesting(
    const WasmFeatures& enabled, Zone* zone, const byte* start,
    const byte* end);

// Decodes the bytes of a wasm function between
// {function_start} and {function_end}.
V8_EXPORT_PRIVATE FunctionResult DecodeWasmFunctionForTesting(
    const WasmFeatures& enabled, Zone* zone, const ModuleWireBytes& wire_bytes,
    const WasmModule* module, const byte* function_start,
    const byte* function_end, Counters* counters);

V8_EXPORT_PRIVATE WasmInitExpr DecodeWasmInitExprForTesting(
    const WasmFeatures& enabled, const byte* start, const byte* end);

struct CustomSectionOffset {
  WireBytesRef section;
  WireBytesRef name;
  WireBytesRef payload;
};

V8_EXPORT_PRIVATE std::vector<CustomSectionOffset> DecodeCustomSections(
    const byte* start, const byte* end);

// Extracts the mapping from wasm byte offset to asm.js source position per
// function.
AsmJsOffsetsResult DecodeAsmJsOffsets(Vector<const uint8_t> encoded_offsets);

// Decode the function names from the name section. Returns the result as an
// unordered map. Only names with valid utf8 encoding are stored and conflicts
// are resolved by choosing the last name read.
void DecodeFunctionNames(const byte* module_start, const byte* module_end,
                         std::unordered_map<uint32_t, WireBytesRef>* names);

// Decode the requested subsection of the name section.
// The result will be empty if no name section is present. On encountering an
// error in the name section, returns all information decoded up to the first
// error.
NameMap DecodeNameMap(Vector<const uint8_t> module_bytes,
                      uint8_t name_section_kind);
IndirectNameMap DecodeIndirectNameMap(Vector<const uint8_t> module_bytes,
                                      uint8_t name_section_kind);

class ModuleDecoderImpl;

class ModuleDecoder {
 public:
  explicit ModuleDecoder(const WasmFeatures& enabled);
  ~ModuleDecoder();

  void StartDecoding(Counters* counters,
                     std::shared_ptr<metrics::Recorder> metrics_recorder,
                     v8::metrics::Recorder::ContextId context_id,
                     AccountingAllocator* allocator,
                     ModuleOrigin origin = ModuleOrigin::kWasmOrigin);

  void DecodeModuleHeader(Vector<const uint8_t> bytes, uint32_t offset);

  void DecodeSection(SectionCode section_code, Vector<const uint8_t> bytes,
                     uint32_t offset, bool verify_functions = true);

  void StartCodeSection();

  bool CheckFunctionsCount(uint32_t functions_count, uint32_t error_offset);

  void DecodeFunctionBody(uint32_t index, uint32_t size, uint32_t offset,
                          bool verify_functions = true);

  ModuleResult FinishDecoding(bool verify_functions = true);

  void set_code_section(uint32_t offset, uint32_t size);

  const std::shared_ptr<WasmModule>& shared_module() const;

  WasmModule* module() const { return shared_module().get(); }

  bool ok();

  // Translates the unknown section that decoder is pointing to to an extended
  // SectionCode if the unknown section is known to decoder.
  // The decoder is expected to point after the section length and just before
  // the identifier string of the unknown section.
  // The return value is the number of bytes that were consumed.
  static size_t IdentifyUnknownSection(ModuleDecoder* decoder,
                                       Vector<const uint8_t> bytes,
                                       uint32_t offset, SectionCode* result);

 private:
  const WasmFeatures enabled_features_;
  std::unique_ptr<ModuleDecoderImpl> impl_;
};

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_MODULE_DECODER_H_
