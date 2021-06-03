// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_MODULE_INL_H_
#define V8_OBJECTS_MODULE_INL_H_

#include "src/objects/module.h"
#include "src/objects/objects-inl.h"  // Needed for write barriers
#include "src/objects/scope-info.h"
#include "src/objects/source-text-module-inl.h"
#include "src/objects/source-text-module.h"
#include "src/objects/string-inl.h"
#include "src/objects/synthetic-module.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/module-tq-inl.inc"

OBJECT_CONSTRUCTORS_IMPL(Module, HeapObject)
TQ_OBJECT_CONSTRUCTORS_IMPL(JSModuleNamespace)

NEVER_READ_ONLY_SPACE_IMPL(Module)
NEVER_READ_ONLY_SPACE_IMPL(ModuleRequest)
NEVER_READ_ONLY_SPACE_IMPL(SourceTextModule)
NEVER_READ_ONLY_SPACE_IMPL(SyntheticModule)

CAST_ACCESSOR(Module)
ACCESSORS(Module, exports, ObjectHashTable, kExportsOffset)
ACCESSORS(Module, module_namespace, HeapObject, kModuleNamespaceOffset)
ACCESSORS(Module, exception, Object, kExceptionOffset)
ACCESSORS(Module, top_level_capability, HeapObject, kTopLevelCapabilityOffset)
SMI_ACCESSORS(Module, status, kStatusOffset)
SMI_ACCESSORS(Module, hash, kHashOffset)

BOOL_ACCESSORS(SourceTextModule, flags, async, AsyncBit::kShift)
BIT_FIELD_ACCESSORS(SourceTextModule, flags, async_evaluating_ordinal,
                    SourceTextModule::AsyncEvaluatingOrdinalBits)
ACCESSORS(SourceTextModule, async_parent_modules, ArrayList,
          kAsyncParentModulesOffset)

struct Module::Hash {
  V8_INLINE size_t operator()(Module const& module) const {
    return module.hash();
  }
};

SourceTextModuleInfo SourceTextModule::info() const {
  return GetSharedFunctionInfo().scope_info().ModuleDescriptorInfo();
}

OBJECT_CONSTRUCTORS_IMPL(SourceTextModuleInfo, FixedArray)
CAST_ACCESSOR(SourceTextModuleInfo)

FixedArray SourceTextModuleInfo::module_requests() const {
  return FixedArray::cast(get(kModuleRequestsIndex));
}

FixedArray SourceTextModuleInfo::special_exports() const {
  return FixedArray::cast(get(kSpecialExportsIndex));
}

FixedArray SourceTextModuleInfo::regular_exports() const {
  return FixedArray::cast(get(kRegularExportsIndex));
}

FixedArray SourceTextModuleInfo::regular_imports() const {
  return FixedArray::cast(get(kRegularImportsIndex));
}

FixedArray SourceTextModuleInfo::namespace_imports() const {
  return FixedArray::cast(get(kNamespaceImportsIndex));
}

#ifdef DEBUG
bool SourceTextModuleInfo::Equals(SourceTextModuleInfo other) const {
  return regular_exports() == other.regular_exports() &&
         regular_imports() == other.regular_imports() &&
         special_exports() == other.special_exports() &&
         namespace_imports() == other.namespace_imports() &&
         module_requests() == other.module_requests();
}
#endif

struct ModuleHandleHash {
  V8_INLINE size_t operator()(Handle<Module> module) const {
    return module->hash();
  }
};

struct ModuleHandleEqual {
  V8_INLINE bool operator()(Handle<Module> lhs, Handle<Module> rhs) const {
    return *lhs == *rhs;
  }
};

class UnorderedModuleSet
    : public std::unordered_set<Handle<Module>, ModuleHandleHash,
                                ModuleHandleEqual,
                                ZoneAllocator<Handle<Module>>> {
 public:
  explicit UnorderedModuleSet(Zone* zone)
      : std::unordered_set<Handle<Module>, ModuleHandleHash, ModuleHandleEqual,
                           ZoneAllocator<Handle<Module>>>(
            2 /* bucket count */, ModuleHandleHash(), ModuleHandleEqual(),
            ZoneAllocator<Handle<Module>>(zone)) {}
};

Handle<SourceTextModule> SourceTextModule::GetCycleRoot(
    Isolate* isolate) const {
  CHECK_GE(status(), kEvaluated);
  DCHECK(!cycle_root().IsTheHole(isolate));
  Handle<SourceTextModule> root(SourceTextModule::cast(cycle_root()), isolate);
  return root;
}

void SourceTextModule::AddAsyncParentModule(Isolate* isolate,
                                            Handle<SourceTextModule> module,
                                            Handle<SourceTextModule> parent) {
  Handle<ArrayList> async_parent_modules(module->async_parent_modules(),
                                         isolate);
  Handle<ArrayList> new_array_list =
      ArrayList::Add(isolate, async_parent_modules, parent);
  module->set_async_parent_modules(*new_array_list);
}

Handle<SourceTextModule> SourceTextModule::GetAsyncParentModule(
    Isolate* isolate, int index) {
  Handle<SourceTextModule> module(
      SourceTextModule::cast(async_parent_modules().Get(index)), isolate);
  return module;
}

int SourceTextModule::AsyncParentModuleCount() {
  return async_parent_modules().Length();
}

bool SourceTextModule::IsAsyncEvaluating() const {
  return async_evaluating_ordinal() >= kFirstAsyncEvaluatingOrdinal;
}

bool SourceTextModule::HasPendingAsyncDependencies() {
  DCHECK_GE(pending_async_dependencies(), 0);
  return pending_async_dependencies() > 0;
}

void SourceTextModule::IncrementPendingAsyncDependencies() {
  set_pending_async_dependencies(pending_async_dependencies() + 1);
}

void SourceTextModule::DecrementPendingAsyncDependencies() {
  set_pending_async_dependencies(pending_async_dependencies() - 1);
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_MODULE_INL_H_
