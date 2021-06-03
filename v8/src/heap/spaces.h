// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_SPACES_H_
#define V8_HEAP_SPACES_H_

#include <atomic>
#include <memory>
#include <vector>

#include "src/base/iterator.h"
#include "src/base/macros.h"
#include "src/common/globals.h"
#include "src/heap/allocation-observer.h"
#include "src/heap/base-space.h"
#include "src/heap/basic-memory-chunk.h"
#include "src/heap/free-list.h"
#include "src/heap/heap.h"
#include "src/heap/list.h"
#include "src/heap/memory-chunk.h"
#include "src/objects/objects.h"
#include "src/utils/allocation.h"
#include "src/utils/utils.h"
#include "testing/gtest/include/gtest/gtest_prod.h"  // nogncheck

namespace v8 {
namespace internal {

namespace heap {
class HeapTester;
class TestCodePageAllocatorScope;
}  // namespace heap

class AllocationObserver;
class FreeList;
class Isolate;
class LargeObjectSpace;
class LargePage;
class LinearAllocationArea;
class Page;
class PagedSpace;
class SemiSpace;

// -----------------------------------------------------------------------------
// Heap structures:
//
// A JS heap consists of a young generation, an old generation, and a large
// object space. The young generation is divided into two semispaces. A
// scavenger implements Cheney's copying algorithm. The old generation is
// separated into a map space and an old object space. The map space contains
// all (and only) map objects, the rest of old objects go into the old space.
// The old generation is collected by a mark-sweep-compact collector.
//
// The semispaces of the young generation are contiguous.  The old and map
// spaces consists of a list of pages. A page has a page header and an object
// area.
//
// There is a separate large object space for objects larger than
// kMaxRegularHeapObjectSize, so that they do not have to move during
// collection. The large object space is paged. Pages in large object space
// may be larger than the page size.
//
// A store-buffer based write barrier is used to keep track of intergenerational
// references.  See heap/store-buffer.h.
//
// During scavenges and mark-sweep collections we sometimes (after a store
// buffer overflow) iterate intergenerational pointers without decoding heap
// object maps so if the page belongs to old space or large object space
// it is essential to guarantee that the page does not contain any
// garbage pointers to new space: every pointer aligned word which satisfies
// the Heap::InNewSpace() predicate must be a pointer to a live heap object in
// new space. Thus objects in old space and large object spaces should have a
// special layout (e.g. no bare integer fields). This requirement does not
// apply to map space which is iterated in a special fashion. However we still
// require pointer fields of dead maps to be cleaned.
//
// To enable lazy cleaning of old space pages we can mark chunks of the page
// as being garbage.  Garbage sections are marked with a special map.  These
// sections are skipped when scanning the page, even if we are otherwise
// scanning without regard for object boundaries.  Garbage sections are chained
// together to form a free list after a GC.  Garbage sections created outside
// of GCs by object trunctation etc. may not be in the free list chain.  Very
// small free spaces are ignored, they need only be cleaned of bogus pointers
// into new space.
//
// Each page may have up to one special garbage section.  The start of this
// section is denoted by the top field in the space.  The end of the section
// is denoted by the limit field in the space.  This special garbage section
// is not marked with a free space map in the data.  The point of this section
// is to enable linear allocation without having to constantly update the byte
// array every time the top field is updated and a new object is created.  The
// special garbage section is not in the chain of garbage sections.
//
// Since the top and limit fields are in the space, not the page, only one page
// has a special garbage section, and if the top and limit are equal then there
// is no special garbage section.

// Some assertion macros used in the debugging mode.

#define DCHECK_OBJECT_SIZE(size) \
  DCHECK((0 < size) && (size <= kMaxRegularHeapObjectSize))

#define DCHECK_CODEOBJECT_SIZE(size, code_space)                          \
  DCHECK((0 < size) &&                                                    \
         (size <= std::min(MemoryChunkLayout::MaxRegularCodeObjectSize(), \
                           code_space->AreaSize())))

// ----------------------------------------------------------------------------
// Space is the abstract superclass for all allocation spaces that are not
// sealed after startup (i.e. not ReadOnlySpace).
class V8_EXPORT_PRIVATE Space : public BaseSpace {
 public:
  Space(Heap* heap, AllocationSpace id, FreeList* free_list)
      : BaseSpace(heap, id),
        free_list_(std::unique_ptr<FreeList>(free_list)) {
    external_backing_store_bytes_ =
        new std::atomic<size_t>[ExternalBackingStoreType::kNumTypes];
    external_backing_store_bytes_[ExternalBackingStoreType::kArrayBuffer] = 0;
    external_backing_store_bytes_[ExternalBackingStoreType::kExternalString] =
        0;
  }

