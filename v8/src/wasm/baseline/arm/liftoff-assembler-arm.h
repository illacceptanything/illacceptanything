// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_WASM_BASELINE_ARM_LIFTOFF_ASSEMBLER_ARM_H_
#define V8_WASM_BASELINE_ARM_LIFTOFF_ASSEMBLER_ARM_H_

#include "src/base/platform/wrappers.h"
#include "src/codegen/arm/register-arm.h"
#include "src/heap/memory-chunk.h"
#include "src/wasm/baseline/liftoff-assembler.h"
#include "src/wasm/baseline/liftoff-register.h"

namespace v8 {
namespace internal {
namespace wasm {

namespace liftoff {

inline constexpr Condition ToCondition(LiftoffCondition liftoff_cond) {
  switch (liftoff_cond) {
    case kEqual:
      return eq;
    case kUnequal:
      return ne;
    case kSignedLessThan:
      return lt;
    case kSignedLessEqual:
      return le;
    case kSignedGreaterThan:
      return gt;
    case kSignedGreaterEqual:
      return ge;
    case kUnsignedLessThan:
      return lo;
    case kUnsignedLessEqual:
      return ls;
    case kUnsignedGreaterThan:
      return hi;
    case kUnsignedGreaterEqual:
      return hs;
  }
}

//  half
//  slot        Frame
//  -----+--------------------+---------------------------
//  n+3  |   parameter n      |
//  ...  |       ...          |
//   4   |   parameter 1      | or parameter 2
//   3   |   parameter 0      | or parameter 1
//   2   |  (result address)  | or parameter 0
//  -----+--------------------+---------------------------
//   1   | return addr (lr)   |
//   0   | previous frame (fp)|
//  -----+--------------------+  <-- frame ptr (fp)
//  -1   | 0xa: WASM          |
//  -2   |     instance       |
//  -----+--------------------+---------------------------
//  -3   |    slot 0 (high)   |   ^
//  -4   |    slot 0 (low)    |   |
//  -5   |    slot 1 (high)   | Frame slots
//  -6   |    slot 1 (low)    |   |
//       |                    |   v
//  -----+--------------------+  <-- stack ptr (sp)
//
static_assert(2 * kSystemPointerSize == LiftoffAssembler::kStackSlotSize,
              "Slot size should be twice the size of the 32 bit pointer.");
constexpr int kInstanceOffset = 2 * kSystemPointerSize;
// kPatchInstructionsRequired sets a maximum limit of how many instructions that
// PatchPrepareStackFrame will use in order to increase the stack appropriately.
// Three instructions are required to sub a large constant, movw + movt + sub.
constexpr int32_t kPatchInstructionsRequired = 3;
constexpr int kHalfStackSlotSize = LiftoffAssembler::kStackSlotSize >> 1;

inline MemOperand GetStackSlot(int offset) { return MemOperand(fp, -offset); }

inline MemOperand GetHalfStackSlot(int offset, RegPairHalf half) {
  int32_t half_offset =
      half == kLowWord ? 0 : LiftoffAssembler::kStackSlotSize / 2;
  return MemOperand(offset > 0 ? fp : sp, -offset + half_offset);
}

inline MemOperand GetInstanceOperand() { return GetStackSlot(kInstanceOffset); }

inline MemOperand GetMemOp(LiftoffAssembler* assm,
                           UseScratchRegisterScope* temps, Register addr,
                           Register offset, int32_t offset_imm) {
  if (offset != no_reg) {
    if (offset_imm == 0) return MemOperand(addr, offset);
    Register tmp = temps->Acquire();
    assm->add(tmp, offset, Operand(offset_imm));
    return MemOperand(addr, tmp);
  }
  return MemOperand(addr, offset_imm);
}

inline Register CalculateActualAddress(LiftoffAssembler* assm,
                                       UseScratchRegisterScope* temps,
                                       Register addr_reg, Register offset_reg,
                                       uintptr_t offset_imm,
                                       Register result_reg = no_reg) {
  if (offset_reg == no_reg && offset_imm == 0) {
    if (result_reg == no_reg) {
      return addr_reg;
    } else {
      assm->mov(result_reg, addr_reg);
      return result_reg;
    }
  }
  Register actual_addr_reg =
      result_reg != no_reg ? result_reg : temps->Acquire();
  if (offset_reg == no_reg) {
    assm->add(actual_addr_reg, addr_reg, Operand(offset_imm));
  } else {
    assm->add(actual_addr_reg, addr_reg, Operand(offset_reg));
    if (offset_imm != 0) {
      assm->add(actual_addr_reg, actual_addr_reg, Operand(offset_imm));
    }
  }
  return actual_addr_reg;
}

inline LiftoffCondition MakeUnsigned(LiftoffCondition cond) {
  switch (cond) {
    case kSignedLessThan:
      return kUnsignedLessThan;
    case kSignedLessEqual:
      return kUnsignedLessEqual;
    case kSignedGreaterThan:
      return kUnsignedGreaterThan;
    case kSignedGreaterEqual:
      return kUnsignedGreaterEqual;
    case kEqual:
    case kUnequal:
    case kUnsignedLessThan:
    case kUnsignedLessEqual:
    case kUnsignedGreaterThan:
    case kUnsignedGreaterEqual:
      return cond;
    default:
      UNREACHABLE();
  }
}

template <void (Assembler::*op)(Register, Register, Register, SBit, Condition),
          void (Assembler::*op_with_carry)(Register, Register, const Operand&,
                                           SBit, Condition)>
inline void I64Binop(LiftoffAssembler* assm, LiftoffRegister dst,
                     LiftoffRegister lhs, LiftoffRegister rhs) {
  Register dst_low = dst.low_gp();
  if (dst_low == lhs.high_gp() || dst_low == rhs.high_gp()) {
    dst_low = assm->GetUnusedRegister(
                      kGpReg, LiftoffRegList::ForRegs(lhs, rhs, dst.high_gp()))
                  .gp();
  }
  (assm->*op)(dst_low, lhs.low_gp(), rhs.low_gp(), SetCC, al);
  (assm->*op_with_carry)(dst.high_gp(), lhs.high_gp(), Operand(rhs.high_gp()),
                         LeaveCC, al);
  if (dst_low != dst.low_gp()) assm->mov(dst.low_gp(), dst_low);
}

template <void (Assembler::*op)(Register, Register, const Operand&, SBit,
                                Condition),
          void (Assembler::*op_with_carry)(Register, Register, const Operand&,
                                           SBit, Condition)>
inline void I64BinopI(LiftoffAssembler* assm, LiftoffRegister dst,
                      LiftoffRegister lhs, int64_t imm) {
  // The compiler allocated registers such that either {dst == lhs} or there is
  // no overlap between the two.
  DCHECK_NE(dst.low_gp(), lhs.high_gp());
  int32_t imm_low_word = static_cast<int32_t>(imm);
  int32_t imm_high_word = static_cast<int32_t>(imm >> 32);
  (assm->*op)(dst.low_gp(), lhs.low_gp(), Operand(imm_low_word), SetCC, al);
  (assm->*op_with_carry)(dst.high_gp(), lhs.high_gp(), Operand(imm_high_word),
                         LeaveCC, al);
}

template <void (TurboAssembler::*op)(Register, Register, Register, Register,
                                     Register),
          bool is_left_shift>
inline void I64Shiftop(LiftoffAssembler* assm, LiftoffRegister dst,
                       LiftoffRegister src, Register amount) {
  Register src_low = src.low_gp();
  Register src_high = src.high_gp();
  Register dst_low = dst.low_gp();
  Register dst_high = dst.high_gp();
  // Left shift writes {dst_high} then {dst_low}, right shifts write {dst_low}
  // then {dst_high}.
  Register clobbered_dst_reg = is_left_shift ? dst_high : dst_low;
  LiftoffRegList pinned = LiftoffRegList::ForRegs(clobbered_dst_reg, src);
  Register amount_capped =
      pinned.set(assm->GetUnusedRegister(kGpReg, pinned)).gp();
  assm->and_(amount_capped, amount, Operand(0x3F));

  // Ensure that writing the first half of {dst} does not overwrite the still
  // needed half of {src}.
  Register* later_src_reg = is_left_shift ? &src_low : &src_high;
  if (*later_src_reg == clobbered_dst_reg) {
    *later_src_reg = assm->GetUnusedRegister(kGpReg, pinned).gp();
    assm->TurboAssembler::Move(*later_src_reg, clobbered_dst_reg);
  }

  (assm->*op)(dst_low, dst_high, src_low, src_high, amount_capped);
}

inline FloatRegister GetFloatRegister(DoubleRegister reg) {
  DCHECK_LT(reg.code(), kDoubleCode_d16);
  return LowDwVfpRegister::from_code(reg.code()).low();
}

inline Simd128Register GetSimd128Register(DoubleRegister reg) {
  return QwNeonRegister::from_code(reg.code() / 2);
}

inline Simd128Register GetSimd128Register(LiftoffRegister reg) {
  return liftoff::GetSimd128Register(reg.low_fp());
}

enum class MinOrMax : uint8_t { kMin, kMax };
template <typename RegisterType>
inline void EmitFloatMinOrMax(LiftoffAssembler* assm, RegisterType dst,
                              RegisterType lhs, RegisterType rhs,
                              MinOrMax min_or_max) {
  DCHECK(RegisterType::kSizeInBytes == 4 || RegisterType::kSizeInBytes == 8);
  if (lhs == rhs) {
    assm->TurboAssembler::Move(dst, lhs);
    return;
  }
  Label done, is_nan;
  if (min_or_max == MinOrMax::kMin) {
    assm->TurboAssembler::FloatMin(dst, lhs, rhs, &is_nan);
  } else {
    assm->TurboAssembler::FloatMax(dst, lhs, rhs, &is_nan);
  }
  assm->b(&done);
  assm->bind(&is_nan);
  // Create a NaN output.
  assm->vadd(dst, lhs, rhs);
  assm->bind(&done);
}

inline Register EnsureNoAlias(Assembler* assm, Register reg,
                              Register must_not_alias,
                              UseScratchRegisterScope* temps) {
  if (reg != must_not_alias) return reg;
  Register tmp = temps->Acquire();
  DCHECK_NE(reg, tmp);
  assm->mov(tmp, reg);
  return tmp;
}

inline void S128NarrowOp(LiftoffAssembler* assm, NeonDataType dt,
                         NeonDataType sdt, LiftoffRegister dst,
                         LiftoffRegister lhs, LiftoffRegister rhs) {
  if (dst == lhs) {
    assm->vqmovn(dt, sdt, dst.low_fp(), liftoff::GetSimd128Register(lhs));
    assm->vqmovn(dt, sdt, dst.high_fp(), liftoff::GetSimd128Register(rhs));
  } else {
    assm->vqmovn(dt, sdt, dst.high_fp(), liftoff::GetSimd128Register(rhs));
    assm->vqmovn(dt, sdt, dst.low_fp(), liftoff::GetSimd128Register(lhs));
  }
}

inline void F64x2Compare(LiftoffAssembler* assm, LiftoffRegister dst,
                         LiftoffRegister lhs, LiftoffRegister rhs,
                         Condition cond) {
  DCHECK(cond == eq || cond == ne || cond == lt || cond == le);

  QwNeonRegister dest = liftoff::GetSimd128Register(dst);
  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);
  UseScratchRegisterScope temps(assm);
  Register scratch = temps.Acquire();

  assm->mov(scratch, Operand(0));
  assm->VFPCompareAndSetFlags(left.low(), right.low());
  assm->mov(scratch, Operand(-1), LeaveCC, cond);
  if (cond == lt || cond == le) {
    // Check for NaN.
    assm->mov(scratch, Operand(0), LeaveCC, vs);
  }
  assm->vmov(dest.low(), scratch, scratch);

