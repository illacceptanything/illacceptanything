// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_ZONE_ZONE_H_
#define V8_ZONE_ZONE_H_

#include <limits>

#include "src/base/logging.h"
#include "src/common/globals.h"
#include "src/utils/utils.h"
#include "src/zone/accounting-allocator.h"
#include "src/zone/type-stats.h"
#include "src/zone/zone-segment.h"
#include "src/zone/zone-type-traits.h"

#ifndef ZONE_NAME
#define ZONE_NAME __func__
#endif

namespace v8 {
namespace internal {

// The Zone supports very fast allocation of small chunks of
// memory. The chunks cannot be deallocated individually, but instead
// the Zone supports deallocating all chunks in one fast
// operation. The Zone is used to hold temporary data structures like
// the abstract syntax tree, which is deallocated after compilation.
//
// Note: There is no need to initialize the Zone; the first time an
// allocation is attempted, a segment of memory will be requested
// through the allocator.
//
// Note: The implementation is inherently not thread safe. Do not use
// from multi-threaded code.

class V8_EXPORT_PRIVATE Zone final {
 public:
  Zone(AccountingAllocator* allocator, const char* name,
       bool support_compression = false);
  ~Zone();

  // Returns true if the zone supports zone pointer compression.
  bool supports_compression() const {
    return COMPRESS_ZONES_BOOL && supports_compression_;
  }

  // Allocate 'size' bytes of uninitialized memory in the Zone; expands the Zone
  // by allocating new segments of memory on demand using AccountingAllocator
  // (see AccountingAllocator::AllocateSegment()).
  //
  // When V8_ENABLE_PRECISE_ZONE_STATS is defined, the allocated bytes are
  // associated with the provided TypeTag type.
  template <typename TypeTag>
  void* Allocate(size_t size) {
#ifdef V8_USE_ADDRESS_SANITIZER
    return AsanNew(size);
#else
    size = RoundUp(size, kAlignmentInBytes);
#ifdef V8_ENABLE_PRECISE_ZONE_STATS
    if (V8_UNLIKELY(TracingFlags::is_zone_stats_enabled())) {
      type_stats_.AddAllocated<TypeTag>(size);
    }
    allocation_size_for_tracing_ += size;
#endif
    Address result = position_;
    if (V8_UNLIKELY(size > limit_ - position_)) {
      result = NewExpand(size);
    } else {
      position_ += size;
    }
    return reinterpret_cast<void*>(result);
#endif  // V8_USE_ADDRESS_SANITIZER
  }

  // Return 'size' bytes of memory back to Zone. These bytes can be reused
  // for following allocations.
  //
  // When V8_ENABLE_PRECISE_ZONE_STATS is defined, the deallocated bytes are
  // associated with the provided TypeTag type.
  template <typename TypeTag = void>
  void Delete(void* pointer, size_t size) {
    DCHECK_NOT_NULL(pointer);
    DCHECK_NE(size, 0);
    size = RoundUp(size, kAlignmentInBytes);
#ifdef V8_ENABLE_PRECISE_ZONE_STATS
    if (V8_UNLIKELY(TracingFlags::is_zone_stats_enabled())) {
      type_stats_.AddDeallocated<TypeTag>(size);
    }
    freed_size_for_tracing_ += size;
#endif

#ifdef DEBUG
    static const unsigned char kZapDeadByte = 0xcd;
    memset(pointer, kZapDeadByte, size);
#endif
  }

  // Allocates memory for T instance and constructs object by calling respective
  // Args... constructor.
  //
  // When V8_ENABLE_PRECISE_ZONE_STATS is defined, the allocated bytes are
  // associated with the T type.
  template <typename T, typename... Args>
  T* New(Args&&... args) {
    void* memory = Allocate<T>(sizeof(T));
    return new (memory) T(std::forward<Args>(args)...);
  }

  // Allocates uninitialized memory for 'length' number of T instances.
  //
  // When V8_ENABLE_PRECISE_ZONE_STATS is defined, the allocated bytes are
  // associated with the provided TypeTag type. It might be useful to tag
  // buffer allocations with meaningful names to make buffer allocation sites
  // distinguishable between each other.
  template <typename T, typename TypeTag = T[]>
  T* NewArray(size_t length) {
    DCHECK_IMPLIES(is_compressed_pointer<T>::value, supports_compression());
    DCHECK_LT(length, std::numeric_limits<size_t>::max() / sizeof(T));
    return static_cast<T*>(Allocate<TypeTag>(length * sizeof(T)));
  }

  // Return array of 'length' elements back to Zone. These bytes can be reused
  // for following allocations.
  //
  // When V8_ENABLE_PRECISE_ZONE_STATS is defined, the deallocated bytes are
  // associated with the provided TypeTag type.
  template <typename T, typename TypeTag = T[]>
  void DeleteArray(T* pointer, size_t length) {
    Delete<TypeTag>(pointer, length * sizeof(T));
  }

  // Seals the zone to prevent any further allocation.
  void Seal() { sealed_ = true; }

  // Allows the zone to be safely reused. Releases the memory and fires zone
  // destruction and creation events for the accounting allocator.
  void ReleaseMemory();

  // Returns true if more memory has been allocated in zones than
  // the limit allows.
  bool excess_allocation() const {
    return segment_bytes_allocated_ > kExcessLimit;
  }

  size_t segment_bytes_allocated() const { return segment_bytes_allocated_; }

