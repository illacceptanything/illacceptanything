// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASELINE_ARM_BASELINE_ASSEMBLER_ARM_INL_H_
#define V8_BASELINE_ARM_BASELINE_ASSEMBLER_ARM_INL_H_

#include "src/baseline/baseline-assembler.h"
#include "src/codegen/arm/assembler-arm-inl.h"
#include "src/codegen/interface-descriptors.h"

namespace v8 {
namespace internal {
namespace baseline {

class BaselineAssembler::ScratchRegisterScope {
 public:
  explicit ScratchRegisterScope(BaselineAssembler* assembler)
      : assembler_(assembler),
        prev_scope_(assembler->scratch_register_scope_),
        wrapped_scope_(assembler->masm()) {
    if (!assembler_->scratch_register_scope_) {
      // If we haven't opened a scratch scope yet, for the first one add a
      // couple of extra registers.
      DCHECK(wrapped_scope_.CanAcquire());
      wrapped_scope_.Include(r8, r9);
      wrapped_scope_.Include(kInterpreterBytecodeOffsetRegister);
    }
    assembler_->scratch_register_scope_ = this;
  }
  ~ScratchRegisterScope() { assembler_->scratch_register_scope_ = prev_scope_; }

  Register AcquireScratch() { return wrapped_scope_.Acquire(); }

 private:
  BaselineAssembler* assembler_;
  ScratchRegisterScope* prev_scope_;
  UseScratchRegisterScope wrapped_scope_;
};

// TODO(v8:11429,leszeks): Unify condition names in the MacroAssembler.
enum class Condition : uint32_t {
  kEqual = static_cast<uint32_t>(eq),
  kNotEqual = static_cast<uint32_t>(ne),

  kLessThan = static_cast<uint32_t>(lt),
  kGreaterThan = static_cast<uint32_t>(gt),
  kLessThanEqual = static_cast<uint32_t>(le),
  kGreaterThanEqual = static_cast<uint32_t>(ge),

  kUnsignedLessThan = static_cast<uint32_t>(lo),
  kUnsignedGreaterThan = static_cast<uint32_t>(hi),
  kUnsignedLessThanEqual = static_cast<uint32_t>(ls),
  kUnsignedGreaterThanEqual = static_cast<uint32_t>(hs),

  kOverflow = static_cast<uint32_t>(vs),
  kNoOverflow = static_cast<uint32_t>(vc),