  assm->mov(scratch, Operand(0));
  assm->VFPCompareAndSetFlags(left.high(), right.high());
  assm->mov(scratch, Operand(-1), LeaveCC, cond);
  if (cond == lt || cond == le) {
    // Check for NaN.
    assm->mov(scratch, Operand(0), LeaveCC, vs);
  }
  assm->vmov(dest.high(), scratch, scratch);
}

inline void Store(LiftoffAssembler* assm, LiftoffRegister src, MemOperand dst,
                  ValueKind kind) {
#ifdef DEBUG
  // The {str} instruction needs a temp register when the immediate in the
  // provided MemOperand does not fit into 12 bits. This happens for large stack
  // frames. This DCHECK checks that the temp register is available when needed.
  DCHECK(UseScratchRegisterScope{assm}.CanAcquire());
#endif
  switch (kind) {
    case kI32:
    case kOptRef:
    case kRef:
    case kRtt:
    case kRttWithDepth:
      assm->str(src.gp(), dst);
      break;
    case kI64:
      // Positive offsets should be lowered to kI32.
      assm->str(src.low_gp(), MemOperand(dst.rn(), dst.offset()));
      assm->str(
          src.high_gp(),
          MemOperand(dst.rn(), dst.offset() + liftoff::kHalfStackSlotSize));
      break;
    case kF32:
      assm->vstr(liftoff::GetFloatRegister(src.fp()), dst);
      break;
    case kF64:
      assm->vstr(src.fp(), dst);
      break;
    case kS128: {
      UseScratchRegisterScope temps(assm);
      Register addr = liftoff::CalculateActualAddress(assm, &temps, dst.rn(),
                                                      no_reg, dst.offset());
      assm->vst1(Neon8, NeonListOperand(src.low_fp(), 2), NeonMemOperand(addr));
      break;
    }
    default:
      UNREACHABLE();
  }
}

inline void Load(LiftoffAssembler* assm, LiftoffRegister dst, MemOperand src,
                 ValueKind kind) {
  switch (kind) {
    case kI32:
    case kOptRef:
    case kRef:
    case kRtt:
    case kRttWithDepth:
      assm->ldr(dst.gp(), src);
      break;
    case kI64:
      assm->ldr(dst.low_gp(), MemOperand(src.rn(), src.offset()));
      assm->ldr(
          dst.high_gp(),
          MemOperand(src.rn(), src.offset() + liftoff::kHalfStackSlotSize));
      break;
    case kF32:
      assm->vldr(liftoff::GetFloatRegister(dst.fp()), src);
      break;
    case kF64:
      assm->vldr(dst.fp(), src);
      break;
    case kS128: {
      // Get memory address of slot to fill from.
      UseScratchRegisterScope temps(assm);
      Register addr = liftoff::CalculateActualAddress(assm, &temps, src.rn(),
                                                      no_reg, src.offset());
      assm->vld1(Neon8, NeonListOperand(dst.low_fp(), 2), NeonMemOperand(addr));
      break;
    }
    default:
      UNREACHABLE();
  }
}

constexpr int MaskFromNeonDataType(NeonDataType dt) {
  switch (dt) {
    case NeonS8:
    case NeonU8:
      return 7;
    case NeonS16:
    case NeonU16:
      return 15;
    case NeonS32:
    case NeonU32:
      return 31;
    case NeonS64:
    case NeonU64:
      return 63;
  }
}

enum ShiftDirection { kLeft, kRight };

template <ShiftDirection dir = kLeft, NeonDataType dt, NeonSize sz>
inline void EmitSimdShift(LiftoffAssembler* assm, LiftoffRegister dst,
                          LiftoffRegister lhs, LiftoffRegister rhs) {
  constexpr int mask = MaskFromNeonDataType(dt);
  UseScratchRegisterScope temps(assm);
  QwNeonRegister tmp = temps.AcquireQ();
  Register shift = temps.Acquire();
  assm->and_(shift, rhs.gp(), Operand(mask));
  assm->vdup(sz, tmp, shift);
  if (dir == kRight) {
    assm->vneg(sz, tmp, tmp);
  }
  assm->vshl(dt, liftoff::GetSimd128Register(dst),
             liftoff::GetSimd128Register(lhs), tmp);
}

template <ShiftDirection dir, NeonDataType dt>
inline void EmitSimdShiftImmediate(LiftoffAssembler* assm, LiftoffRegister dst,
                                   LiftoffRegister lhs, int32_t rhs) {
  // vshr by 0 is not allowed, so check for it, and only move if dst != lhs.
  int32_t shift = rhs & MaskFromNeonDataType(dt);
  if (shift) {
    if (dir == kLeft) {
      assm->vshl(dt, liftoff::GetSimd128Register(dst),
                 liftoff::GetSimd128Register(lhs), shift);
    } else {
      assm->vshr(dt, liftoff::GetSimd128Register(dst),
                 liftoff::GetSimd128Register(lhs), shift);
    }
  } else if (dst != lhs) {
    assm->vmov(liftoff::GetSimd128Register(dst),
               liftoff::GetSimd128Register(lhs));
  }
}

inline void EmitAnyTrue(LiftoffAssembler* assm, LiftoffRegister dst,
                        LiftoffRegister src) {
  UseScratchRegisterScope temps(assm);
  DwVfpRegister scratch = temps.AcquireD();
  assm->vpmax(NeonU32, scratch, src.low_fp(), src.high_fp());
  assm->vpmax(NeonU32, scratch, scratch, scratch);
  assm->ExtractLane(dst.gp(), scratch, NeonS32, 0);
  assm->cmp(dst.gp(), Operand(0));
  assm->mov(dst.gp(), Operand(1), LeaveCC, ne);
}

}  // namespace liftoff

int LiftoffAssembler::PrepareStackFrame() {
  if (!CpuFeatures::IsSupported(ARMv7)) {
    bailout(kUnsupportedArchitecture, "Armv6 not supported");
    return 0;
  }
  uint32_t offset = static_cast<uint32_t>(pc_offset());
  // PatchPrepareStackFrame will patch this in order to increase the stack
  // appropriately. Additional nops are required as the bytes operand might
  // require extra moves to encode.
  for (int i = 0; i < liftoff::kPatchInstructionsRequired; i++) {
    nop();
  }
  DCHECK_EQ(offset + liftoff::kPatchInstructionsRequired * kInstrSize,
            pc_offset());
  return offset;
}

void LiftoffAssembler::PrepareTailCall(int num_callee_stack_params,
                                       int stack_param_delta) {
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();

  // Push the return address and frame pointer to complete the stack frame.
  sub(sp, sp, Operand(8));
  ldr(scratch, MemOperand(fp, 4));
  str(scratch, MemOperand(sp, 4));
  ldr(scratch, MemOperand(fp, 0));
  str(scratch, MemOperand(sp, 0));

  // Shift the whole frame upwards.
  int slot_count = num_callee_stack_params + 2;
  for (int i = slot_count - 1; i >= 0; --i) {
    ldr(scratch, MemOperand(sp, i * 4));
    str(scratch, MemOperand(fp, (i - stack_param_delta) * 4));
  }

  // Set the new stack and frame pointer.
  sub(sp, fp, Operand(stack_param_delta * 4));
  Pop(lr, fp);
}

void LiftoffAssembler::AlignFrameSize() {}

void LiftoffAssembler::PatchPrepareStackFrame(int offset) {
  // The frame_size includes the frame marker. The frame marker has already been
  // pushed on the stack though, so we don't need to allocate memory for it
  // anymore.
  int frame_size = GetTotalFrameSize() - kSystemPointerSize;

  // When using the simulator, deal with Liftoff which allocates the stack
  // before checking it.
  // TODO(arm): Remove this when the stack check mechanism will be updated.
  // Note: This check is only needed for simulator runs, but we run it
  // unconditionally to make sure that the simulator executes the same code
  // that's also executed on native hardware (see https://crbug.com/v8/11041).
  if (frame_size > KB / 2) {
    bailout(kOtherReason,
            "Stack limited to 512 bytes to avoid a bug in StackCheck");
    return;
  }

  PatchingAssembler patching_assembler(AssemblerOptions{},
                                       buffer_start_ + offset,
                                       liftoff::kPatchInstructionsRequired);
#if V8_OS_WIN
  if (frame_size > kStackPageSize) {
    // Generate OOL code (at the end of the function, where the current
    // assembler is pointing) to do the explicit stack limit check (see
    // https://docs.microsoft.com/en-us/previous-versions/visualstudio/
    // visual-studio-6.0/aa227153(v=vs.60)).
    // At the function start, emit a jump to that OOL code (from {offset} to
    // {pc_offset()}).
    int ool_offset = pc_offset() - offset;
    patching_assembler.b(ool_offset - Instruction::kPcLoadDelta);
    patching_assembler.PadWithNops();

    // Now generate the OOL code.
    AllocateStackSpace(frame_size);
    // Jump back to the start of the function (from {pc_offset()} to {offset +
    // liftoff::kPatchInstructionsRequired * kInstrSize}).
    int func_start_offset =
        offset + liftoff::kPatchInstructionsRequired * kInstrSize - pc_offset();
    b(func_start_offset - Instruction::kPcLoadDelta);
    return;
  }
#endif
  patching_assembler.sub(sp, sp, Operand(frame_size));
  patching_assembler.PadWithNops();
}

void LiftoffAssembler::FinishCode() { CheckConstPool(true, false); }

void LiftoffAssembler::AbortCompilation() { AbortedCodeGeneration(); }

// static
constexpr int LiftoffAssembler::StaticStackFrameSize() {
  return liftoff::kInstanceOffset;
}

int LiftoffAssembler::SlotSizeForType(ValueKind kind) {
  switch (kind) {
    case kS128:
      return element_size_bytes(kind);
    default:
      return kStackSlotSize;
  }
}

bool LiftoffAssembler::NeedsAlignment(ValueKind kind) {
  return kind == kS128 || is_reference(kind);
}

void LiftoffAssembler::LoadConstant(LiftoffRegister reg, WasmValue value,
                                    RelocInfo::Mode rmode) {
  switch (value.type().kind()) {
    case kI32:
      TurboAssembler::Move(reg.gp(), Operand(value.to_i32(), rmode));
      break;
    case kI64: {
      DCHECK(RelocInfo::IsNone(rmode));
      int32_t low_word = value.to_i64();
      int32_t high_word = value.to_i64() >> 32;
      TurboAssembler::Move(reg.low_gp(), Operand(low_word));
      TurboAssembler::Move(reg.high_gp(), Operand(high_word));
      break;
    }
    case kF32:
      vmov(liftoff::GetFloatRegister(reg.fp()), value.to_f32_boxed());
      break;
    case kF64: {
      Register extra_scratch = GetUnusedRegister(kGpReg, {}).gp();
      vmov(reg.fp(), Double(value.to_f64_boxed().get_bits()), extra_scratch);
      break;
    }
    default:
      UNREACHABLE();
  }
}

void LiftoffAssembler::LoadInstanceFromFrame(Register dst) {
  ldr(dst, liftoff::GetInstanceOperand());
}

void LiftoffAssembler::LoadFromInstance(Register dst, Register instance,
                                        int offset, int size) {
  DCHECK_LE(0, offset);
  MemOperand src{instance, offset};
  switch (size) {
    case 1:
      ldrb(dst, src);
      break;
    case 4:
      ldr(dst, src);
      break;
    default:
      UNIMPLEMENTED();
  }
}

void LiftoffAssembler::LoadTaggedPointerFromInstance(Register dst,
                                                     Register instance,
                                                     int offset) {
  STATIC_ASSERT(kTaggedSize == kSystemPointerSize);
  ldr(dst, MemOperand{instance, offset});
}

void LiftoffAssembler::SpillInstance(Register instance) {
  str(instance, liftoff::GetInstanceOperand());
}

void LiftoffAssembler::ResetOSRTarget() {}

void LiftoffAssembler::FillInstanceInto(Register dst) {
  ldr(dst, liftoff::GetInstanceOperand());
}

namespace liftoff {
#define __ lasm->
inline void LoadInternal(LiftoffAssembler* lasm, LiftoffRegister dst,
                         Register src_addr, Register offset_reg,
                         int32_t offset_imm, LoadType type,
                         LiftoffRegList pinned,
                         uint32_t* protected_load_pc = nullptr,
                         bool is_load_mem = false) {
  DCHECK_IMPLIES(type.value_type() == kWasmI64, dst.is_gp_pair());
  UseScratchRegisterScope temps(lasm);
  if (type.value() == LoadType::kF64Load ||
      type.value() == LoadType::kF32Load ||
      type.value() == LoadType::kS128Load) {
    Register actual_src_addr = liftoff::CalculateActualAddress(
        lasm, &temps, src_addr, offset_reg, offset_imm);
    if (type.value() == LoadType::kF64Load) {
      // Armv6 is not supported so Neon can be used to avoid alignment issues.
      CpuFeatureScope scope(lasm, NEON);
      __ vld1(Neon64, NeonListOperand(dst.fp()),
              NeonMemOperand(actual_src_addr));
    } else if (type.value() == LoadType::kF32Load) {
      // TODO(arm): Use vld1 for f32 when implemented in simulator as used for
      // f64. It supports unaligned access.
      Register scratch =
          (actual_src_addr == src_addr) ? temps.Acquire() : actual_src_addr;
      __ ldr(scratch, MemOperand(actual_src_addr));
      __ vmov(liftoff::GetFloatRegister(dst.fp()), scratch);
    } else {
      // Armv6 is not supported so Neon can be used to avoid alignment issues.
      CpuFeatureScope scope(lasm, NEON);
      __ vld1(Neon8, NeonListOperand(dst.low_fp(), 2),
              NeonMemOperand(actual_src_addr));
    }
  } else {
    MemOperand src_op =
        liftoff::GetMemOp(lasm, &temps, src_addr, offset_reg, offset_imm);
    if (protected_load_pc) *protected_load_pc = __ pc_offset();
    switch (type.value()) {
      case LoadType::kI32Load8U:
        __ ldrb(dst.gp(), src_op);
        break;
      case LoadType::kI64Load8U:
        __ ldrb(dst.low_gp(), src_op);
        __ mov(dst.high_gp(), Operand(0));
        break;
      case LoadType::kI32Load8S:
        __ ldrsb(dst.gp(), src_op);
        break;
      case LoadType::kI64Load8S:
        __ ldrsb(dst.low_gp(), src_op);
        __ asr(dst.high_gp(), dst.low_gp(), Operand(31));
        break;
      case LoadType::kI32Load16U:
        __ ldrh(dst.gp(), src_op);
        break;
      case LoadType::kI64Load16U:
        __ ldrh(dst.low_gp(), src_op);
        __ mov(dst.high_gp(), Operand(0));
        break;
      case LoadType::kI32Load16S:
        __ ldrsh(dst.gp(), src_op);
        break;
      case LoadType::kI32Load:
        __ ldr(dst.gp(), src_op);
        break;
      case LoadType::kI64Load16S:
        __ ldrsh(dst.low_gp(), src_op);
        __ asr(dst.high_gp(), dst.low_gp(), Operand(31));
        break;
      case LoadType::kI64Load32U:
        __ ldr(dst.low_gp(), src_op);
        __ mov(dst.high_gp(), Operand(0));
        break;
      case LoadType::kI64Load32S:
        __ ldr(dst.low_gp(), src_op);
        __ asr(dst.high_gp(), dst.low_gp(), Operand(31));
        break;
      case LoadType::kI64Load:
        __ ldr(dst.low_gp(), src_op);
        // GetMemOp may use a scratch register as the offset register, in which
        // case, calling GetMemOp again will fail due to the assembler having
        // ran out of scratch registers.
        if (temps.CanAcquire()) {
          src_op = liftoff::GetMemOp(lasm, &temps, src_addr, offset_reg,
                                     offset_imm + kSystemPointerSize);
        } else {
          __ add(src_op.rm(), src_op.rm(), Operand(kSystemPointerSize));
        }
        __ ldr(dst.high_gp(), src_op);
        break;
      default:
        UNREACHABLE();
    }
  }
}
#undef __
}  // namespace liftoff

void LiftoffAssembler::LoadTaggedPointer(Register dst, Register src_addr,
                                         Register offset_reg,
                                         int32_t offset_imm,
                                         LiftoffRegList pinned) {
  STATIC_ASSERT(kTaggedSize == kInt32Size);
  liftoff::LoadInternal(this, LiftoffRegister(dst), src_addr, offset_reg,
                        offset_imm, LoadType::kI32Load, pinned);
}

void LiftoffAssembler::LoadFullPointer(Register dst, Register src_addr,
                                       int32_t offset_imm) {
  UseScratchRegisterScope temps(this);
  MemOperand src_op =
      liftoff::GetMemOp(this, &temps, src_addr, no_reg, offset_imm);
  ldr(dst, src_op);
}

void LiftoffAssembler::StoreTaggedPointer(Register dst_addr,
                                          Register offset_reg,
                                          int32_t offset_imm,
                                          LiftoffRegister src,
                                          LiftoffRegList pinned,
                                          SkipWriteBarrier skip_write_barrier) {
  STATIC_ASSERT(kTaggedSize == kInt32Size);
  Register actual_offset_reg = offset_reg;
  if (offset_reg != no_reg && offset_imm != 0) {
    if (cache_state()->is_used(LiftoffRegister(offset_reg))) {
      actual_offset_reg = GetUnusedRegister(kGpReg, pinned).gp();
    }
    add(actual_offset_reg, offset_reg, Operand(offset_imm));
  }
  MemOperand dst_op = actual_offset_reg == no_reg
                          ? MemOperand(dst_addr, offset_imm)
                          : MemOperand(dst_addr, actual_offset_reg);
  str(src.gp(), dst_op);

  if (skip_write_barrier || FLAG_disable_write_barriers) return;

  // The write barrier.
  Label write_barrier;
  Label exit;
  CheckPageFlag(dst_addr, MemoryChunk::kPointersFromHereAreInterestingMask, ne,
                &write_barrier);
  b(&exit);
  bind(&write_barrier);
  JumpIfSmi(src.gp(), &exit);
  CheckPageFlag(src.gp(), MemoryChunk::kPointersToHereAreInterestingMask, eq,
                &exit);
  CallRecordWriteStub(dst_addr,
                      actual_offset_reg == no_reg ? Operand(offset_imm)
                                                  : Operand(actual_offset_reg),
                      RememberedSetAction::kEmit, SaveFPRegsMode::kSave,
                      StubCallMode::kCallWasmRuntimeStub);
  bind(&exit);
}

void LiftoffAssembler::Load(LiftoffRegister dst, Register src_addr,
                            Register offset_reg, uint32_t offset_imm,
                            LoadType type, LiftoffRegList pinned,
                            uint32_t* protected_load_pc, bool is_load_mem) {
  // Offsets >=2GB are statically OOB on 32-bit systems.
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  liftoff::LoadInternal(this, dst, src_addr, offset_reg,
                        static_cast<int32_t>(offset_imm), type, pinned,
                        protected_load_pc, is_load_mem);
}

void LiftoffAssembler::Store(Register dst_addr, Register offset_reg,
                             uint32_t offset_imm, LiftoffRegister src,
                             StoreType type, LiftoffRegList pinned,
                             uint32_t* protected_store_pc, bool is_store_mem) {
  // Offsets >=2GB are statically OOB on 32-bit systems.
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  UseScratchRegisterScope temps(this);
  if (type.value() == StoreType::kF64Store) {
    Register actual_dst_addr = liftoff::CalculateActualAddress(
        this, &temps, dst_addr, offset_reg, offset_imm);
    // Armv6 is not supported so Neon can be used to avoid alignment issues.
    CpuFeatureScope scope(this, NEON);
    vst1(Neon64, NeonListOperand(src.fp()), NeonMemOperand(actual_dst_addr));
  } else if (type.value() == StoreType::kS128Store) {
    Register actual_dst_addr = liftoff::CalculateActualAddress(
        this, &temps, dst_addr, offset_reg, offset_imm);
    // Armv6 is not supported so Neon can be used to avoid alignment issues.
    CpuFeatureScope scope(this, NEON);
    vst1(Neon8, NeonListOperand(src.low_fp(), 2),
         NeonMemOperand(actual_dst_addr));
  } else if (type.value() == StoreType::kF32Store) {
    // TODO(arm): Use vst1 for f32 when implemented in simulator as used for
    // f64. It supports unaligned access.
    // CalculateActualAddress will only not use a scratch register if the
    // following condition holds, otherwise another register must be
    // retrieved.
    Register scratch = (offset_reg == no_reg && offset_imm == 0)
                           ? temps.Acquire()
                           : GetUnusedRegister(kGpReg, pinned).gp();
    Register actual_dst_addr = liftoff::CalculateActualAddress(
        this, &temps, dst_addr, offset_reg, offset_imm);
    vmov(scratch, liftoff::GetFloatRegister(src.fp()));
    str(scratch, MemOperand(actual_dst_addr));
  } else {
    MemOperand dst_op =
        liftoff::GetMemOp(this, &temps, dst_addr, offset_reg, offset_imm);
    if (protected_store_pc) *protected_store_pc = pc_offset();
    switch (type.value()) {
      case StoreType::kI64Store8:
        src = src.low();
        V8_FALLTHROUGH;
      case StoreType::kI32Store8:
        strb(src.gp(), dst_op);
        break;
      case StoreType::kI64Store16:
        src = src.low();
        V8_FALLTHROUGH;
      case StoreType::kI32Store16:
        strh(src.gp(), dst_op);
        break;
      case StoreType::kI64Store32:
        src = src.low();
        V8_FALLTHROUGH;
      case StoreType::kI32Store:
        str(src.gp(), dst_op);
        break;
      case StoreType::kI64Store:
        str(src.low_gp(), dst_op);
        // GetMemOp may use a scratch register as the offset register, in which
        // case, calling GetMemOp again will fail due to the assembler having
        // ran out of scratch registers.
        if (temps.CanAcquire()) {
          dst_op = liftoff::GetMemOp(this, &temps, dst_addr, offset_reg,
                                     offset_imm + kSystemPointerSize);
        } else {
          add(dst_op.rm(), dst_op.rm(), Operand(kSystemPointerSize));
        }
        str(src.high_gp(), dst_op);
        break;
      default:
        UNREACHABLE();
    }
  }
}

namespace liftoff {
#define __ lasm->

inline void AtomicOp32(
    LiftoffAssembler* lasm, Register dst_addr, Register offset_reg,
    uint32_t offset_imm, LiftoffRegister value, LiftoffRegister result,
    LiftoffRegList pinned,
    void (Assembler::*load)(Register, Register, Condition),
    void (Assembler::*store)(Register, Register, Register, Condition),
    void (*op)(LiftoffAssembler*, Register, Register, Register)) {
  Register store_result = pinned.set(__ GetUnusedRegister(kGpReg, pinned)).gp();

  // Allocate an additional {temp} register to hold the result that should be
  // stored to memory. Note that {temp} and {store_result} are not allowed to be
  // the same register.
  Register temp = pinned.set(__ GetUnusedRegister(kGpReg, pinned)).gp();

  // Make sure that {result} is unique.
  Register result_reg = result.gp();
  if (result_reg == value.gp() || result_reg == dst_addr ||
      result_reg == offset_reg) {
    result_reg = __ GetUnusedRegister(kGpReg, pinned).gp();
  }

  UseScratchRegisterScope temps(lasm);
  Register actual_addr = liftoff::CalculateActualAddress(
      lasm, &temps, dst_addr, offset_reg, offset_imm);

  __ dmb(ISH);
  Label retry;
  __ bind(&retry);
  (lasm->*load)(result_reg, actual_addr, al);
  op(lasm, temp, result_reg, value.gp());
  (lasm->*store)(store_result, temp, actual_addr, al);
  __ cmp(store_result, Operand(0));
  __ b(ne, &retry);
  __ dmb(ISH);
  if (result_reg != result.gp()) {
    __ mov(result.gp(), result_reg);
  }
}

inline void Add(LiftoffAssembler* lasm, Register dst, Register lhs,
                Register rhs) {
  __ add(dst, lhs, rhs);
}

inline void Sub(LiftoffAssembler* lasm, Register dst, Register lhs,
                Register rhs) {
  __ sub(dst, lhs, rhs);
}

inline void And(LiftoffAssembler* lasm, Register dst, Register lhs,
                Register rhs) {
  __ and_(dst, lhs, rhs);
}

inline void Or(LiftoffAssembler* lasm, Register dst, Register lhs,
               Register rhs) {
  __ orr(dst, lhs, rhs);
}

inline void Xor(LiftoffAssembler* lasm, Register dst, Register lhs,
                Register rhs) {
  __ eor(dst, lhs, rhs);
}

inline void Exchange(LiftoffAssembler* lasm, Register dst, Register lhs,
                     Register rhs) {
  __ mov(dst, rhs);
}

inline void AtomicBinop32(LiftoffAssembler* lasm, Register dst_addr,
                          Register offset_reg, uint32_t offset_imm,
                          LiftoffRegister value, LiftoffRegister result,
                          StoreType type,
                          void (*op)(LiftoffAssembler*, Register, Register,
                                     Register)) {
  LiftoffRegList pinned =
      LiftoffRegList::ForRegs(dst_addr, offset_reg, value, result);
  switch (type.value()) {
    case StoreType::kI64Store8:
      __ LoadConstant(result.high(), WasmValue(0));
      result = result.low();
      value = value.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store8:
      liftoff::AtomicOp32(lasm, dst_addr, offset_reg, offset_imm, value, result,
                          pinned, &Assembler::ldrexb, &Assembler::strexb, op);
      return;
    case StoreType::kI64Store16:
      __ LoadConstant(result.high(), WasmValue(0));
      result = result.low();
      value = value.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store16:
      liftoff::AtomicOp32(lasm, dst_addr, offset_reg, offset_imm, value, result,
                          pinned, &Assembler::ldrexh, &Assembler::strexh, op);
      return;
    case StoreType::kI64Store32:
      __ LoadConstant(result.high(), WasmValue(0));
      result = result.low();
      value = value.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store:
      liftoff::AtomicOp32(lasm, dst_addr, offset_reg, offset_imm, value, result,
                          pinned, &Assembler::ldrex, &Assembler::strex, op);
      return;
    default:
      UNREACHABLE();
  }
}

inline void AtomicOp64(LiftoffAssembler* lasm, Register dst_addr,
                       Register offset_reg, uint32_t offset_imm,
                       LiftoffRegister value,
                       base::Optional<LiftoffRegister> result,
                       void (*op)(LiftoffAssembler*, LiftoffRegister,
                                  LiftoffRegister, LiftoffRegister)) {
  // strexd loads a 64 bit word into two registers. The first register needs
  // to have an even index, e.g. r8, the second register needs to be the one
  // with the next higher index, e.g. r9 if the first register is r8. In the
  // following code we use the fixed register pair r8/r9 to make the code here
  // simpler, even though other register pairs would also be possible.
  constexpr Register dst_low = r8;
  constexpr Register dst_high = r9;

  // Make sure {dst_low} and {dst_high} are not occupied by any other value.
  Register value_low = value.low_gp();
  Register value_high = value.high_gp();
  LiftoffRegList pinned = LiftoffRegList::ForRegs(
      dst_addr, offset_reg, value_low, value_high, dst_low, dst_high);
  __ ClearRegister(dst_low, {&dst_addr, &offset_reg, &value_low, &value_high},
                   pinned);
  pinned = pinned |
           LiftoffRegList::ForRegs(dst_addr, offset_reg, value_low, value_high);
  __ ClearRegister(dst_high, {&dst_addr, &offset_reg, &value_low, &value_high},
                   pinned);
  pinned = pinned |
           LiftoffRegList::ForRegs(dst_addr, offset_reg, value_low, value_high);

  // Make sure that {result}, if it exists, also does not overlap with
  // {dst_low} and {dst_high}. We don't have to transfer the value stored in
  // {result}.
  Register result_low = no_reg;
  Register result_high = no_reg;
  if (result.has_value()) {
    result_low = result.value().low_gp();
    if (pinned.has(result_low)) {
      result_low = __ GetUnusedRegister(kGpReg, pinned).gp();
    }
    pinned.set(result_low);

    result_high = result.value().high_gp();
    if (pinned.has(result_high)) {
      result_high = __ GetUnusedRegister(kGpReg, pinned).gp();
    }
    pinned.set(result_high);
  }

  Register store_result = __ GetUnusedRegister(kGpReg, pinned).gp();

  UseScratchRegisterScope temps(lasm);
  Register actual_addr = liftoff::CalculateActualAddress(
      lasm, &temps, dst_addr, offset_reg, offset_imm);

  __ dmb(ISH);
  Label retry;
  __ bind(&retry);
  // {ldrexd} is needed here so that the {strexd} instruction below can
  // succeed. We don't need the value we are reading. We use {dst_low} and
  // {dst_high} as the destination registers because {ldrexd} has the same
  // restrictions on registers as {strexd}, see the comment above.
  __ ldrexd(dst_low, dst_high, actual_addr);
  if (result.has_value()) {
    __ mov(result_low, dst_low);
    __ mov(result_high, dst_high);
  }
  op(lasm, LiftoffRegister::ForPair(dst_low, dst_high),
     LiftoffRegister::ForPair(dst_low, dst_high),
     LiftoffRegister::ForPair(value_low, value_high));
  __ strexd(store_result, dst_low, dst_high, actual_addr);
  __ cmp(store_result, Operand(0));
  __ b(ne, &retry);
  __ dmb(ISH);

  if (result.has_value()) {
    if (result_low != result.value().low_gp()) {
      __ mov(result.value().low_gp(), result_low);
    }
    if (result_high != result.value().high_gp()) {
      __ mov(result.value().high_gp(), result_high);
    }
  }
}

inline void I64Store(LiftoffAssembler* lasm, LiftoffRegister dst,
                     LiftoffRegister, LiftoffRegister src) {
  __ mov(dst.low_gp(), src.low_gp());
  __ mov(dst.high_gp(), src.high_gp());
}

#undef __
}  // namespace liftoff

void LiftoffAssembler::AtomicLoad(LiftoffRegister dst, Register src_addr,
                                  Register offset_reg, uint32_t offset_imm,
                                  LoadType type, LiftoffRegList pinned) {
  if (type.value() != LoadType::kI64Load) {
    Load(dst, src_addr, offset_reg, offset_imm, type, pinned, nullptr, true);
    dmb(ISH);
    return;
  }
  // ldrexd loads a 64 bit word into two registers. The first register needs to
  // have an even index, e.g. r8, the second register needs to be the one with
  // the next higher index, e.g. r9 if the first register is r8. In the
  // following code we use the fixed register pair r8/r9 to make the code here
  // simpler, even though other register pairs would also be possible.
  constexpr Register dst_low = r8;
  constexpr Register dst_high = r9;
  SpillRegisters(dst_low, dst_high);
  {
    UseScratchRegisterScope temps(this);
    Register actual_addr = liftoff::CalculateActualAddress(
        this, &temps, src_addr, offset_reg, offset_imm);
    ldrexd(dst_low, dst_high, actual_addr);
    dmb(ISH);
  }

  ParallelRegisterMove(
      {{dst, LiftoffRegister::ForPair(dst_low, dst_high), kI64}});
}

void LiftoffAssembler::AtomicStore(Register dst_addr, Register offset_reg,
                                   uint32_t offset_imm, LiftoffRegister src,
                                   StoreType type, LiftoffRegList pinned) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, src, {},
                        liftoff::I64Store);
    return;
  }

  dmb(ISH);
  Store(dst_addr, offset_reg, offset_imm, src, type, pinned, nullptr, true);
  dmb(ISH);
  return;
}

