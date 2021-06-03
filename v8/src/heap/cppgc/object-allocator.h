// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_CPPGC_OBJECT_ALLOCATOR_H_
#define V8_HEAP_CPPGC_OBJECT_ALLOCATOR_H_

#include "include/cppgc/allocation.h"
#include "include/cppgc/internal/gc-info.h"
#include "include/cppgc/macros.h"
#include "src/base/logging.h"
#include "src/heap/cppgc/heap-object-header.h"
#include "src/heap/cppgc/heap-page.h"
#include "src/heap/cppgc/heap-space.h"
#include "src/heap/cppgc/memory.h"
#include "src/heap/cppgc/object-start-bitmap.h"
#include "src/heap/cppgc/raw-heap.h"

namespace cppgc {

class V8_EXPORT AllocationHandle {
 private:
  AllocationHandle() = default;
  friend class internal::ObjectAllocator;
};

namespace internal {

class StatsCollector;
class PageBackend;

class V8_EXPORT_PRIVATE ObjectAllocator final : public cppgc::AllocationHandle {
 public:
  static constexpr size_t kSmallestSpaceSize = 32;

  ObjectAllocator(RawHeap* heap, PageBackend* page_backend,
                  StatsCollector* stats_collector);

  inline void* AllocateObject(size_t size, GCInfoIndex gcinfo);
  inline void* AllocateObject(size_t size, GCInfoIndex gcinfo,
                              CustomSpaceIndex space_index);

  void ResetLinearAllocationBuffers();

  // Terminate the allocator. Subsequent allocation calls result in a crash.
  void Terminate();

 private:
  bool in_disallow_gc_scope() const;

  // Returns the initially tried SpaceType to allocate an object of |size| bytes
  // on. Returns the largest regular object size bucket for large objects.
  inline static RawHeap::RegularSpaceType GetInitialSpaceIndexForSize(
      size_t size);

  inline void* AllocateObjectOnSpace(NormalPageSpace* space, size_t size,
                                     GCInfoIndex gcinfo);
  void* OutOfLineAllocate(NormalPageSpace*, size_t, GCInfoIndex);
  void* OutOfLineAllocateImpl(NormalPageSpace*, size_t, GCInfoIndex);
  void* AllocateFromFreeList(NormalPageSpace*, size_t, GCInfoIndex);

  RawHeap* raw_heap_;
  PageBackend* page_backend_;
  StatsCollector* stats_collector_;
};

void* ObjectAllocator::AllocateObject(size_t size, GCInfoIndex gcinfo) {
  DCHECK(!in_disallow_gc_scope());
  const size_t allocation_size =
      RoundUp<kAllocationGranularity>(size + sizeof(HeapObjectHeader));
  const RawHeap::RegularSpaceType type =
      GetInitialSpaceIndexForSize(allocation_size);
  return AllocateObjectOnSpace(NormalPageSpace::From(raw_heap_->Space(type)),
                               allocation_size, gcinfo);
}

void* ObjectAllocator::AllocateObject(size_t size, GCInfoIndex gcinfo,
                                      CustomSpaceIndex space_index) {
  DCHECK(!in_disallow_gc_scope());
  const size_t allocation_size =
      RoundUp<kAllocationGranularity>(size + sizeof(HeapObjectHeader));
  return AllocateObjectOnSpace(
      NormalPageSpace::From(raw_heap_->CustomSpace(space_index)),
      allocation_size, gcinfo);
}

// static
RawHeap::RegularSpaceType ObjectAllocator::GetInitialSpaceIndexForSize(
    size_t size) {
  static_assert(kSmallestSpaceSize == 32,
                "should be half the next larger size");
  if (size < 64) {
    if (size < kSmallestSpaceSize) return RawHeap::RegularSpaceType::kNormal1;
    return RawHeap::RegularSpaceType::kNormal2;
  }
  if (size < 128) return RawHeap::RegularSpaceType::kNormal3;
  return RawHeap::RegularSpaceType::kNormal4;
}

void* ObjectAllocator::AllocateObjectOnSpace(NormalPageSpace* space,
                                             size_t size, GCInfoIndex gcinfo) {
  DCHECK_LT(0u, gcinfo);

  NormalPageSpace::LinearAllocationBuffer& current_lab =
      space->linear_allocation_buffer();
  if (current_lab.size() < size) {
    return OutOfLineAllocate(space, size, gcinfo);
  }

  void* raw = current_lab.Allocate(size);
#if !defined(V8_USE_MEMORY_SANITIZER) && !defined(V8_USE_ADDRESS_SANITIZER) && \
    DEBUG
  // For debug builds, unzap only the payload.
  SetMemoryAccessible(static_cast<char*>(raw) + sizeof(HeapObjectHeader),
                      size - sizeof(HeapObjectHeader));
#else
  SetMemoryAccessible(raw, size);
#endif
  auto* header = new (raw) HeapObjectHeader(size, gcinfo);

  // The marker needs to find the object start concurrently.
  NormalPage::From(BasePage::FromPayload(header))
      ->object_start_bitmap()
      .SetBit<AccessMode::kAtomic>(reinterpret_cast<ConstAddress>(header));

  return header->ObjectStart();
}

}  // namespace internal
}  // namespace cppgc

#endif  // V8_HEAP_CPPGC_OBJECT_ALLOCATOR_H_
