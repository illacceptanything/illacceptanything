// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/codegen/external-reference-table.h"

#include "src/builtins/accessors.h"
#include "src/codegen/external-reference.h"
#include "src/ic/stub-cache.h"
#include "src/logging/counters.h"

#if defined(DEBUG) && defined(V8_OS_LINUX) && !defined(V8_OS_ANDROID)
#define SYMBOLIZE_FUNCTION
#include <execinfo.h>

#include <vector>

#include "src/base/platform/wrappers.h"
#endif  // DEBUG && V8_OS_LINUX && !V8_OS_ANDROID

namespace v8 {
namespace internal {

#define ADD_EXT_REF_NAME(name, desc) desc,
#define ADD_BUILTIN_NAME(Name, ...) "Builtin_" #Name,
#define ADD_RUNTIME_FUNCTION(name, ...) "Runtime::" #name,
#define ADD_ISOLATE_ADDR(Name, name) "Isolate::" #name "_address",
#define ADD_ACCESSOR_INFO_NAME(_, __, AccessorName, ...) \
  "Accessors::" #AccessorName "Getter",
#define ADD_ACCESSOR_SETTER_NAME(name) "Accessors::" #name,
#define ADD_STATS_COUNTER_NAME(name, ...) "StatsCounter::" #name,
// static
// clang-format off
const char* const
    ExternalReferenceTable::ref_name_[ExternalReferenceTable::kSize] = {
        // === Isolate independent ===
        // Special references:
        "nullptr",
        // External references (without isolate):
        EXTERNAL_REFERENCE_LIST(ADD_EXT_REF_NAME)
        // Builtins:
        BUILTIN_LIST_C(ADD_BUILTIN_NAME)
        // Runtime functions:
        FOR_EACH_INTRINSIC(ADD_RUNTIME_FUNCTION)
        // Accessors:
        ACCESSOR_INFO_LIST_GENERATOR(ADD_ACCESSOR_INFO_NAME, /* not used */)
        ACCESSOR_SETTER_LIST(ADD_ACCESSOR_SETTER_NAME)

        // === Isolate dependent ===
        // External references (with isolate):
        EXTERNAL_REFERENCE_LIST_WITH_ISOLATE(ADD_EXT_REF_NAME)
        // Isolate addresses:
        FOR_EACH_ISOLATE_ADDRESS_NAME(ADD_ISOLATE_ADDR)
        // Stub cache:
        "Load StubCache::primary_->key",
        "Load StubCache::primary_->value",
        "Load StubCache::primary_->map",
        "Load StubCache::secondary_->key",
        "Load StubCache::secondary_->value",
        "Load StubCache::secondary_->map",
        "Store StubCache::primary_->key",
        "Store StubCache::primary_->value",
        "Store StubCache::primary_->map",
        "Store StubCache::secondary_->key",
        "Store StubCache::secondary_->value",
        "Store StubCache::secondary_->map",
        // Native code counters:
        STATS_COUNTER_NATIVE_CODE_LIST(ADD_STATS_COUNTER_NAME)
};
// clang-format on
#undef ADD_EXT_REF_NAME
#undef ADD_BUILTIN_NAME
#undef ADD_RUNTIME_FUNCTION
#undef ADD_ISOLATE_ADDR
#undef ADD_ACCESSOR_INFO_NAME
#undef ADD_ACCESSOR_SETTER_NAME
#undef ADD_STATS_COUNTER_NAME

namespace {
static Address ref_addr_isolate_independent_
    [ExternalReferenceTable::kSizeIsolateIndependent] = {0};
}  // namespace

// Forward declarations for C++ builtins.
#define FORWARD_DECLARE(Name) \
  Address Builtin_##Name(int argc, Address* args, Isolate* isolate);
BUILTIN_LIST_C(FORWARD_DECLARE)
#undef FORWARD_DECLARE

void ExternalReferenceTable::Init(Isolate* isolate) {
  int index = 0;

  CopyIsolateIndependentReferences(&index);

  AddIsolateDependentReferences(isolate, &index);
  AddIsolateAddresses(isolate, &index);
  AddStubCache(isolate, &index);
  AddNativeCodeStatsCounters(isolate, &index);
  is_initialized_ = static_cast<uint32_t>(true);

  CHECK_EQ(kSize, index);
}

const char* ExternalReferenceTable::ResolveSymbol(void* address) {
#ifdef SYMBOLIZE_FUNCTION
  char** names = backtrace_symbols(&address, 1);
  const char* name = names[0];
  // The array of names is malloc'ed. However, each name string is static
  // and do not need to be freed.
  base::Free(names);
  return name;
#else
  return "<unresolved>";
#endif  // SYMBOLIZE_FUNCTION
}

void ExternalReferenceTable::InitializeOncePerProcess() {
  int index = 0;

  // kNullAddress is preserved through serialization/deserialization.
  AddIsolateIndependent(kNullAddress, &index);
  AddIsolateIndependentReferences(&index);
  AddBuiltins(&index);
  AddRuntimeFunctions(&index);
  AddAccessors(&index);

  CHECK_EQ(kSizeIsolateIndependent, index);
}

const char* ExternalReferenceTable::NameOfIsolateIndependentAddress(
    Address address) {
  for (int i = 0; i < kSizeIsolateIndependent; i++) {
    if (ref_addr_isolate_independent_[i] == address) {
      return ref_name_[i];
    }
  }
  return "<unknown>";
}

void ExternalReferenceTable::Add(Address address, int* index) {
  ref_addr_[(*index)++] = address;
}

void ExternalReferenceTable::AddIsolateIndependent(Address address,
                                                   int* index) {
  ref_addr_isolate_independent_[(*index)++] = address;
}

void ExternalReferenceTable::AddIsolateIndependentReferences(int* index) {
  CHECK_EQ(kSpecialReferenceCount, *index);

#define ADD_EXTERNAL_REFERENCE(name, desc) \
  AddIsolateIndependent(ExternalReference::name().address(), index);
  EXTERNAL_REFERENCE_LIST(ADD_EXTERNAL_REFERENCE)
#undef ADD_EXTERNAL_REFERENCE

  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent,
           *index);
}

void ExternalReferenceTable::AddIsolateDependentReferences(Isolate* isolate,
                                                           int* index) {
  CHECK_EQ(kSizeIsolateIndependent, *index);

#define ADD_EXTERNAL_REFERENCE(name, desc) \
  Add(ExternalReference::name(isolate).address(), index);
  EXTERNAL_REFERENCE_LIST_WITH_ISOLATE(ADD_EXTERNAL_REFERENCE)
#undef ADD_EXTERNAL_REFERENCE

  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent,
           *index);
}

