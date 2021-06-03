// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASELINE_ARM64_BASELINE_ASSEMBLER_ARM64_INL_H_
#define V8_BASELINE_ARM64_BASELINE_ASSEMBLER_ARM64_INL_H_

#include "src/baseline/baseline-assembler.h"
#include "src/codegen/arm64/macro-assembler-arm64-inl.h"
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
      wrapped_scope_.Include(x14, x15);
      wrapped_scope_.Include(x19);
    }
    assembler_->scratch_register_scope_ = this;
  }
  ~ScratchRegisterScope() { assembler_->scratch_register_scope_ = prev_scope_; }

  Register AcquireScratch() { return wrapped_scope_.AcquireX(); }

 private:
  BaselineAssembler* assembler_;
  ScratchRegisterScope* prev_scope_;
  UseScratchRegisterScope wrapped_scope_;
};

// TODO(v8:11461): Unify condition names in the MacroAssembler.
enum class Condition : uint32_t {
  kEqual = eq,
  kNotEqual = ne,

  kLessThan = lt,
  kGreaterThan = gt,
  kLessThanEqual = le,
  kGreaterThanEqual = ge,

  kUnsignedLessThan = lo,
  kUnsignedGreaterThan = hi,
  kUnsignedLessThanEqual = ls,
  kUnsignedGreaterThanEqual = hs,

  kOverflow = vs,
  kNoOverflow = vc,

  kZero = eq,
  kNotZero = ne,
};

inline internal::Condition AsMasmCondition(Condition cond) {
  return static_cast<internal::Condition>(cond);
}

namespace detail {

#ifdef DEBUG
inline bool Clobbers(Register target, MemOperand op) {
  return op.base() == target || op.regoffset() == target;
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

void BaselineAssembler::Bind(Label* label) {
  // All baseline compiler binds on arm64 are assumed to be for jump targets.
  __ BindJumpTarget(label);
}

void BaselineAssembler::BindWithoutJumpTarget(Label* label) { __ Bind(label); }

void BaselineAssembler::JumpTarget() { __ JumpTarget(); }

void BaselineAssembler::Jump(Label* target, Label::Distance distance) {
  __ B(target);
}
void BaselineAssembler::JumpIf(Condition cc, Label* target, Label::Distance) {
  __ B(AsMasmCondition(cc), target);
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
  if (masm()->options().short_builtin_calls) {
    // Generate pc-relative call.
    __ CallBuiltin(builtin);
  } else {
    ScratchRegisterScope temps(this);
    Register temp = temps.AcquireScratch();
    __ LoadEntryFromBuiltinIndex(builtin, temp);
    __ Call(temp);
  }
}

void BaselineAssembler::TailCallBuiltin(Builtins::Name builtin) {
  if (masm()->options().short_builtin_calls) {
    // Generate pc-relative call.
    __ TailCallBuiltin(builtin);
  } else {
    // The control flow integrity (CFI) feature allows us to "sign" code entry
    // points as a target for calls, jumps or both. Arm64 has special
    // instructions for this purpose, so-called "landing pads" (see
    // TurboAssembler::CallTarget(), TurboAssembler::JumpTarget() and
    // TurboAssembler::JumpOrCallTarget()). Currently, we generate "Call"
    // landing pads for CPP builtins. In order to allow tail calling to those
    // builtins we have to use a workaround.
    // x17 is used to allow using "Call" (i.e. `bti c`) rather than "Jump" (i.e.
    // `bti j`) landing pads for the tail-called code.
    Register temp = x17;

    // Make sure we're don't use this register as a temporary.
    UseScratchRegisterScope temps(masm());
    temps.Exclude(temp);

    __ LoadEntryFromBuiltinIndex(builtin, temp);
    __ Jump(temp);
  }
}

void BaselineAssembler::Test(Register value, int mask) {
  __ Tst(value, Immediate(mask));
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
void BaselineAssembler::Cmp(Register value, Smi smi) { __ Cmp(value, smi); }
void BaselineAssembler::ComparePointer(Register value, MemOperand operand) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ Ldr(tmp, operand);
  __ Cmp(value, tmp);
}
void BaselineAssembler::SmiCompare(Register lhs, Register rhs) {
  __ AssertSmi(lhs);
  __ AssertSmi(rhs);
  __ CmpTagged(lhs, rhs);
}
void BaselineAssembler::CompareTagged(Register value, MemOperand operand) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ Ldr(tmp, operand);
  __ CmpTagged(value, tmp);
}
void BaselineAssembler::CompareTagged(MemOperand operand, Register value) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ Ldr(tmp, operand);
  __ CmpTagged(tmp, value);
}
void BaselineAssembler::CompareByte(Register value, int32_t byte) {
  __ Cmp(value, Immediate(byte));
}

