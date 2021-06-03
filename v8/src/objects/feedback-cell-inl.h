// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_FEEDBACK_CELL_INL_H_
#define V8_OBJECTS_FEEDBACK_CELL_INL_H_

#include "src/objects/feedback-cell.h"

#include "src/heap/heap-write-barrier-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/struct-inl.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/feedback-cell-tq-inl.inc"

TQ_OBJECT_CONSTRUCTORS_IMPL(FeedbackCell)

RELEASE_ACQUIRE_ACCESSORS(FeedbackCell, value, HeapObject, kValueOffset)

void FeedbackCell::clear_padding() {
  if (FeedbackCell::kAlignedSize == FeedbackCell::kUnalignedSize) return;
  DCHECK_GE(FeedbackCell::kAlignedSize, FeedbackCell::kUnalignedSize);
  memset(reinterpret_cast<byte*>(address() + FeedbackCell::kUnalignedSize), 0,
         FeedbackCell::kAlignedSize - FeedbackCell::kUnalignedSize);
}

void FeedbackCell::reset_feedback_vector(
    base::Optional<std::function<void(HeapObject object, ObjectSlot slot,
                                      HeapObject target)>>
        gc_notify_updated_slot) {
  SetInitialInterruptBudget();
  if (value().IsUndefined() || value().IsClosureFeedbackCellArray()) return;

  CHECK(value().IsFeedbackVector());
  ClosureFeedbackCellArray closure_feedback_cell_array =
      FeedbackVector::cast(value()).closure_feedback_cell_array();
  set_value(closure_feedback_cell_array);
  if (gc_notify_updated_slot) {
    (*gc_notify_updated_slot)(*this, RawField(FeedbackCell::kValueOffset),
                              closure_feedback_cell_array);
  }
}

void FeedbackCell::SetInitialInterruptBudget() {
  if (FLAG_lazy_feedback_allocation) {
    set_interrupt_budget(FLAG_budget_for_feedback_vector_allocation);
  } else {
    set_interrupt_budget(FLAG_interrupt_budget);
  }
}


void FeedbackCell::IncrementClosureCount(Isolate* isolate) {
  ReadOnlyRoots r(isolate);
  if (map() == r.no_closures_cell_map()) {
    set_map(r.one_closure_cell_map());
  } else if (map() == r.one_closure_cell_map()) {
    set_map(r.many_closures_cell_map());
  } else {
    DCHECK(map() == r.many_closures_cell_map());
  }
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_FEEDBACK_CELL_INL_H_
