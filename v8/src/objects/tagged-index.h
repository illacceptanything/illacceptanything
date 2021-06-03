// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_TAGGED_INDEX_H_
#define V8_OBJECTS_TAGGED_INDEX_H_

#include "src/common/globals.h"
#include "src/objects/heap-object.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

// TaggedIndex represents integer values that can be stored in 31 bits.
// The on 32-bit architectures ptr_ value has the following format:
//   [31 bit signed int] 0
// The on 64-bit architectures ptr_ value has the following format:
//   [32 bits of sign-extended lower part][31 bit signed int] 0
// Thus, on 32-bit architectures TaggedIndex is exactly the same as Smi but
// on 64-bit architectures TaggedIndex differs from Smi in the following
// aspects:
// 1) TaggedIndex payload is always 31 bit independent of the Smi payload size
// 2) TaggedIndex is always properly sign-extended independent of whether
//    pointer compression is enabled or not. In the former case, upper 32 bits
//    of a Smi value may contain 0 or sign or isolate root value.
//
// Given the above constraints TaggedIndex has the following properties:
// 1) it still looks like a Smi from GC point of view and therefore it's safe
//   to pass TaggedIndex values to runtime functions or builtins on the stack
// 2) since the TaggedIndex values are already properly sign-extended it's
//   safe to use them as indices in offset-computation functions.
class TaggedIndex : public Object {
 public:
  // This replaces the OBJECT_CONSTRUCTORS macro, because TaggedIndex are
  // special in that we want them to be constexprs.
  constexpr TaggedIndex() : Object() {}
  explicit constexpr TaggedIndex(Address ptr) : Object(ptr) {
    DCHECK(HAS_SMI_TAG(ptr));
  }

  // Returns the integer value.
  inline intptr_t value() const {
    // Truncate and shift down (requires >> to be sign extending).
    return static_cast<intptr_t>(ptr()) >> kSmiTagSize;
  }

  // Convert a value to a TaggedIndex object.
  static inline TaggedIndex FromIntptr(intptr_t value) {
    DCHECK(TaggedIndex::IsValid(value));
    return TaggedIndex((static_cast<Address>(value) << kSmiTagSize) | kSmiTag);
  }

  // Returns whether value can be represented in a TaggedIndex.
  static inline bool constexpr IsValid(intptr_t value) {
    return kMinValue <= value && value <= kMaxValue;
  }

  DECL_CAST(TaggedIndex)

  // Dispatched behavior.
  DECL_VERIFIER(TaggedIndex)

  STATIC_ASSERT(kSmiTagSize == 1);
  static constexpr int kTaggedValueSize = 31;
  static constexpr intptr_t kMinValue =
      static_cast<intptr_t>(kUintptrAllBitsSet << (kTaggedValueSize - 1));
  static constexpr intptr_t kMaxValue = -(kMinValue + 1);
};

CAST_ACCESSOR(TaggedIndex)

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_TAGGED_INDEX_H_
