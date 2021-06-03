// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_WASM_LIMITS_H_
#define V8_WASM_WASM_LIMITS_H_

#include <cstddef>
#include <cstdint>
#include <limits>

#include "src/base/macros.h"
#include "src/wasm/wasm-constants.h"

namespace v8 {
namespace internal {
namespace wasm {

// This constant is defined in the Wasm JS API spec and as such only
// concern JS embeddings.
constexpr size_t kSpecMaxMemoryPages = 65536;

// The following limits are imposed by V8 on WebAssembly modules.
// The limits are agreed upon with other engines for consistency.
constexpr size_t kV8MaxWasmTypes = 1000000;
constexpr size_t kV8MaxWasmFunctions = 1000000;
constexpr size_t kV8MaxWasmImports = 100000;
constexpr size_t kV8MaxWasmExports = 100000;
constexpr size_t kV8MaxWasmGlobals = 1000000;
constexpr size_t kV8MaxWasmExceptions = 1000000;
constexpr size_t kV8MaxWasmExceptionTypes = 1000000;
constexpr size_t kV8MaxWasmDataSegments = 100000;
// This indicates the maximum memory size our implementation supports.
// Don't use this limit directly; use {max_mem_pages()} instead to take the
// spec'ed limit as well as command line flag into account.
constexpr size_t kV8MaxWasmMemoryPages = 65536;  // = 4 GiB
constexpr size_t kV8MaxWasmStringSize = 100000;
constexpr size_t kV8MaxWasmModuleSize = 1024 * 1024 * 1024;  // = 1 GiB
constexpr size_t kV8MaxWasmFunctionSize = 7654321;
constexpr size_t kV8MaxWasmFunctionLocals = 50000;
constexpr size_t kV8MaxWasmFunctionParams = 1000;
constexpr size_t kV8MaxWasmFunctionReturns = 1000;
constexpr size_t kV8MaxWasmFunctionBrTableSize = 65520;
// Don't use this limit directly, but use the value of FLAG_wasm_max_table_size.
constexpr size_t kV8MaxWasmTableSize = 10000000;
constexpr size_t kV8MaxWasmTableInitEntries = 10000000;
constexpr size_t kV8MaxWasmTables = 1;
constexpr size_t kV8MaxWasmMemories = 1;

// GC proposal. These limits are not standardized yet.
constexpr size_t kV8MaxWasmStructFields = 999;
constexpr uint32_t kV8MaxRttSubtypingDepth = 31;
// Maximum supported by implementation: ((1<<27)-3).
// Reason: total object size in bytes must fit into a Smi, for filler objects.
constexpr size_t kV8MaxWasmArrayLength = 1u << 26;

static_assert(kV8MaxWasmTableSize <= 4294967295,  // 2^32 - 1
              "v8 should not exceed WebAssembly's non-web embedding limits");
static_assert(kV8MaxWasmTableInitEntries <= kV8MaxWasmTableSize,
              "JS-API should not exceed v8's limit");

constexpr uint64_t kWasmMaxHeapOffset =
    static_cast<uint64_t>(
        std::numeric_limits<uint32_t>::max())  // maximum base value
    + std::numeric_limits<uint32_t>::max();    // maximum index value

// Defined in wasm-engine.cc.
// TODO(wasm): Make this size_t for wasm64. Currently the --wasm-max-mem-pages
// flag is only uint32_t.
V8_EXPORT_PRIVATE uint32_t max_mem_pages();
uint32_t max_table_init_entries();
size_t max_module_size();

inline uint64_t max_mem_bytes() {
  return uint64_t{max_mem_pages()} * kWasmPageSize;
}

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_WASM_LIMITS_H_