void LiftoffAssembler::AtomicAdd(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, value, {result},
                        liftoff::I64Binop<&Assembler::add, &Assembler::adc>);
    return;
  }
  liftoff::AtomicBinop32(this, dst_addr, offset_reg, offset_imm, value, result,
                         type, &liftoff::Add);
}

void LiftoffAssembler::AtomicSub(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, value, {result},
                        liftoff::I64Binop<&Assembler::sub, &Assembler::sbc>);
    return;
  }
  liftoff::AtomicBinop32(this, dst_addr, offset_reg, offset_imm, value, result,
                         type, &liftoff::Sub);
}

void LiftoffAssembler::AtomicAnd(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, value, {result},
                        liftoff::I64Binop<&Assembler::and_, &Assembler::and_>);
    return;
  }
  liftoff::AtomicBinop32(this, dst_addr, offset_reg, offset_imm, value, result,
                         type, &liftoff::And);
}

void LiftoffAssembler::AtomicOr(Register dst_addr, Register offset_reg,
                                uint32_t offset_imm, LiftoffRegister value,
                                LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, value, {result},
                        liftoff::I64Binop<&Assembler::orr, &Assembler::orr>);
    return;
  }
  liftoff::AtomicBinop32(this, dst_addr, offset_reg, offset_imm, value, result,
                         type, &liftoff::Or);
}

void LiftoffAssembler::AtomicXor(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, value, {result},
                        liftoff::I64Binop<&Assembler::eor, &Assembler::eor>);
    return;
  }
  liftoff::AtomicBinop32(this, dst_addr, offset_reg, offset_imm, value, result,
                         type, &liftoff::Xor);
}

void LiftoffAssembler::AtomicExchange(Register dst_addr, Register offset_reg,
                                      uint32_t offset_imm,
                                      LiftoffRegister value,
                                      LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicOp64(this, dst_addr, offset_reg, offset_imm, value, {result},
                        liftoff::I64Store);
    return;
  }
  liftoff::AtomicBinop32(this, dst_addr, offset_reg, offset_imm, value, result,
                         type, &liftoff::Exchange);
}

namespace liftoff {
#define __ lasm->

inline void AtomicI64CompareExchange(LiftoffAssembler* lasm,
                                     Register dst_addr_reg, Register offset_reg,
                                     uint32_t offset_imm,
                                     LiftoffRegister expected,
                                     LiftoffRegister new_value,
                                     LiftoffRegister result) {
  // To implement I64AtomicCompareExchange, we nearly need all registers, with
  // some registers having special constraints, e.g. like for {new_value} and
  // {result} the low-word register has to have an even register code, and the
  // high-word has to be in the next higher register. To avoid complicated
  // register allocation code here, we just assign fixed registers to all
  // values here, and then move all values into the correct register.
  Register dst_addr = r0;
  Register offset = r1;
  Register result_low = r4;
  Register result_high = r5;
  Register new_value_low = r2;
  Register new_value_high = r3;
  Register store_result = r6;
  Register expected_low = r8;
  Register expected_high = r9;

  // We spill all registers, so that we can re-assign them afterwards.
  __ SpillRegisters(dst_addr, offset, result_low, result_high, new_value_low,
                    new_value_high, store_result, expected_low, expected_high);

  __ ParallelRegisterMove(
      {{LiftoffRegister::ForPair(new_value_low, new_value_high), new_value,
        kI64},
       {LiftoffRegister::ForPair(expected_low, expected_high), expected, kI64},
       {dst_addr, dst_addr_reg, kI32},
       {offset, offset_reg != no_reg ? offset_reg : offset, kI32}});

  {
    UseScratchRegisterScope temps(lasm);
    Register temp = liftoff::CalculateActualAddress(
        lasm, &temps, dst_addr, offset_reg == no_reg ? no_reg : offset,
        offset_imm, dst_addr);
    // Make sure the actual address is stored in the right register.
    DCHECK_EQ(dst_addr, temp);
    USE(temp);
  }

  Label retry;
  Label done;
  __ dmb(ISH);
  __ bind(&retry);
  __ ldrexd(result_low, result_high, dst_addr);
  __ cmp(result_low, expected_low);
  __ b(ne, &done);
  __ cmp(result_high, expected_high);
  __ b(ne, &done);
  __ strexd(store_result, new_value_low, new_value_high, dst_addr);
  __ cmp(store_result, Operand(0));
  __ b(ne, &retry);
  __ dmb(ISH);
  __ bind(&done);

  __ ParallelRegisterMove(
      {{result, LiftoffRegister::ForPair(result_low, result_high), kI64}});
}
#undef __
}  // namespace liftoff

void LiftoffAssembler::AtomicCompareExchange(
    Register dst_addr, Register offset_reg, uint32_t offset_imm,
    LiftoffRegister expected, LiftoffRegister new_value, LiftoffRegister result,
    StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicI64CompareExchange(this, dst_addr, offset_reg, offset_imm,
                                      expected, new_value, result);
    return;
  }

  // The other versions of CompareExchange can share code, but need special load
  // and store instructions.
  void (Assembler::*load)(Register, Register, Condition) = nullptr;
  void (Assembler::*store)(Register, Register, Register, Condition) = nullptr;

  LiftoffRegList pinned = LiftoffRegList::ForRegs(dst_addr, offset_reg);
  // We need to remember the high word of {result}, so we can set it to zero in
  // the end if necessary.
  Register result_high = no_reg;
  switch (type.value()) {
    case StoreType::kI64Store8:
      result_high = result.high_gp();
      result = result.low();
      new_value = new_value.low();
      expected = expected.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store8:
      load = &Assembler::ldrexb;
      store = &Assembler::strexb;
      // We have to clear the high bits of {expected}, as we can only do a
      // 32-bit comparison. If the {expected} register is used, we spill it
      // first.
      if (cache_state()->is_used(expected)) {
        SpillRegister(expected);
      }
      uxtb(expected.gp(), expected.gp());
      break;
    case StoreType::kI64Store16:
      result_high = result.high_gp();
      result = result.low();
      new_value = new_value.low();
      expected = expected.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store16:
      load = &Assembler::ldrexh;
      store = &Assembler::strexh;
      // We have to clear the high bits of {expected}, as we can only do a
      // 32-bit comparison. If the {expected} register is used, we spill it
      // first.
      if (cache_state()->is_used(expected)) {
        SpillRegister(expected);
      }
      uxth(expected.gp(), expected.gp());
      break;
    case StoreType::kI64Store32:
      result_high = result.high_gp();
      result = result.low();
      new_value = new_value.low();
      expected = expected.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store:
      load = &Assembler::ldrex;
      store = &Assembler::strex;
      break;
    default:
      UNREACHABLE();
  }
  pinned.set(new_value);
  pinned.set(expected);

  Register result_reg = result.gp();
  if (pinned.has(result)) {
    result_reg = GetUnusedRegister(kGpReg, pinned).gp();
  }
  pinned.set(LiftoffRegister(result));
  Register store_result = GetUnusedRegister(kGpReg, pinned).gp();

  UseScratchRegisterScope temps(this);
  Register actual_addr = liftoff::CalculateActualAddress(
      this, &temps, dst_addr, offset_reg, offset_imm);

  Label retry;
  Label done;
  dmb(ISH);
  bind(&retry);
  (this->*load)(result.gp(), actual_addr, al);
  cmp(result.gp(), expected.gp());
  b(ne, &done);
  (this->*store)(store_result, new_value.gp(), actual_addr, al);
  cmp(store_result, Operand(0));
  b(ne, &retry);
  dmb(ISH);
  bind(&done);

  if (result.gp() != result_reg) {
    mov(result.gp(), result_reg);
  }
  if (result_high != no_reg) {
    LoadConstant(LiftoffRegister(result_high), WasmValue(0));
  }
}

void LiftoffAssembler::AtomicFence() { dmb(ISH); }

void LiftoffAssembler::LoadCallerFrameSlot(LiftoffRegister dst,
                                           uint32_t caller_slot_idx,
                                           ValueKind kind) {
  MemOperand src(fp, (caller_slot_idx + 1) * kSystemPointerSize);
  liftoff::Load(this, dst, src, kind);
}

void LiftoffAssembler::StoreCallerFrameSlot(LiftoffRegister src,
                                            uint32_t caller_slot_idx,
                                            ValueKind kind) {
  MemOperand dst(fp, (caller_slot_idx + 1) * kSystemPointerSize);
  liftoff::Store(this, src, dst, kind);
}

void LiftoffAssembler::LoadReturnStackSlot(LiftoffRegister dst, int offset,
                                           ValueKind kind) {
  MemOperand src(sp, offset);
  liftoff::Load(this, dst, src, kind);
}

void LiftoffAssembler::MoveStackValue(uint32_t dst_offset, uint32_t src_offset,
                                      ValueKind kind) {
  DCHECK_NE(dst_offset, src_offset);
  LiftoffRegister reg = GetUnusedRegister(reg_class_for(kind), {});
  Fill(reg, src_offset, kind);
  Spill(dst_offset, reg, kind);
}

