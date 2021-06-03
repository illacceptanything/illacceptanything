// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "v8config.h"  // NOLINT(build/include_directory)

#if !defined(CPPGC_CAGED_HEAP)
#error "Must be compiled with caged heap enabled"
#endif

#include "src/heap/cppgc/caged-heap.h"

#include "include/cppgc/internal/caged-heap-local-data.h"
#include "src/base/bounded-page-allocator.h"
#include "src/base/logging.h"
#include "src/heap/cppgc/globals.h"

namespace cppgc {
namespace internal {

STATIC_ASSERT(api_constants::kCagedHeapReservationSize ==
              kCagedHeapReservationSize);
STATIC_ASSERT(api_constants::kCagedHeapReservationAlignment ==
              kCagedHeapReservationAlignment);

namespace {

VirtualMemory ReserveCagedHeap(PageAllocator* platform_allocator) {
  DCHECK_NOT_NULL(platform_allocator);
  DCHECK_EQ(0u,
            kCagedHeapReservationSize % platform_allocator->AllocatePageSize());

  static constexpr size_t kAllocationTries = 4;
  for (size_t i = 0; i < kAllocationTries; ++i) {
    void* hint = reinterpret_cast<void*>(RoundDown(
        reinterpret_cast<uintptr_t>(platform_allocator->GetRandomMmapAddr()),
        kCagedHeapReservationAlignment));

    VirtualMemory memory(platform_allocator, kCagedHeapReservationSize,
                         kCagedHeapReservationAlignment, hint);
    if (memory.IsReserved()) return memory;
  }

  FATAL("Fatal process out of memory: Failed to reserve memory for caged heap");
  UNREACHABLE();
}

class CppgcBoundedPageAllocator final : public v8::base::BoundedPageAllocator {
 public:
  CppgcBoundedPageAllocator(v8::PageAllocator* page_allocator, Address start,
                            size_t size, size_t allocate_page_size)
      : BoundedPageAllocator(page_allocator, start, size, allocate_page_size) {}

  bool FreePages(void* address, size_t size) final {
    // BoundedPageAllocator is not guaranteed to allocate zeroed page.
    // Specifically it is possible that BPA frees a page and then tries to
    // reallocate the same page before the OS has had a chance to asyncroniously
    // reclaim that page. In such cases, the contents of the page would not have
    // been cleared by the OS and the reallocated page will keep its previous
    // contents. To mitigate this problem, CppgcBoundedPageAllocator clears all
    // pages before they are freed. This also includes protected guard pages, so
    // CppgcBoundedPageAllocator needs to update permissions before clearing.
    SetPermissions(address, size, Permission::kReadWrite);
    memset(address, 0, size);
    return v8::base::BoundedPageAllocator::FreePages(address, size);
  }
};

}  // namespace

CagedHeap::CagedHeap(HeapBase* heap_base, PageAllocator* platform_allocator)
    : reserved_area_(ReserveCagedHeap(platform_allocator)) {
  using CagedAddress = CagedHeap::AllocatorType::Address;

  DCHECK_NOT_NULL(heap_base);

  CHECK(platform_allocator->SetPermissions(
      reserved_area_.address(),
      RoundUp(sizeof(CagedHeapLocalData), platform_allocator->CommitPageSize()),
      PageAllocator::kReadWrite));

  auto* local_data =
      new (reserved_area_.address()) CagedHeapLocalData(heap_base);
#if defined(CPPGC_YOUNG_GENERATION)
  local_data->age_table.Reset(platform_allocator);
#endif
  USE(local_data);

  const CagedAddress caged_heap_start =
      RoundUp(reinterpret_cast<CagedAddress>(reserved_area_.address()) +
                  sizeof(CagedHeapLocalData),
              kPageSize);
  const size_t local_data_size_with_padding =
      caged_heap_start -
      reinterpret_cast<CagedAddress>(reserved_area_.address());

  bounded_allocator_ = std::make_unique<CppgcBoundedPageAllocator>(
      platform_allocator, caged_heap_start,
      reserved_area_.size() - local_data_size_with_padding, kPageSize);
}

}  // namespace internal
}  // namespace cppgc