  kZero = static_cast<uint32_t>(eq),
  kNotZero = static_cast<uint32_t>(ne),
};

inline internal::Condition AsMasmCondition(Condition cond) {
  // This is important for arm, where the internal::Condition where each value
  // represents an encoded bit field value.
  STATIC_ASSERT(sizeof(internal::Condition) == sizeof(Condition));
  return static_cast<internal::Condition>(cond);
}

namespace detail {

#ifdef DEBUG
inline bool Clobbers(Register target, MemOperand op) {
  return op.rn() == target || op.rm() == target;
}
#endif

}  // namespace detail

#define __ masm_->

MemOperand BaselineAssembler::RegisterFrameOperand(
    interpreter::Register interpreter_register) {
  return MemOperand(fp, interpreter_register.ToOperand() * kSystemPointerSize);
}
MemOperand BaselineAssembler::FeedbackVectorOperand() {
  return MemOperand(fp, BaselineFrameConstants::kFeedbackVectorFromFp);
}

void BaselineAssembler::Bind(Label* label) { __ bind(label); }
void BaselineAssembler::BindWithoutJumpTarget(Label* label) { __ bind(label); }

void BaselineAssembler::JumpTarget() {
  // NOP on arm.
}

void BaselineAssembler::Jump(Label* target, Label::Distance distance) {
  __ b(target);
}
void BaselineAssembler::JumpIf(Condition cc, Label* target, Label::Distance) {
  __ b(AsMasmCondition(cc), target);
}
void BaselineAssembler::JumpIfRoot(Register value, RootIndex index,
                                   Label* target, Label::Distance) {
  __ JumpIfRoot(value, index, target);
}
void BaselineAssembler::JumpIfNotRoot(Register value, RootIndex index,
                                      Label* target, Label::Distance) {
  __ JumpIfNotRoot(value, index, target);
}
void BaselineAssembler::JumpIfSmi(Register value, Label* target,
                                  Label::Distance) {
  __ JumpIfSmi(value, target);
}
void BaselineAssembler::JumpIfNotSmi(Register value, Label* target,
                                     Label::Distance) {
  __ JumpIfNotSmi(value, target);
}

void BaselineAssembler::CallBuiltin(Builtins::Name builtin) {
  //  __ CallBuiltin(static_cast<int>(builtin));
  __ RecordCommentForOffHeapTrampoline(builtin);
  ScratchRegisterScope temps(this);
  Register temp = temps.AcquireScratch();
  __ LoadEntryFromBuiltinIndex(builtin, temp);
  __ Call(temp);
  __ RecordComment("]");
}

void BaselineAssembler::TailCallBuiltin(Builtins::Name builtin) {
  __ RecordCommentForOffHeapTrampoline(builtin);
  ScratchRegisterScope temps(this);
  Register temp = temps.AcquireScratch();
  __ LoadEntryFromBuiltinIndex(builtin, temp);
  __ Jump(temp);
  __ RecordComment("]");
}

void BaselineAssembler::Test(Register value, int mask) {
  __ tst(value, Operand(mask));
}

void BaselineAssembler::CmpObjectType(Register object,
                                      InstanceType instance_type,
                                      Register map) {
  ScratchRegisterScope temps(this);
  Register type = temps.AcquireScratch();
  __ CompareObjectType(object, map, type, instance_type);
}
void BaselineAssembler::CmpInstanceType(Register map,
                                        InstanceType instance_type) {
  ScratchRegisterScope temps(this);
  Register type = temps.AcquireScratch();
  if (FLAG_debug_code) {
    __ AssertNotSmi(map);
    __ CompareObjectType(map, type, type, MAP_TYPE);
    __ Assert(eq, AbortReason::kUnexpectedValue);
  }
  __ CompareInstanceType(map, type, instance_type);
}
void BaselineAssembler::Cmp(Register value, Smi smi) {
  __ cmp(value, Operand(smi));
}
void BaselineAssembler::ComparePointer(Register value, MemOperand operand) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ ldr(tmp, operand);
  __ cmp(value, tmp);
}
void BaselineAssembler::SmiCompare(Register lhs, Register rhs) {
  __ AssertSmi(lhs);
  __ AssertSmi(rhs);
  __ cmp(lhs, rhs);
}
void BaselineAssembler::CompareTagged(Register value, MemOperand operand) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ ldr(tmp, operand);
  __ cmp(value, tmp);
}
void BaselineAssembler::CompareTagged(MemOperand operand, Register value) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ ldr(tmp, operand);
  __ cmp(tmp, value);
}
void BaselineAssembler::CompareByte(Register value, int32_t byte) {
  __ cmp(value, Operand(byte));
}

void BaselineAssembler::Move(interpreter::Register output, Register source) {
  Move(RegisterFrameOperand(output), source);
}
void BaselineAssembler::Move(Register output, TaggedIndex value) {
  __ mov(output, Operand(value.ptr()));
}
void BaselineAssembler::Move(MemOperand output, Register source) {
  __ str(source, output);
}
void BaselineAssembler::Move(Register output, ExternalReference reference) {
  __ Move32BitImmediate(output, Operand(reference));
}
void BaselineAssembler::Move(Register output, Handle<HeapObject> value) {
  __ Move32BitImmediate(output, Operand(value));
}
void BaselineAssembler::Move(Register output, int32_t value) {
  __ mov(output, Operand(value));
}
void BaselineAssembler::MoveMaybeSmi(Register output, Register source) {
  __ mov(output, source);
}
void BaselineAssembler::MoveSmi(Register output, Register source) {
  __ mov(output, source);
}

