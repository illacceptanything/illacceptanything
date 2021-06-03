// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_CPPGC_GC_INVOKER_H_
#define V8_HEAP_CPPGC_GC_INVOKER_H_

#include "include/cppgc/heap.h"
#include "src/base/macros.h"
#include "src/heap/cppgc/garbage-collector.h"

namespace cppgc {

class Platform;

namespace internal {

// GC invoker that dispatches GC depending on StackSupport and StackState:
// 1. If StackState specifies no stack scan needed the GC is invoked
//    synchronously.
// 2. If StackState specifies conservative GC and StackSupport prohibits stack
//    scanning: Delay GC until it can be invoked without accessing the stack.
//    To do so, a precise GC without stack scan is scheduled using the platform
//    if non-nestable tasks are supported, and otherwise no operation is carried
//    out. This means that the heuristics allows to arbitrary go over the limit
//    in case non-nestable tasks are not supported and only conservative GCs are
//    requested.
class V8_EXPORT_PRIVATE GCInvoker final : public GarbageCollector {
 public:
  GCInvoker(GarbageCollector*, cppgc::Platform*, cppgc::Heap::StackSupport);
  ~GCInvoker();

  GCInvoker(const GCInvoker&) = delete;
  GCInvoker& operator=(const GCInvoker&) = delete;

  void CollectGarbage(GarbageCollector::Config) final;
  void StartIncrementalGarbageCollection(GarbageCollector::Config) final;
  size_t epoch() const final;

 private:
  class GCInvokerImpl;
  std::unique_ptr<GCInvokerImpl> impl_;
};

}  // namespace internal
}  // namespace cppgc

#endif  // V8_HEAP_CPPGC_GC_INVOKER_H_