  Space(const Space&) = delete;
  Space& operator=(const Space&) = delete;

  static inline void MoveExternalBackingStoreBytes(
      ExternalBackingStoreType type, Space* from, Space* to, size_t amount);

  ~Space() override {
    delete[] external_backing_store_bytes_;
    external_backing_store_bytes_ = nullptr;
  }

  virtual void AddAllocationObserver(AllocationObserver* observer);

  virtual void RemoveAllocationObserver(AllocationObserver* observer);

  virtual void PauseAllocationObservers();

  virtual void ResumeAllocationObservers();

  virtual void StartNextInlineAllocationStep() {}

  // Returns size of objects. Can differ from the allocated size
  // (e.g. see OldLargeObjectSpace).
  virtual size_t SizeOfObjects() { return Size(); }

  // Return the available bytes without growing.
  virtual size_t Available() = 0;

  virtual int RoundSizeDownToObjectAlignment(int size) {
    if (id_ == CODE_SPACE) {
      return RoundDown(size, kCodeAlignment);
    } else {
      return RoundDown(size, kTaggedSize);
    }
  }

  virtual std::unique_ptr<ObjectIterator> GetObjectIterator(Heap* heap) = 0;

  inline void IncrementExternalBackingStoreBytes(ExternalBackingStoreType type,
                                                 size_t amount);

  inline void DecrementExternalBackingStoreBytes(ExternalBackingStoreType type,
                                                 size_t amount);

  // Returns amount of off-heap memory in-use by objects in this Space.
  virtual size_t ExternalBackingStoreBytes(
      ExternalBackingStoreType type) const {
    return external_backing_store_bytes_[type];
  }

  MemoryChunk* first_page() { return memory_chunk_list_.front(); }
  MemoryChunk* last_page() { return memory_chunk_list_.back(); }

  const MemoryChunk* first_page() const { return memory_chunk_list_.front(); }
  const MemoryChunk* last_page() const { return memory_chunk_list_.back(); }

  heap::List<MemoryChunk>& memory_chunk_list() { return memory_chunk_list_; }

  FreeList* free_list() { return free_list_.get(); }

  Address FirstPageAddress() const { return first_page()->address(); }

#ifdef DEBUG
  virtual void Print() = 0;
#endif

 protected:
  AllocationCounter allocation_counter_;

  // The List manages the pages that belong to the given space.
  heap::List<MemoryChunk> memory_chunk_list_;

  // Tracks off-heap memory used by this space.
  std::atomic<size_t>* external_backing_store_bytes_;

  std::unique_ptr<FreeList> free_list_;
};

STATIC_ASSERT(sizeof(std::atomic<intptr_t>) == kSystemPointerSize);

// -----------------------------------------------------------------------------
// A page is a memory chunk of a size 256K. Large object pages may be larger.
//
// The only way to get a page pointer is by calling factory methods:
//   Page* p = Page::FromAddress(addr); or
//   Page* p = Page::FromAllocationAreaAddress(address);
class Page : public MemoryChunk {
 public:
  static const intptr_t kCopyAllFlags = ~0;

  // Page flags copied from from-space to to-space when flipping semispaces.
  static const intptr_t kCopyOnFlipFlagsMask =
      static_cast<intptr_t>(MemoryChunk::POINTERS_TO_HERE_ARE_INTERESTING) |
      static_cast<intptr_t>(MemoryChunk::POINTERS_FROM_HERE_ARE_INTERESTING) |
      static_cast<intptr_t>(MemoryChunk::INCREMENTAL_MARKING);

