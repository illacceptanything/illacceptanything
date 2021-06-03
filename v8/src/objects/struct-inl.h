// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_STRUCT_INL_H_
#define V8_OBJECTS_STRUCT_INL_H_

#include "src/objects/struct.h"

#include "src/heap/heap-write-barrier-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/oddball.h"
#include "src/roots/roots-inl.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/struct-tq-inl.inc"

TQ_OBJECT_CONSTRUCTORS_IMPL(Struct)
TQ_OBJECT_CONSTRUCTORS_IMPL(Tuple2)
TQ_OBJECT_CONSTRUCTORS_IMPL(AccessorPair)

NEVER_READ_ONLY_SPACE_IMPL(AccessorPair)

TQ_OBJECT_CONSTRUCTORS_IMPL(ClassPositions)

Object AccessorPair::get(AccessorComponent component) {
  return component == ACCESSOR_GETTER ? getter() : setter();
}

void AccessorPair::set(AccessorComponent component, Object value) {
  if (component == ACCESSOR_GETTER) {
    set_getter(value);
  } else {
    set_setter(value);
  }
}

void AccessorPair::SetComponents(Object getter, Object setter) {
  if (!getter.IsNull()) set_getter(getter);
  if (!setter.IsNull()) set_setter(setter);
}

bool AccessorPair::Equals(Object getter_value, Object setter_value) {
  return (getter() == getter_value) && (setter() == setter_value);
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_STRUCT_INL_H_
