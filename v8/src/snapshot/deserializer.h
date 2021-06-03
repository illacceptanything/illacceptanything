// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_SNAPSHOT_DESERIALIZER_H_
#define V8_SNAPSHOT_DESERIALIZER_H_

#include <utility>
#include <vector>

#include "src/common/globals.h"
#include "src/objects/allocation-site.h"
#include "src/objects/api-callbacks.h"
#include "src/objects/backing-store.h"
#include "src/objects/code.h"
#include "src/objects/js-array.h"
#include "src/objects/map.h"
#include "src/objects/string-table.h"
#include "src/objects/string.h"
#include "src/snapshot/serializer-deserializer.h"
#include "src/snapshot/snapshot-source-sink.h"

namespace v8 {
namespace internal {

class HeapObject;
class Object;

// Used for platforms with embedded constant pools to trigger deserialization
// of objects found in code.
#if defined(V8_TARGET_ARCH_MIPS) || defined(V8_TARGET_ARCH_MIPS64) ||   \
    defined(V8_TARGET_ARCH_PPC) || defined(V8_TARGET_ARCH_S390) ||      \
    defined(V8_TARGET_ARCH_PPC64) || defined(V8_TARGET_ARCH_RISCV64) || \
    V8_EMBEDDED_CONSTANT_POOL
#define V8_CODE_EMBEDS_OBJECT_POINTER 1
#else
#define V8_CODE_EMBEDS_OBJECT_POINTER 0
#endif

// A Deserializer reads a snapshot and reconstructs the Object graph it defines.
class V8_EXPORT_PRIVATE Deserializer : public SerializerDeserializer {
 public:
  // Smi value for filling in not-yet initialized tagged field values with a
  // valid tagged pointer. A field value equal to this doesn't necessarily
  // indicate that a field is uninitialized, but an uninitialized field should
  // definitely equal this value.
  //
  // This _has_ to be kNullAddress, so that an uninitialized_field_value read as
  // an embedded pointer field is interpreted as nullptr. This is so that
  // uninitialised embedded pointers are not forwarded to the embedded as part
  // of embedder tracing (and similar mechanisms), as nullptrs are skipped for
  // those cases and otherwise the embedder would try to dereference the
  // uninitialized pointer value.
  static constexpr Smi uninitialized_field_value() { return Smi(kNullAddress); }

  ~Deserializer() override;
  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;

  uint32_t GetChecksum() const { return source_.GetChecksum(); }

 protected:
  // Create a deserializer from a snapshot byte source.
  Deserializer(Isolate* isolate, Vector<const byte> payload,
               uint32_t magic_number, bool deserializing_user_code,
               bool can_rehash);

  void DeserializeDeferredObjects();

  // Create Log events for newly deserialized objects.
  void LogNewObjectEvents();
  void LogScriptEvents(Script script);
  void LogNewMapEvents();

  // Descriptor arrays are deserialized as "strong", so that there is no risk of
  // them getting trimmed during a partial deserialization. This method makes
  // them "weak" again after deserialization completes.
  void WeakenDescriptorArrays();

  // This returns the address of an object that has been described in the
  // snapshot by object vector index.
  Handle<HeapObject> GetBackReferencedObject();

  // Add an object to back an attached reference. The order to add objects must
  // mirror the order they are added in the serializer.
  void AddAttachedObject(Handle<HeapObject> attached_object) {
    attached_objects_.push_back(attached_object);
  }

  void CheckNoArrayBufferBackingStores() {
    CHECK_EQ(new_off_heap_array_buffers().size(), 0);
  }

  Isolate* isolate() const { return isolate_; }

  SnapshotByteSource* source() { return &source_; }
  const std::vector<Handle<AllocationSite>>& new_allocation_sites() const {
    return new_allocation_sites_;
  }
  const std::vector<Handle<Code>>& new_code_objects() const {
    return new_code_objects_;
  }
  const std::vector<Handle<Map>>& new_maps() const { return new_maps_; }
  const std::vector<Handle<AccessorInfo>>& accessor_infos() const {
    return accessor_infos_;
  }
  const std::vector<Handle<CallHandlerInfo>>& call_handler_infos() const {
    return call_handler_infos_;
  }
  const std::vector<Handle<Script>>& new_scripts() const {
    return new_scripts_;
  }

  const std::vector<Handle<JSArrayBuffer>>& new_off_heap_array_buffers() const {
    return new_off_heap_array_buffers_;
  }

  const std::vector<Handle<DescriptorArray>>& new_descriptor_arrays() const {
    return new_descriptor_arrays_;
  }

  std::shared_ptr<BackingStore> backing_store(size_t i) {
    DCHECK_LT(i, backing_stores_.size());
    return backing_stores_[i];
  }

  bool deserializing_user_code() const { return deserializing_user_code_; }
  bool can_rehash() const { return can_rehash_; }

  void Rehash();

  Handle<HeapObject> ReadObject();

 private:
  class RelocInfoVisitor;
  // A circular queue of hot objects. This is added to in the same order as in
  // Serializer::HotObjectsList, but this stores the objects as a vector of
  // existing handles. This allows us to add Handles to the queue without having
  // to create new handles. Note that this depends on those Handles staying
  // valid as long as the HotObjectsList is alive.
  class HotObjectsList {
   public:
    HotObjectsList() = default;
    HotObjectsList(const HotObjectsList&) = delete;
    HotObjectsList& operator=(const HotObjectsList&) = delete;