  // Returns the page containing a given address. The address ranges
  // from [page_addr .. page_addr + kPageSize[. This only works if the object
  // is in fact in a page.
  static Page* FromAddress(Address addr) {
    DCHECK(!V8_ENABLE_THIRD_PARTY_HEAP_BOOL);
    return reinterpret_cast<Page*>(addr & ~kPageAlignmentMask);
  }
  static Page* FromHeapObject(HeapObject o) {
    DCHECK(!V8_ENABLE_THIRD_PARTY_HEAP_BOOL);
    return reinterpret_cast<Page*>(o.ptr() & ~kAlignmentMask);
  }

  // Returns the page containing the address provided. The address can
  // potentially point righter after the page. To be also safe for tagged values
  // we subtract a hole word. The valid address ranges from
  // [page_addr + area_start_ .. page_addr + kPageSize + kTaggedSize].
  static Page* FromAllocationAreaAddress(Address address) {
    DCHECK(!V8_ENABLE_THIRD_PARTY_HEAP_BOOL);
    return Page::FromAddress(address - kTaggedSize);
  }

  // Checks if address1 and address2 are on the same new space page.
  static bool OnSamePage(Address address1, Address address2) {
    return Page::FromAddress(address1) == Page::FromAddress(address2);
  }

  // Checks whether an address is page aligned.
  static bool IsAlignedToPageSize(Address addr) {
    return (addr & kPageAlignmentMask) == 0;
  }

  static Page* ConvertNewToOld(Page* old_page);

  inline void MarkNeverAllocateForTesting();
  inline void MarkEvacuationCandidate();
  inline void ClearEvacuationCandidate();

  Page* next_page() { return static_cast<Page*>(list_node_.next()); }
  Page* prev_page() { return static_cast<Page*>(list_node_.prev()); }

  const Page* next_page() const {
    return static_cast<const Page*>(list_node_.next());
  }
  const Page* prev_page() const {
    return static_cast<const Page*>(list_node_.prev());
  }

  template <typename Callback>
  inline void ForAllFreeListCategories(Callback callback) {
    for (int i = kFirstCategory;
         i < owner()->free_list()->number_of_categories(); i++) {
      callback(categories_[i]);
    }
  }

  size_t AvailableInFreeList();

  size_t AvailableInFreeListFromAllocatedBytes() {
    DCHECK_GE(area_size(), wasted_memory() + allocated_bytes());
    return area_size() - wasted_memory() - allocated_bytes();
  }

  FreeListCategory* free_list_category(FreeListCategoryType type) {
    return categories_[type];
  }

  size_t ShrinkToHighWaterMark();

  V8_EXPORT_PRIVATE void CreateBlackArea(Address start, Address end);
  V8_EXPORT_PRIVATE void CreateBlackAreaBackground(Address start, Address end);
  void DestroyBlackArea(Address start, Address end);
  void DestroyBlackAreaBackground(Address start, Address end);

  void InitializeFreeListCategories();
  void AllocateFreeListCategories();
  void ReleaseFreeListCategories();

  void MoveOldToNewRememberedSetForSweeping();
  void MergeOldToNewRememberedSets();

