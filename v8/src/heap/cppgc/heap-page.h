// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_CPPGC_HEAP_PAGE_H_
#define V8_HEAP_CPPGC_HEAP_PAGE_H_

#include "src/base/iterator.h"
#include "src/base/macros.h"
#include "src/heap/cppgc/globals.h"
#include "src/heap/cppgc/heap-object-header.h"
#include "src/heap/cppgc/object-start-bitmap.h"

namespace cppgc {
namespace internal {

class BaseSpace;
class NormalPageSpace;
class LargePageSpace;
class HeapBase;
class PageBackend;

class V8_EXPORT_PRIVATE BasePage {
 public:
  static inline BasePage* FromPayload(void*);
  static inline const BasePage* FromPayload(const void*);

  static BasePage* FromInnerAddress(const HeapBase*, void*);
  static const BasePage* FromInnerAddress(const HeapBase*, const void*);

  static void Destroy(BasePage*);

  BasePage(const BasePage&) = delete;
  BasePage& operator=(const BasePage&) = delete;

  HeapBase* heap() const { return heap_; }

  BaseSpace* space() { return space_; }
  const BaseSpace* space() const { return space_; }
  void set_space(BaseSpace* space) { space_ = space; }

  bool is_large() const { return type_ == PageType::kLarge; }

  Address PayloadStart();
  ConstAddress PayloadStart() const;
  Address PayloadEnd();
  ConstAddress PayloadEnd() const;

  // Returns the size of live objects on the page at the last GC.
  // The counter is update after sweeping.
  size_t AllocatedBytesAtLastGC() const;

  // |address| must refer to real object.
  template <AccessMode = AccessMode::kNonAtomic>
  HeapObjectHeader& ObjectHeaderFromInnerAddress(void* address) const;
  template <AccessMode = AccessMode::kNonAtomic>
  const HeapObjectHeader& ObjectHeaderFromInnerAddress(
      const void* address) const;

  // |address| is guaranteed to point into the page but not payload. Returns
  // nullptr when pointing into free list entries and the valid header
  // otherwise.
  HeapObjectHeader* TryObjectHeaderFromInnerAddress(void* address) const;
  const HeapObjectHeader* TryObjectHeaderFromInnerAddress(
      const void* address) const;

  // SynchronizedLoad and SynchronizedStore are used to sync pages after they
  // are allocated. std::atomic_thread_fence is sufficient in practice but is
  // not recognized by tsan. Atomic load and store of the |type_| field are
  // added for tsan builds.
  void SynchronizedLoad() const {
#if defined(THREAD_SANITIZER)
    v8::base::AsAtomicPtr(&type_)->load(std::memory_order_acquire);
#endif
  }
  void SynchronizedStore() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
#if defined(THREAD_SANITIZER)
    v8::base::AsAtomicPtr(&type_)->store(type_, std::memory_order_release);
#endif
  }

 protected:
  enum class PageType : uint8_t { kNormal, kLarge };
  BasePage(HeapBase*, BaseSpace*, PageType);

 private:
  HeapBase* heap_;
  BaseSpace* space_;
  PageType type_;
};

class V8_EXPORT_PRIVATE NormalPage final : public BasePage {
  template <typename T>
  class IteratorImpl : v8::base::iterator<std::forward_iterator_tag, T> {
   public:
    explicit IteratorImpl(T* p, ConstAddress lab_start = nullptr,
                          size_t lab_size = 0)
        : p_(p), lab_start_(lab_start), lab_size_(lab_size) {
      DCHECK(p);
      DCHECK_EQ(0, (lab_size & (sizeof(T) - 1)));
      if (reinterpret_cast<ConstAddress>(p_) == lab_start_) {
        p_ += (lab_size_ / sizeof(T));
      }
    }

    T& operator*() { return *p_; }
    const T& operator*() const { return *p_; }

    bool operator==(IteratorImpl other) const { return p_ == other.p_; }
    bool operator!=(IteratorImpl other) const { return !(*this == other); }

    IteratorImpl& operator++() {
      const size_t size = p_->AllocatedSize();
      DCHECK_EQ(0, (size & (sizeof(T) - 1)));
      p_ += (size / sizeof(T));
      if (reinterpret_cast<ConstAddress>(p_) == lab_start_) {
        p_ += (lab_size_ / sizeof(T));
      }
      return *this;
    }
    IteratorImpl operator++(int) {
      IteratorImpl temp(*this);
      ++(*this);
      return temp;
    }

    T* base() const { return p_; }

   private:
    T* p_;
    ConstAddress lab_start_;
    size_t lab_size_;
  };

 public:
  using iterator = IteratorImpl<HeapObjectHeader>;
  using const_iterator = IteratorImpl<const HeapObjectHeader>;

  // Allocates a new page in the detached state.
  static NormalPage* Create(PageBackend*, NormalPageSpace*);
  // Destroys and frees the page. The page must be detached from the
  // corresponding space (i.e. be swept when called).
  static void Destroy(NormalPage*);

  static NormalPage* From(BasePage* page) {
    DCHECK(!page->is_large());
    return static_cast<NormalPage*>(page);
  }
  static const NormalPage* From(const BasePage* page) {
    return From(const_cast<BasePage*>(page));
  }