    void Add(Handle<HeapObject> object) {
      circular_queue_[index_] = object;
      index_ = (index_ + 1) & kSizeMask;
    }

    Handle<HeapObject> Get(int index) {
      DCHECK(!circular_queue_[index].is_null());
      return circular_queue_[index];
    }

   private:
    static const int kSize = kHotObjectCount;
    static const int kSizeMask = kSize - 1;
    STATIC_ASSERT(base::bits::IsPowerOfTwo(kSize));
    Handle<HeapObject> circular_queue_[kSize];
    int index_ = 0;
  };

  void VisitRootPointers(Root root, const char* description,
                         FullObjectSlot start, FullObjectSlot end) override;

  void Synchronize(VisitorSynchronization::SyncTag tag) override;

  template <typename TSlot>
  inline int WriteAddress(TSlot dest, Address value);

  template <typename TSlot>
  inline int WriteExternalPointer(TSlot dest, Address value,
                                  ExternalPointerTag tag);

  // Fills in a heap object's data from start to end (exclusive). Start and end
  // are slot indices within the object.
  void ReadData(Handle<HeapObject> object, int start_slot_index,
                int end_slot_index);

  // Fills in a contiguous range of full object slots (e.g. root pointers) from
  // start to end (exclusive).
  void ReadData(FullMaybeObjectSlot start, FullMaybeObjectSlot end);

  // Helper for ReadData which reads the given bytecode and fills in some heap
  // data into the given slot. May fill in zero or multiple slots, so it returns
  // the number of slots filled.
  template <typename SlotAccessor>
  int ReadSingleBytecodeData(byte data, SlotAccessor slot_accessor);

  // A helper function for ReadData for reading external references.
  inline Address ReadExternalReferenceCase();

  Handle<HeapObject> ReadObject(SnapshotSpace space_number);
  Handle<HeapObject> ReadMetaMap();

  HeapObjectReferenceType GetAndResetNextReferenceType();

  template <typename SlotGetter>
  int ReadRepeatedObject(SlotGetter slot_getter, int repeat_count);

  // Special handling for serialized code like hooking up internalized strings.
  void PostProcessNewObject(Handle<Map> map, Handle<HeapObject> obj,
                            SnapshotSpace space);

  HeapObject Allocate(SnapshotSpace space, int size,
                      AllocationAlignment alignment);

  // Cached current isolate.
  Isolate* isolate_;

  // Objects from the attached object descriptions in the serialized user code.
  std::vector<Handle<HeapObject>> attached_objects_;

  SnapshotByteSource source_;
  uint32_t magic_number_;

  HotObjectsList hot_objects_;
  std::vector<Handle<Map>> new_maps_;
  std::vector<Handle<AllocationSite>> new_allocation_sites_;
  std::vector<Handle<Code>> new_code_objects_;
  std::vector<Handle<AccessorInfo>> accessor_infos_;
  std::vector<Handle<CallHandlerInfo>> call_handler_infos_;
  std::vector<Handle<Script>> new_scripts_;
  std::vector<Handle<JSArrayBuffer>> new_off_heap_array_buffers_;
  std::vector<Handle<DescriptorArray>> new_descriptor_arrays_;
  std::vector<std::shared_ptr<BackingStore>> backing_stores_;

  // Vector of allocated objects that can be accessed by a backref, by index.
  std::vector<Handle<HeapObject>> back_refs_;

  // Unresolved forward references (registered with kRegisterPendingForwardRef)
  // are collected in order as (object, field offset) pairs. The subsequent
  // forward ref resolution (with kResolvePendingForwardRef) accesses this
  // vector by index.
  //
  // The vector is cleared when there are no more unresolved forward refs.
  struct UnresolvedForwardRef {
    UnresolvedForwardRef(Handle<HeapObject> object, int offset,
                         HeapObjectReferenceType ref_type)
        : object(object), offset(offset), ref_type(ref_type) {}

    Handle<HeapObject> object;
    int offset;
    HeapObjectReferenceType ref_type;
  };
  std::vector<UnresolvedForwardRef> unresolved_forward_refs_;
  int num_unresolved_forward_refs_ = 0;

  const bool deserializing_user_code_;

  bool next_reference_is_weak_ = false;

  // TODO(6593): generalize rehashing, and remove this flag.
  bool can_rehash_;
  std::vector<Handle<HeapObject>> to_rehash_;

#ifdef DEBUG
  uint32_t num_api_references_;

  // Record the previous object allocated for DCHECKs.
  Handle<HeapObject> previous_allocation_obj_;
  int previous_allocation_size_ = 0;
#endif  // DEBUG
};

// Used to insert a deserialized internalized string into the string table.
class StringTableInsertionKey final : public StringTableKey {
 public:
  explicit StringTableInsertionKey(Handle<String> string);

  bool IsMatch(Isolate* isolate, String string);

  V8_WARN_UNUSED_RESULT Handle<String> AsHandle(Isolate* isolate);
  V8_WARN_UNUSED_RESULT Handle<String> AsHandle(LocalIsolate* isolate);

 private:
  uint32_t ComputeRawHashField(String string);

  Handle<String> string_;
  DISALLOW_GARBAGE_COLLECTION(no_gc)
};

}  // namespace internal
}  // namespace v8

#endif  // V8_SNAPSHOT_DESERIALIZER_H_