namespace detail {

template <typename Arg>
inline Register ToRegister(BaselineAssembler* basm,
                           BaselineAssembler::ScratchRegisterScope* scope,
                           Arg arg) {
  Register reg = scope->AcquireScratch();
  basm->Move(reg, arg);
  return reg;
}
inline Register ToRegister(BaselineAssembler* basm,
                           BaselineAssembler::ScratchRegisterScope* scope,
                           Register reg) {
  return reg;
}

template <typename... Args>
struct PushAllHelper;
template <>
struct PushAllHelper<> {
  static int Push(BaselineAssembler* basm) { return 0; }
  static int PushReverse(BaselineAssembler* basm) { return 0; }
};
// TODO(ishell): try to pack sequence of pushes into one instruction by
// looking at regiser codes. For example, Push(r1, r2, r5, r0, r3, r4)
// could be generated as two pushes: Push(r1, r2, r5) and Push(r0, r3, r4).
template <typename Arg>
struct PushAllHelper<Arg> {
  static int Push(BaselineAssembler* basm, Arg arg) {
    BaselineAssembler::ScratchRegisterScope scope(basm);
    basm->masm()->Push(ToRegister(basm, &scope, arg));
    return 1;
  }
  static int PushReverse(BaselineAssembler* basm, Arg arg) {
    return Push(basm, arg);
  }
};
// TODO(ishell): try to pack sequence of pushes into one instruction by
// looking at regiser codes. For example, Push(r1, r2, r5, r0, r3, r4)
// could be generated as two pushes: Push(r1, r2, r5) and Push(r0, r3, r4).
template <typename Arg, typename... Args>
struct PushAllHelper<Arg, Args...> {
  static int Push(BaselineAssembler* basm, Arg arg, Args... args) {
    PushAllHelper<Arg>::Push(basm, arg);
    return 1 + PushAllHelper<Args...>::Push(basm, args...);
  }
  static int PushReverse(BaselineAssembler* basm, Arg arg, Args... args) {
    int nargs = PushAllHelper<Args...>::PushReverse(basm, args...);
    PushAllHelper<Arg>::Push(basm, arg);
    return nargs + 1;
  }
};
template <>
struct PushAllHelper<interpreter::RegisterList> {
  static int Push(BaselineAssembler* basm, interpreter::RegisterList list) {
    for (int reg_index = 0; reg_index < list.register_count(); ++reg_index) {
      PushAllHelper<interpreter::Register>::Push(basm, list[reg_index]);
    }
    return list.register_count();
  }
  static int PushReverse(BaselineAssembler* basm,
                         interpreter::RegisterList list) {
    for (int reg_index = list.register_count() - 1; reg_index >= 0;
         --reg_index) {
      PushAllHelper<interpreter::Register>::Push(basm, list[reg_index]);
    }
    return list.register_count();
  }
};

template <typename... T>
struct PopAllHelper;
template <>
struct PopAllHelper<> {
  static void Pop(BaselineAssembler* basm) {}
};
// TODO(ishell): try to pack sequence of pops into one instruction by
// looking at regiser codes. For example, Pop(r1, r2, r5, r0, r3, r4)
// could be generated as two pops: Pop(r1, r2, r5) and Pop(r0, r3, r4).
template <>
struct PopAllHelper<Register> {
  static void Pop(BaselineAssembler* basm, Register reg) {
    basm->masm()->Pop(reg);
  }
};
template <typename... T>
struct PopAllHelper<Register, T...> {
  static void Pop(BaselineAssembler* basm, Register reg, T... tail) {
    PopAllHelper<Register>::Pop(basm, reg);
    PopAllHelper<T...>::Pop(basm, tail...);
  }
};

}  // namespace detail

template <typename... T>
int BaselineAssembler::Push(T... vals) {
  return detail::PushAllHelper<T...>::Push(this, vals...);
}

template <typename... T>
void BaselineAssembler::PushReverse(T... vals) {
  detail::PushAllHelper<T...>::PushReverse(this, vals...);
}

template <typename... T>
void BaselineAssembler::Pop(T... registers) {
  detail::PopAllHelper<T...>::Pop(this, registers...);
}

