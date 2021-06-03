// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_PROPERTY_CELL_H_
#define V8_OBJECTS_PROPERTY_CELL_H_

#include "src/objects/heap-object.h"
#include "torque-generated/field-offsets.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

class PropertyCell : public HeapObject {
 public:
  // [name]: the name of the global property.
  DECL_GETTER(name, Name)

  // [property_details]: details of the global property.
  DECL_GETTER(property_details_raw, Smi)
  DECL_ACQUIRE_GETTER(property_details_raw, Smi)
  inline PropertyDetails property_details() const;
  inline PropertyDetails property_details(AcquireLoadTag tag) const;
  inline void UpdatePropertyDetailsExceptCellType(PropertyDetails details);

  // [value]: value of the global property.
  DECL_GETTER(value, Object)
  DECL_ACQUIRE_GETTER(value, Object)

  // [dependent_code]: code that depends on the type of the global property.
  DECL_ACCESSORS(dependent_code, DependentCode)

  // Changes the value and/or property details.
  // For global properties:
  inline void Transition(PropertyDetails new_details, Handle<Object> new_value);
  // For protectors:
  void InvalidateProtector();

  static PropertyCellType InitialType(Isolate* isolate, Handle<Object> value);

  // Computes the new type of the cell's contents for the given value, but
  // without actually modifying the details.
  static PropertyCellType UpdatedType(Isolate* isolate,
                                      Handle<PropertyCell> cell,
                                      Handle<Object> value,
                                      PropertyDetails details);

  // Prepares property cell at given entry for receiving given value and sets
  // that value.  As a result the old cell could be invalidated and/or dependent
  // code could be deoptimized. Returns the (possibly new) property cell.
  static Handle<PropertyCell> PrepareForAndSetValue(
      Isolate* isolate, Handle<GlobalDictionary> dictionary,
      InternalIndex entry, Handle<Object> value, PropertyDetails details);

  void ClearAndInvalidate(ReadOnlyRoots roots);
  static Handle<PropertyCell> InvalidateAndReplaceEntry(
      Isolate* isolate, Handle<GlobalDictionary> dictionary,
      InternalIndex entry, PropertyDetails new_details,
      Handle<Object> new_value);

  // Whether or not the {details} and {value} fit together. This is an
  // approximation with false positives.
  static bool CheckDataIsCompatible(PropertyDetails details, Object value);

  DECL_CAST(PropertyCell)
  DECL_PRINTER(PropertyCell)
  DECL_VERIFIER(PropertyCell)

  DEFINE_FIELD_OFFSET_CONSTANTS(HeapObject::kHeaderSize,
                                TORQUE_GENERATED_PROPERTY_CELL_FIELDS)

  using BodyDescriptor = FixedBodyDescriptor<kNameOffset, kSize, kSize>;

  OBJECT_CONSTRUCTORS(PropertyCell, HeapObject);

 private:
  friend class Factory;

  DECL_SETTER(name, Name)
  DECL_SETTER(value, Object)
  DECL_RELEASE_SETTER(value, Object)
  DECL_SETTER(property_details_raw, Smi)
  DECL_RELEASE_SETTER(property_details_raw, Smi)

#ifdef DEBUG
  // Whether the property cell can transition to the given state. This is an
  // approximation with false positives.
  bool CanTransitionTo(PropertyDetails new_details, Object new_value) const;
#endif  // DEBUG
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_PROPERTY_CELL_H_