void ExternalReferenceTable::AddBuiltins(int* index) {
  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent,
           *index);

  static const Address c_builtins[] = {
#define DEF_ENTRY(Name, ...) FUNCTION_ADDR(&Builtin_##Name),
      BUILTIN_LIST_C(DEF_ENTRY)
#undef DEF_ENTRY
  };
  for (Address addr : c_builtins) {
    AddIsolateIndependent(ExternalReference::Create(addr).address(), index);
  }

  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent +
               kBuiltinsReferenceCount,
           *index);
}

void ExternalReferenceTable::AddRuntimeFunctions(int* index) {
  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent +
               kBuiltinsReferenceCount,
           *index);

  static constexpr Runtime::FunctionId runtime_functions[] = {
#define RUNTIME_ENTRY(name, ...) Runtime::k##name,
      FOR_EACH_INTRINSIC(RUNTIME_ENTRY)
#undef RUNTIME_ENTRY
  };

  for (Runtime::FunctionId fId : runtime_functions) {
    AddIsolateIndependent(ExternalReference::Create(fId).address(), index);
  }

  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent +
               kBuiltinsReferenceCount + kRuntimeReferenceCount,
           *index);
}

void ExternalReferenceTable::CopyIsolateIndependentReferences(int* index) {
  CHECK_EQ(0, *index);

  std::copy(ref_addr_isolate_independent_,
            ref_addr_isolate_independent_ + kSizeIsolateIndependent, ref_addr_);
  *index += kSizeIsolateIndependent;
}