void LiftoffAssembler::Move(Register dst, Register src, ValueKind kind) {
  DCHECK_NE(dst, src);
  DCHECK(kind == kI32 || is_reference(kind));
  TurboAssembler::Move(dst, src);
}

void LiftoffAssembler::Move(DoubleRegister dst, DoubleRegister src,
                            ValueKind kind) {
  DCHECK_NE(dst, src);
  if (kind == kF32) {
    vmov(liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(src));
  } else if (kind == kF64) {
    vmov(dst, src);
  } else {
    DCHECK_EQ(kS128, kind);
    vmov(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src));
  }
}

void LiftoffAssembler::Spill(int offset, LiftoffRegister reg, ValueKind kind) {
  // The {str} instruction needs a temp register when the immediate in the
  // provided MemOperand does not fit into 12 bits. This happens for large stack
  // frames. This DCHECK checks that the temp register is available when needed.
  DCHECK(UseScratchRegisterScope{this}.CanAcquire());
  DCHECK_LT(0, offset);
  RecordUsedSpillOffset(offset);
  MemOperand dst(fp, -offset);
  liftoff::Store(this, reg, dst, kind);
}

void LiftoffAssembler::Spill(int offset, WasmValue value) {
  RecordUsedSpillOffset(offset);
  MemOperand dst = liftoff::GetStackSlot(offset);
  UseScratchRegisterScope temps(this);
  Register src = no_reg;
  // The scratch register will be required by str if multiple instructions
  // are required to encode the offset, and so we cannot use it in that case.
  if (!ImmediateFitsAddrMode2Instruction(dst.offset())) {
    src = GetUnusedRegister(kGpReg, {}).gp();
  } else {
    src = temps.Acquire();
  }
  switch (value.type().kind()) {
    case kI32:
      mov(src, Operand(value.to_i32()));
      str(src, dst);
      break;
    case kI64: {
      int32_t low_word = value.to_i64();
      mov(src, Operand(low_word));
      str(src, liftoff::GetHalfStackSlot(offset, kLowWord));
      int32_t high_word = value.to_i64() >> 32;
      mov(src, Operand(high_word));
      str(src, liftoff::GetHalfStackSlot(offset, kHighWord));
      break;
    }
    default:
      // We do not track f32 and f64 constants, hence they are unreachable.
      UNREACHABLE();
  }
}

void LiftoffAssembler::Fill(LiftoffRegister reg, int offset, ValueKind kind) {
  liftoff::Load(this, reg, liftoff::GetStackSlot(offset), kind);
}

void LiftoffAssembler::FillI64Half(Register reg, int offset, RegPairHalf half) {
  ldr(reg, liftoff::GetHalfStackSlot(offset, half));
}

void LiftoffAssembler::FillStackSlotsWithZero(int start, int size) {
  DCHECK_LT(0, size);
  DCHECK_EQ(0, size % 4);
  RecordUsedSpillOffset(start + size);

  // We need a zero reg. Always use r0 for that, and push it before to restore
  // its value afterwards.
  push(r0);
  mov(r0, Operand(0));

  if (size <= 36) {
    // Special straight-line code for up to 9 words. Generates one
    // instruction per word.
    for (int offset = 4; offset <= size; offset += 4) {
      str(r0, liftoff::GetHalfStackSlot(start + offset, kLowWord));
    }
  } else {
    // General case for bigger counts (9 instructions).
    // Use r1 for start address (inclusive), r2 for end address (exclusive).
    push(r1);
    push(r2);
    sub(r1, fp, Operand(start + size));
    sub(r2, fp, Operand(start));

    Label loop;
    bind(&loop);
    str(r0, MemOperand(r1, /* offset */ kSystemPointerSize, PostIndex));
    cmp(r1, r2);
    b(&loop, ne);

    pop(r2);
    pop(r1);
  }

  pop(r0);
}

#define I32_BINOP(name, instruction)                             \
  void LiftoffAssembler::emit_##name(Register dst, Register lhs, \
                                     Register rhs) {             \
    instruction(dst, lhs, rhs);                                  \
  }
#define I32_BINOP_I(name, instruction)                              \
  I32_BINOP(name, instruction)                                      \
  void LiftoffAssembler::emit_##name##i(Register dst, Register lhs, \
                                        int32_t imm) {              \
    instruction(dst, lhs, Operand(imm));                            \
  }
#define I32_SHIFTOP(name, instruction)                              \
  void LiftoffAssembler::emit_##name(Register dst, Register src,    \
                                     Register amount) {             \
    UseScratchRegisterScope temps(this);                            \
    Register scratch = temps.Acquire();                             \
    and_(scratch, amount, Operand(0x1f));                           \
    instruction(dst, src, Operand(scratch));                        \
  }                                                                 \
  void LiftoffAssembler::emit_##name##i(Register dst, Register src, \
                                        int32_t amount) {           \
    if (V8_LIKELY((amount & 31) != 0)) {                            \
      instruction(dst, src, Operand(amount & 31));                  \
    } else if (dst != src) {                                        \
      mov(dst, src);                                                \
    }                                                               \
  }
#define FP32_UNOP(name, instruction)                                           \
  void LiftoffAssembler::emit_##name(DoubleRegister dst, DoubleRegister src) { \
    instruction(liftoff::GetFloatRegister(dst),                                \
                liftoff::GetFloatRegister(src));                               \
  }
#define FP32_BINOP(name, instruction)                                        \
  void LiftoffAssembler::emit_##name(DoubleRegister dst, DoubleRegister lhs, \
                                     DoubleRegister rhs) {                   \
    instruction(liftoff::GetFloatRegister(dst),                              \
                liftoff::GetFloatRegister(lhs),                              \
                liftoff::GetFloatRegister(rhs));                             \
  }
#define FP64_UNOP(name, instruction)                                           \
  void LiftoffAssembler::emit_##name(DoubleRegister dst, DoubleRegister src) { \
    instruction(dst, src);                                                     \
  }
#define FP64_BINOP(name, instruction)                                        \
  void LiftoffAssembler::emit_##name(DoubleRegister dst, DoubleRegister lhs, \
                                     DoubleRegister rhs) {                   \
    instruction(dst, lhs, rhs);                                              \
  }

I32_BINOP_I(i32_add, add)
I32_BINOP_I(i32_sub, sub)
I32_BINOP(i32_mul, mul)
I32_BINOP_I(i32_and, and_)
I32_BINOP_I(i32_or, orr)
I32_BINOP_I(i32_xor, eor)
I32_SHIFTOP(i32_shl, lsl)
I32_SHIFTOP(i32_sar, asr)
I32_SHIFTOP(i32_shr, lsr)
FP32_BINOP(f32_add, vadd)
FP32_BINOP(f32_sub, vsub)
FP32_BINOP(f32_mul, vmul)
FP32_BINOP(f32_div, vdiv)
FP32_UNOP(f32_abs, vabs)
FP32_UNOP(f32_neg, vneg)
FP32_UNOP(f32_sqrt, vsqrt)
FP64_BINOP(f64_add, vadd)
FP64_BINOP(f64_sub, vsub)
FP64_BINOP(f64_mul, vmul)
FP64_BINOP(f64_div, vdiv)
FP64_UNOP(f64_abs, vabs)
FP64_UNOP(f64_neg, vneg)
FP64_UNOP(f64_sqrt, vsqrt)

#undef I32_BINOP
#undef I32_SHIFTOP
#undef FP32_UNOP
#undef FP32_BINOP
#undef FP64_UNOP
#undef FP64_BINOP

void LiftoffAssembler::emit_i32_clz(Register dst, Register src) {
  clz(dst, src);
}

void LiftoffAssembler::emit_i32_ctz(Register dst, Register src) {
  rbit(dst, src);
  clz(dst, dst);
}

namespace liftoff {
inline void GeneratePopCnt(Assembler* assm, Register dst, Register src,
                           Register scratch1, Register scratch2) {
  DCHECK(!AreAliased(dst, scratch1, scratch2));
  if (src == scratch1) std::swap(scratch1, scratch2);
  // x = x - ((x & (0x55555555 << 1)) >> 1)
  assm->and_(scratch1, src, Operand(0xaaaaaaaa));
  assm->sub(dst, src, Operand(scratch1, LSR, 1));
  // x = (x & 0x33333333) + ((x & (0x33333333 << 2)) >> 2)
  assm->mov(scratch1, Operand(0x33333333));
  assm->and_(scratch2, dst, Operand(scratch1, LSL, 2));
  assm->and_(scratch1, dst, scratch1);
  assm->add(dst, scratch1, Operand(scratch2, LSR, 2));
  // x = (x + (x >> 4)) & 0x0F0F0F0F
  assm->add(dst, dst, Operand(dst, LSR, 4));
  assm->and_(dst, dst, Operand(0x0f0f0f0f));
  // x = x + (x >> 8)
  assm->add(dst, dst, Operand(dst, LSR, 8));
  // x = x + (x >> 16)
  assm->add(dst, dst, Operand(dst, LSR, 16));
  // x = x & 0x3F
  assm->and_(dst, dst, Operand(0x3f));
}
}  // namespace liftoff

bool LiftoffAssembler::emit_i32_popcnt(Register dst, Register src) {
  LiftoffRegList pinned = LiftoffRegList::ForRegs(dst);
  Register scratch1 = pinned.set(GetUnusedRegister(kGpReg, pinned)).gp();
  Register scratch2 = GetUnusedRegister(kGpReg, pinned).gp();
  liftoff::GeneratePopCnt(this, dst, src, scratch1, scratch2);
  return true;
}

void LiftoffAssembler::emit_i32_divs(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero,
                                     Label* trap_div_unrepresentable) {
  if (!CpuFeatures::IsSupported(SUDIV)) {
    bailout(kMissingCPUFeature, "i32_divs");
    return;
  }
  CpuFeatureScope scope(this, SUDIV);
  // Issue division early so we can perform the trapping checks whilst it
  // completes.
  bool speculative_sdiv = dst != lhs && dst != rhs;
  if (speculative_sdiv) {
    sdiv(dst, lhs, rhs);
  }
  Label noTrap;
  // Check for division by zero.
  cmp(rhs, Operand(0));
  b(trap_div_by_zero, eq);
  // Check for kMinInt / -1. This is unrepresentable.
  cmp(rhs, Operand(-1));
  b(&noTrap, ne);
  cmp(lhs, Operand(kMinInt));
  b(trap_div_unrepresentable, eq);
  bind(&noTrap);
  if (!speculative_sdiv) {
    sdiv(dst, lhs, rhs);
  }
}

void LiftoffAssembler::emit_i32_divu(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero) {
  if (!CpuFeatures::IsSupported(SUDIV)) {
    bailout(kMissingCPUFeature, "i32_divu");
    return;
  }
  CpuFeatureScope scope(this, SUDIV);
  // Check for division by zero.
  cmp(rhs, Operand(0));
  b(trap_div_by_zero, eq);
  udiv(dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32_rems(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero) {
  if (!CpuFeatures::IsSupported(SUDIV)) {
    // When this case is handled, a check for ARMv7 is required to use mls.
    // Mls support is implied with SUDIV support.
    bailout(kMissingCPUFeature, "i32_rems");
    return;
  }
  CpuFeatureScope scope(this, SUDIV);
  // No need to check kMinInt / -1 because the result is kMinInt and then
  // kMinInt * -1 -> kMinInt. In this case, the Msub result is therefore 0.
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  sdiv(scratch, lhs, rhs);
  // Check for division by zero.
  cmp(rhs, Operand(0));
  b(trap_div_by_zero, eq);
  // Compute remainder.
  mls(dst, scratch, rhs, lhs);
}

void LiftoffAssembler::emit_i32_remu(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero) {
  if (!CpuFeatures::IsSupported(SUDIV)) {
    // When this case is handled, a check for ARMv7 is required to use mls.
    // Mls support is implied with SUDIV support.
    bailout(kMissingCPUFeature, "i32_remu");
    return;
  }
  CpuFeatureScope scope(this, SUDIV);
  // No need to check kMinInt / -1 because the result is kMinInt and then
  // kMinInt * -1 -> kMinInt. In this case, the Msub result is therefore 0.
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  udiv(scratch, lhs, rhs);
  // Check for division by zero.
  cmp(rhs, Operand(0));
  b(trap_div_by_zero, eq);
  // Compute remainder.
  mls(dst, scratch, rhs, lhs);
}

void LiftoffAssembler::emit_i64_add(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::I64Binop<&Assembler::add, &Assembler::adc>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_addi(LiftoffRegister dst, LiftoffRegister lhs,
                                     int64_t imm) {
  liftoff::I64BinopI<&Assembler::add, &Assembler::adc>(this, dst, lhs, imm);
}

void LiftoffAssembler::emit_i64_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::I64Binop<&Assembler::sub, &Assembler::sbc>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  // Idea:
  //        [           lhs_hi  |           lhs_lo  ] * [  rhs_hi  |  rhs_lo  ]
  //    =   [  lhs_hi * rhs_lo  |                   ]  (32 bit mul, shift 32)
  //      + [  lhs_lo * rhs_hi  |                   ]  (32 bit mul, shift 32)
  //      + [             lhs_lo * rhs_lo           ]  (32x32->64 mul, shift 0)
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  // scratch = lhs_hi * rhs_lo
  mul(scratch, lhs.high_gp(), rhs.low_gp());
  // scratch += lhs_lo * rhs_hi
  mla(scratch, lhs.low_gp(), rhs.high_gp(), scratch);
  // TODO(arm): use umlal once implemented correctly in the simulator.
  // [dst_hi|dst_lo] = lhs_lo * rhs_lo
  umull(dst.low_gp(), dst.high_gp(), lhs.low_gp(), rhs.low_gp());
  // dst_hi += scratch
  add(dst.high_gp(), dst.high_gp(), scratch);
}

bool LiftoffAssembler::emit_i64_divs(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs,
                                     Label* trap_div_by_zero,
                                     Label* trap_div_unrepresentable) {
  return false;
}

bool LiftoffAssembler::emit_i64_divu(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs,
                                     Label* trap_div_by_zero) {
  return false;
}

bool LiftoffAssembler::emit_i64_rems(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs,
                                     Label* trap_div_by_zero) {
  return false;
}

bool LiftoffAssembler::emit_i64_remu(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs,
                                     Label* trap_div_by_zero) {
  return false;
}

void LiftoffAssembler::emit_i64_shl(LiftoffRegister dst, LiftoffRegister src,
                                    Register amount) {
  liftoff::I64Shiftop<&TurboAssembler::LslPair, true>(this, dst, src, amount);
}

void LiftoffAssembler::emit_i64_shli(LiftoffRegister dst, LiftoffRegister src,
                                     int32_t amount) {
  UseScratchRegisterScope temps(this);
  // {src.low_gp()} will still be needed after writing {dst.high_gp()}.
  Register src_low =
      liftoff::EnsureNoAlias(this, src.low_gp(), dst.high_gp(), &temps);

  LslPair(dst.low_gp(), dst.high_gp(), src_low, src.high_gp(), amount & 63);
}

void LiftoffAssembler::emit_i64_sar(LiftoffRegister dst, LiftoffRegister src,
                                    Register amount) {
  liftoff::I64Shiftop<&TurboAssembler::AsrPair, false>(this, dst, src, amount);
}

void LiftoffAssembler::emit_i64_sari(LiftoffRegister dst, LiftoffRegister src,
                                     int32_t amount) {
  UseScratchRegisterScope temps(this);
  // {src.high_gp()} will still be needed after writing {dst.low_gp()}.
  Register src_high =
      liftoff::EnsureNoAlias(this, src.high_gp(), dst.low_gp(), &temps);

  AsrPair(dst.low_gp(), dst.high_gp(), src.low_gp(), src_high, amount & 63);
}

void LiftoffAssembler::emit_i64_shr(LiftoffRegister dst, LiftoffRegister src,
                                    Register amount) {
  liftoff::I64Shiftop<&TurboAssembler::LsrPair, false>(this, dst, src, amount);
}

void LiftoffAssembler::emit_i64_shri(LiftoffRegister dst, LiftoffRegister src,
                                     int32_t amount) {
  UseScratchRegisterScope temps(this);
  // {src.high_gp()} will still be needed after writing {dst.low_gp()}.
  Register src_high =
      liftoff::EnsureNoAlias(this, src.high_gp(), dst.low_gp(), &temps);

  LsrPair(dst.low_gp(), dst.high_gp(), src.low_gp(), src_high, amount & 63);
}

void LiftoffAssembler::emit_i64_clz(LiftoffRegister dst, LiftoffRegister src) {
  // return high == 0 ? 32 + CLZ32(low) : CLZ32(high);
  Label done;
  Label high_is_zero;
  cmp(src.high_gp(), Operand(0));
  b(&high_is_zero, eq);

  clz(dst.low_gp(), src.high_gp());
  jmp(&done);

  bind(&high_is_zero);
  clz(dst.low_gp(), src.low_gp());
  add(dst.low_gp(), dst.low_gp(), Operand(32));

  bind(&done);
  mov(dst.high_gp(), Operand(0));  // High word of result is always 0.
}

void LiftoffAssembler::emit_i64_ctz(LiftoffRegister dst, LiftoffRegister src) {
  // return low == 0 ? 32 + CTZ32(high) : CTZ32(low);
  // CTZ32(x) = CLZ(RBIT(x))
  Label done;
  Label low_is_zero;
  cmp(src.low_gp(), Operand(0));
  b(&low_is_zero, eq);

  rbit(dst.low_gp(), src.low_gp());
  clz(dst.low_gp(), dst.low_gp());
  jmp(&done);

  bind(&low_is_zero);
  rbit(dst.low_gp(), src.high_gp());
  clz(dst.low_gp(), dst.low_gp());
  add(dst.low_gp(), dst.low_gp(), Operand(32));

  bind(&done);
  mov(dst.high_gp(), Operand(0));  // High word of result is always 0.
}

bool LiftoffAssembler::emit_i64_popcnt(LiftoffRegister dst,
                                       LiftoffRegister src) {
  // Produce partial popcnts in the two dst registers, making sure not to
  // overwrite the second src register before using it.
  Register src1 = src.high_gp() == dst.low_gp() ? src.high_gp() : src.low_gp();
  Register src2 = src.high_gp() == dst.low_gp() ? src.low_gp() : src.high_gp();
  LiftoffRegList pinned = LiftoffRegList::ForRegs(dst, src2);
  Register scratch1 = pinned.set(GetUnusedRegister(kGpReg, pinned)).gp();
  Register scratch2 = GetUnusedRegister(kGpReg, pinned).gp();
  liftoff::GeneratePopCnt(this, dst.low_gp(), src1, scratch1, scratch2);
  liftoff::GeneratePopCnt(this, dst.high_gp(), src2, scratch1, scratch2);
  // Now add the two into the lower dst reg and clear the higher dst reg.
  add(dst.low_gp(), dst.low_gp(), dst.high_gp());
  mov(dst.high_gp(), Operand(0));
  return true;
}

bool LiftoffAssembler::emit_f32_ceil(DoubleRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintp(liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(src));
    return true;
  }
  return false;
}

bool LiftoffAssembler::emit_f32_floor(DoubleRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintm(liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(src));
    return true;
  }
  return false;
}

