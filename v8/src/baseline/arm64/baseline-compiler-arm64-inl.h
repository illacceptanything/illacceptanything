// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASELINE_ARM64_BASELINE_COMPILER_ARM64_INL_H_
#define V8_BASELINE_ARM64_BASELINE_COMPILER_ARM64_INL_H_

#include "src/baseline/baseline-compiler.h"

namespace v8 {
namespace internal {
namespace baseline {

#define __ basm_.

void BaselineCompiler::Prologue() {
  // Enter the frame here, since CallBuiltin will override lr.
  __ masm()->EnterFrame(StackFrame::BASELINE);
  DCHECK_EQ(kJSFunctionRegister, kJavaScriptCallTargetRegister);
  int max_frame_size = bytecode_->frame_size() + max_call_args_;
  CallBuiltin<Builtins::kBaselineOutOfLinePrologue>(
      kContextRegister, kJSFunctionRegister, kJavaScriptCallArgCountRegister,
      max_frame_size, kJavaScriptCallNewTargetRegister, bytecode_);

  __ masm()->AssertSpAligned();
  PrologueFillFrame();
  __ masm()->AssertSpAligned();
}

void BaselineCompiler::PrologueFillFrame() {
  __ RecordComment("[ Fill frame");
  // Inlined register frame fill
  interpreter::Register new_target_or_generator_register =
      bytecode_->incoming_new_target_or_generator_register();
  __ LoadRoot(kInterpreterAccumulatorRegister, RootIndex::kUndefinedValue);
  int register_count = bytecode_->register_count();
  // Magic value
  const int kLoopUnrollSize = 8;
  const int new_target_index = new_target_or_generator_register.index();
  const bool has_new_target = new_target_index != kMaxInt;
  // BaselineOutOfLinePrologue already pushed one undefined.
  register_count -= 1;
  if (has_new_target) {
    if (new_target_index == 0) {
      // Oops, need to fix up that undefined that BaselineOutOfLinePrologue
      // pushed.
      __ masm()->Poke(kJavaScriptCallNewTargetRegister, Operand(0));
    } else {
      DCHECK_LE(new_target_index, register_count);
      int index = 1;
      for (; index + 2 <= new_target_index; index += 2) {
        __ masm()->Push(kInterpreterAccumulatorRegister,
                        kInterpreterAccumulatorRegister);
      }
      if (index == new_target_index) {
        __ masm()->Push(kJavaScriptCallNewTargetRegister,
                        kInterpreterAccumulatorRegister);
      } else {
        DCHECK_EQ(index, new_target_index - 1);
        __ masm()->Push(kInterpreterAccumulatorRegister,
                        kJavaScriptCallNewTargetRegister);
      }
      // We pushed "index" registers, minus the one the prologue pushed, plus
      // the two registers that included new_target.
      register_count -= (index - 1 + 2);
    }
  }
  if (register_count < 2 * kLoopUnrollSize) {
    // If the frame is small enough, just unroll the frame fill completely.
    for (int i = 0; i < register_count; i += 2) {
      __ masm()->Push(kInterpreterAccumulatorRegister,
                      kInterpreterAccumulatorRegister);
    }
  } else {
    BaselineAssembler::ScratchRegisterScope temps(&basm_);
    Register scratch = temps.AcquireScratch();

    // Extract the first few registers to round to the unroll size.
    int first_registers = register_count % kLoopUnrollSize;
    for (int i = 0; i < first_registers; i += 2) {
      __ masm()->Push(kInterpreterAccumulatorRegister,
                      kInterpreterAccumulatorRegister);
    }
    __ Move(scratch, register_count / kLoopUnrollSize);
    // We enter the loop unconditionally, so make sure we need to loop at least
    // once.
    DCHECK_GT(register_count / kLoopUnrollSize, 0);
    Label loop;
    __ Bind(&loop);
    for (int i = 0; i < kLoopUnrollSize; i += 2) {
      __ masm()->Push(kInterpreterAccumulatorRegister,
                      kInterpreterAccumulatorRegister);
    }
    __ masm()->Subs(scratch, scratch, 1);
    __ JumpIf(Condition::kGreaterThan, &loop);
  }
  __ RecordComment("]");
}

void BaselineCompiler::VerifyFrameSize() {
  __ masm()->Add(x15, sp,
                 RoundUp(InterpreterFrameConstants::kFixedFrameSizeFromFp +
                             bytecode_->frame_size(),
                         2 * kSystemPointerSize));
  __ masm()->Cmp(x15, fp);
  __ masm()->Assert(eq, AbortReason::kUnexpectedStackPointer);
}

#undef __

}  // namespace baseline
}  // namespace internal
}  // namespace v8

#endif  // V8_BASELINE_ARM64_BASELINE_COMPILER_ARM64_INL_H_
