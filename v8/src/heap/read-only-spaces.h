// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_READ_ONLY_SPACES_H_
#define V8_HEAP_READ_ONLY_SPACES_H_

#include <memory>
#include <utility>

#include "include/v8-platform.h"
#include "src/base/macros.h"
#include "src/common/globals.h"
#include "src/heap/allocation-stats.h"
#include "src/heap/base-space.h"
#include "src/heap/basic-memory-chunk.h"
#include "src/heap/list.h"
#include "src/heap/memory-chunk.h"

namespace v8 {
namespace internal {

class MemoryAllocator;
class ReadOnlyHeap;
class SnapshotData;

class ReadOnlyPage : public BasicMemoryChunk {
 public:
  // Clears any pointers in the header that point out of the page that would
  // otherwise make the header non-relocatable.
  void MakeHeaderRelocatable();

  size_t ShrinkToHighWaterMark();

  // Returns the address for a given offset in this page.
  Address OffsetToAddress(size_t offset) const {
    Address address_in_page = address() + offset;
    if (V8_SHARED_RO_HEAP_BOOL && COMPRESS_POINTERS_IN_ISOLATE_CAGE_BOOL) {
      // Pointer compression with a per-Isolate cage and shared ReadOnlyPages
      // means that the area_start and area_end cannot be defined since they are
      // stored within the pages which can be mapped at multiple memory
      // addresses.
      DCHECK_LT(offset, size());
    } else {
      DCHECK_GE(address_in_page, area_start());
      DCHECK_LT(address_in_page, area_end());
    }
    return address_in_page;
  }

  // Returns the start area of the page without using area_start() which cannot
  // return the correct result when the page is remapped multiple times.
  Address GetAreaStart() const {
    return address() +
           MemoryChunkLayout::ObjectStartOffsetInMemoryChunk(RO_SPACE);
  }

 private:
  friend class ReadOnlySpace;
};

// -----------------------------------------------------------------------------
// Artifacts used to construct a new SharedReadOnlySpace
class ReadOnlyArtifacts {
 public:
  virtual ~ReadOnlyArtifacts() = default;

  // Initialize the ReadOnlyArtifacts from an Isolate that has just been created
  // either by serialization or by creating the objects directly.
  virtual void Initialize(Isolate* isolate, std::vector<ReadOnlyPage*>&& pages,
                          const AllocationStats& stats) = 0;

  // This replaces the ReadOnlySpace in the given Heap with a newly constructed
  // SharedReadOnlySpace that has pages created from the ReadOnlyArtifacts. This
  // is only called for the first Isolate, where the ReadOnlySpace is created
  // during the bootstrap process.

  virtual void ReinstallReadOnlySpace(Isolate* isolate) = 0;
  // Creates a ReadOnlyHeap for a specific Isolate. This will be populated with
  // a SharedReadOnlySpace object that points to the Isolate's heap. Should only
  // be used when the read-only heap memory is shared with or without pointer
  // compression. This is called for all subsequent Isolates created after the
  // first one.
  virtual ReadOnlyHeap* GetReadOnlyHeapForIsolate(Isolate* isolate) = 0;

  virtual void VerifyHeapAndSpaceRelationships(Isolate* isolate) = 0;

  std::vector<ReadOnlyPage*>& pages() { return pages_; }

  void set_accounting_stats(const AllocationStats& stats) { stats_ = stats; }
  const AllocationStats& accounting_stats() const { return stats_; }

  void set_shared_read_only_space(
      std::unique_ptr<SharedReadOnlySpace> shared_space) {
    shared_read_only_space_ = std::move(shared_space);
  }
  SharedReadOnlySpace* shared_read_only_space() {
    return shared_read_only_space_.get();
  }

  void set_read_only_heap(std::unique_ptr<ReadOnlyHeap> read_only_heap);
  ReadOnlyHeap* read_only_heap() const { return read_only_heap_.get(); }