 private:
  friend class MemoryAllocator;
};

// Validate our estimates on the header size.
STATIC_ASSERT(sizeof(BasicMemoryChunk) <= BasicMemoryChunk::kHeaderSize);
STATIC_ASSERT(sizeof(MemoryChunk) <= MemoryChunk::kHeaderSize);
STATIC_ASSERT(sizeof(Page) <= MemoryChunk::kHeaderSize);

// -----------------------------------------------------------------------------
// Interface for heap object iterator to be implemented by all object space
// object iterators.

class V8_EXPORT_PRIVATE ObjectIterator : public Malloced {
 public:
  virtual ~ObjectIterator() = default;
  virtual HeapObject Next() = 0;
};

template <class PAGE_TYPE>
class PageIteratorImpl
    : public base::iterator<std::forward_iterator_tag, PAGE_TYPE> {
 public:
  explicit PageIteratorImpl(PAGE_TYPE* p) : p_(p) {}
  PageIteratorImpl(const PageIteratorImpl<PAGE_TYPE>& other) : p_(other.p_) {}
  PAGE_TYPE* operator*() { return p_; }
  bool operator==(const PageIteratorImpl<PAGE_TYPE>& rhs) {
    return rhs.p_ == p_;
  }
  bool operator!=(const PageIteratorImpl<PAGE_TYPE>& rhs) {
    return rhs.p_ != p_;
  }
  inline PageIteratorImpl<PAGE_TYPE>& operator++();
  inline PageIteratorImpl<PAGE_TYPE> operator++(int);

 private:
  PAGE_TYPE* p_;
};

using PageIterator = PageIteratorImpl<Page>;
using ConstPageIterator = PageIteratorImpl<const Page>;
using LargePageIterator = PageIteratorImpl<LargePage>;

class PageRange {
 public:
  using iterator = PageIterator;
  PageRange(Page* begin, Page* end) : begin_(begin), end_(end) {}
  explicit PageRange(Page* page) : PageRange(page, page->next_page()) {}
  inline PageRange(Address start, Address limit);

  iterator begin() { return iterator(begin_); }
  iterator end() { return iterator(end_); }

 private:
  Page* begin_;
  Page* end_;
};

// -----------------------------------------------------------------------------
// A space has a circular list of pages. The next page can be accessed via
// Page::next_page() call.

// An abstraction of allocation and relocation pointers in a page-structured
// space.
class LinearAllocationArea {
 public:
  LinearAllocationArea()
      : start_(kNullAddress), top_(kNullAddress), limit_(kNullAddress) {}
  LinearAllocationArea(Address top, Address limit)
      : start_(top), top_(top), limit_(limit) {}

  void Reset(Address top, Address limit) {
    start_ = top;
    set_top(top);
    set_limit(limit);
  }

  void MoveStartToTop() { start_ = top_; }

  V8_INLINE Address start() const { return start_; }

  V8_INLINE void set_top(Address top) {
    SLOW_DCHECK(top == kNullAddress || (top & kHeapObjectTagMask) == 0);
    top_ = top;
  }

  V8_INLINE Address top() const {
    SLOW_DCHECK(top_ == kNullAddress || (top_ & kHeapObjectTagMask) == 0);
    return top_;
  }

  Address* top_address() { return &top_; }

  V8_INLINE void set_limit(Address limit) { limit_ = limit; }

  V8_INLINE Address limit() const { return limit_; }

  Address* limit_address() { return &limit_; }

#ifdef DEBUG
  bool VerifyPagedAllocation() {
    return (Page::FromAllocationAreaAddress(top_) ==
            Page::FromAllocationAreaAddress(limit_)) &&
           (top_ <= limit_);
  }
#endif

 private:
  // Current allocation top.
  Address start_;
  // Current allocation top.
  Address top_;
  // Current allocation limit.
  Address limit_;
};


// LocalAllocationBuffer represents a linear allocation area that is created
// from a given {AllocationResult} and can be used to allocate memory without
// synchronization.
//
// The buffer is properly closed upon destruction and reassignment.
// Example:
//   {
//     AllocationResult result = ...;
//     LocalAllocationBuffer a(heap, result, size);
//     LocalAllocationBuffer b = a;
//     CHECK(!a.IsValid());
//     CHECK(b.IsValid());
//     // {a} is invalid now and cannot be used for further allocations.
//   }
//   // Since {b} went out of scope, the LAB is closed, resulting in creating a
//   // filler object for the remaining area.
class LocalAllocationBuffer {
 public:
  // Indicates that a buffer cannot be used for allocations anymore. Can result
  // from either reassigning a buffer, or trying to construct it from an
  // invalid {AllocationResult}.
  static LocalAllocationBuffer InvalidBuffer() {
    return LocalAllocationBuffer(
        nullptr, LinearAllocationArea(kNullAddress, kNullAddress));
  }

  // Creates a new LAB from a given {AllocationResult}. Results in
  // InvalidBuffer if the result indicates a retry.
  static inline LocalAllocationBuffer FromResult(Heap* heap,
                                                 AllocationResult result,
                                                 intptr_t size);

