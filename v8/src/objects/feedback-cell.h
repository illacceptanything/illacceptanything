// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_FEEDBACK_CELL_H_
#define V8_OBJECTS_FEEDBACK_CELL_H_

#include "src/objects/struct.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/feedback-cell-tq.inc"

// This is a special cell used to maintain both the link between a
// closure and its feedback vector, as well as a way to count the
// number of closures created for a certain function per native
// context. There's at most one FeedbackCell for each function in
// a native context.
class FeedbackCell : public TorqueGeneratedFeedbackCell<FeedbackCell, Struct> {
 public:
  // Dispatched behavior.
  DECL_PRINTER(FeedbackCell)

  static const int kUnalignedSize = kSize;
  static const int kAlignedSize = RoundUp<kObjectAlignment>(int{kSize});

  using TorqueGeneratedFeedbackCell<FeedbackCell, Struct>::value;
  using TorqueGeneratedFeedbackCell<FeedbackCell, Struct>::set_value;

  DECL_RELEASE_ACQUIRE_ACCESSORS(value, HeapObject)

  inline void clear_padding();
  inline void reset_feedback_vector(
      base::Optional<std::function<void(HeapObject object, ObjectSlot slot,
                                        HeapObject target)>>
          gc_notify_updated_slot = base::nullopt);
  inline void SetInitialInterruptBudget();

  // The closure count is encoded in the cell's map, which distinguishes
  // between zero, one, or many closures. This function records a new closure
  // creation by updating the map.
  inline void IncrementClosureCount(Isolate* isolate);

  using BodyDescriptor =
      FixedBodyDescriptor<kValueOffset, kInterruptBudgetOffset, kAlignedSize>;

  TQ_OBJECT_CONSTRUCTORS(FeedbackCell)
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_FEEDBACK_CELL_H_
