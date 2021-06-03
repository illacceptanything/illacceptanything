// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASELINE_BASELINE_ASSEMBLER_INL_H_
#define V8_BASELINE_BASELINE_ASSEMBLER_INL_H_

// TODO(v8:11421): Remove #if once baseline compiler is ported to other
// architectures.
#if V8_TARGET_ARCH_IA32 || V8_TARGET_ARCH_X64 || V8_TARGET_ARCH_ARM64 || \
    V8_TARGET_ARCH_ARM || V8_TARGET_ARCH_RISCV64

#include <type_traits>
#include <unordered_map>

#include "src/baseline/baseline-assembler.h"
#include "src/codegen/interface-descriptors-inl.h"
#include "src/interpreter/bytecode-register.h"
#include "src/objects/feedback-cell.h"
#include "src/objects/js-function.h"
#include "src/objects/map.h"

#if V8_TARGET_ARCH_X64
#include "src/baseline/x64/baseline-assembler-x64-inl.h"
#elif V8_TARGET_ARCH_ARM64
#include "src/baseline/arm64/baseline-assembler-arm64-inl.h"
#elif V8_TARGET_ARCH_IA32
#include "src/baseline/ia32/baseline-assembler-ia32-inl.h"
#elif V8_TARGET_ARCH_ARM
#include "src/baseline/arm/baseline-assembler-arm-inl.h"
#elif V8_TARGET_ARCH_RISCV64
#include "src/baseline/riscv64/baseline-assembler-riscv64-inl.h"
#else
#error Unsupported target architecture.
#endif

namespace v8 {
namespace internal {
namespace baseline {

#define __ masm_->

void BaselineAssembler::GetCode(Isolate* isolate, CodeDesc* desc) {
  __ GetCode(isolate, desc);
}
int BaselineAssembler::pc_offset() const { return __ pc_offset(); }
void BaselineAssembler::CodeEntry() const { __ CodeEntry(); }
void BaselineAssembler::ExceptionHandler() const { __ ExceptionHandler(); }
void BaselineAssembler::RecordComment(const char* string) {
  if (!FLAG_code_comments) return;
  __ RecordComment(string);
}
void BaselineAssembler::Trap() { __ Trap(); }
void BaselineAssembler::DebugBreak() { __ DebugBreak(); }
void BaselineAssembler::CallRuntime(Runtime::FunctionId function, int nargs) {
  __ CallRuntime(function, nargs);
}

MemOperand BaselineAssembler::ContextOperand() {
  return RegisterFrameOperand(interpreter::Register::current_context());
}
MemOperand BaselineAssembler::FunctionOperand() {
  return RegisterFrameOperand(interpreter::Register::function_closure());
}

void BaselineAssembler::LoadMap(Register output, Register value) {
  __ LoadMap(output, value);
}
void BaselineAssembler::LoadRoot(Register output, RootIndex index) {
  __ LoadRoot(output, index);
}
void BaselineAssembler::LoadNativeContextSlot(Register output, uint32_t index) {
  __ LoadNativeContextSlot(output, index);
}

void BaselineAssembler::Move(Register output, interpreter::Register source) {
  return __ Move(output, RegisterFrameOperand(source));
}
void BaselineAssembler::Move(Register output, RootIndex source) {
  return __ LoadRoot(output, source);
}
void BaselineAssembler::Move(Register output, Register source) {
  __ Move(output, source);
}
void BaselineAssembler::Move(Register output, MemOperand operand) {
  __ Move(output, operand);
}
void BaselineAssembler::Move(Register output, Smi value) {
  __ Move(output, value);
}

void BaselineAssembler::SmiUntag(Register reg) { __ SmiUntag(reg); }
void BaselineAssembler::SmiUntag(Register output, Register value) {
  __ SmiUntag(output, value);
}

void BaselineAssembler::LoadFixedArrayElement(Register output, Register array,
                                              int32_t index) {
  LoadTaggedAnyField(output, array,
                     FixedArray::kHeaderSize + index * kTaggedSize);
}

void BaselineAssembler::LoadPrototype(Register prototype, Register object) {
  __ LoadMap(prototype, object);
  LoadTaggedPointerField(prototype, prototype, Map::kPrototypeOffset);
}
void BaselineAssembler::LoadContext(Register output) {
  LoadRegister(output, interpreter::Register::current_context());
}
void BaselineAssembler::LoadFunction(Register output) {
  LoadRegister(output, interpreter::Register::function_closure());
}
void BaselineAssembler::StoreContext(Register context) {
  StoreRegister(interpreter::Register::current_context(), context);
}
void BaselineAssembler::LoadRegister(Register output,
                                     interpreter::Register source) {
  Move(output, source);
}
void BaselineAssembler::StoreRegister(interpreter::Register output,
                                      Register value) {
  Move(output, value);
}

SaveAccumulatorScope::SaveAccumulatorScope(BaselineAssembler* assembler)
    : assembler_(assembler) {
  assembler_->Push(kInterpreterAccumulatorRegister);
}

SaveAccumulatorScope::~SaveAccumulatorScope() {
  assembler_->Pop(kInterpreterAccumulatorRegister);
}

#undef __

}  // namespace baseline
}  // namespace internal
}  // namespace v8

#endif

#endif  // V8_BASELINE_BASELINE_ASSEMBLER_INL_H_