bool LiftoffAssembler::emit_f32_trunc(DoubleRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintz(liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(src));
    return true;
  }
  return false;
}

bool LiftoffAssembler::emit_f32_nearest_int(DoubleRegister dst,
                                            DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintn(liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(src));
    return true;
  }
  return false;
}

void LiftoffAssembler::emit_f32_min(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax(
      this, liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(lhs),
      liftoff::GetFloatRegister(rhs), liftoff::MinOrMax::kMin);
}

void LiftoffAssembler::emit_f32_max(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax(
      this, liftoff::GetFloatRegister(dst), liftoff::GetFloatRegister(lhs),
      liftoff::GetFloatRegister(rhs), liftoff::MinOrMax::kMax);
}

bool LiftoffAssembler::emit_f64_ceil(DoubleRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintp(dst, src);
    return true;
  }
  return false;
}

bool LiftoffAssembler::emit_f64_floor(DoubleRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintm(dst, src);
    return true;
  }
  return false;
}

bool LiftoffAssembler::emit_f64_trunc(DoubleRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintz(dst, src);
    return true;
  }
  return false;
}

bool LiftoffAssembler::emit_f64_nearest_int(DoubleRegister dst,
                                            DoubleRegister src) {
  if (CpuFeatures::IsSupported(ARMv8)) {
    CpuFeatureScope scope(this, ARMv8);
    vrintn(dst, src);
    return true;
  }
  return false;
}

void LiftoffAssembler::emit_f64_min(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax(this, dst, lhs, rhs, liftoff::MinOrMax::kMin);
}

void LiftoffAssembler::emit_f64_max(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax(this, dst, lhs, rhs, liftoff::MinOrMax::kMax);
}

void LiftoffAssembler::emit_u32_to_intptr(Register dst, Register src) {
  // This is a nop on arm.
}

void LiftoffAssembler::emit_f32_copysign(DoubleRegister dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  constexpr uint32_t kF32SignBit = uint32_t{1} << 31;
  UseScratchRegisterScope temps(this);
  Register scratch = GetUnusedRegister(kGpReg, {}).gp();
  Register scratch2 = temps.Acquire();
  VmovLow(scratch, lhs);
  // Clear sign bit in {scratch}.
  bic(scratch, scratch, Operand(kF32SignBit));
  VmovLow(scratch2, rhs);
  // Isolate sign bit in {scratch2}.
  and_(scratch2, scratch2, Operand(kF32SignBit));
  // Combine {scratch2} into {scratch}.
  orr(scratch, scratch, scratch2);
  VmovLow(dst, scratch);
}

void LiftoffAssembler::emit_f64_copysign(DoubleRegister dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  constexpr uint32_t kF64SignBitHighWord = uint32_t{1} << 31;
  // On arm, we cannot hold the whole f64 value in a gp register, so we just
  // operate on the upper half (UH).
  UseScratchRegisterScope temps(this);
  Register scratch = GetUnusedRegister(kGpReg, {}).gp();
  Register scratch2 = temps.Acquire();
  VmovHigh(scratch, lhs);
  // Clear sign bit in {scratch}.
  bic(scratch, scratch, Operand(kF64SignBitHighWord));
  VmovHigh(scratch2, rhs);
  // Isolate sign bit in {scratch2}.
  and_(scratch2, scratch2, Operand(kF64SignBitHighWord));
  // Combine {scratch2} into {scratch}.
  orr(scratch, scratch, scratch2);
  vmov(dst, lhs);
  VmovHigh(dst, scratch);
}

bool LiftoffAssembler::emit_type_conversion(WasmOpcode opcode,
                                            LiftoffRegister dst,
                                            LiftoffRegister src, Label* trap) {
  switch (opcode) {
    case kExprI32ConvertI64:
      TurboAssembler::Move(dst.gp(), src.low_gp());
      return true;
    case kExprI32SConvertF32: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_s32_f32(
          scratch_f,
          liftoff::GetFloatRegister(src.fp()));  // f32 -> i32 round to zero.
      vmov(dst.gp(), scratch_f);
      // Check underflow and NaN.
      vmov(scratch_f, Float32(static_cast<float>(INT32_MIN)));
      VFPCompareAndSetFlags(liftoff::GetFloatRegister(src.fp()), scratch_f);
      b(trap, lt);
      // Check overflow.
      cmp(dst.gp(), Operand(-1));
      b(trap, vs);
      return true;
    }
    case kExprI32UConvertF32: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_u32_f32(
          scratch_f,
          liftoff::GetFloatRegister(src.fp()));  // f32 -> i32 round to zero.
      vmov(dst.gp(), scratch_f);
      // Check underflow and NaN.
      vmov(scratch_f, Float32(-1.0f));
      VFPCompareAndSetFlags(liftoff::GetFloatRegister(src.fp()), scratch_f);
      b(trap, le);
      // Check overflow.
      cmp(dst.gp(), Operand(-1));
      b(trap, eq);
      return true;
    }
    case kExprI32SConvertF64: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_s32_f64(scratch_f, src.fp());  // f64 -> i32 round to zero.
      vmov(dst.gp(), scratch_f);
      // Check underflow and NaN.
      DwVfpRegister scratch_d = temps.AcquireD();
      vmov(scratch_d, Double(static_cast<double>(INT32_MIN - 1.0)));
      VFPCompareAndSetFlags(src.fp(), scratch_d);
      b(trap, le);
      // Check overflow.
      vmov(scratch_d, Double(static_cast<double>(INT32_MAX + 1.0)));
      VFPCompareAndSetFlags(src.fp(), scratch_d);
      b(trap, ge);
      return true;
    }
    case kExprI32UConvertF64: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_u32_f64(scratch_f, src.fp());  // f64 -> i32 round to zero.
      vmov(dst.gp(), scratch_f);
      // Check underflow and NaN.
      DwVfpRegister scratch_d = temps.AcquireD();
      vmov(scratch_d, Double(static_cast<double>(-1.0)));
      VFPCompareAndSetFlags(src.fp(), scratch_d);
      b(trap, le);
      // Check overflow.
      vmov(scratch_d, Double(static_cast<double>(UINT32_MAX + 1.0)));
      VFPCompareAndSetFlags(src.fp(), scratch_d);
      b(trap, ge);
      return true;
    }
    case kExprI32SConvertSatF32: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_s32_f32(
          scratch_f,
          liftoff::GetFloatRegister(src.fp()));  // f32 -> i32 round to zero.
      vmov(dst.gp(), scratch_f);
      return true;
    }
    case kExprI32UConvertSatF32: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_u32_f32(
          scratch_f,
          liftoff::GetFloatRegister(src.fp()));  // f32 -> u32 round to zero.
      vmov(dst.gp(), scratch_f);
      return true;
    }
    case kExprI32SConvertSatF64: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_s32_f64(scratch_f, src.fp());  // f64 -> i32 round to zero.
      vmov(dst.gp(), scratch_f);
      return true;
    }
    case kExprI32UConvertSatF64: {
      UseScratchRegisterScope temps(this);
      SwVfpRegister scratch_f = temps.AcquireS();
      vcvt_u32_f64(scratch_f, src.fp());  // f64 -> u32 round to zero.
      vmov(dst.gp(), scratch_f);
      return true;
    }
    case kExprI32ReinterpretF32:
      vmov(dst.gp(), liftoff::GetFloatRegister(src.fp()));
      return true;
    case kExprI64SConvertI32:
      if (dst.low_gp() != src.gp()) mov(dst.low_gp(), src.gp());
      mov(dst.high_gp(), Operand(src.gp(), ASR, 31));
      return true;
    case kExprI64UConvertI32:
      if (dst.low_gp() != src.gp()) mov(dst.low_gp(), src.gp());
      mov(dst.high_gp(), Operand(0));
      return true;
    case kExprI64ReinterpretF64:
      vmov(dst.low_gp(), dst.high_gp(), src.fp());
      return true;
    case kExprF32SConvertI32: {
      SwVfpRegister dst_float = liftoff::GetFloatRegister(dst.fp());
      vmov(dst_float, src.gp());
      vcvt_f32_s32(dst_float, dst_float);
      return true;
    }
    case kExprF32UConvertI32: {
      SwVfpRegister dst_float = liftoff::GetFloatRegister(dst.fp());
      vmov(dst_float, src.gp());
      vcvt_f32_u32(dst_float, dst_float);
      return true;
    }
    case kExprF32ConvertF64:
      vcvt_f32_f64(liftoff::GetFloatRegister(dst.fp()), src.fp());
      return true;
    case kExprF32ReinterpretI32:
      vmov(liftoff::GetFloatRegister(dst.fp()), src.gp());
      return true;
    case kExprF64SConvertI32: {
      vmov(liftoff::GetFloatRegister(dst.fp()), src.gp());
      vcvt_f64_s32(dst.fp(), liftoff::GetFloatRegister(dst.fp()));
      return true;
    }
    case kExprF64UConvertI32: {
      vmov(liftoff::GetFloatRegister(dst.fp()), src.gp());
      vcvt_f64_u32(dst.fp(), liftoff::GetFloatRegister(dst.fp()));
      return true;
    }
    case kExprF64ConvertF32:
      vcvt_f64_f32(dst.fp(), liftoff::GetFloatRegister(src.fp()));
      return true;
    case kExprF64ReinterpretI64:
      vmov(dst.fp(), src.low_gp(), src.high_gp());
      return true;
    case kExprF64SConvertI64:
    case kExprF64UConvertI64:
    case kExprI64SConvertF32:
    case kExprI64UConvertF32:
    case kExprI64SConvertSatF32:
    case kExprI64UConvertSatF32:
    case kExprF32SConvertI64:
    case kExprF32UConvertI64:
    case kExprI64SConvertF64:
    case kExprI64UConvertF64:
    case kExprI64SConvertSatF64:
    case kExprI64UConvertSatF64:
      // These cases can be handled by the C fallback function.
      return false;
    default:
      UNREACHABLE();
  }
}

void LiftoffAssembler::emit_i32_signextend_i8(Register dst, Register src) {
  sxtb(dst, src);
}

void LiftoffAssembler::emit_i32_signextend_i16(Register dst, Register src) {
  sxth(dst, src);
}

void LiftoffAssembler::emit_i64_signextend_i8(LiftoffRegister dst,
                                              LiftoffRegister src) {
  emit_i32_signextend_i8(dst.low_gp(), src.low_gp());
  mov(dst.high_gp(), Operand(dst.low_gp(), ASR, 31));
}

void LiftoffAssembler::emit_i64_signextend_i16(LiftoffRegister dst,
                                               LiftoffRegister src) {
  emit_i32_signextend_i16(dst.low_gp(), src.low_gp());
  mov(dst.high_gp(), Operand(dst.low_gp(), ASR, 31));
}

void LiftoffAssembler::emit_i64_signextend_i32(LiftoffRegister dst,
                                               LiftoffRegister src) {
  TurboAssembler::Move(dst.low_gp(), src.low_gp());
  mov(dst.high_gp(), Operand(src.low_gp(), ASR, 31));
}

void LiftoffAssembler::emit_jump(Label* label) { b(label); }

void LiftoffAssembler::emit_jump(Register target) { bx(target); }

void LiftoffAssembler::emit_cond_jump(LiftoffCondition liftoff_cond,
                                      Label* label, ValueKind kind,
                                      Register lhs, Register rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);

  if (rhs == no_reg) {
    DCHECK_EQ(kind, kI32);
    cmp(lhs, Operand(0));
  } else {
    DCHECK(kind == kI32 || (is_reference(kind) && (liftoff_cond == kEqual ||
                                                   liftoff_cond == kUnequal)));
    cmp(lhs, rhs);
  }
  b(label, cond);
}

void LiftoffAssembler::emit_i32_cond_jumpi(LiftoffCondition liftoff_cond,
                                           Label* label, Register lhs,
                                           int32_t imm) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  cmp(lhs, Operand(imm));
  b(label, cond);
}

void LiftoffAssembler::emit_i32_eqz(Register dst, Register src) {
  clz(dst, src);
  mov(dst, Operand(dst, LSR, kRegSizeInBitsLog2));
}

void LiftoffAssembler::emit_i32_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, Register lhs,
                                         Register rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  cmp(lhs, rhs);
  mov(dst, Operand(0), LeaveCC);
  mov(dst, Operand(1), LeaveCC, cond);
}

void LiftoffAssembler::emit_i64_eqz(Register dst, LiftoffRegister src) {
  orr(dst, src.low_gp(), src.high_gp());
  clz(dst, dst);
  mov(dst, Operand(dst, LSR, 5));
}

void LiftoffAssembler::emit_i64_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, LiftoffRegister lhs,
                                         LiftoffRegister rhs) {
  // For signed i64 comparisons, we still need to use unsigned comparison for
  // the low word (the only bit carrying signedness information is the MSB in
  // the high word).
  Condition cond = liftoff::ToCondition(liftoff_cond);
  Condition unsigned_cond =
      liftoff::ToCondition(liftoff::MakeUnsigned(liftoff_cond));
  Label set_cond;
  Label cont;
  LiftoffRegister dest = LiftoffRegister(dst);
  bool speculative_move = !dest.overlaps(lhs) && !dest.overlaps(rhs);
  if (speculative_move) {
    mov(dst, Operand(0));
  }
  // Compare high word first. If it differs, use it for the set_cond. If it's
  // equal, compare the low word and use that for set_cond.
  cmp(lhs.high_gp(), rhs.high_gp());
  if (unsigned_cond == cond) {
    cmp(lhs.low_gp(), rhs.low_gp(), eq);
    if (!speculative_move) {
      mov(dst, Operand(0));
    }
    mov(dst, Operand(1), LeaveCC, cond);
  } else {
    // If the condition predicate for the low differs from that for the high
    // word, the conditional move instructions must be separated.
    b(ne, &set_cond);
    cmp(lhs.low_gp(), rhs.low_gp());
    if (!speculative_move) {
      mov(dst, Operand(0));
    }
    mov(dst, Operand(1), LeaveCC, unsigned_cond);
    b(&cont);
    bind(&set_cond);
    if (!speculative_move) {
      mov(dst, Operand(0));
    }
    mov(dst, Operand(1), LeaveCC, cond);
    bind(&cont);
  }
}

void LiftoffAssembler::emit_f32_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  VFPCompareAndSetFlags(liftoff::GetFloatRegister(lhs),
                        liftoff::GetFloatRegister(rhs));
  mov(dst, Operand(0), LeaveCC);
  mov(dst, Operand(1), LeaveCC, cond);
  if (cond != ne) {
    // If V flag set, at least one of the arguments was a Nan -> false.
    mov(dst, Operand(0), LeaveCC, vs);
  }
}

void LiftoffAssembler::emit_f64_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  VFPCompareAndSetFlags(lhs, rhs);
  mov(dst, Operand(0), LeaveCC);
  mov(dst, Operand(1), LeaveCC, cond);
  if (cond != ne) {
    // If V flag set, at least one of the arguments was a Nan -> false.
    mov(dst, Operand(0), LeaveCC, vs);
  }
}

bool LiftoffAssembler::emit_select(LiftoffRegister dst, Register condition,
                                   LiftoffRegister true_value,
                                   LiftoffRegister false_value,
                                   ValueKind kind) {
  return false;
}

void LiftoffAssembler::emit_smi_check(Register obj, Label* target,
                                      SmiCheckMode mode) {
  tst(obj, Operand(kSmiTagMask));
  Condition condition = mode == kJumpOnSmi ? eq : ne;
  b(condition, target);
}