  const char* name() const { return name_; }

  // Returns precise value of used zone memory, allowed to be called only
  // from thread owning the zone.
  size_t allocation_size() const {
    size_t extra = segment_head_ ? position_ - segment_head_->start() : 0;
    return allocation_size_ + extra;
  }

  // When V8_ENABLE_PRECISE_ZONE_STATS is not defined, returns used zone memory
  // not including the head segment.
  // Can be called from threads not owning the zone.
  size_t allocation_size_for_tracing() const {
#ifdef V8_ENABLE_PRECISE_ZONE_STATS
    return allocation_size_for_tracing_;
#else
    return allocation_size_;
#endif
  }

  // Returns number of bytes freed in this zone via Delete<T>()/DeleteArray<T>()
  // calls. Returns non-zero values only when V8_ENABLE_PRECISE_ZONE_STATS is
  // defined.
  size_t freed_size_for_tracing() const {
#ifdef V8_ENABLE_PRECISE_ZONE_STATS
    return freed_size_for_tracing_;
#else
    return 0;
#endif
  }

  AccountingAllocator* allocator() const { return allocator_; }

#ifdef V8_ENABLE_PRECISE_ZONE_STATS
  const TypeStats& type_stats() const { return type_stats_; }
#endif

 private:
  void* AsanNew(size_t size);

  // Deletes all objects and free all memory allocated in the Zone.
  void DeleteAll();

  // All pointers returned from New() are 8-byte aligned.
  static const size_t kAlignmentInBytes = 8;

  // Never allocate segments smaller than this size in bytes.
  static const size_t kMinimumSegmentSize = 8 * KB;

  // Never allocate segments larger than this size in bytes.
  static const size_t kMaximumSegmentSize = 32 * KB;

  // Report zone excess when allocation exceeds this limit.
  static const size_t kExcessLimit = 256 * MB;

  // The number of bytes allocated in this zone so far.
  size_t allocation_size_ = 0;

  // The number of bytes allocated in segments.  Note that this number
  // includes memory allocated from the OS but not yet allocated from
  // the zone.
  size_t segment_bytes_allocated_ = 0;

  // Expand the Zone to hold at least 'size' more bytes and allocate
  // the bytes. Returns the address of the newly allocated chunk of
  // memory in the Zone. Should only be called if there isn't enough
  // room in the Zone already.
  Address NewExpand(size_t size);

  // The free region in the current (front) segment is represented as
  // the half-open interval [position, limit). The 'position' variable
  // is guaranteed to be aligned as dictated by kAlignment.
  Address position_ = 0;
  Address limit_ = 0;

  AccountingAllocator* allocator_;

  Segment* segment_head_ = nullptr;
  const char* name_;
  const bool supports_compression_;
  bool sealed_ = false;

#ifdef V8_ENABLE_PRECISE_ZONE_STATS
  TypeStats type_stats_;
  size_t allocation_size_for_tracing_ = 0;

  // The number of bytes freed in this zone so far.
  size_t freed_size_for_tracing_ = 0;
#endif
};

// ZoneObject is an abstraction that helps define classes of objects
// allocated in the Zone. Use it as a base class; see ast.h.
class ZoneObject {
 public:
  // The accidential old-style pattern
  //    new (zone) SomeObject(...)
  // now produces compilation error. The proper way of allocating objects in
  // Zones looks like this:
  //    zone->New<SomeObject>(...)
  void* operator new(size_t, Zone*) = delete;  // See explanation above.
  // Allow non-allocating placement new.
  void* operator new(size_t size, void* ptr) {  // See explanation above.
    return ptr;
  }

  // Ideally, the delete operator should be private instead of
  // public, but unfortunately the compiler sometimes synthesizes
  // (unused) destructors for classes derived from ZoneObject, which
  // require the operator to be visible. MSVC requires the delete
  // operator to be public.

  // ZoneObjects should never be deleted individually; use
  // Zone::DeleteAll() to delete all zone objects in one go.
  // Note, that descructors will not be called.
  void operator delete(void*, size_t) { UNREACHABLE(); }
  void operator delete(void* pointer, Zone* zone) = delete;
};

// The ZoneAllocationPolicy is used to specialize generic data
// structures to allocate themselves and their elements in the Zone.
class ZoneAllocationPolicy {
 public:
  // Creates unusable allocation policy.
  ZoneAllocationPolicy() : zone_(nullptr) {}
  explicit ZoneAllocationPolicy(Zone* zone) : zone_(zone) {}

  template <typename T, typename TypeTag = T[]>
  V8_INLINE T* NewArray(size_t length) {
    return zone()->NewArray<T, TypeTag>(length);
  }
  template <typename T, typename TypeTag = T[]>
  V8_INLINE void DeleteArray(T* p, size_t length) {
    zone()->DeleteArray<T, TypeTag>(p, length);
  }

  Zone* zone() const { return zone_; }

 private:
  Zone* zone_;
};

}  // namespace internal
}  // namespace v8

// The accidential old-style pattern
//    new (zone) SomeObject(...)
// now produces compilation error. The proper way of allocating objects in
// Zones looks like this:
//    zone->New<SomeObject>(...)
void* operator new(size_t, v8::internal::Zone*) = delete;   // See explanation.
void operator delete(void*, v8::internal::Zone*) = delete;  // See explanation.

#endif  // V8_ZONE_ZONE_H_
