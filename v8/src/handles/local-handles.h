// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HANDLES_LOCAL_HANDLES_H_
#define V8_HANDLES_LOCAL_HANDLES_H_

#include "include/v8-internal.h"
#include "src/base/functional.h"
#include "src/base/macros.h"
#include "src/handles/handles.h"
#include "src/heap/local-heap.h"

namespace v8 {
namespace internal {

class RootVisitor;

class LocalHandles {
 public:
  LocalHandles();
  ~LocalHandles();

  void Iterate(RootVisitor* visitor);

#ifdef DEBUG
  bool Contains(Address* location);
#endif

 private:
  HandleScopeData scope_;
  std::vector<Address*> blocks_;

  V8_EXPORT_PRIVATE Address* AddBlock();
  V8_EXPORT_PRIVATE void RemoveUnusedBlocks();

#ifdef ENABLE_HANDLE_ZAPPING
  V8_EXPORT_PRIVATE static void ZapRange(Address* start, Address* end);
#endif

  friend class LocalHandleScope;
};

class V8_NODISCARD LocalHandleScope {
 public:
  explicit inline LocalHandleScope(LocalIsolate* local_isolate);
  explicit inline LocalHandleScope(LocalHeap* local_heap);
  inline ~LocalHandleScope();
  LocalHandleScope(const LocalHandleScope&) = delete;
  LocalHandleScope& operator=(const LocalHandleScope&) = delete;

  template <typename T>
  Handle<T> CloseAndEscape(Handle<T> handle_value);

  V8_INLINE static Address* GetHandle(LocalHeap* local_heap, Address value);

 private:
  // Prevent heap allocation or illegal handle scopes.
  void* operator new(size_t size) = delete;
  void operator delete(void* size_t) = delete;

  // Close the handle scope resetting limits to a previous state.
  static inline void CloseScope(LocalHeap* local_heap, Address* prev_next,
                                Address* prev_limit);
  V8_EXPORT_PRIVATE static void CloseMainThreadScope(LocalHeap* local_heap,
                                                     Address* prev_next,
                                                     Address* prev_limit);

  V8_EXPORT_PRIVATE void OpenMainThreadScope(LocalHeap* local_heap);

  V8_EXPORT_PRIVATE static Address* GetMainThreadHandle(LocalHeap* local_heap,
                                                        Address value);

  LocalHeap* local_heap_;
  Address* prev_limit_;
  Address* prev_next_;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_HANDLES_LOCAL_HANDLES_H_
