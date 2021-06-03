// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_PROPERTY_CELL_INL_H_
#define V8_OBJECTS_PROPERTY_CELL_INL_H_

#include "src/objects/property-cell.h"

#include "src/heap/heap-write-barrier-inl.h"
#include "src/objects/code-inl.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

OBJECT_CONSTRUCTORS_IMPL(PropertyCell, HeapObject)

CAST_ACCESSOR(PropertyCell)

ACCESSORS(PropertyCell, dependent_code, DependentCode, kDependentCodeOffset)
ACCESSORS(PropertyCell, name, Name, kNameOffset)
ACCESSORS(PropertyCell, property_details_raw, Smi, kPropertyDetailsRawOffset)
RELEASE_ACQUIRE_ACCESSORS(PropertyCell, property_details_raw, Smi,
                          kPropertyDetailsRawOffset)
ACCESSORS(PropertyCell, value, Object, kValueOffset)
RELEASE_ACQUIRE_ACCESSORS(PropertyCell, value, Object, kValueOffset)

PropertyDetails PropertyCell::property_details() const {
  return PropertyDetails(Smi::cast(property_details_raw()));
}

PropertyDetails PropertyCell::property_details(AcquireLoadTag tag) const {
  return PropertyDetails(Smi::cast(property_details_raw(tag)));
}

void PropertyCell::UpdatePropertyDetailsExceptCellType(
    PropertyDetails details) {
  DCHECK(CheckDataIsCompatible(details, value()));
  PropertyDetails old_details = property_details();
  CHECK_EQ(old_details.cell_type(), details.cell_type());
  set_property_details_raw(details.AsSmi(), kReleaseStore);
  // Deopt when making a writable property read-only. The reverse direction
  // is uninteresting because Turbofan does not currently rely on read-only
  // unless the property is also configurable, in which case it will stay
  // read-only forever.
  if (!old_details.IsReadOnly() && details.IsReadOnly()) {
    dependent_code().DeoptimizeDependentCodeGroup(
        DependentCode::kPropertyCellChangedGroup);
  }
}

void PropertyCell::Transition(PropertyDetails new_details,
                              Handle<Object> new_value) {
  DCHECK(CanTransitionTo(new_details, *new_value));
  // This code must be in sync with its counterpart in
  // PropertyCellData::Serialize.
  set_value(*new_value, kReleaseStore);
  set_property_details_raw(new_details.AsSmi(), kReleaseStore);
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_PROPERTY_CELL_INL_H_