  ~LocalAllocationBuffer() { CloseAndMakeIterable(); }

  LocalAllocationBuffer(const LocalAllocationBuffer& other) = delete;
  V8_EXPORT_PRIVATE LocalAllocationBuffer(LocalAllocationBuffer&& other)
      V8_NOEXCEPT;

  LocalAllocationBuffer& operator=(const LocalAllocationBuffer& other) = delete;
  V8_EXPORT_PRIVATE LocalAllocationBuffer& operator=(
      LocalAllocationBuffer&& other) V8_NOEXCEPT;

  V8_WARN_UNUSED_RESULT inline AllocationResult AllocateRawAligned(
      int size_in_bytes, AllocationAlignment alignment);

  inline bool IsValid() { return allocation_info_.top() != kNullAddress; }

  // Try to merge LABs, which is only possible when they are adjacent in memory.
  // Returns true if the merge was successful, false otherwise.
  inline bool TryMerge(LocalAllocationBuffer* other);

  inline bool TryFreeLast(HeapObject object, int object_size);

  // Close a LAB, effectively invalidating it. Returns the unused area.
  V8_EXPORT_PRIVATE LinearAllocationArea CloseAndMakeIterable();
  void MakeIterable();

  Address top() const { return allocation_info_.top(); }
  Address limit() const { return allocation_info_.limit(); }

 private:
  V8_EXPORT_PRIVATE LocalAllocationBuffer(
      Heap* heap, LinearAllocationArea allocation_info) V8_NOEXCEPT;

  Heap* heap_;
  LinearAllocationArea allocation_info_;
};

class SpaceWithLinearArea : public Space {
 public:
  SpaceWithLinearArea(Heap* heap, AllocationSpace id, FreeList* free_list)
      : Space(heap, id, free_list) {
    allocation_info_.Reset(kNullAddress, kNullAddress);
  }

  virtual bool SupportsAllocationObserver() = 0;

  // Returns the allocation pointer in this space.
  Address top() { return allocation_info_.top(); }
  Address limit() { return allocation_info_.limit(); }

  // The allocation top address.
  Address* allocation_top_address() { return allocation_info_.top_address(); }

  // The allocation limit address.
  Address* allocation_limit_address() {
    return allocation_info_.limit_address();
  }

  // Methods needed for allocation observers.
  V8_EXPORT_PRIVATE void AddAllocationObserver(
      AllocationObserver* observer) override;
  V8_EXPORT_PRIVATE void RemoveAllocationObserver(
      AllocationObserver* observer) override;
  V8_EXPORT_PRIVATE void ResumeAllocationObservers() override;
  V8_EXPORT_PRIVATE void PauseAllocationObservers() override;

  V8_EXPORT_PRIVATE void AdvanceAllocationObservers();
  V8_EXPORT_PRIVATE void InvokeAllocationObservers(Address soon_object,
                                                   size_t size_in_bytes,
                                                   size_t aligned_size_in_bytes,
                                                   size_t allocation_size);

  void MarkLabStartInitialized();

  // When allocation observers are active we may use a lower limit to allow the
  // observers to 'interrupt' earlier than the natural limit. Given a linear
  // area bounded by [start, end), this function computes the limit to use to
  // allow proper observation based on existing observers. min_size specifies
  // the minimum size that the limited area should have.
  Address ComputeLimit(Address start, Address end, size_t min_size);
  V8_EXPORT_PRIVATE virtual void UpdateInlineAllocationLimit(
      size_t min_size) = 0;

  V8_EXPORT_PRIVATE void UpdateAllocationOrigins(AllocationOrigin origin);

  void PrintAllocationsOrigins();

 protected:
  // TODO(ofrobots): make these private after refactoring is complete.
  LinearAllocationArea allocation_info_;

  size_t allocations_origins_[static_cast<int>(
      AllocationOrigin::kNumberOfAllocationOrigins)] = {0};
};

}  // namespace internal
}  // namespace v8

#endif  // V8_HEAP_SPACES_H_