void LiftoffAssembler::LoadTransform(LiftoffRegister dst, Register src_addr,
                                     Register offset_reg, uintptr_t offset_imm,
                                     LoadType type,
                                     LoadTransformationKind transform,
                                     uint32_t* protected_load_pc) {
  UseScratchRegisterScope temps(this);
  Register actual_src_addr = liftoff::CalculateActualAddress(
      this, &temps, src_addr, offset_reg, offset_imm);
  *protected_load_pc = pc_offset();
  MachineType memtype = type.mem_type();

  if (transform == LoadTransformationKind::kExtend) {
    if (memtype == MachineType::Int8()) {
      vld1(Neon8, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      vmovl(NeonS8, liftoff::GetSimd128Register(dst), dst.low_fp());
    } else if (memtype == MachineType::Uint8()) {
      vld1(Neon8, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      vmovl(NeonU8, liftoff::GetSimd128Register(dst), dst.low_fp());
    } else if (memtype == MachineType::Int16()) {
      vld1(Neon16, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      vmovl(NeonS16, liftoff::GetSimd128Register(dst), dst.low_fp());
    } else if (memtype == MachineType::Uint16()) {
      vld1(Neon16, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      vmovl(NeonU16, liftoff::GetSimd128Register(dst), dst.low_fp());
    } else if (memtype == MachineType::Int32()) {
      vld1(Neon32, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      vmovl(NeonS32, liftoff::GetSimd128Register(dst), dst.low_fp());
    } else if (memtype == MachineType::Uint32()) {
      vld1(Neon32, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      vmovl(NeonU32, liftoff::GetSimd128Register(dst), dst.low_fp());
    }
  } else if (transform == LoadTransformationKind::kZeroExtend) {
    Simd128Register dest = liftoff::GetSimd128Register(dst);
    if (memtype == MachineType::Int32()) {
      vmov(dest, 0);
      vld1s(Neon32, NeonListOperand(dst.low_fp()), 0,
            NeonMemOperand(actual_src_addr));
    } else {
      DCHECK_EQ(MachineType::Int64(), memtype);
      vmov(dest.high(), 0);
      vld1(Neon64, NeonListOperand(dest.low()),
           NeonMemOperand(actual_src_addr));
    }
  } else {
    DCHECK_EQ(LoadTransformationKind::kSplat, transform);
    if (memtype == MachineType::Int8()) {
      vld1r(Neon8, NeonListOperand(liftoff::GetSimd128Register(dst)),
            NeonMemOperand(actual_src_addr));
    } else if (memtype == MachineType::Int16()) {
      vld1r(Neon16, NeonListOperand(liftoff::GetSimd128Register(dst)),
            NeonMemOperand(actual_src_addr));
    } else if (memtype == MachineType::Int32()) {
      vld1r(Neon32, NeonListOperand(liftoff::GetSimd128Register(dst)),
            NeonMemOperand(actual_src_addr));
    } else if (memtype == MachineType::Int64()) {
      vld1(Neon32, NeonListOperand(dst.low_fp()),
           NeonMemOperand(actual_src_addr));
      TurboAssembler::Move(dst.high_fp(), dst.low_fp());
    }
  }
}

void LiftoffAssembler::LoadLane(LiftoffRegister dst, LiftoffRegister src,
                                Register addr, Register offset_reg,
                                uintptr_t offset_imm, LoadType type,
                                uint8_t laneidx, uint32_t* protected_load_pc) {
  UseScratchRegisterScope temps(this);
  Register actual_src_addr = liftoff::CalculateActualAddress(
      this, &temps, addr, offset_reg, offset_imm);
  TurboAssembler::Move(liftoff::GetSimd128Register(dst),
                       liftoff::GetSimd128Register(src));
  *protected_load_pc = pc_offset();
  LoadStoreLaneParams load_params(type.mem_type().representation(), laneidx);
  NeonListOperand dst_op =
      NeonListOperand(load_params.low_op ? dst.low_fp() : dst.high_fp());
  TurboAssembler::LoadLane(load_params.sz, dst_op, load_params.laneidx,
                           NeonMemOperand(actual_src_addr));
}

void LiftoffAssembler::StoreLane(Register dst, Register offset,
                                 uintptr_t offset_imm, LiftoffRegister src,
                                 StoreType type, uint8_t laneidx,
                                 uint32_t* protected_store_pc) {
  UseScratchRegisterScope temps(this);
  Register actual_dst_addr =
      liftoff::CalculateActualAddress(this, &temps, dst, offset, offset_imm);
  *protected_store_pc = pc_offset();

  LoadStoreLaneParams store_params(type.mem_rep(), laneidx);
  NeonListOperand src_op =
      NeonListOperand(store_params.low_op ? src.low_fp() : src.high_fp());
  TurboAssembler::StoreLane(store_params.sz, src_op, store_params.laneidx,
                            NeonMemOperand(actual_dst_addr));
}

void LiftoffAssembler::emit_i8x16_swizzle(LiftoffRegister dst,
                                          LiftoffRegister lhs,
                                          LiftoffRegister rhs) {
  UseScratchRegisterScope temps(this);

  NeonListOperand table(liftoff::GetSimd128Register(lhs));
  if (dst == lhs) {
    // dst will be overwritten, so keep the table somewhere else.
    QwNeonRegister tbl = temps.AcquireQ();
    TurboAssembler::Move(tbl, liftoff::GetSimd128Register(lhs));
    table = NeonListOperand(tbl);
  }

  vtbl(dst.low_fp(), table, rhs.low_fp());
  vtbl(dst.high_fp(), table, rhs.high_fp());
}

void LiftoffAssembler::emit_f64x2_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  TurboAssembler::Move(dst.low_fp(), src.fp());
  TurboAssembler::Move(dst.high_fp(), src.fp());
}

void LiftoffAssembler::emit_f64x2_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  ExtractLane(dst.fp(), liftoff::GetSimd128Register(lhs), imm_lane_idx);
}

void LiftoffAssembler::emit_f64x2_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  ReplaceLane(liftoff::GetSimd128Register(dst),
              liftoff::GetSimd128Register(src1), src2.fp(), imm_lane_idx);
}

void LiftoffAssembler::emit_f64x2_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vabs(dst.low_fp(), src.low_fp());
  vabs(dst.high_fp(), src.high_fp());
}

void LiftoffAssembler::emit_f64x2_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vneg(dst.low_fp(), src.low_fp());
  vneg(dst.high_fp(), src.high_fp());
}

void LiftoffAssembler::emit_f64x2_sqrt(LiftoffRegister dst,
                                       LiftoffRegister src) {
  vsqrt(dst.low_fp(), src.low_fp());
  vsqrt(dst.high_fp(), src.high_fp());
}

bool LiftoffAssembler::emit_f64x2_ceil(LiftoffRegister dst,
                                       LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintp(dst.low_fp(), src.low_fp());
  vrintp(dst.high_fp(), src.high_fp());
  return true;
}

bool LiftoffAssembler::emit_f64x2_floor(LiftoffRegister dst,
                                        LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintm(dst.low_fp(), src.low_fp());
  vrintm(dst.high_fp(), src.high_fp());
  return true;
}

bool LiftoffAssembler::emit_f64x2_trunc(LiftoffRegister dst,
                                        LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintz(dst.low_fp(), src.low_fp());
  vrintz(dst.high_fp(), src.high_fp());
  return true;
}

bool LiftoffAssembler::emit_f64x2_nearest_int(LiftoffRegister dst,
                                              LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintn(dst.low_fp(), src.low_fp());
  vrintn(dst.high_fp(), src.high_fp());
  return true;
}

void LiftoffAssembler::emit_f64x2_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vadd(dst.low_fp(), lhs.low_fp(), rhs.low_fp());
  vadd(dst.high_fp(), lhs.high_fp(), rhs.high_fp());
}

void LiftoffAssembler::emit_f64x2_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vsub(dst.low_fp(), lhs.low_fp(), rhs.low_fp());
  vsub(dst.high_fp(), lhs.high_fp(), rhs.high_fp());
}

void LiftoffAssembler::emit_f64x2_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vmul(dst.low_fp(), lhs.low_fp(), rhs.low_fp());
  vmul(dst.high_fp(), lhs.high_fp(), rhs.high_fp());
}

void LiftoffAssembler::emit_f64x2_div(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vdiv(dst.low_fp(), lhs.low_fp(), rhs.low_fp());
  vdiv(dst.high_fp(), lhs.high_fp(), rhs.high_fp());
}

void LiftoffAssembler::emit_f64x2_min(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  Simd128Register dest = liftoff::GetSimd128Register(dst);
  Simd128Register left = liftoff::GetSimd128Register(lhs);
  Simd128Register right = liftoff::GetSimd128Register(rhs);

  liftoff::EmitFloatMinOrMax(this, dest.low(), left.low(), right.low(),
                             liftoff::MinOrMax::kMin);
  liftoff::EmitFloatMinOrMax(this, dest.high(), left.high(), right.high(),
                             liftoff::MinOrMax::kMin);
}

void LiftoffAssembler::emit_f64x2_max(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  Simd128Register dest = liftoff::GetSimd128Register(dst);
  Simd128Register left = liftoff::GetSimd128Register(lhs);
  Simd128Register right = liftoff::GetSimd128Register(rhs);

  liftoff::EmitFloatMinOrMax(this, dest.low(), left.low(), right.low(),
                             liftoff::MinOrMax::kMax);
  liftoff::EmitFloatMinOrMax(this, dest.high(), left.high(), right.high(),
                             liftoff::MinOrMax::kMax);
}

void LiftoffAssembler::emit_f64x2_pmin(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  QwNeonRegister dest = liftoff::GetSimd128Register(dst);
  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);

  if (dst != rhs) {
    vmov(dest, left);
  }

  VFPCompareAndSetFlags(right.low(), left.low());
  vmov(dest.low(), right.low(), mi);
  VFPCompareAndSetFlags(right.high(), left.high());
  vmov(dest.high(), right.high(), mi);
}

void LiftoffAssembler::emit_f64x2_pmax(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  QwNeonRegister dest = liftoff::GetSimd128Register(dst);
  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);

  if (dst != rhs) {
    vmov(dest, left);
  }

  VFPCompareAndSetFlags(right.low(), left.low());
  vmov(dest.low(), right.low(), gt);
  VFPCompareAndSetFlags(right.high(), left.high());
  vmov(dest.high(), right.high(), gt);
}