void ExternalReferenceTable::AddIsolateAddresses(Isolate* isolate, int* index) {
  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent,
           *index);

  for (int i = 0; i < IsolateAddressId::kIsolateAddressCount; ++i) {
    Add(isolate->get_address_from_id(static_cast<IsolateAddressId>(i)), index);
  }

  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent +
               kIsolateAddressReferenceCount,
           *index);
}

void ExternalReferenceTable::AddAccessors(int* index) {
  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent +
               kBuiltinsReferenceCount + kRuntimeReferenceCount,
           *index);

  static const Address accessors[] = {
  // Getters:
#define ACCESSOR_INFO_DECLARATION(_, __, AccessorName, ...) \
  FUNCTION_ADDR(&Accessors::AccessorName##Getter),
      ACCESSOR_INFO_LIST_GENERATOR(ACCESSOR_INFO_DECLARATION, /* not used */)
#undef ACCESSOR_INFO_DECLARATION
  // Setters:
#define ACCESSOR_SETTER_DECLARATION(name) FUNCTION_ADDR(&Accessors::name),
          ACCESSOR_SETTER_LIST(ACCESSOR_SETTER_DECLARATION)
#undef ACCESSOR_SETTER_DECLARATION
  };

  for (Address addr : accessors) {
    AddIsolateIndependent(addr, index);
  }

  CHECK_EQ(kSpecialReferenceCount + kExternalReferenceCountIsolateIndependent +
               kBuiltinsReferenceCount + kRuntimeReferenceCount +
               kAccessorReferenceCount,
           *index);
}

void ExternalReferenceTable::AddStubCache(Isolate* isolate, int* index) {
  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent +
               kIsolateAddressReferenceCount,
           *index);

  StubCache* load_stub_cache = isolate->load_stub_cache();

  // Stub cache tables
  Add(load_stub_cache->key_reference(StubCache::kPrimary).address(), index);
  Add(load_stub_cache->value_reference(StubCache::kPrimary).address(), index);
  Add(load_stub_cache->map_reference(StubCache::kPrimary).address(), index);
  Add(load_stub_cache->key_reference(StubCache::kSecondary).address(), index);
  Add(load_stub_cache->value_reference(StubCache::kSecondary).address(), index);
  Add(load_stub_cache->map_reference(StubCache::kSecondary).address(), index);

  StubCache* store_stub_cache = isolate->store_stub_cache();

  // Stub cache tables
  Add(store_stub_cache->key_reference(StubCache::kPrimary).address(), index);
  Add(store_stub_cache->value_reference(StubCache::kPrimary).address(), index);
  Add(store_stub_cache->map_reference(StubCache::kPrimary).address(), index);
  Add(store_stub_cache->key_reference(StubCache::kSecondary).address(), index);
  Add(store_stub_cache->value_reference(StubCache::kSecondary).address(),
      index);
  Add(store_stub_cache->map_reference(StubCache::kSecondary).address(), index);

  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent +
               kIsolateAddressReferenceCount + kStubCacheReferenceCount,
           *index);
}

Address ExternalReferenceTable::GetStatsCounterAddress(StatsCounter* counter) {
  int* address = counter->Enabled()
                     ? counter->GetInternalPointer()
                     : reinterpret_cast<int*>(&dummy_stats_counter_);
  return reinterpret_cast<Address>(address);
}

void ExternalReferenceTable::AddNativeCodeStatsCounters(Isolate* isolate,
                                                        int* index) {
  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent +
               kIsolateAddressReferenceCount + kStubCacheReferenceCount,
           *index);

  Counters* counters = isolate->counters();

#define SC(name, caption) Add(GetStatsCounterAddress(counters->name()), index);
  STATS_COUNTER_NATIVE_CODE_LIST(SC)
#undef SC

  CHECK_EQ(kSizeIsolateIndependent + kExternalReferenceCountIsolateDependent +
               kIsolateAddressReferenceCount + kStubCacheReferenceCount +
               kStatsCountersReferenceCount,
           *index);
  CHECK_EQ(kSize, *index);
}

}  // namespace internal
}  // namespace v8

#undef SYMBOLIZE_FUNCTION