  void InitializeChecksum(SnapshotData* read_only_snapshot_data);
  void VerifyChecksum(SnapshotData* read_only_snapshot_data,
                      bool read_only_heap_created);

 protected:
  ReadOnlyArtifacts() = default;

  std::vector<ReadOnlyPage*> pages_;
  AllocationStats stats_;
  std::unique_ptr<SharedReadOnlySpace> shared_read_only_space_;
  std::unique_ptr<ReadOnlyHeap> read_only_heap_;
#ifdef DEBUG
  // The checksum of the blob the read-only heap was deserialized from, if
  // any.
  base::Optional<uint32_t> read_only_blob_checksum_;
#endif  // DEBUG
};

// -----------------------------------------------------------------------------
// Artifacts used to construct a new SharedReadOnlySpace when pointer
// compression is disabled and so there is a single ReadOnlySpace with one set
// of pages shared between all Isolates.
class SingleCopyReadOnlyArtifacts : public ReadOnlyArtifacts {
 public:
  ~SingleCopyReadOnlyArtifacts() override;

  ReadOnlyHeap* GetReadOnlyHeapForIsolate(Isolate* isolate) override;
  void Initialize(Isolate* isolate, std::vector<ReadOnlyPage*>&& pages,
                  const AllocationStats& stats) override;
  void ReinstallReadOnlySpace(Isolate* isolate) override;
  void VerifyHeapAndSpaceRelationships(Isolate* isolate) override;

 private:
  v8::PageAllocator* page_allocator_ = nullptr;
};

// -----------------------------------------------------------------------------
// Artifacts used to construct a new SharedReadOnlySpace when pointer
// compression is enabled and so there is a ReadOnlySpace for each Isolate with
// with its own set of pages mapped from the canonical set stored here.
class PointerCompressedReadOnlyArtifacts : public ReadOnlyArtifacts {
 public:
  ReadOnlyHeap* GetReadOnlyHeapForIsolate(Isolate* isolate) override;
  void Initialize(Isolate* isolate, std::vector<ReadOnlyPage*>&& pages,
                  const AllocationStats& stats) override;
  void ReinstallReadOnlySpace(Isolate* isolate) override;
  void VerifyHeapAndSpaceRelationships(Isolate* isolate) override;

 private:
  SharedReadOnlySpace* CreateReadOnlySpace(Isolate* isolate);
  Tagged_t OffsetForPage(size_t index) const { return page_offsets_[index]; }
  void InitializeRootsIn(Isolate* isolate);
  void InitializeRootsFrom(Isolate* isolate);

  std::unique_ptr<v8::PageAllocator::SharedMemoryMapping> RemapPageTo(
      size_t i, Address new_address, ReadOnlyPage*& new_page);

  static constexpr size_t kReadOnlyRootsCount =
      static_cast<size_t>(RootIndex::kReadOnlyRootsCount);

  Address read_only_roots_[kReadOnlyRootsCount];
  std::vector<Tagged_t> page_offsets_;
  std::vector<std::unique_ptr<PageAllocator::SharedMemory>> shared_memory_;
};

// -----------------------------------------------------------------------------
// Read Only space for all Immortal Immovable and Immutable objects
class ReadOnlySpace : public BaseSpace {
 public:
  V8_EXPORT_PRIVATE explicit ReadOnlySpace(Heap* heap);

  // Detach the pages and add them to artifacts for using in creating a
  // SharedReadOnlySpace. Since the current space no longer has any pages, it
  // should be replaced straight after this in its Heap.
  void DetachPagesAndAddToArtifacts(
      std::shared_ptr<ReadOnlyArtifacts> artifacts);

  V8_EXPORT_PRIVATE ~ReadOnlySpace() override;
  V8_EXPORT_PRIVATE virtual void TearDown(MemoryAllocator* memory_allocator);

  bool IsDetached() const { return heap_ == nullptr; }

  bool writable() const { return !is_marked_read_only_; }

  bool Contains(Address a) = delete;
  bool Contains(Object o) = delete;