  iterator begin();
  const_iterator begin() const;

  iterator end() {
    return iterator(reinterpret_cast<HeapObjectHeader*>(PayloadEnd()));
  }
  const_iterator end() const {
    return const_iterator(
        reinterpret_cast<const HeapObjectHeader*>(PayloadEnd()));
  }

  Address PayloadStart();
  ConstAddress PayloadStart() const;
  Address PayloadEnd();
  ConstAddress PayloadEnd() const;

  static size_t PayloadSize();

  bool PayloadContains(ConstAddress address) const {
    return (PayloadStart() <= address) && (address < PayloadEnd());
  }

  size_t AllocatedBytesAtLastGC() const { return allocated_bytes_at_last_gc_; }

  void SetAllocatedBytesAtLastGC(size_t bytes) {
    allocated_bytes_at_last_gc_ = bytes;
  }

  PlatformAwareObjectStartBitmap& object_start_bitmap() {
    return object_start_bitmap_;
  }
  const PlatformAwareObjectStartBitmap& object_start_bitmap() const {
    return object_start_bitmap_;
  }

 private:
  NormalPage(HeapBase* heap, BaseSpace* space);
  ~NormalPage();

  size_t allocated_bytes_at_last_gc_ = 0;
  PlatformAwareObjectStartBitmap object_start_bitmap_;
};

class V8_EXPORT_PRIVATE LargePage final : public BasePage {
 public:
  // Returns the allocation size required for a payload of size |size|.
  static size_t AllocationSize(size_t size);
  // Allocates a new page in the detached state.
  static LargePage* Create(PageBackend*, LargePageSpace*, size_t);
  // Destroys and frees the page. The page must be detached from the
  // corresponding space (i.e. be swept when called).
  static void Destroy(LargePage*);

  static LargePage* From(BasePage* page) {
    DCHECK(page->is_large());
    return static_cast<LargePage*>(page);
  }
  static const LargePage* From(const BasePage* page) {
    return From(const_cast<BasePage*>(page));
  }

  HeapObjectHeader* ObjectHeader();
  const HeapObjectHeader* ObjectHeader() const;

  Address PayloadStart();
  ConstAddress PayloadStart() const;
  Address PayloadEnd();
  ConstAddress PayloadEnd() const;

  size_t PayloadSize() const { return payload_size_; }
  size_t ObjectSize() const {
    DCHECK_GT(payload_size_, sizeof(HeapObjectHeader));
    return payload_size_ - sizeof(HeapObjectHeader);
  }

  size_t AllocatedBytesAtLastGC() const { return ObjectSize(); }

  bool PayloadContains(ConstAddress address) const {
    return (PayloadStart() <= address) && (address < PayloadEnd());
  }

 private:
  LargePage(HeapBase* heap, BaseSpace* space, size_t);
  ~LargePage();

  size_t payload_size_;
};

// static
BasePage* BasePage::FromPayload(void* payload) {
  return reinterpret_cast<BasePage*>(
      (reinterpret_cast<uintptr_t>(payload) & kPageBaseMask) + kGuardPageSize);
}

// static
const BasePage* BasePage::FromPayload(const void* payload) {
  return reinterpret_cast<const BasePage*>(
      (reinterpret_cast<uintptr_t>(const_cast<void*>(payload)) &
       kPageBaseMask) +
      kGuardPageSize);
}

template <AccessMode mode = AccessMode::kNonAtomic>
const HeapObjectHeader* ObjectHeaderFromInnerAddressImpl(const BasePage* page,
                                                         const void* address) {
  if (page->is_large()) {
    return LargePage::From(page)->ObjectHeader();
  }
  const PlatformAwareObjectStartBitmap& bitmap =
      NormalPage::From(page)->object_start_bitmap();
  const HeapObjectHeader* header =
      bitmap.FindHeader<mode>(static_cast<ConstAddress>(address));
  DCHECK_LT(address, reinterpret_cast<ConstAddress>(header) +
                         header->AllocatedSize<AccessMode::kAtomic>());
  return header;
}

template <AccessMode mode>
HeapObjectHeader& BasePage::ObjectHeaderFromInnerAddress(void* address) const {
  return const_cast<HeapObjectHeader&>(
      ObjectHeaderFromInnerAddress<mode>(const_cast<const void*>(address)));
}

template <AccessMode mode>
const HeapObjectHeader& BasePage::ObjectHeaderFromInnerAddress(
    const void* address) const {
  // This method might be called for |address| found via a Trace method of
  // another object. If |address| is on a newly allocated page , there will
  // be no sync between the page allocation and a concurrent marking thread,
  // resulting in a race with page initialization (specifically with writing
  // the page |type_| field). This can occur when tracing a Member holding a
  // reference to a mixin type
  SynchronizedLoad();
  const HeapObjectHeader* header =
      ObjectHeaderFromInnerAddressImpl<mode>(this, address);
  DCHECK_NE(kFreeListGCInfoIndex, header->GetGCInfoIndex());
  return *header;
}

}  // namespace internal
}  // namespace cppgc

#endif  // V8_HEAP_CPPGC_HEAP_PAGE_H_
