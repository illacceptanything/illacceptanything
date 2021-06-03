// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_JS_WEAK_REFS_H_
#define V8_OBJECTS_JS_WEAK_REFS_H_

#include "src/objects/js-objects.h"
#include "torque-generated/bit-fields.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

class NativeContext;
class WeakCell;

#include "torque-generated/src/objects/js-weak-refs-tq.inc"

// FinalizationRegistry object from the JS Weak Refs spec proposal:
// https://github.com/tc39/proposal-weakrefs
class JSFinalizationRegistry : public JSObject {
 public:
  DECL_PRINTER(JSFinalizationRegistry)
  EXPORT_DECL_VERIFIER(JSFinalizationRegistry)
  DECL_CAST(JSFinalizationRegistry)

  DECL_ACCESSORS(native_context, NativeContext)
  DECL_ACCESSORS(cleanup, Object)

  DECL_ACCESSORS(active_cells, HeapObject)
  DECL_ACCESSORS(cleared_cells, HeapObject)
  DECL_ACCESSORS(key_map, Object)

  DECL_ACCESSORS(next_dirty, Object)

  DECL_INT_ACCESSORS(flags)

  DECL_BOOLEAN_ACCESSORS(scheduled_for_cleanup)

  class BodyDescriptor;

  inline static void RegisterWeakCellWithUnregisterToken(
      Handle<JSFinalizationRegistry> finalization_registry,
      Handle<WeakCell> weak_cell, Isolate* isolate);
  inline static bool Unregister(
      Handle<JSFinalizationRegistry> finalization_registry,
      Handle<JSReceiver> unregister_token, Isolate* isolate);

  // RemoveUnregisterToken is called from both Unregister and during GC. Since
  // it modifies slots in key_map and WeakCells and the normal write barrier is
  // disabled during GC, we need to tell the GC about the modified slots via the
  // gc_notify_updated_slot function.
  template <typename MatchCallback, typename GCNotifyUpdatedSlotCallback>
  inline bool RemoveUnregisterToken(
      JSReceiver unregister_token, Isolate* isolate,
      MatchCallback match_callback,
      GCNotifyUpdatedSlotCallback gc_notify_updated_slot);

  // Returns true if the cleared_cells list is non-empty.
  inline bool NeedsCleanup() const;

  // Remove the already-popped weak_cell from its unregister token linked list,
  // as well as removing the entry from the key map if it is the only WeakCell
  // with its unregister token. This method cannot GC and does not shrink the
  // key map. Asserts that weak_cell has a non-undefined unregister token.
  //
  // It takes raw Addresses because it is called from CSA and Torque.
  V8_EXPORT_PRIVATE static void RemoveCellFromUnregisterTokenMap(
      Isolate* isolate, Address raw_finalization_registry,
      Address raw_weak_cell);

  // Layout description.
  DEFINE_FIELD_OFFSET_CONSTANTS(
      JSObject::kHeaderSize, TORQUE_GENERATED_JS_FINALIZATION_REGISTRY_FIELDS)

  // Bitfields in flags.
  DEFINE_TORQUE_GENERATED_FINALIZATION_REGISTRY_FLAGS()

  OBJECT_CONSTRUCTORS(JSFinalizationRegistry, JSObject);
};

// Internal object for storing weak references in JSFinalizationRegistry.
class WeakCell : public TorqueGeneratedWeakCell<WeakCell, HeapObject> {
 public:
  DECL_PRINTER(WeakCell)
  EXPORT_DECL_VERIFIER(WeakCell)

  class BodyDescriptor;

  // Provide relaxed load access to target field.
  inline HeapObject relaxed_target() const;

  // Nullify is called during GC and it modifies the pointers in WeakCell and
  // JSFinalizationRegistry. Thus we need to tell the GC about the modified
  // slots via the gc_notify_updated_slot function. The normal write barrier is
  // not enough, since it's disabled before GC.
  template <typename GCNotifyUpdatedSlotCallback>
  inline void Nullify(Isolate* isolate,
                      GCNotifyUpdatedSlotCallback gc_notify_updated_slot);

  inline void RemoveFromFinalizationRegistryCells(Isolate* isolate);

  TQ_OBJECT_CONSTRUCTORS(WeakCell)
};

class JSWeakRef : public TorqueGeneratedJSWeakRef<JSWeakRef, JSObject> {
 public:
  DECL_PRINTER(JSWeakRef)
  EXPORT_DECL_VERIFIER(JSWeakRef)

  class BodyDescriptor;

  TQ_OBJECT_CONSTRUCTORS(JSWeakRef)
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_JS_WEAK_REFS_H_