  V8_EXPORT_PRIVATE
  AllocationResult AllocateRaw(int size_in_bytes,
                               AllocationAlignment alignment);

  V8_EXPORT_PRIVATE void ClearStringPaddingIfNeeded();

  enum class SealMode {
    kDetachFromHeap,
    kDetachFromHeapAndUnregisterMemory,
    kDoNotDetachFromHeap
  };

  // Seal the space by marking it read-only, optionally detaching it
  // from the heap and forgetting it for memory bookkeeping purposes (e.g.
  // prevent space's memory from registering as leaked).
  V8_EXPORT_PRIVATE void Seal(SealMode ro_mode);

  // During boot the free_space_map is created, and afterwards we may need
  // to write it into the free space nodes that were already created.
  void RepairFreeSpacesAfterDeserialization();

  size_t Size() override { return accounting_stats_.Size(); }
  V8_EXPORT_PRIVATE size_t CommittedPhysicalMemory() override;

  const std::vector<ReadOnlyPage*>& pages() const { return pages_; }
  Address top() const { return top_; }
  Address limit() const { return limit_; }
  size_t Capacity() const { return capacity_; }

  bool ContainsSlow(Address addr);
  V8_EXPORT_PRIVATE void ShrinkPages();
#ifdef VERIFY_HEAP
  void Verify(Isolate* isolate);
#ifdef DEBUG
  void VerifyCounters(Heap* heap);
#endif  // DEBUG
#endif  // VERIFY_HEAP

  // Return size of allocatable area on a page in this space.
  int AreaSize() const { return static_cast<int>(area_size_); }

  ReadOnlyPage* InitializePage(BasicMemoryChunk* chunk);

  Address FirstPageAddress() const { return pages_.front()->address(); }

 protected:
  friend class SingleCopyReadOnlyArtifacts;

  void SetPermissionsForPages(MemoryAllocator* memory_allocator,
                              PageAllocator::Permission access);

  bool is_marked_read_only_ = false;

  // Accounting information for this space.
  AllocationStats accounting_stats_;

  std::vector<ReadOnlyPage*> pages_;

  Address top_;
  Address limit_;

 private:
  // Unseal the space after it has been sealed, by making it writable.
  void Unseal();

  void DetachFromHeap() { heap_ = nullptr; }

  AllocationResult AllocateRawUnaligned(int size_in_bytes);
  AllocationResult AllocateRawAligned(int size_in_bytes,
                                      AllocationAlignment alignment);

  HeapObject TryAllocateLinearlyAligned(int size_in_bytes,
                                        AllocationAlignment alignment);
  void EnsureSpaceForAllocation(int size_in_bytes);
  void FreeLinearAllocationArea();

  // String padding must be cleared just before serialization and therefore
  // the string padding in the space will already have been cleared if the
  // space was deserialized.
  bool is_string_padding_cleared_;

  size_t capacity_;
  const size_t area_size_;
};

class SharedReadOnlySpace : public ReadOnlySpace {
 public:
  explicit SharedReadOnlySpace(Heap* heap) : ReadOnlySpace(heap) {
    is_marked_read_only_ = true;
  }

  SharedReadOnlySpace(Heap* heap,
                      PointerCompressedReadOnlyArtifacts* artifacts);
  SharedReadOnlySpace(
      Heap* heap, std::vector<ReadOnlyPage*>&& new_pages,
      std::vector<std::unique_ptr<::v8::PageAllocator::SharedMemoryMapping>>&&
          mappings,
      AllocationStats&& new_stats);
  SharedReadOnlySpace(Heap* heap, SingleCopyReadOnlyArtifacts* artifacts);
  SharedReadOnlySpace(const SharedReadOnlySpace&) = delete;

  void TearDown(MemoryAllocator* memory_allocator) override;

  // Holds any shared memory mapping that must be freed when the space is
  // deallocated.
  std::vector<std::unique_ptr<v8::PageAllocator::SharedMemoryMapping>>
      shared_memory_mappings_;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_HEAP_READ_ONLY_SPACES_H_
