// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/cppgc/explicit-management.h"

#include <tuple>

#include "src/heap/cppgc/heap-base.h"
#include "src/heap/cppgc/heap-object-header.h"
#include "src/heap/cppgc/heap-page.h"
#include "src/heap/cppgc/memory.h"

namespace cppgc {
namespace internal {

namespace {

bool InGC(HeapHandle& heap_handle) {
  const auto& heap = HeapBase::From(heap_handle);
  // Whenever the GC is active, avoid modifying the object as it may mess with
  // state that the GC needs.
  return heap.in_atomic_pause() || heap.marker() ||
         heap.sweeper().IsSweepingInProgress();
}

}  // namespace

void FreeUnreferencedObject(HeapHandle& heap_handle, void* object) {
  if (InGC(heap_handle)) {
    return;
  }

  auto& header = HeapObjectHeader::FromObject(object);
  header.Finalize();

  // `object` is guaranteed to be of type GarbageCollected, so getting the
  // BasePage is okay for regular and large objects.
  BasePage* base_page = BasePage::FromPayload(object);
  if (base_page->is_large()) {  // Large object.
    base_page->space()->RemovePage(base_page);
    base_page->heap()->stats_collector()->NotifyExplicitFree(
        LargePage::From(base_page)->PayloadSize());
    LargePage::Destroy(LargePage::From(base_page));
  } else {  // Regular object.
    const size_t header_size = header.AllocatedSize();
    auto* normal_page = NormalPage::From(base_page);
    auto& normal_space = *static_cast<NormalPageSpace*>(base_page->space());
    auto& lab = normal_space.linear_allocation_buffer();
    ConstAddress payload_end = header.ObjectEnd();
    SetMemoryInaccessible(&header, header_size);
    if (payload_end == lab.start()) {  // Returning to LAB.
      lab.Set(reinterpret_cast<Address>(&header), lab.size() + header_size);
      normal_page->object_start_bitmap().ClearBit(lab.start());
    } else {  // Returning to free list.
      base_page->heap()->stats_collector()->NotifyExplicitFree(header_size);
      normal_space.free_list().Add({&header, header_size});
      // No need to update the bitmap as the same bit is reused for the free
      // list entry.
    }
  }
}

namespace {

bool Grow(HeapObjectHeader& header, BasePage& base_page, size_t new_size,
          size_t size_delta) {
  DCHECK_GE(new_size, header.AllocatedSize() + kAllocationGranularity);
  DCHECK_GE(size_delta, kAllocationGranularity);
  DCHECK(!base_page.is_large());

  auto& normal_space = *static_cast<NormalPageSpace*>(base_page.space());
  auto& lab = normal_space.linear_allocation_buffer();
  if (lab.start() == header.ObjectEnd() && lab.size() >= size_delta) {
    // LABs are considered used memory which means that no allocated size
    // adjustments are needed.
    Address delta_start = lab.Allocate(size_delta);
    SetMemoryAccessible(delta_start, size_delta);
    header.SetAllocatedSize(new_size);
    return true;
  }
  return false;
}

bool Shrink(HeapObjectHeader& header, BasePage& base_page, size_t new_size,
            size_t size_delta) {
  DCHECK_GE(header.AllocatedSize(), new_size + kAllocationGranularity);
  DCHECK_GE(size_delta, kAllocationGranularity);
  DCHECK(!base_page.is_large());

  auto& normal_space = *static_cast<NormalPageSpace*>(base_page.space());
  auto& lab = normal_space.linear_allocation_buffer();
  Address free_start = header.ObjectEnd() - size_delta;
  if (lab.start() == header.ObjectEnd()) {
    DCHECK_EQ(free_start, lab.start() - size_delta);
    // LABs are considered used memory which means that no allocated size
    // adjustments are needed.
    lab.Set(free_start, lab.size() + size_delta);
    SetMemoryInaccessible(lab.start(), size_delta);
    header.SetAllocatedSize(new_size);
    return true;
  }
  // Heuristic: Only return memory to the free list if the block is larger than
  // the smallest size class.
  if (size_delta >= ObjectAllocator::kSmallestSpaceSize) {
    SetMemoryInaccessible(free_start, size_delta);
    base_page.heap()->stats_collector()->NotifyExplicitFree(size_delta);
    normal_space.free_list().Add({free_start, size_delta});
    NormalPage::From(&base_page)->object_start_bitmap().SetBit(free_start);
    header.SetAllocatedSize(new_size);
  }
  // Return success in any case, as we want to avoid that embedders start
  // copying memory because of small deltas.
  return true;
}

}  // namespace

bool Resize(void* object, size_t new_object_size) {
  // `object` is guaranteed to be of type GarbageCollected, so getting the
  // BasePage is okay for regular and large objects.
  BasePage* base_page = BasePage::FromPayload(object);

  if (InGC(*base_page->heap())) {
    return false;
  }

  // TODO(chromium:1056170): Consider supporting large objects within certain
  // restrictions.
  if (base_page->is_large()) {
    return false;
  }

  const size_t new_size = RoundUp<kAllocationGranularity>(
      sizeof(HeapObjectHeader) + new_object_size);
  auto& header = HeapObjectHeader::FromObject(object);
  const size_t old_size = header.AllocatedSize();

  if (new_size > old_size) {
    return Grow(header, *base_page, new_size, new_size - old_size);
  } else if (old_size > new_size) {
    return Shrink(header, *base_page, new_size, old_size - new_size);
  }
  // Same size considering internal restrictions, e.g. alignment.
  return true;
}

}  // namespace internal
}  // namespace cppgc