void LiftoffAssembler::emit_f64x2_convert_low_i32x4_s(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  F64x2ConvertLowI32x4S(liftoff::GetSimd128Register(dst),
                        liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f64x2_convert_low_i32x4_u(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  F64x2ConvertLowI32x4U(liftoff::GetSimd128Register(dst),
                        liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f64x2_promote_low_f32x4(LiftoffRegister dst,
                                                    LiftoffRegister src) {
  F64x2PromoteLowF32x4(liftoff::GetSimd128Register(dst),
                       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f32x4_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  vdup(Neon32, liftoff::GetSimd128Register(dst), src.fp(), 0);
}

void LiftoffAssembler::emit_f32x4_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  ExtractLane(liftoff::GetFloatRegister(dst.fp()),
              liftoff::GetSimd128Register(lhs), imm_lane_idx);
}

void LiftoffAssembler::emit_f32x4_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  ReplaceLane(liftoff::GetSimd128Register(dst),
              liftoff::GetSimd128Register(src1),
              liftoff::GetFloatRegister(src2.fp()), imm_lane_idx);
}

void LiftoffAssembler::emit_f32x4_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vabs(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f32x4_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vneg(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f32x4_sqrt(LiftoffRegister dst,
                                       LiftoffRegister src) {
  // The list of d registers available to us is from d0 to d15, which always
  // maps to 2 s registers.
  LowDwVfpRegister dst_low = LowDwVfpRegister::from_code(dst.low_fp().code());
  LowDwVfpRegister src_low = LowDwVfpRegister::from_code(src.low_fp().code());

  LowDwVfpRegister dst_high = LowDwVfpRegister::from_code(dst.high_fp().code());
  LowDwVfpRegister src_high = LowDwVfpRegister::from_code(src.high_fp().code());

  vsqrt(dst_low.low(), src_low.low());
  vsqrt(dst_low.high(), src_low.high());
  vsqrt(dst_high.low(), src_high.low());
  vsqrt(dst_high.high(), src_high.high());
}

bool LiftoffAssembler::emit_f32x4_ceil(LiftoffRegister dst,
                                       LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintp(NeonS32, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
  return true;
}

bool LiftoffAssembler::emit_f32x4_floor(LiftoffRegister dst,
                                        LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintm(NeonS32, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
  return true;
}

bool LiftoffAssembler::emit_f32x4_trunc(LiftoffRegister dst,
                                        LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintz(NeonS32, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
  return true;
}

bool LiftoffAssembler::emit_f32x4_nearest_int(LiftoffRegister dst,
                                              LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(ARMv8)) {
    return false;
  }

  CpuFeatureScope scope(this, ARMv8);
  vrintn(NeonS32, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
  return true;
}

void LiftoffAssembler::emit_f32x4_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vadd(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vsub(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vmul(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_div(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  // The list of d registers available to us is from d0 to d15, which always
  // maps to 2 s registers.
  LowDwVfpRegister dst_low = LowDwVfpRegister::from_code(dst.low_fp().code());
  LowDwVfpRegister lhs_low = LowDwVfpRegister::from_code(lhs.low_fp().code());
  LowDwVfpRegister rhs_low = LowDwVfpRegister::from_code(rhs.low_fp().code());

  LowDwVfpRegister dst_high = LowDwVfpRegister::from_code(dst.high_fp().code());
  LowDwVfpRegister lhs_high = LowDwVfpRegister::from_code(lhs.high_fp().code());
  LowDwVfpRegister rhs_high = LowDwVfpRegister::from_code(rhs.high_fp().code());

  vdiv(dst_low.low(), lhs_low.low(), rhs_low.low());
  vdiv(dst_low.high(), lhs_low.high(), rhs_low.high());
  vdiv(dst_high.low(), lhs_high.low(), rhs_high.low());
  vdiv(dst_high.high(), lhs_high.high(), rhs_high.high());
}

void LiftoffAssembler::emit_f32x4_min(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vmin(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_max(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vmax(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_pmin(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  UseScratchRegisterScope temps(this);

  QwNeonRegister tmp = liftoff::GetSimd128Register(dst);
  if (dst == lhs || dst == rhs) {
    tmp = temps.AcquireQ();
  }

  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);
  vcgt(tmp, left, right);
  vbsl(tmp, right, left);

  if (dst == lhs || dst == rhs) {
    vmov(liftoff::GetSimd128Register(dst), tmp);
  }
}

void LiftoffAssembler::emit_f32x4_pmax(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  UseScratchRegisterScope temps(this);

  QwNeonRegister tmp = liftoff::GetSimd128Register(dst);
  if (dst == lhs || dst == rhs) {
    tmp = temps.AcquireQ();
  }

  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);
  vcgt(tmp, right, left);
  vbsl(tmp, right, left);

  if (dst == lhs || dst == rhs) {
    vmov(liftoff::GetSimd128Register(dst), tmp);
  }
}

void LiftoffAssembler::emit_i64x2_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  Simd128Register dst_simd = liftoff::GetSimd128Register(dst);
  vdup(Neon32, dst_simd, src.low_gp());
  ReplaceLane(dst_simd, dst_simd, src.high_gp(), NeonS32, 1);
  ReplaceLane(dst_simd, dst_simd, src.high_gp(), NeonS32, 3);
}

void LiftoffAssembler::emit_i64x2_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  ExtractLane(dst.low_gp(), liftoff::GetSimd128Register(lhs), NeonS32,
              imm_lane_idx * 2);
  ExtractLane(dst.high_gp(), liftoff::GetSimd128Register(lhs), NeonS32,
              imm_lane_idx * 2 + 1);
}

void LiftoffAssembler::emit_i64x2_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  Simd128Register dst_simd = liftoff::GetSimd128Register(dst);
  Simd128Register src1_simd = liftoff::GetSimd128Register(src1);
  ReplaceLane(dst_simd, src1_simd, src2.low_gp(), NeonS32, imm_lane_idx * 2);
  ReplaceLane(dst_simd, dst_simd, src2.high_gp(), NeonS32,
              imm_lane_idx * 2 + 1);
}

void LiftoffAssembler::emit_i64x2_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  QwNeonRegister zero =
      dst == src ? temps.AcquireQ() : liftoff::GetSimd128Register(dst);
  vmov(zero, uint64_t{0});
  vsub(Neon64, liftoff::GetSimd128Register(dst), zero,
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i64x2_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  I64x2AllTrue(dst.gp(), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i64x2_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kLeft, NeonS64, Neon32>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  vshl(NeonS64, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), rhs & 63);
}

void LiftoffAssembler::emit_i64x2_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonS64, Neon32>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonS64>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i64x2_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonU64, Neon32>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonU64>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i64x2_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vadd(Neon64, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i64x2_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vsub(Neon64, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i64x2_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  UseScratchRegisterScope temps(this);

  QwNeonRegister dst_neon = liftoff::GetSimd128Register(dst);
  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);

  // These temporary registers will be modified. We can directly modify lhs and
  // rhs if they are not uesd, saving on temporaries.
  QwNeonRegister tmp1 = left;
  QwNeonRegister tmp2 = right;

  LiftoffRegList used_plus_dst =
      cache_state()->used_registers | LiftoffRegList::ForRegs(dst);

  if (used_plus_dst.has(lhs) && used_plus_dst.has(rhs)) {
    tmp1 = temps.AcquireQ();
    // We only have 1 scratch Q register, so acquire another ourselves.
    LiftoffRegList pinned = LiftoffRegList::ForRegs(dst);
    LiftoffRegister unused_pair = GetUnusedRegister(kFpRegPair, pinned);
    tmp2 = liftoff::GetSimd128Register(unused_pair);
  } else if (used_plus_dst.has(lhs)) {
    tmp1 = temps.AcquireQ();
  } else if (used_plus_dst.has(rhs)) {
    tmp2 = temps.AcquireQ();
  }

  // Algorithm from code-generator-arm.cc, refer to comments there for details.
  if (tmp1 != left) {
    vmov(tmp1, left);
  }
  if (tmp2 != right) {
    vmov(tmp2, right);
  }

  vtrn(Neon32, tmp1.low(), tmp1.high());
  vtrn(Neon32, tmp2.low(), tmp2.high());

  vmull(NeonU32, dst_neon, tmp1.low(), tmp2.high());
  vmlal(NeonU32, dst_neon, tmp1.high(), tmp2.low());
  vshl(NeonU64, dst_neon, dst_neon, 32);

  vmlal(NeonU32, dst_neon, tmp1.low(), tmp2.low());
}

void LiftoffAssembler::emit_i64x2_extmul_low_i32x4_s(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  vmull(NeonS32, liftoff::GetSimd128Register(dst), src1.low_fp(),
        src2.low_fp());
}

void LiftoffAssembler::emit_i64x2_extmul_low_i32x4_u(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  vmull(NeonU32, liftoff::GetSimd128Register(dst), src1.low_fp(),
        src2.low_fp());
}

void LiftoffAssembler::emit_i64x2_extmul_high_i32x4_s(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  vmull(NeonS32, liftoff::GetSimd128Register(dst), src1.high_fp(),
        src2.high_fp());
}

void LiftoffAssembler::emit_i64x2_extmul_high_i32x4_u(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  vmull(NeonU32, liftoff::GetSimd128Register(dst), src1.high_fp(),
        src2.high_fp());
}

void LiftoffAssembler::emit_i64x2_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  I64x2BitMask(dst.gp(), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i64x2_sconvert_i32x4_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  vmovl(NeonS32, liftoff::GetSimd128Register(dst), src.low_fp());
}

void LiftoffAssembler::emit_i64x2_sconvert_i32x4_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  vmovl(NeonS32, liftoff::GetSimd128Register(dst), src.high_fp());
}

void LiftoffAssembler::emit_i64x2_uconvert_i32x4_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  vmovl(NeonU32, liftoff::GetSimd128Register(dst), src.low_fp());
}

void LiftoffAssembler::emit_i64x2_uconvert_i32x4_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  vmovl(NeonU32, liftoff::GetSimd128Register(dst), src.high_fp());
}

void LiftoffAssembler::emit_i32x4_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  vdup(Neon32, liftoff::GetSimd128Register(dst), src.gp());
}

void LiftoffAssembler::emit_i32x4_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  ExtractLane(dst.gp(), liftoff::GetSimd128Register(lhs), NeonS32,
              imm_lane_idx);
}

void LiftoffAssembler::emit_i32x4_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  ReplaceLane(liftoff::GetSimd128Register(dst),
              liftoff::GetSimd128Register(src1), src2.gp(), NeonS32,
              imm_lane_idx);
}

void LiftoffAssembler::emit_i32x4_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vneg(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i32x4_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  DwVfpRegister scratch = temps.AcquireD();
  vpmin(NeonU32, scratch, src.low_fp(), src.high_fp());
  vpmin(NeonU32, scratch, scratch, scratch);
  ExtractLane(dst.gp(), scratch, NeonS32, 0);
  cmp(dst.gp(), Operand(0));
  mov(dst.gp(), Operand(1), LeaveCC, ne);
}

void LiftoffAssembler::emit_i32x4_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  Simd128Register tmp = liftoff::GetSimd128Register(src);
  Simd128Register mask = temps.AcquireQ();

  if (cache_state()->is_used(src)) {
    // We only have 1 scratch Q register, so try and reuse src.
    LiftoffRegList pinned = LiftoffRegList::ForRegs(src);
    LiftoffRegister unused_pair = GetUnusedRegister(kFpRegPair, pinned);
    mask = liftoff::GetSimd128Register(unused_pair);
  }

  vshr(NeonS32, tmp, liftoff::GetSimd128Register(src), 31);
  // Set i-th bit of each lane i. When AND with tmp, the lanes that
  // are signed will have i-th bit set, unsigned will be 0.
  vmov(mask.low(), Double((uint64_t)0x0000'0002'0000'0001));
  vmov(mask.high(), Double((uint64_t)0x0000'0008'0000'0004));
  vand(tmp, mask, tmp);
  vpadd(Neon32, tmp.low(), tmp.low(), tmp.high());
  vpadd(Neon32, tmp.low(), tmp.low(), kDoubleRegZero);
  VmovLow(dst.gp(), tmp.low());
}

void LiftoffAssembler::emit_i32x4_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kLeft, NeonS32, Neon32>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  vshl(NeonS32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), rhs & 31);
}

void LiftoffAssembler::emit_i32x4_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonS32, Neon32>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonS32>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i32x4_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonU32, Neon32>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonU32>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i32x4_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vadd(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vsub(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vmul(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_min_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmin(NeonS32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_min_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmin(NeonU32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_max_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmax(NeonS32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_max_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmax(NeonU32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_dot_i16x8_s(LiftoffRegister dst,
                                              LiftoffRegister lhs,
                                              LiftoffRegister rhs) {
  QwNeonRegister dest = liftoff::GetSimd128Register(dst);
  QwNeonRegister left = liftoff::GetSimd128Register(lhs);
  QwNeonRegister right = liftoff::GetSimd128Register(rhs);

  UseScratchRegisterScope temps(this);
  Simd128Register scratch = temps.AcquireQ();

  vmull(NeonS16, scratch, left.low(), right.low());
  vpadd(Neon32, dest.low(), scratch.low(), scratch.high());

  vmull(NeonS16, scratch, left.high(), right.high());
  vpadd(Neon32, dest.high(), scratch.low(), scratch.high());
}

void LiftoffAssembler::emit_i32x4_extadd_pairwise_i16x8_s(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  vpaddl(NeonS16, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i32x4_extadd_pairwise_i16x8_u(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  vpaddl(NeonU16, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i32x4_extmul_low_i16x8_s(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  vmull(NeonS16, liftoff::GetSimd128Register(dst), src1.low_fp(),
        src2.low_fp());
}

void LiftoffAssembler::emit_i32x4_extmul_low_i16x8_u(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  vmull(NeonU16, liftoff::GetSimd128Register(dst), src1.low_fp(),
        src2.low_fp());
}

void LiftoffAssembler::emit_i32x4_extmul_high_i16x8_s(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  vmull(NeonS16, liftoff::GetSimd128Register(dst), src1.high_fp(),
        src2.high_fp());
}

void LiftoffAssembler::emit_i32x4_extmul_high_i16x8_u(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  vmull(NeonU16, liftoff::GetSimd128Register(dst), src1.high_fp(),
        src2.high_fp());
}

void LiftoffAssembler::emit_i16x8_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  vdup(Neon16, liftoff::GetSimd128Register(dst), src.gp());
}

void LiftoffAssembler::emit_i16x8_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vneg(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i16x8_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  DwVfpRegister scratch = temps.AcquireD();
  vpmin(NeonU16, scratch, src.low_fp(), src.high_fp());
  vpmin(NeonU16, scratch, scratch, scratch);
  vpmin(NeonU16, scratch, scratch, scratch);
  ExtractLane(dst.gp(), scratch, NeonS16, 0);
  cmp(dst.gp(), Operand(0));
  mov(dst.gp(), Operand(1), LeaveCC, ne);
}

void LiftoffAssembler::emit_i16x8_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  Simd128Register tmp = liftoff::GetSimd128Register(src);
  Simd128Register mask = temps.AcquireQ();

  if (cache_state()->is_used(src)) {
    // We only have 1 scratch Q register, so try and reuse src.
    LiftoffRegList pinned = LiftoffRegList::ForRegs(src);
    LiftoffRegister unused_pair = GetUnusedRegister(kFpRegPair, pinned);
    mask = liftoff::GetSimd128Register(unused_pair);
  }

  vshr(NeonS16, tmp, liftoff::GetSimd128Register(src), 15);
  // Set i-th bit of each lane i. When AND with tmp, the lanes that
  // are signed will have i-th bit set, unsigned will be 0.
  vmov(mask.low(), Double((uint64_t)0x0008'0004'0002'0001));
  vmov(mask.high(), Double((uint64_t)0x0080'0040'0020'0010));
  vand(tmp, mask, tmp);
  vpadd(Neon16, tmp.low(), tmp.low(), tmp.high());
  vpadd(Neon16, tmp.low(), tmp.low(), tmp.low());
  vpadd(Neon16, tmp.low(), tmp.low(), tmp.low());
  vmov(NeonU16, dst.gp(), tmp.low(), 0);
}

void LiftoffAssembler::emit_i16x8_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kLeft, NeonS16, Neon16>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  vshl(NeonS16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), rhs & 15);
}

void LiftoffAssembler::emit_i16x8_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonS16, Neon16>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonS16>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i16x8_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonU16, Neon16>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonU16>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i16x8_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vadd(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_add_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqadd(NeonS16, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vsub(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_sub_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqsub(NeonS16, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_sub_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqsub(NeonU16, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vmul(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_add_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqadd(NeonU16, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_min_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmin(NeonS16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_min_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmin(NeonU16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_max_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmax(NeonS16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_max_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmax(NeonU16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_extract_lane_u(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  ExtractLane(dst.gp(), liftoff::GetSimd128Register(lhs), NeonU16,
              imm_lane_idx);
}

void LiftoffAssembler::emit_i16x8_extract_lane_s(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  ExtractLane(dst.gp(), liftoff::GetSimd128Register(lhs), NeonS16,
              imm_lane_idx);
}

void LiftoffAssembler::emit_i16x8_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  ReplaceLane(liftoff::GetSimd128Register(dst),
              liftoff::GetSimd128Register(src1), src2.gp(), NeonS16,
              imm_lane_idx);
}

void LiftoffAssembler::emit_i16x8_extadd_pairwise_i8x16_s(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  vpaddl(NeonS8, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i16x8_extadd_pairwise_i8x16_u(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  vpaddl(NeonU8, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i16x8_extmul_low_i8x16_s(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  vmull(NeonS8, liftoff::GetSimd128Register(dst), src1.low_fp(), src2.low_fp());
}

void LiftoffAssembler::emit_i16x8_extmul_low_i8x16_u(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  vmull(NeonU8, liftoff::GetSimd128Register(dst), src1.low_fp(), src2.low_fp());
}

void LiftoffAssembler::emit_i16x8_extmul_high_i8x16_s(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  vmull(NeonS8, liftoff::GetSimd128Register(dst), src1.high_fp(),
        src2.high_fp());
}

void LiftoffAssembler::emit_i16x8_extmul_high_i8x16_u(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  vmull(NeonU8, liftoff::GetSimd128Register(dst), src1.high_fp(),
        src2.high_fp());
}

void LiftoffAssembler::emit_i16x8_q15mulr_sat_s(LiftoffRegister dst,
                                                LiftoffRegister src1,
                                                LiftoffRegister src2) {
  vqrdmulh(NeonS16, liftoff::GetSimd128Register(dst),
           liftoff::GetSimd128Register(src1),
           liftoff::GetSimd128Register(src2));
}

void LiftoffAssembler::emit_i8x16_shuffle(LiftoffRegister dst,
                                          LiftoffRegister lhs,
                                          LiftoffRegister rhs,
                                          const uint8_t shuffle[16],
                                          bool is_swizzle) {
  Simd128Register dest = liftoff::GetSimd128Register(dst);
  Simd128Register src1 = liftoff::GetSimd128Register(lhs);
  Simd128Register src2 = liftoff::GetSimd128Register(rhs);
  UseScratchRegisterScope temps(this);
  Simd128Register scratch = temps.AcquireQ();
  if ((src1 != src2) && src1.code() + 1 != src2.code()) {
    // vtbl requires the operands to be consecutive or the same.
    // If they are the same, we build a smaller list operand (table_size = 2).
    // If they are not the same, and not consecutive, we move the src1 and src2
    // to q14 and q15, which will be unused since they are not allocatable in
    // Liftoff. If the operands are the same, then we build a smaller list
    // operand below.
    static_assert(!(kLiftoffAssemblerFpCacheRegs &
                    (d28.bit() | d29.bit() | d30.bit() | d31.bit())),
                  "This only works if q14-q15 (d28-d31) are not used.");
    vmov(q14, src1);
    src1 = q14;
    vmov(q15, src2);
    src2 = q15;
  }

  int table_size = src1 == src2 ? 2 : 4;

  int scratch_s_base = scratch.code() * 4;
  for (int j = 0; j < 4; j++) {
    uint32_t imm = 0;
    for (int i = 3; i >= 0; i--) {
      imm = (imm << 8) | shuffle[j * 4 + i];
    }
    DCHECK_EQ(0, imm & (table_size == 2 ? 0xF0F0F0F0 : 0xE0E0E0E0));
    // Ensure indices are in [0,15] if table_size is 2, or [0,31] if 4.
    vmov(SwVfpRegister::from_code(scratch_s_base + j), Float32::FromBits(imm));
  }

  DwVfpRegister table_base = src1.low();
  NeonListOperand table(table_base, table_size);

  if (dest != src1 && dest != src2) {
    vtbl(dest.low(), table, scratch.low());
    vtbl(dest.high(), table, scratch.high());
  } else {
    vtbl(scratch.low(), table, scratch.low());
    vtbl(scratch.high(), table, scratch.high());
    vmov(dest, scratch);
  }
}

void LiftoffAssembler::emit_i8x16_popcnt(LiftoffRegister dst,
                                         LiftoffRegister src) {
  vcnt(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i8x16_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  vdup(Neon8, liftoff::GetSimd128Register(dst), src.gp());
}

void LiftoffAssembler::emit_i8x16_extract_lane_u(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  ExtractLane(dst.gp(), liftoff::GetSimd128Register(lhs), NeonU8, imm_lane_idx);
}

void LiftoffAssembler::emit_i8x16_extract_lane_s(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  ExtractLane(dst.gp(), liftoff::GetSimd128Register(lhs), NeonS8, imm_lane_idx);
}

void LiftoffAssembler::emit_i8x16_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  ReplaceLane(liftoff::GetSimd128Register(dst),
              liftoff::GetSimd128Register(src1), src2.gp(), NeonS8,
              imm_lane_idx);
}

void LiftoffAssembler::emit_i8x16_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vneg(Neon8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_v128_anytrue(LiftoffRegister dst,
                                         LiftoffRegister src) {
  liftoff::EmitAnyTrue(this, dst, src);
}

void LiftoffAssembler::emit_i8x16_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  DwVfpRegister scratch = temps.AcquireD();
  vpmin(NeonU8, scratch, src.low_fp(), src.high_fp());
  vpmin(NeonU8, scratch, scratch, scratch);
  vpmin(NeonU8, scratch, scratch, scratch);
  vpmin(NeonU8, scratch, scratch, scratch);
  ExtractLane(dst.gp(), scratch, NeonS8, 0);
  cmp(dst.gp(), Operand(0));
  mov(dst.gp(), Operand(1), LeaveCC, ne);
}

void LiftoffAssembler::emit_i8x16_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  UseScratchRegisterScope temps(this);
  Simd128Register tmp = liftoff::GetSimd128Register(src);
  Simd128Register mask = temps.AcquireQ();

  if (cache_state()->is_used(src)) {
    // We only have 1 scratch Q register, so try and reuse src.
    LiftoffRegList pinned = LiftoffRegList::ForRegs(src);
    LiftoffRegister unused_pair = GetUnusedRegister(kFpRegPair, pinned);
    mask = liftoff::GetSimd128Register(unused_pair);
  }

  vshr(NeonS8, tmp, liftoff::GetSimd128Register(src), 7);
  // Set i-th bit of each lane i. When AND with tmp, the lanes that
  // are signed will have i-th bit set, unsigned will be 0.
  vmov(mask.low(), Double((uint64_t)0x8040'2010'0804'0201));
  vmov(mask.high(), Double((uint64_t)0x8040'2010'0804'0201));
  vand(tmp, mask, tmp);
  vext(mask, tmp, tmp, 8);
  vzip(Neon8, mask, tmp);
  vpadd(Neon16, tmp.low(), tmp.low(), tmp.high());
  vpadd(Neon16, tmp.low(), tmp.low(), tmp.low());
  vpadd(Neon16, tmp.low(), tmp.low(), tmp.low());
  vmov(NeonU16, dst.gp(), tmp.low(), 0);
}

void LiftoffAssembler::emit_i8x16_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kLeft, NeonS8, Neon8>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  vshl(NeonS8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), rhs & 7);
}

void LiftoffAssembler::emit_i8x16_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonS8, Neon8>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonS8>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShift<liftoff::kRight, NeonU8, Neon8>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftImmediate<liftoff::kRight, NeonU8>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vadd(Neon8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_add_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqadd(NeonS8, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  vsub(Neon8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_sub_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqsub(NeonS8, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_sub_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqsub(NeonU8, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_add_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  vqadd(NeonU8, liftoff::GetSimd128Register(dst),
        liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_min_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmin(NeonS8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_min_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmin(NeonU8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_max_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmax(NeonS8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_max_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  vmax(NeonU8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(Neon8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(Neon8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
  vmvn(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(dst));
}

void LiftoffAssembler::emit_i8x16_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcgt(NeonS8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcgt(NeonU8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcge(NeonS8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcge(NeonU8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
  vmvn(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(dst));
}

void LiftoffAssembler::emit_i16x8_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcgt(NeonS16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcgt(NeonU16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcge(NeonS16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcge(NeonU16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
  vmvn(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(dst));
}

void LiftoffAssembler::emit_i32x4_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcgt(NeonS32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcgt(NeonU32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcge(NeonS32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i32x4_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  vcge(NeonU32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i64x2_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  I64x2Eq(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
          liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i64x2_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  I64x2Ne(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
          liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i64x2_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  I64x2GtS(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
           liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i64x2_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  I64x2GeS(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
           liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_f32x4_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vceq(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
  vmvn(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(dst));
}

void LiftoffAssembler::emit_f32x4_lt(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vcgt(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(rhs),
       liftoff::GetSimd128Register(lhs));
}

void LiftoffAssembler::emit_f32x4_le(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vcge(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(rhs),
       liftoff::GetSimd128Register(lhs));
}

void LiftoffAssembler::emit_f64x2_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::F64x2Compare(this, dst, lhs, rhs, eq);
}

void LiftoffAssembler::emit_f64x2_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::F64x2Compare(this, dst, lhs, rhs, ne);
}

void LiftoffAssembler::emit_f64x2_lt(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::F64x2Compare(this, dst, lhs, rhs, lt);
}

void LiftoffAssembler::emit_f64x2_le(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::F64x2Compare(this, dst, lhs, rhs, le);
}

void LiftoffAssembler::emit_s128_const(LiftoffRegister dst,
                                       const uint8_t imms[16]) {
  uint64_t vals[2];
  base::Memcpy(vals, imms, sizeof(vals));
  vmov(dst.low_fp(), Double(vals[0]));
  vmov(dst.high_fp(), Double(vals[1]));
}

void LiftoffAssembler::emit_s128_not(LiftoffRegister dst, LiftoffRegister src) {
  vmvn(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_s128_and(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  vand(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_s128_or(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  vorr(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_s128_xor(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  veor(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_s128_select(LiftoffRegister dst,
                                        LiftoffRegister src1,
                                        LiftoffRegister src2,
                                        LiftoffRegister mask) {
  if (dst != mask) {
    vmov(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(mask));
  }
  vbsl(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src1),
       liftoff::GetSimd128Register(src2));
}

void LiftoffAssembler::emit_i32x4_sconvert_f32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  vcvt_s32_f32(liftoff::GetSimd128Register(dst),
               liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i32x4_uconvert_f32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  vcvt_u32_f32(liftoff::GetSimd128Register(dst),
               liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f32x4_sconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  vcvt_f32_s32(liftoff::GetSimd128Register(dst),
               liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f32x4_uconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  vcvt_f32_u32(liftoff::GetSimd128Register(dst),
               liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_f32x4_demote_f64x2_zero(LiftoffRegister dst,
                                                    LiftoffRegister src) {
  LowDwVfpRegister dst_d = LowDwVfpRegister::from_code(dst.low_fp().code());
  vcvt_f32_f64(dst_d.low(), src.low_fp());
  vcvt_f32_f64(dst_d.high(), src.high_fp());
  vmov(dst.high_fp(), 0);
}

void LiftoffAssembler::emit_i8x16_sconvert_i16x8(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::S128NarrowOp(this, NeonS8, NeonS8, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_uconvert_i16x8(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::S128NarrowOp(this, NeonU8, NeonS8, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_sconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::S128NarrowOp(this, NeonS16, NeonS16, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_uconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::S128NarrowOp(this, NeonU16, NeonS16, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_sconvert_i8x16_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  vmovl(NeonS8, liftoff::GetSimd128Register(dst), src.low_fp());
}

void LiftoffAssembler::emit_i16x8_sconvert_i8x16_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  vmovl(NeonS8, liftoff::GetSimd128Register(dst), src.high_fp());
}

void LiftoffAssembler::emit_i16x8_uconvert_i8x16_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  vmovl(NeonU8, liftoff::GetSimd128Register(dst), src.low_fp());
}

void LiftoffAssembler::emit_i16x8_uconvert_i8x16_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  vmovl(NeonU8, liftoff::GetSimd128Register(dst), src.high_fp());
}

void LiftoffAssembler::emit_i32x4_sconvert_i16x8_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  vmovl(NeonS16, liftoff::GetSimd128Register(dst), src.low_fp());
}

void LiftoffAssembler::emit_i32x4_sconvert_i16x8_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  vmovl(NeonS16, liftoff::GetSimd128Register(dst), src.high_fp());
}

void LiftoffAssembler::emit_i32x4_uconvert_i16x8_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  vmovl(NeonU16, liftoff::GetSimd128Register(dst), src.low_fp());
}

void LiftoffAssembler::emit_i32x4_uconvert_i16x8_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  vmovl(NeonU16, liftoff::GetSimd128Register(dst), src.high_fp());
}

void LiftoffAssembler::emit_i32x4_trunc_sat_f64x2_s_zero(LiftoffRegister dst,
                                                         LiftoffRegister src) {
  LowDwVfpRegister dst_d = LowDwVfpRegister::from_code(dst.low_fp().code());
  vcvt_s32_f64(dst_d.low(), src.low_fp());
  vcvt_s32_f64(dst_d.high(), src.high_fp());
  vmov(dst.high_fp(), 0);
}

void LiftoffAssembler::emit_i32x4_trunc_sat_f64x2_u_zero(LiftoffRegister dst,
                                                         LiftoffRegister src) {
  LowDwVfpRegister dst_d = LowDwVfpRegister::from_code(dst.low_fp().code());
  vcvt_u32_f64(dst_d.low(), src.low_fp());
  vcvt_u32_f64(dst_d.high(), src.high_fp());
  vmov(dst.high_fp(), 0);
}

void LiftoffAssembler::emit_s128_and_not(LiftoffRegister dst,
                                         LiftoffRegister lhs,
                                         LiftoffRegister rhs) {
  vbic(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(lhs),
       liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_rounding_average_u(LiftoffRegister dst,
                                                     LiftoffRegister lhs,
                                                     LiftoffRegister rhs) {
  vrhadd(NeonU8, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i16x8_rounding_average_u(LiftoffRegister dst,
                                                     LiftoffRegister lhs,
                                                     LiftoffRegister rhs) {
  vrhadd(NeonU16, liftoff::GetSimd128Register(dst),
         liftoff::GetSimd128Register(lhs), liftoff::GetSimd128Register(rhs));
}

void LiftoffAssembler::emit_i8x16_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vabs(Neon8, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i16x8_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vabs(Neon16, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i32x4_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  vabs(Neon32, liftoff::GetSimd128Register(dst),
       liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::emit_i64x2_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  I64x2Abs(liftoff::GetSimd128Register(dst), liftoff::GetSimd128Register(src));
}

void LiftoffAssembler::StackCheck(Label* ool_code, Register limit_address) {
  ldr(limit_address, MemOperand(limit_address));
  cmp(sp, limit_address);
  b(ool_code, ls);
}

void LiftoffAssembler::CallTrapCallbackForTesting() {
  PrepareCallCFunction(0, 0);
  CallCFunction(ExternalReference::wasm_call_trap_callback_for_testing(), 0);
}

void LiftoffAssembler::AssertUnreachable(AbortReason reason) {
  // Asserts unreachable within the wasm code.
  TurboAssembler::AssertUnreachable(reason);
}

void LiftoffAssembler::PushRegisters(LiftoffRegList regs) {
  RegList core_regs = regs.GetGpList();
  if (core_regs != 0) {
    stm(db_w, sp, core_regs);
  }
  LiftoffRegList fp_regs = regs & kFpCacheRegList;
  while (!fp_regs.is_empty()) {
    LiftoffRegister reg = fp_regs.GetFirstRegSet();
    DoubleRegister first = reg.fp();
    DoubleRegister last = first;
    fp_regs.clear(reg);
    while (!fp_regs.is_empty()) {
      LiftoffRegister reg = fp_regs.GetFirstRegSet();
      int code = reg.fp().code();
      // vstm can not push more than 16 registers. We have to make sure the
      // condition is met.
      if ((code != last.code() + 1) || ((code - first.code() + 1) > 16)) break;
      last = reg.fp();
      fp_regs.clear(reg);
    }
    vstm(db_w, sp, first, last);
  }
}

void LiftoffAssembler::PopRegisters(LiftoffRegList regs) {
  LiftoffRegList fp_regs = regs & kFpCacheRegList;
  while (!fp_regs.is_empty()) {
    LiftoffRegister reg = fp_regs.GetLastRegSet();
    DoubleRegister last = reg.fp();
    DoubleRegister first = last;
    fp_regs.clear(reg);
    while (!fp_regs.is_empty()) {
      LiftoffRegister reg = fp_regs.GetLastRegSet();
      int code = reg.fp().code();
      if ((code != first.code() - 1) || ((last.code() - code + 1) > 16)) break;
      first = reg.fp();
      fp_regs.clear(reg);
    }
    vldm(ia_w, sp, first, last);
  }
  RegList core_regs = regs.GetGpList();
  if (core_regs != 0) {
    ldm(ia_w, sp, core_regs);
  }
}

void LiftoffAssembler::RecordSpillsInSafepoint(Safepoint& safepoint,
                                               LiftoffRegList all_spills,
                                               LiftoffRegList ref_spills,
                                               int spill_offset) {
  int spill_space_size = 0;
  while (!all_spills.is_empty()) {
    LiftoffRegister reg = all_spills.GetLastRegSet();
    if (ref_spills.has(reg)) {
      safepoint.DefinePointerSlot(spill_offset);
    }
    all_spills.clear(reg);
    ++spill_offset;
    spill_space_size += kSystemPointerSize;
  }
  // Record the number of additional spill slots.
  RecordOolSpillSpaceSize(spill_space_size);
}

void LiftoffAssembler::DropStackSlotsAndRet(uint32_t num_stack_slots) {
  Drop(num_stack_slots);
  Ret();
}

void LiftoffAssembler::CallC(const ValueKindSig* sig,
                             const LiftoffRegister* args,
                             const LiftoffRegister* rets,
                             ValueKind out_argument_kind, int stack_bytes,
                             ExternalReference ext_ref) {
  // Arguments are passed by pushing them all to the stack and then passing
  // a pointer to them.
  DCHECK(IsAligned(stack_bytes, kSystemPointerSize));
  // Reserve space in the stack.
  AllocateStackSpace(stack_bytes);

  int arg_bytes = 0;
  for (ValueKind param_kind : sig->parameters()) {
    switch (param_kind) {
      case kI32:
        str(args->gp(), MemOperand(sp, arg_bytes));
        break;
      case kI64:
        str(args->low_gp(), MemOperand(sp, arg_bytes));
        str(args->high_gp(), MemOperand(sp, arg_bytes + kSystemPointerSize));
        break;
      case kF32:
        vstr(liftoff::GetFloatRegister(args->fp()), MemOperand(sp, arg_bytes));
        break;
      case kF64:
        vstr(args->fp(), MemOperand(sp, arg_bytes));
        break;
      case kS128:
        vstr(args->low_fp(), MemOperand(sp, arg_bytes));
        vstr(args->high_fp(),
             MemOperand(sp, arg_bytes + 2 * kSystemPointerSize));
        break;
      default:
        UNREACHABLE();
    }
    args++;
    arg_bytes += element_size_bytes(param_kind);
  }
  DCHECK_LE(arg_bytes, stack_bytes);

  // Pass a pointer to the buffer with the arguments to the C function.
  mov(r0, sp);

  // Now call the C function.
  constexpr int kNumCCallArgs = 1;
  PrepareCallCFunction(kNumCCallArgs);
  CallCFunction(ext_ref, kNumCCallArgs);

  // Move return value to the right register.
  const LiftoffRegister* result_reg = rets;
  if (sig->return_count() > 0) {
    DCHECK_EQ(1, sig->return_count());
    constexpr Register kReturnReg = r0;
    if (kReturnReg != rets->gp()) {
      Move(*rets, LiftoffRegister(kReturnReg), sig->GetReturn(0));
    }
    result_reg++;
  }

  // Load potential output value from the buffer on the stack.
  if (out_argument_kind != kVoid) {
    switch (out_argument_kind) {
      case kI32:
        ldr(result_reg->gp(), MemOperand(sp));
        break;
      case kI64:
        ldr(result_reg->low_gp(), MemOperand(sp));
        ldr(result_reg->high_gp(), MemOperand(sp, kSystemPointerSize));
        break;
      case kF32:
        vldr(liftoff::GetFloatRegister(result_reg->fp()), MemOperand(sp));
        break;
      case kF64:
        vldr(result_reg->fp(), MemOperand(sp));
        break;
      case kS128:
        vld1(Neon8, NeonListOperand(result_reg->low_fp(), 2),
             NeonMemOperand(sp));
        break;
      default:
        UNREACHABLE();
    }
  }
  add(sp, sp, Operand(stack_bytes));
}

void LiftoffAssembler::CallNativeWasmCode(Address addr) {
  Call(addr, RelocInfo::WASM_CALL);
}

void LiftoffAssembler::TailCallNativeWasmCode(Address addr) {
  Jump(addr, RelocInfo::WASM_CALL);
}

void LiftoffAssembler::CallIndirect(const ValueKindSig* sig,
                                    compiler::CallDescriptor* call_descriptor,
                                    Register target) {
  DCHECK(target != no_reg);
  Call(target);
}

void LiftoffAssembler::TailCallIndirect(Register target) {
  DCHECK(target != no_reg);
  Jump(target);
}

void LiftoffAssembler::CallRuntimeStub(WasmCode::RuntimeStubId sid) {
  // A direct call to a wasm runtime stub defined in this module.
  // Just encode the stub index. This will be patched at relocation.
  Call(static_cast<Address>(sid), RelocInfo::WASM_STUB_CALL);
}

void LiftoffAssembler::AllocateStackSlot(Register addr, uint32_t size) {
  AllocateStackSpace(size);
  mov(addr, sp);
}

void LiftoffAssembler::DeallocateStackSlot(uint32_t size) {
  add(sp, sp, Operand(size));
}

void LiftoffAssembler::MaybeOSR() {}

void LiftoffStackSlots::Construct(int param_slots) {
  DCHECK_LT(0, slots_.size());
  SortInPushOrder();
  int last_stack_slot = param_slots;
  for (auto& slot : slots_) {
    const int stack_slot = slot.dst_slot_;
    int stack_decrement = (last_stack_slot - stack_slot) * kSystemPointerSize;
    DCHECK_LT(0, stack_decrement);
    last_stack_slot = stack_slot;
    const LiftoffAssembler::VarState& src = slot.src_;
    switch (src.loc()) {
      case LiftoffAssembler::VarState::kStack: {
        switch (src.kind()) {
          // i32 and i64 can be treated as similar cases, i64 being previously
          // split into two i32 registers
          case kI32:
          case kI64:
          case kF32:
          case kRef:
          case kOptRef: {
            asm_->AllocateStackSpace(stack_decrement - kSystemPointerSize);
            UseScratchRegisterScope temps(asm_);
            Register scratch = temps.Acquire();
            asm_->ldr(scratch,
                      liftoff::GetHalfStackSlot(slot.src_offset_, slot.half_));
            asm_->Push(scratch);
          } break;
          case kF64: {
            asm_->AllocateStackSpace(stack_decrement - kDoubleSize);
            UseScratchRegisterScope temps(asm_);
            DwVfpRegister scratch = temps.AcquireD();
            asm_->vldr(scratch, liftoff::GetStackSlot(slot.src_offset_));
            asm_->vpush(scratch);
          } break;
          case kS128: {
            asm_->AllocateStackSpace(stack_decrement - kSimd128Size);
            MemOperand mem_op = liftoff::GetStackSlot(slot.src_offset_);
            UseScratchRegisterScope temps(asm_);
            Register addr = liftoff::CalculateActualAddress(
                asm_, &temps, mem_op.rn(), no_reg, mem_op.offset());
            QwNeonRegister scratch = temps.AcquireQ();
            asm_->vld1(Neon8, NeonListOperand(scratch), NeonMemOperand(addr));
            asm_->vpush(scratch);
            break;
          }
          default:
            UNREACHABLE();
        }
        break;
      }
      case LiftoffAssembler::VarState::kRegister: {
        int pushed_bytes = SlotSizeInBytes(slot);
        asm_->AllocateStackSpace(stack_decrement - pushed_bytes);
        switch (src.kind()) {
          case kI64: {
            LiftoffRegister reg =
                slot.half_ == kLowWord ? src.reg().low() : src.reg().high();
            asm_->push(reg.gp());
          } break;
          case kI32:
          case kRef:
          case kOptRef:
            asm_->push(src.reg().gp());
            break;
          case kF32:
            asm_->vpush(liftoff::GetFloatRegister(src.reg().fp()));
            break;
          case kF64:
            asm_->vpush(src.reg().fp());
            break;
          case kS128:
            asm_->vpush(liftoff::GetSimd128Register(src.reg()));
            break;
          default:
            UNREACHABLE();
        }
        break;
      }
      case LiftoffAssembler::VarState::kIntConst: {
        asm_->AllocateStackSpace(stack_decrement - kSystemPointerSize);
        DCHECK(src.kind() == kI32 || src.kind() == kI64);
        UseScratchRegisterScope temps(asm_);
        Register scratch = temps.Acquire();
        // The high word is the sign extension of the low word.
        asm_->mov(scratch,
                  Operand(slot.half_ == kLowWord ? src.i32_const()
                                                 : src.i32_const() >> 31));
        asm_->push(scratch);
        break;
      }
    }
  }
}

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_BASELINE_ARM_LIFTOFF_ASSEMBLER_ARM_H_