void BaselineAssembler::LoadTaggedPointerField(Register output, Register source,
                                               int offset) {
  __ ldr(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::LoadTaggedSignedField(Register output, Register source,
                                              int offset) {
  __ ldr(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::LoadTaggedAnyField(Register output, Register source,
                                           int offset) {
  __ ldr(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::LoadByteField(Register output, Register source,
                                      int offset) {
  __ ldrb(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::StoreTaggedSignedField(Register target, int offset,
                                               Smi value) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ mov(tmp, Operand(value));
  __ str(tmp, FieldMemOperand(target, offset));
}
void BaselineAssembler::StoreTaggedFieldWithWriteBarrier(Register target,
                                                         int offset,
                                                         Register value) {
  __ str(value, FieldMemOperand(target, offset));
  __ RecordWriteField(target, offset, value, kLRHasNotBeenSaved,
                      SaveFPRegsMode::kIgnore);
}
void BaselineAssembler::StoreTaggedFieldNoWriteBarrier(Register target,
                                                       int offset,
                                                       Register value) {
  __ str(value, FieldMemOperand(target, offset));
}

void BaselineAssembler::AddToInterruptBudget(int32_t weight) {
  ScratchRegisterScope scratch_scope(this);
  Register feedback_cell = scratch_scope.AcquireScratch();
  LoadFunction(feedback_cell);
  LoadTaggedPointerField(feedback_cell, feedback_cell,
                         JSFunction::kFeedbackCellOffset);

  Register interrupt_budget = scratch_scope.AcquireScratch();
  __ ldr(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
  // Remember to set flags as part of the add!
  __ add(interrupt_budget, interrupt_budget, Operand(weight), SetCC);
  __ str(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
}

void BaselineAssembler::AddToInterruptBudget(Register weight) {
  ScratchRegisterScope scratch_scope(this);
  Register feedback_cell = scratch_scope.AcquireScratch();
  LoadFunction(feedback_cell);
  LoadTaggedPointerField(feedback_cell, feedback_cell,
                         JSFunction::kFeedbackCellOffset);

  Register interrupt_budget = scratch_scope.AcquireScratch();
  __ ldr(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
  // Remember to set flags as part of the add!
  __ add(interrupt_budget, interrupt_budget, weight, SetCC);
  __ str(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
}

void BaselineAssembler::AddSmi(Register lhs, Smi rhs) {
  __ add(lhs, lhs, Operand(rhs));
}

void BaselineAssembler::Switch(Register reg, int case_value_base,
                               Label** labels, int num_labels) {
  Label fallthrough;
  if (case_value_base > 0) {
    __ sub(reg, reg, Operand(case_value_base));
  }

  // Mostly copied from code-generator-arm.cc
  ScratchRegisterScope scope(this);
  __ cmp(reg, Operand(num_labels));
  JumpIf(Condition::kUnsignedGreaterThanEqual, &fallthrough);
  // Ensure to emit the constant pool first if necessary.
  __ CheckConstPool(true, true);
  __ BlockConstPoolFor(num_labels);
  int entry_size_log2 = 2;
  __ add(pc, pc, Operand(reg, LSL, entry_size_log2), LeaveCC, lo);
  __ b(&fallthrough);
  for (int i = 0; i < num_labels; ++i) {
    __ b(labels[i]);
  }
  __ bind(&fallthrough);
}

#undef __

#define __ basm.

void BaselineAssembler::EmitReturn(MacroAssembler* masm) {
  BaselineAssembler basm(masm);

  Register weight = BaselineLeaveFrameDescriptor::WeightRegister();
  Register params_size = BaselineLeaveFrameDescriptor::ParamsSizeRegister();

  __ RecordComment("[ Update Interrupt Budget");
  __ AddToInterruptBudget(weight);

  // Use compare flags set by add
  Label skip_interrupt_label;
  __ JumpIf(Condition::kGreaterThanEqual, &skip_interrupt_label);
  {
    __ masm()->SmiTag(params_size);
    __ Push(params_size, kInterpreterAccumulatorRegister);

    __ LoadContext(kContextRegister);
    __ LoadFunction(kJSFunctionRegister);
    __ Push(kJSFunctionRegister);
    __ CallRuntime(Runtime::kBytecodeBudgetInterruptFromBytecode, 1);

    __ Pop(kInterpreterAccumulatorRegister, params_size);
    __ masm()->SmiUntag(params_size);
  }
  __ RecordComment("]");

  __ Bind(&skip_interrupt_label);

  BaselineAssembler::ScratchRegisterScope temps(&basm);
  Register actual_params_size = temps.AcquireScratch();
  // Compute the size of the actual parameters + receiver (in bytes).
  __ Move(actual_params_size,
          MemOperand(fp, StandardFrameConstants::kArgCOffset));

  // If actual is bigger than formal, then we should use it to free up the stack
  // arguments.
  Label corrected_args_count;
  __ masm()->cmp(params_size, actual_params_size);
  __ JumpIf(Condition::kGreaterThanEqual, &corrected_args_count);
  __ masm()->mov(params_size, actual_params_size);
  __ Bind(&corrected_args_count);

  // Leave the frame (also dropping the register file).
  __ masm()->LeaveFrame(StackFrame::BASELINE);

  // Drop receiver + arguments.
  __ masm()->add(params_size, params_size,
                 Operand(1));  // Include the receiver.
  __ masm()->Drop(params_size);
  __ masm()->Ret();
}

#undef __

}  // namespace baseline
}  // namespace internal
}  // namespace v8

#endif  // V8_BASELINE_ARM_BASELINE_ASSEMBLER_ARM_INL_H_