void BaselineAssembler::Move(interpreter::Register output, Register source) {
  Move(RegisterFrameOperand(output), source);
}
void BaselineAssembler::Move(Register output, TaggedIndex value) {
  __ Mov(output, Immediate(value.ptr()));
}
void BaselineAssembler::Move(MemOperand output, Register source) {
  __ Str(source, output);
}
void BaselineAssembler::Move(Register output, ExternalReference reference) {
  __ Mov(output, Operand(reference));
}
void BaselineAssembler::Move(Register output, Handle<HeapObject> value) {
  __ Mov(output, Operand(value));
}
void BaselineAssembler::Move(Register output, int32_t value) {
  __ Mov(output, Immediate(value));
}
void BaselineAssembler::MoveMaybeSmi(Register output, Register source) {
  __ Mov(output, source);
}
void BaselineAssembler::MoveSmi(Register output, Register source) {
  __ Mov(output, source);
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
struct CountPushHelper;
template <>
struct CountPushHelper<> {
  static int Count() { return 0; }
};
template <typename Arg, typename... Args>
struct CountPushHelper<Arg, Args...> {
  static int Count(Arg arg, Args... args) {
    return 1 + CountPushHelper<Args...>::Count(args...);
  }
};
template <typename... Args>
struct CountPushHelper<interpreter::RegisterList, Args...> {
  static int Count(interpreter::RegisterList list, Args... args) {
    return list.register_count() + CountPushHelper<Args...>::Count(args...);
  }
};

template <typename... Args>
struct PushAllHelper;
template <typename... Args>
inline void PushAll(BaselineAssembler* basm, Args... args) {
  PushAllHelper<Args...>::Push(basm, args...);
}
template <typename... Args>
inline void PushAllReverse(BaselineAssembler* basm, Args... args) {
  PushAllHelper<Args...>::PushReverse(basm, args...);
}

template <>
struct PushAllHelper<> {
  static void Push(BaselineAssembler* basm) {}
  static void PushReverse(BaselineAssembler* basm) {}
};
template <typename Arg>
struct PushAllHelper<Arg> {
  static void Push(BaselineAssembler* basm, Arg) { FATAL("Unaligned push"); }
  static void PushReverse(BaselineAssembler* basm, Arg arg) {
    // Push the padding register to round up the amount of values pushed.
    return PushAllReverse(basm, arg, padreg);
  }
};
template <typename Arg1, typename Arg2, typename... Args>
struct PushAllHelper<Arg1, Arg2, Args...> {
  static void Push(BaselineAssembler* basm, Arg1 arg1, Arg2 arg2,
                   Args... args) {
    {
      BaselineAssembler::ScratchRegisterScope scope(basm);
      basm->masm()->Push(ToRegister(basm, &scope, arg1),
                         ToRegister(basm, &scope, arg2));
    }
    PushAll(basm, args...);
  }
  static void PushReverse(BaselineAssembler* basm, Arg1 arg1, Arg2 arg2,
                          Args... args) {
    PushAllReverse(basm, args...);
    {
      BaselineAssembler::ScratchRegisterScope scope(basm);
      basm->masm()->Push(ToRegister(basm, &scope, arg2),
                         ToRegister(basm, &scope, arg1));
    }
  }
};
// Currently RegisterLists are always be the last argument, so we don't
// specialize for the case where they're not. We do still specialise for the
// aligned and unaligned cases.
template <typename Arg>
struct PushAllHelper<Arg, interpreter::RegisterList> {
  static void Push(BaselineAssembler* basm, Arg arg,
                   interpreter::RegisterList list) {
    DCHECK_EQ(list.register_count() % 2, 1);
    PushAll(basm, arg, list[0], list.PopLeft());
  }
  static void PushReverse(BaselineAssembler* basm, Arg arg,
                          interpreter::RegisterList list) {
    if (list.register_count() == 0) {
      PushAllReverse(basm, arg);
    } else {
      PushAllReverse(basm, arg, list[0], list.PopLeft());
    }
  }
};
template <>
struct PushAllHelper<interpreter::RegisterList> {
  static void Push(BaselineAssembler* basm, interpreter::RegisterList list) {
    DCHECK_EQ(list.register_count() % 2, 0);
    for (int reg_index = 0; reg_index < list.register_count(); reg_index += 2) {
      PushAll(basm, list[reg_index], list[reg_index + 1]);
    }
  }
  static void PushReverse(BaselineAssembler* basm,
                          interpreter::RegisterList list) {
    int reg_index = list.register_count() - 1;
    if (reg_index % 2 == 0) {
      // Push the padding register to round up the amount of values pushed.
      PushAllReverse(basm, list[reg_index], padreg);
      reg_index--;
    }
    for (; reg_index >= 1; reg_index -= 2) {
      PushAllReverse(basm, list[reg_index - 1], list[reg_index]);
    }
  }
};

template <typename... T>
struct PopAllHelper;
template <>
struct PopAllHelper<> {
  static void Pop(BaselineAssembler* basm) {}
};
template <>
struct PopAllHelper<Register> {
  static void Pop(BaselineAssembler* basm, Register reg) {
    basm->masm()->Pop(reg, padreg);
  }
};
template <typename... T>
struct PopAllHelper<Register, Register, T...> {
  static void Pop(BaselineAssembler* basm, Register reg1, Register reg2,
                  T... tail) {
    basm->masm()->Pop(reg1, reg2);
    PopAllHelper<T...>::Pop(basm, tail...);
  }
};

}  // namespace detail

