// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_CODEGEN_X64_INTERFACE_DESCRIPTORS_X64_INL_H_
#define V8_CODEGEN_X64_INTERFACE_DESCRIPTORS_X64_INL_H_

#if V8_TARGET_ARCH_X64

#include "src/codegen/interface-descriptors.h"

namespace v8 {
namespace internal {

constexpr auto CallInterfaceDescriptor::DefaultRegisterArray() {
  auto registers = RegisterArray(rax, rbx, rcx, rdx, rdi);
  STATIC_ASSERT(registers.size() == kMaxBuiltinRegisterParams);
  return registers;
}

// static
constexpr auto WriteBarrierDescriptor::registers() {
  return RegisterArray(arg_reg_1, arg_reg_2, arg_reg_3, arg_reg_4,
                       kReturnRegister0);
}

// static
constexpr auto DynamicCheckMapsDescriptor::registers() {
  return RegisterArray(kReturnRegister0, arg_reg_1, arg_reg_2, arg_reg_3,
                       kRuntimeCallFunctionRegister, kContextRegister);
}

// static
constexpr Register LoadDescriptor::ReceiverRegister() { return rdx; }
// static
constexpr Register LoadDescriptor::NameRegister() { return rcx; }
// static
constexpr Register LoadDescriptor::SlotRegister() { return rax; }

// static
constexpr Register LoadWithVectorDescriptor::VectorRegister() { return rbx; }

// static
constexpr Register
LoadWithReceiverAndVectorDescriptor::LookupStartObjectRegister() {
  return rdi;
}

// static
constexpr Register StoreDescriptor::ReceiverRegister() { return rdx; }
// static
constexpr Register StoreDescriptor::NameRegister() { return rcx; }
// static
constexpr Register StoreDescriptor::ValueRegister() { return rax; }
// static
constexpr Register StoreDescriptor::SlotRegister() { return rdi; }

// static
constexpr Register StoreWithVectorDescriptor::VectorRegister() { return rbx; }

// static
constexpr Register StoreTransitionDescriptor::MapRegister() { return r11; }

// static
constexpr Register ApiGetterDescriptor::HolderRegister() { return rcx; }
// static
constexpr Register ApiGetterDescriptor::CallbackRegister() { return rbx; }

// static
constexpr Register GrowArrayElementsDescriptor::ObjectRegister() { return rax; }
// static
constexpr Register GrowArrayElementsDescriptor::KeyRegister() { return rbx; }

// static
constexpr Register BaselineLeaveFrameDescriptor::ParamsSizeRegister() {
  return rbx;
}
// static
constexpr Register BaselineLeaveFrameDescriptor::WeightRegister() {
  return rcx;
}

// static
constexpr Register TypeConversionDescriptor::ArgumentRegister() { return rax; }

// static
constexpr auto TypeofDescriptor::registers() { return RegisterArray(rbx); }

// static
constexpr auto CallTrampolineDescriptor::registers() {
  // rax : number of arguments
  // rdi : the target to call
  return RegisterArray(rdi, rax);
}

// static
constexpr auto CallVarargsDescriptor::registers() {
  // rax : number of arguments (on the stack, not including receiver)
  // rdi : the target to call
  // rcx : arguments list length (untagged)
  // rbx : arguments list (FixedArray)
  return RegisterArray(rdi, rax, rcx, rbx);
}

// static
constexpr auto CallForwardVarargsDescriptor::registers() {
  // rax : number of arguments
  // rcx : start index (to support rest parameters)
  // rdi : the target to call
  return RegisterArray(rdi, rax, rcx);
}

// static
constexpr auto CallFunctionTemplateDescriptor::registers() {
  // rdx: the function template info
  // rcx: number of arguments (on the stack, not including receiver)
  return RegisterArray(rdx, rcx);
}

// static
constexpr auto CallWithSpreadDescriptor::registers() {
  // rax : number of arguments (on the stack, not including receiver)
  // rdi : the target to call
  // rbx : the object to spread
  return RegisterArray(rdi, rax, rbx);
}

// static
constexpr auto CallWithArrayLikeDescriptor::registers() {
  // rdi : the target to call
  // rbx : the arguments list
  return RegisterArray(rdi, rbx);
}

// static
constexpr auto ConstructVarargsDescriptor::registers() {
  // rax : number of arguments (on the stack, not including receiver)
  // rdi : the target to call
  // rdx : the new target
  // rcx : arguments list length (untagged)
  // rbx : arguments list (FixedArray)
  return RegisterArray(rdi, rdx, rax, rcx, rbx);
}

// static
constexpr auto ConstructForwardVarargsDescriptor::registers() {
  // rax : number of arguments
  // rdx : the new target
  // rcx : start index (to support rest parameters)
  // rdi : the target to call
  return RegisterArray(rdi, rdx, rax, rcx);
}

// static
constexpr auto ConstructWithSpreadDescriptor::registers() {
  // rax : number of arguments (on the stack, not including receiver)
  // rdi : the target to call
  // rdx : the new target
  // rbx : the object to spread
  return RegisterArray(rdi, rdx, rax, rbx);
}

// static
constexpr auto ConstructWithArrayLikeDescriptor::registers() {
  // rdi : the target to call
  // rdx : the new target
  // rbx : the arguments list
  return RegisterArray(rdi, rdx, rbx);
}

// static
constexpr auto ConstructStubDescriptor::registers() {
  // rax : number of arguments
  // rdx : the new target
  // rdi : the target to call
  // rbx : allocation site or undefined
  return RegisterArray(rdi, rdx, rax, rbx);
}

// static
constexpr auto AbortDescriptor::registers() { return RegisterArray(rdx); }

// static
constexpr auto CompareDescriptor::registers() {
  return RegisterArray(rdx, rax);
}

// static
constexpr auto BinaryOpDescriptor::registers() {
  return RegisterArray(rdx, rax);
}

// static
constexpr auto Compare_BaselineDescriptor::registers() {
  return RegisterArray(rdx, rax, rbx);
}

// static
constexpr auto BinaryOp_BaselineDescriptor::registers() {
  return RegisterArray(rdx, rax, rbx);
}

// static
constexpr auto ApiCallbackDescriptor::registers() {
  return RegisterArray(rdx,   // api function address
                       rcx,   // argument count (not including receiver)
                       rbx,   // call data
                       rdi);  // holder
}

// static
constexpr auto InterpreterDispatchDescriptor::registers() {
  return RegisterArray(
      kInterpreterAccumulatorRegister, kInterpreterBytecodeOffsetRegister,
      kInterpreterBytecodeArrayRegister, kInterpreterDispatchTableRegister);
}

// static
constexpr auto InterpreterPushArgsThenCallDescriptor::registers() {
  return RegisterArray(rax,   // argument count (not including receiver)
                       rbx,   // address of first argument
                       rdi);  // the target callable to be call
}

// static
constexpr auto InterpreterPushArgsThenConstructDescriptor::registers() {
  return RegisterArray(
      rax,   // argument count (not including receiver)
      rcx,   // address of first argument
      rdi,   // constructor to call
      rdx,   // new target
      rbx);  // allocation site feedback if available, undefined otherwise
}

// static
constexpr auto ResumeGeneratorDescriptor::registers() {
  return RegisterArray(
      rax,   // the value to pass to the generator
      rdx);  // the JSGeneratorObject / JSAsyncGeneratorObject to resume
}

// static
constexpr auto RunMicrotasksEntryDescriptor::registers() {
  return RegisterArray(arg_reg_1, arg_reg_2);
}

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_X64

#endif  // V8_CODEGEN_X64_INTERFACE_DESCRIPTORS_X64_INL_H_
