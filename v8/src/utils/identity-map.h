// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_UTILS_IDENTITY_MAP_H_
#define V8_UTILS_IDENTITY_MAP_H_

#include <type_traits>

#include "src/base/functional.h"
#include "src/handles/handles.h"
#include "src/objects/heap-object.h"

namespace v8 {
namespace internal {

// Forward declarations.
class Heap;
class StrongRootsEntry;

template <typename T>
struct IdentityMapFindResult {
  T* entry;
  bool already_exists;
};

// Base class of identity maps contains shared code for all template
// instantions.
class V8_EXPORT_PRIVATE IdentityMapBase {
 public:
  IdentityMapBase(const IdentityMapBase&) = delete;
  IdentityMapBase& operator=(const IdentityMapBase&) = delete;
  bool empty() const { return size_ == 0; }
  int size() const { return size_; }
  int capacity() const { return capacity_; }
  bool is_iterable() const { return is_iterable_; }

 protected:
  // Allow Tester to access internals, including changing the address of objects
  // within the {keys_} array in order to simulate a moving GC.
  friend class IdentityMapTester;

  using RawEntry = uintptr_t*;

  explicit IdentityMapBase(Heap* heap)
      : heap_(heap),
        gc_counter_(-1),
        size_(0),
        capacity_(0),
        mask_(0),
        keys_(nullptr),
        strong_roots_entry_(nullptr),
        values_(nullptr),
        is_iterable_(false) {}
  virtual ~IdentityMapBase();

  IdentityMapFindResult<uintptr_t> FindOrInsertEntry(Address key);
  RawEntry FindEntry(Address key) const;
  RawEntry InsertEntry(Address key);
  bool DeleteEntry(Address key, uintptr_t* deleted_value);
  void Clear();

  Address KeyAtIndex(int index) const;

  RawEntry EntryAtIndex(int index) const;
  int NextIndex(int index) const;

  void EnableIteration();
  void DisableIteration();

  virtual uintptr_t* NewPointerArray(size_t length) = 0;
  virtual void DeletePointerArray(uintptr_t* array, size_t length) = 0;

 private:
  // Internal implementation should not be called directly by subclasses.
  int ScanKeysFor(Address address, uint32_t hash) const;
  std::pair<int, bool> InsertKey(Address address, uint32_t hash);
  int Lookup(Address key) const;
  std::pair<int, bool> LookupOrInsert(Address key);
  bool DeleteIndex(int index, uintptr_t* deleted_value);
  void Rehash();
  void Resize(int new_capacity);
  uint32_t Hash(Address address) const;

  base::hash<uintptr_t> hasher_;
  Heap* heap_;
  int gc_counter_;
  int size_;
  int capacity_;
  int mask_;
  Address* keys_;
  StrongRootsEntry* strong_roots_entry_;
  uintptr_t* values_;
  bool is_iterable_;
};

// Implements an identity map from object addresses to a given value type {V}.
// The map is robust w.r.t. garbage collection by synchronization with the
// supplied {heap}.
//  * Keys are treated as strong roots.
//  * The value type {V} must be reinterpret_cast'able to {uintptr_t}
//  * The value type {V} must not be a heap type.
template <typename V, class AllocationPolicy>
class IdentityMap : public IdentityMapBase {
 public:
  STATIC_ASSERT(sizeof(V) <= sizeof(uintptr_t));
  STATIC_ASSERT(std::is_trivially_copyable<V>::value);
  STATIC_ASSERT(std::is_trivially_destructible<V>::value);

  explicit IdentityMap(Heap* heap,
                       AllocationPolicy allocator = AllocationPolicy())
      : IdentityMapBase(heap), allocator_(allocator) {}
  IdentityMap(const IdentityMap&) = delete;
  IdentityMap& operator=(const IdentityMap&) = delete;
  ~IdentityMap() override { Clear(); }

  // Searches this map for the given key using the object's address
  // as the identity, returning:
  //    found => a pointer to the storage location for the value, true
  //    not found => a pointer to a new storage location for the value, false
  IdentityMapFindResult<V> FindOrInsert(Handle<Object> key) {
    return FindOrInsert(*key);
  }
  IdentityMapFindResult<V> FindOrInsert(Object key) {
    auto raw = FindOrInsertEntry(key.ptr());
    return {reinterpret_cast<V*>(raw.entry), raw.already_exists};
  }

  // Searches this map for the given key using the object's address
  // as the identity, returning:
  //    found => a pointer to the storage location for the value
  //    not found => {nullptr}
  V* Find(Handle<Object> key) const { return Find(*key); }
  V* Find(Object key) const {
    return reinterpret_cast<V*>(FindEntry(key.ptr()));
  }

  // Insert the value for the given key. The key must not have previously
  // existed.
  void Insert(Handle<Object> key, V v) { Insert(*key, v); }
  void Insert(Object key, V v) {
    *reinterpret_cast<V*>(InsertEntry(key.ptr())) = v;
  }

  bool Delete(Handle<Object> key, V* deleted_value) {
    return Delete(*key, deleted_value);
  }
  bool Delete(Object key, V* deleted_value) {
    uintptr_t v;
    bool deleted_something = DeleteEntry(key.ptr(), &v);
    if (deleted_value != nullptr && deleted_something) {
      *deleted_value = *reinterpret_cast<V*>(&v);
    }
    return deleted_something;
  }

  // Removes all elements from the map.
  void Clear() { IdentityMapBase::Clear(); }

  // Iterator over IdentityMap. The IteratableScope used to create this Iterator
  // must be live for the duration of the iteration.
  class Iterator {
   public:
    Iterator& operator++() {
      index_ = map_->NextIndex(index_);
      return *this;
    }

    Object key() const { return Object(map_->KeyAtIndex(index_)); }
    V* entry() const {
      return reinterpret_cast<V*>(map_->EntryAtIndex(index_));
    }

    V* operator*() { return entry(); }
    V* operator->() { return entry(); }
    bool operator!=(const Iterator& other) { return index_ != other.index_; }

   private:
    Iterator(IdentityMap* map, int index) : map_(map), index_(index) {}

    IdentityMap* map_;
    int index_;

    friend class IdentityMap;
  };

  class V8_NODISCARD IteratableScope {
   public:
    explicit IteratableScope(IdentityMap* map) : map_(map) {
      CHECK(!map_->is_iterable());
      map_->EnableIteration();
    }
    IteratableScope(const IteratableScope&) = delete;
    IteratableScope& operator=(const IteratableScope&) = delete;
    ~IteratableScope() {
      CHECK(map_->is_iterable());
      map_->DisableIteration();
    }

    Iterator begin() { return Iterator(map_, map_->NextIndex(-1)); }
    Iterator end() { return Iterator(map_, map_->capacity()); }

   private:
    IdentityMap* map_;
  };

 protected:
  // This struct is just a type tag for Zone::NewArray<T>(size_t) call.
  struct Buffer {};

  // TODO(ishell): consider removing virtual methods in favor of combining
  // IdentityMapBase and IdentityMap into one class. This would also save
  // space when sizeof(V) is less than sizeof(uintptr_t).
  uintptr_t* NewPointerArray(size_t length) override {
    return allocator_.template NewArray<uintptr_t, Buffer>(length);
  }
  void DeletePointerArray(uintptr_t* array, size_t length) override {
    allocator_.template DeleteArray<uintptr_t, Buffer>(array, length);
  }

 private:
  AllocationPolicy allocator_;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_UTILS_IDENTITY_MAP_H_
