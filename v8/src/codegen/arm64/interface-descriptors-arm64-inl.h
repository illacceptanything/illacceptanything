// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_CODEGEN_ARM64_INTERFACE_DESCRIPTORS_ARM64_INL_H_
#define V8_CODEGEN_ARM64_INTERFACE_DESCRIPTORS_ARM64_INL_H_

#if V8_TARGET_ARCH_ARM64

#include "src/base/template-utils.h"
#include "src/codegen/interface-descriptors.h"
#include "src/execution/frames.h"

namespace v8 {
namespace internal {

constexpr auto CallInterfaceDescriptor::DefaultRegisterArray() {
  auto registers = RegisterArray(x0, x1, x2, x3, x4);
  STATIC_ASSERT(registers.size() == kMaxBuiltinRegisterParams);
  return registers;
}

// static
constexpr auto WriteBarrierDescriptor::registers() {
  return RegisterArray(x0, x1, x2, x3, x4, kReturnRegister0);
}

// static
constexpr auto DynamicCheckMapsDescriptor::registers() {
  return RegisterArray(x0, x1, x2, x3, cp);
}

// static
constexpr Register LoadDescriptor::ReceiverRegister() { return x1; }
// static
constexpr Register LoadDescriptor::NameRegister() { return x2; }
// static
constexpr Register LoadDescriptor::SlotRegister() { return x0; }

// static
constexpr Register LoadWithVectorDescriptor::VectorRegister() { return x3; }

// static
constexpr Register
LoadWithReceiverAndVectorDescriptor::LookupStartObjectRegister() {
  return x4;
}

// static
constexpr Register StoreDescriptor::ReceiverRegister() { return x1; }
// static
constexpr Register StoreDescriptor::NameRegister() { return x2; }
// static
constexpr Register StoreDescriptor::ValueRegister() { return x0; }
// static
constexpr Register StoreDescriptor::SlotRegister() { return x4; }

// static
constexpr Register StoreWithVectorDescriptor::VectorRegister() { return x3; }

// static
constexpr Register StoreTransitionDescriptor::MapRegister() { return x5; }

// static
constexpr Register ApiGetterDescriptor::HolderRegister() { return x0; }
// static
constexpr Register ApiGetterDescriptor::CallbackRegister() { return x3; }

// static
constexpr Register GrowArrayElementsDescriptor::ObjectRegister() { return x0; }
// static
constexpr Register GrowArrayElementsDescriptor::KeyRegister() { return x3; }

// static
constexpr Register BaselineLeaveFrameDescriptor::ParamsSizeRegister() {
  return x3;
}
// static
constexpr Register BaselineLeaveFrameDescriptor::WeightRegister() { return x4; }

// static
// static
constexpr Register TypeConversionDescriptor::ArgumentRegister() { return x0; }

// static
constexpr auto TypeofDescriptor::registers() { return RegisterArray(x3); }

// static
constexpr auto CallTrampolineDescriptor::registers() {
  // x1: target
  // x0: number of arguments
  return RegisterArray(x1, x0);
}

// static
constexpr auto CallVarargsDescriptor::registers() {
  // x0 : number of arguments (on the stack, not including receiver)
  // x1 : the target to call
  // x4 : arguments list length (untagged)
  // x2 : arguments list (FixedArray)
  return RegisterArray(x1, x0, x4, x2);
}

// static
constexpr auto CallForwardVarargsDescriptor::registers() {
  // x1: target
  // x0: number of arguments
  // x2: start index (to supported rest parameters)
  return RegisterArray(x1, x0, x2);
}

// static
constexpr auto CallFunctionTemplateDescriptor::registers() {
  // x1 : function template info
  // x2 : number of arguments (on the stack, not including receiver)
  return RegisterArray(x1, x2);
}

// static
constexpr auto CallWithSpreadDescriptor::registers() {
  // x0 : number of arguments (on the stack, not including receiver)
  // x1 : the target to call
  // x2 : the object to spread
  return RegisterArray(x1, x0, x2);
}

// static
constexpr auto CallWithArrayLikeDescriptor::registers() {
  // x1 : the target to call
  // x2 : the arguments list
  return RegisterArray(x1, x2);
}

// static
constexpr auto ConstructVarargsDescriptor::registers() {
  // x0 : number of arguments (on the stack, not including receiver)
  // x1 : the target to call
  // x3 : the new target
  // x4 : arguments list length (untagged)
  // x2 : arguments list (FixedArray)
  return RegisterArray(x1, x3, x0, x4, x2);
}

// static
constexpr auto ConstructForwardVarargsDescriptor::registers() {
  // x3: new target
  // x1: target
  // x0: number of arguments
  // x2: start index (to supported rest parameters)
  return RegisterArray(x1, x3, x0, x2);
}

// static
constexpr auto ConstructWithSpreadDescriptor::registers() {
  // x0 : number of arguments (on the stack, not including receiver)
  // x1 : the target to call
  // x3 : the new target
  // x2 : the object to spread
  return RegisterArray(x1, x3, x0, x2);
}

// static
constexpr auto ConstructWithArrayLikeDescriptor::registers() {
  // x1 : the target to call
  // x3 : the new target
  // x2 : the arguments list
  return RegisterArray(x1, x3, x2);
}

// static
constexpr auto ConstructStubDescriptor::registers() {
  // x3: new target
  // x1: target
  // x0: number of arguments
  // x2: allocation site or undefined
  return RegisterArray(x1, x3, x0, x2);
}

// static
constexpr auto AbortDescriptor::registers() { return RegisterArray(x1); }

// static
constexpr auto CompareDescriptor::registers() {
  // x1: left operand
  // x0: right operand
  return RegisterArray(x1, x0);
}

// static
constexpr auto Compare_BaselineDescriptor::registers() {
  // x1: left operand
  // x0: right operand
  // x2: feedback slot
  return RegisterArray(x1, x0, x2);
}

// static
constexpr auto BinaryOpDescriptor::registers() {
  // x1: left operand
  // x0: right operand
  return RegisterArray(x1, x0);
}

// static
constexpr auto BinaryOp_BaselineDescriptor::registers() {
  // x1: left operand
  // x0: right operand
  // x2: feedback slot
  return RegisterArray(x1, x0, x2);
}

// static
constexpr auto ApiCallbackDescriptor::registers() {
  return RegisterArray(x1,   // kApiFunctionAddress
                       x2,   // kArgc
                       x3,   // kCallData
                       x0);  // kHolder
}

// static
constexpr auto InterpreterDispatchDescriptor::registers() {
  return RegisterArray(
      kInterpreterAccumulatorRegister, kInterpreterBytecodeOffsetRegister,
      kInterpreterBytecodeArrayRegister, kInterpreterDispatchTableRegister);
}

// static
constexpr auto InterpreterPushArgsThenCallDescriptor::registers() {
  return RegisterArray(x0,   // argument count (not including receiver)
                       x2,   // address of first argument
                       x1);  // the target callable to be call
}

// static
constexpr auto InterpreterPushArgsThenConstructDescriptor::registers() {
  return RegisterArray(
      x0,   // argument count (not including receiver)
      x4,   // address of the first argument
      x1,   // constructor to call
      x3,   // new target
      x2);  // allocation site feedback if available, undefined otherwise
}

// static
constexpr auto ResumeGeneratorDescriptor::registers() {
  return RegisterArray(x0,   // the value to pass to the generator
                       x1);  // the JSGeneratorObject to resume
}

// static
constexpr auto RunMicrotasksEntryDescriptor::registers() {
  return RegisterArray(x0, x1);
}

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_ARM64

#endif  // V8_CODEGEN_ARM64_INTERFACE_DESCRIPTORS_ARM64_INL_H_
