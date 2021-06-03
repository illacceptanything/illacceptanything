// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_COMPRESSED_SLOTS_INL_H_
#define V8_OBJECTS_COMPRESSED_SLOTS_INL_H_

#ifdef V8_COMPRESS_POINTERS

#include "src/common/ptr-compr-inl.h"
#include "src/objects/compressed-slots.h"
#include "src/objects/heap-object-inl.h"
#include "src/objects/maybe-object-inl.h"

namespace v8 {
namespace internal {

//
// CompressedObjectSlot implementation.
//

CompressedObjectSlot::CompressedObjectSlot(Object* object)
    : SlotBase(reinterpret_cast<Address>(&object->ptr_)) {}

bool CompressedObjectSlot::contains_value(Address raw_value) const {
  AtomicTagged_t value = AsAtomicTagged::Relaxed_Load(location());
  return static_cast<uint32_t>(value) ==
         static_cast<uint32_t>(static_cast<Tagged_t>(raw_value));
}

bool CompressedObjectSlot::contains_map_value(Address raw_value) const {
  // Simply forward to contains_value because map packing is not supported with
  // pointer compression.
  DCHECK(!V8_MAP_PACKING_BOOL);
  return contains_value(raw_value);
}

Object CompressedObjectSlot::operator*() const {
  Tagged_t value = *location();
  return Object(DecompressTaggedAny(address(), value));
}

Object CompressedObjectSlot::load(PtrComprCageBase cage_base) const {
  Tagged_t value = *location();
  return Object(DecompressTaggedAny(cage_base, value));
}

void CompressedObjectSlot::store(Object value) const {
  *location() = CompressTagged(value.ptr());
}

void CompressedObjectSlot::store_map(Map map) const {
  // Simply forward to store because map packing is not supported with pointer
  // compression.
  DCHECK(!V8_MAP_PACKING_BOOL);
  store(map);
}

Map CompressedObjectSlot::load_map() const {
  // Simply forward to Relaxed_Load because map packing is not supported with
  // pointer compression.
  DCHECK(!V8_MAP_PACKING_BOOL);
  return Map::unchecked_cast(Relaxed_Load());
}

Object CompressedObjectSlot::Acquire_Load() const {
  AtomicTagged_t value = AsAtomicTagged::Acquire_Load(location());
  return Object(DecompressTaggedAny(address(), value));
}

Object CompressedObjectSlot::Relaxed_Load() const {
  AtomicTagged_t value = AsAtomicTagged::Relaxed_Load(location());
  return Object(DecompressTaggedAny(address(), value));
}

Object CompressedObjectSlot::Relaxed_Load(PtrComprCageBase cage_base) const {
  AtomicTagged_t value = AsAtomicTagged::Relaxed_Load(location());
  return Object(DecompressTaggedAny(cage_base, value));
}

void CompressedObjectSlot::Relaxed_Store(Object value) const {
  Tagged_t ptr = CompressTagged(value.ptr());
  AsAtomicTagged::Relaxed_Store(location(), ptr);
}

void CompressedObjectSlot::Release_Store(Object value) const {
  Tagged_t ptr = CompressTagged(value.ptr());
  AsAtomicTagged::Release_Store(location(), ptr);
}

Object CompressedObjectSlot::Release_CompareAndSwap(Object old,
                                                    Object target) const {
  Tagged_t old_ptr = CompressTagged(old.ptr());
  Tagged_t target_ptr = CompressTagged(target.ptr());
  Tagged_t result =
      AsAtomicTagged::Release_CompareAndSwap(location(), old_ptr, target_ptr);
  return Object(DecompressTaggedAny(address(), result));
}

//
// CompressedMaybeObjectSlot implementation.
//

MaybeObject CompressedMaybeObjectSlot::operator*() const {
  Tagged_t value = *location();
  return MaybeObject(DecompressTaggedAny(address(), value));
}

MaybeObject CompressedMaybeObjectSlot::load(PtrComprCageBase cage_base) const {
  Tagged_t value = *location();
  return MaybeObject(DecompressTaggedAny(cage_base, value));
}

void CompressedMaybeObjectSlot::store(MaybeObject value) const {
  *location() = CompressTagged(value.ptr());
}

MaybeObject CompressedMaybeObjectSlot::Relaxed_Load() const {
  AtomicTagged_t value = AsAtomicTagged::Relaxed_Load(location());
  return MaybeObject(DecompressTaggedAny(address(), value));
}

MaybeObject CompressedMaybeObjectSlot::Relaxed_Load(
    PtrComprCageBase cage_base) const {
  AtomicTagged_t value = AsAtomicTagged::Relaxed_Load(location());
  return MaybeObject(DecompressTaggedAny(cage_base, value));
}

void CompressedMaybeObjectSlot::Relaxed_Store(MaybeObject value) const {
  Tagged_t ptr = CompressTagged(value.ptr());
  AsAtomicTagged::Relaxed_Store(location(), ptr);
}

void CompressedMaybeObjectSlot::Release_CompareAndSwap(
    MaybeObject old, MaybeObject target) const {
  Tagged_t old_ptr = CompressTagged(old.ptr());
  Tagged_t target_ptr = CompressTagged(target.ptr());
  AsAtomicTagged::Release_CompareAndSwap(location(), old_ptr, target_ptr);
}

//
// CompressedHeapObjectSlot implementation.
//

HeapObjectReference CompressedHeapObjectSlot::operator*() const {
  Tagged_t value = *location();
  return HeapObjectReference(DecompressTaggedPointer(address(), value));
}

HeapObjectReference CompressedHeapObjectSlot::load(
    PtrComprCageBase cage_base) const {
  Tagged_t value = *location();
  return HeapObjectReference(DecompressTaggedPointer(cage_base, value));
}

void CompressedHeapObjectSlot::store(HeapObjectReference value) const {
  *location() = CompressTagged(value.ptr());
}

HeapObject CompressedHeapObjectSlot::ToHeapObject() const {
  Tagged_t value = *location();
  DCHECK_EQ(value & kHeapObjectTagMask, kHeapObjectTag);
  return HeapObject::cast(Object(DecompressTaggedPointer(address(), value)));
}

void CompressedHeapObjectSlot::StoreHeapObject(HeapObject value) const {
  *location() = CompressTagged(value.ptr());
}

//
// OffHeapCompressedObjectSlot implementation.
//

Object OffHeapCompressedObjectSlot::load(PtrComprCageBase cage_base) const {
  Tagged_t value = *location();
  return Object(DecompressTaggedAny(cage_base, value));
}

void OffHeapCompressedObjectSlot::store(Object value) const {
  *location() = CompressTagged(value.ptr());
}

Object OffHeapCompressedObjectSlot::Relaxed_Load(
    PtrComprCageBase cage_base) const {
  AtomicTagged_t value = AsAtomicTagged::Relaxed_Load(location());
  return Object(DecompressTaggedAny(cage_base, value));
}

Object OffHeapCompressedObjectSlot::Acquire_Load(
    PtrComprCageBase cage_base) const {
  AtomicTagged_t value = AsAtomicTagged::Acquire_Load(location());
  return Object(DecompressTaggedAny(cage_base, value));
}

void OffHeapCompressedObjectSlot::Relaxed_Store(Object value) const {
  Tagged_t ptr = CompressTagged(value.ptr());
  AsAtomicTagged::Relaxed_Store(location(), ptr);
}

void OffHeapCompressedObjectSlot::Release_Store(Object value) const {
  Tagged_t ptr = CompressTagged(value.ptr());
  AsAtomicTagged::Release_Store(location(), ptr);
}

void OffHeapCompressedObjectSlot::Release_CompareAndSwap(Object old,
                                                         Object target) const {
  Tagged_t old_ptr = CompressTagged(old.ptr());
  Tagged_t target_ptr = CompressTagged(target.ptr());
  AsAtomicTagged::Release_CompareAndSwap(location(), old_ptr, target_ptr);
}

}  // namespace internal
}  // namespace v8

#endif  // V8_COMPRESS_POINTERS

#endif  // V8_OBJECTS_COMPRESSED_SLOTS_INL_H_