template <typename... T>
int BaselineAssembler::Push(T... vals) {
  // We have to count the pushes first, to decide whether to add padding before
  // the first push.
  int push_count = detail::CountPushHelper<T...>::Count(vals...);
  if (push_count % 2 == 0) {
    detail::PushAll(this, vals...);
  } else {
    detail::PushAll(this, padreg, vals...);
  }
  return push_count;
}

template <typename... T>
void BaselineAssembler::PushReverse(T... vals) {
  detail::PushAllReverse(this, vals...);
}

template <typename... T>
void BaselineAssembler::Pop(T... registers) {
  detail::PopAllHelper<T...>::Pop(this, registers...);
}

void BaselineAssembler::LoadTaggedPointerField(Register output, Register source,
                                               int offset) {
  __ LoadTaggedPointerField(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::LoadTaggedSignedField(Register output, Register source,
                                              int offset) {
  __ LoadTaggedSignedField(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::LoadTaggedAnyField(Register output, Register source,
                                           int offset) {
  __ LoadAnyTaggedField(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::LoadByteField(Register output, Register source,
                                      int offset) {
  __ Ldrb(output, FieldMemOperand(source, offset));
}
void BaselineAssembler::StoreTaggedSignedField(Register target, int offset,
                                               Smi value) {
  ScratchRegisterScope temps(this);
  Register tmp = temps.AcquireScratch();
  __ Mov(tmp, Operand(value));
  __ StoreTaggedField(tmp, FieldMemOperand(target, offset));
}
void BaselineAssembler::StoreTaggedFieldWithWriteBarrier(Register target,
                                                         int offset,
                                                         Register value) {
  __ StoreTaggedField(value, FieldMemOperand(target, offset));
  __ RecordWriteField(target, offset, value, kLRHasNotBeenSaved,
                      SaveFPRegsMode::kIgnore);
}
void BaselineAssembler::StoreTaggedFieldNoWriteBarrier(Register target,
                                                       int offset,
                                                       Register value) {
  __ StoreTaggedField(value, FieldMemOperand(target, offset));
}

void BaselineAssembler::AddToInterruptBudget(int32_t weight) {
  ScratchRegisterScope scratch_scope(this);
  Register feedback_cell = scratch_scope.AcquireScratch();
  LoadFunction(feedback_cell);
  LoadTaggedPointerField(feedback_cell, feedback_cell,
                         JSFunction::kFeedbackCellOffset);

  Register interrupt_budget = scratch_scope.AcquireScratch().W();
  __ Ldr(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
  // Remember to set flags as part of the add!
  __ Adds(interrupt_budget, interrupt_budget, weight);
  __ Str(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
}

void BaselineAssembler::AddToInterruptBudget(Register weight) {
  ScratchRegisterScope scratch_scope(this);
  Register feedback_cell = scratch_scope.AcquireScratch();
  LoadFunction(feedback_cell);
  LoadTaggedPointerField(feedback_cell, feedback_cell,
                         JSFunction::kFeedbackCellOffset);

  Register interrupt_budget = scratch_scope.AcquireScratch().W();
  __ Ldr(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
  // Remember to set flags as part of the add!
  __ Adds(interrupt_budget, interrupt_budget, weight.W());
  __ Str(interrupt_budget,
         FieldMemOperand(feedback_cell, FeedbackCell::kInterruptBudgetOffset));
}

void BaselineAssembler::AddSmi(Register lhs, Smi rhs) {
  if (SmiValuesAre31Bits()) {
    __ Add(lhs.W(), lhs.W(), Immediate(rhs));
  } else {
    DCHECK(lhs.IsX());
    __ Add(lhs, lhs, Immediate(rhs));
  }
}

void BaselineAssembler::Switch(Register reg, int case_value_base,
                               Label** labels, int num_labels) {
  Label fallthrough;
  if (case_value_base > 0) {
    __ Sub(reg, reg, Immediate(case_value_base));
  }

  // Mostly copied from code-generator-arm64.cc
  ScratchRegisterScope scope(this);
  Register temp = scope.AcquireScratch();
  Label table;
  __ Cmp(reg, num_labels);
  JumpIf(Condition::kUnsignedGreaterThanEqual, &fallthrough);
  __ Adr(temp, &table);
  int entry_size_log2 = 2;
#ifdef V8_ENABLE_CONTROL_FLOW_INTEGRITY
  ++entry_size_log2;  // Account for BTI.
#endif
  __ Add(temp, temp, Operand(reg, UXTW, entry_size_log2));
  __ Br(temp);
  {
    TurboAssembler::BlockPoolsScope block_pools(masm_, num_labels * kInstrSize);
    __ Bind(&table);
    for (int i = 0; i < num_labels; ++i) {
      __ JumpTarget();
      __ B(labels[i]);
    }
    __ JumpTarget();
    __ Bind(&fallthrough);
  }
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
    __ masm()->Push(params_size, kInterpreterAccumulatorRegister);

    __ LoadContext(kContextRegister);
    __ LoadFunction(kJSFunctionRegister);
    __ masm()->PushArgument(kJSFunctionRegister);
    __ CallRuntime(Runtime::kBytecodeBudgetInterruptFromBytecode, 1);

    __ masm()->Pop(kInterpreterAccumulatorRegister, params_size);
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
  __ masm()->Cmp(params_size, actual_params_size);
  __ JumpIf(Condition::kGreaterThanEqual, &corrected_args_count);
  __ masm()->Mov(params_size, actual_params_size);
  __ Bind(&corrected_args_count);

  // Leave the frame (also dropping the register file).
  __ masm()->LeaveFrame(StackFrame::BASELINE);

  // Drop receiver + arguments.
  __ masm()->Add(params_size, params_size, 1);  // Include the receiver.
  __ masm()->DropArguments(params_size);
  __ masm()->Ret();
}

#undef __

}  // namespace baseline
}  // namespace internal
}  // namespace v8

#endif  // V8_BASELINE_ARM64_BASELINE_ASSEMBLER_ARM64_INL_H_
