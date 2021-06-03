// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_STACK_FRAME_INFO_INL_H_
#define V8_OBJECTS_STACK_FRAME_INFO_INL_H_

#include "src/objects/stack-frame-info.h"

#include "src/heap/heap-write-barrier-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/struct-inl.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/stack-frame-info-tq-inl.inc"

TQ_OBJECT_CONSTRUCTORS_IMPL(StackFrameInfo)
NEVER_READ_ONLY_SPACE_IMPL(StackFrameInfo)

#if V8_ENABLE_WEBASSEMBLY
BOOL_GETTER(StackFrameInfo, flags, IsWasm, IsWasmBit::kShift)
BOOL_GETTER(StackFrameInfo, flags, IsAsmJsWasm, IsAsmJsWasmBit::kShift)
BOOL_GETTER(StackFrameInfo, flags, IsAsmJsAtNumberConversion,
            IsAsmJsAtNumberConversionBit::kShift)
#endif  // V8_ENABLE_WEBASSEMBLY
BOOL_GETTER(StackFrameInfo, flags, IsStrict, IsStrictBit::kShift)
BOOL_GETTER(StackFrameInfo, flags, IsConstructor, IsConstructorBit::kShift)
BOOL_GETTER(StackFrameInfo, flags, IsAsync, IsAsyncBit::kShift)

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_STACK_FRAME_INFO_INL_H_
