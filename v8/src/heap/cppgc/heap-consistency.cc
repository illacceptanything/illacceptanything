// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/cppgc/heap-consistency.h"

#include "include/cppgc/heap.h"
#include "src/base/logging.h"
#include "src/heap/cppgc/heap-base.h"

namespace cppgc {
namespace subtle {

// static
bool DisallowGarbageCollectionScope::IsGarbageCollectionAllowed(
    cppgc::HeapHandle& heap_handle) {
  auto& heap_base = internal::HeapBase::From(heap_handle);
  return !heap_base.in_disallow_gc_scope();
}

// static
void DisallowGarbageCollectionScope::Enter(cppgc::HeapHandle& heap_handle) {
  auto& heap_base = internal::HeapBase::From(heap_handle);
  heap_base.disallow_gc_scope_++;
}

// static
void DisallowGarbageCollectionScope::Leave(cppgc::HeapHandle& heap_handle) {
  auto& heap_base = internal::HeapBase::From(heap_handle);
  DCHECK_GT(heap_base.disallow_gc_scope_, 0);
  heap_base.disallow_gc_scope_--;
}

DisallowGarbageCollectionScope::DisallowGarbageCollectionScope(
    cppgc::HeapHandle& heap_handle)
    : heap_handle_(heap_handle) {
  Enter(heap_handle);
}

DisallowGarbageCollectionScope::~DisallowGarbageCollectionScope() {
  Leave(heap_handle_);
}

// static
void NoGarbageCollectionScope::Enter(cppgc::HeapHandle& heap_handle) {
  auto& heap_base = internal::HeapBase::From(heap_handle);
  heap_base.no_gc_scope_++;
}

// static
void NoGarbageCollectionScope::Leave(cppgc::HeapHandle& heap_handle) {
  auto& heap_base = internal::HeapBase::From(heap_handle);
  DCHECK_GT(heap_base.no_gc_scope_, 0);
  heap_base.no_gc_scope_--;
}

NoGarbageCollectionScope::NoGarbageCollectionScope(
    cppgc::HeapHandle& heap_handle)
    : heap_handle_(heap_handle) {
  Enter(heap_handle);
}

NoGarbageCollectionScope::~NoGarbageCollectionScope() { Leave(heap_handle_); }

}  // namespace subtle
}  // namespace cppgc
