// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_WASM_BASELINE_IA32_LIFTOFF_ASSEMBLER_IA32_H_
#define V8_WASM_BASELINE_IA32_LIFTOFF_ASSEMBLER_IA32_H_

#include "src/base/platform/wrappers.h"
#include "src/codegen/assembler.h"
#include "src/heap/memory-chunk.h"
#include "src/wasm/baseline/liftoff-assembler.h"
#include "src/wasm/simd-shuffle.h"
#include "src/wasm/value-type.h"

namespace v8 {
namespace internal {
namespace wasm {

#define RETURN_FALSE_IF_MISSING_CPU_FEATURE(name)    \
  if (!CpuFeatures::IsSupported(name)) return false; \
  CpuFeatureScope feature(this, name);

namespace liftoff {

inline constexpr Condition ToCondition(LiftoffCondition liftoff_cond) {
  switch (liftoff_cond) {
    case kEqual:
      return equal;
    case kUnequal:
      return not_equal;
    case kSignedLessThan:
      return less;
    case kSignedLessEqual:
      return less_equal;
    case kSignedGreaterThan:
      return greater;
    case kSignedGreaterEqual:
      return greater_equal;
    case kUnsignedLessThan:
      return below;
    case kUnsignedLessEqual:
      return below_equal;
    case kUnsignedGreaterThan:
      return above;
    case kUnsignedGreaterEqual:
      return above_equal;
  }
}

// ebp-4 holds the stack marker, ebp-8 is the instance parameter.
constexpr int kInstanceOffset = 8;

inline Operand GetStackSlot(int offset) { return Operand(ebp, -offset); }

inline MemOperand GetHalfStackSlot(int offset, RegPairHalf half) {
  int32_t half_offset =
      half == kLowWord ? 0 : LiftoffAssembler::kStackSlotSize / 2;
  return Operand(offset > 0 ? ebp : esp, -offset + half_offset);
}

// TODO(clemensb): Make this a constexpr variable once Operand is constexpr.
inline Operand GetInstanceOperand() { return GetStackSlot(kInstanceOffset); }

static constexpr LiftoffRegList kByteRegs =
    LiftoffRegList::FromBits<Register::ListOf(eax, ecx, edx)>();

inline void Load(LiftoffAssembler* assm, LiftoffRegister dst, Register base,
                 int32_t offset, ValueKind kind) {
  Operand src(base, offset);
  switch (kind) {
    case kI32:
    case kOptRef:
    case kRef:
    case kRtt:
    case kRttWithDepth:
      assm->mov(dst.gp(), src);
      break;
    case kI64:
      assm->mov(dst.low_gp(), src);
      assm->mov(dst.high_gp(), Operand(base, offset + 4));
      break;
    case kF32:
      assm->movss(dst.fp(), src);
      break;
    case kF64:
      assm->movsd(dst.fp(), src);
      break;
    case kS128:
      assm->movdqu(dst.fp(), src);
      break;
    default:
      UNREACHABLE();
  }
}

inline void Store(LiftoffAssembler* assm, Register base, int32_t offset,
                  LiftoffRegister src, ValueKind kind) {
  Operand dst(base, offset);
  switch (kind) {
    case kI32:
    case kOptRef:
    case kRef:
    case kRtt:
    case kRttWithDepth:
      assm->mov(dst, src.gp());
      break;
    case kI64:
      assm->mov(dst, src.low_gp());
      assm->mov(Operand(base, offset + 4), src.high_gp());
      break;
    case kF32:
      assm->movss(dst, src.fp());
      break;
    case kF64:
      assm->movsd(dst, src.fp());
      break;
    case kS128:
      assm->movdqu(dst, src.fp());
      break;
    default:
      UNREACHABLE();
  }
}

inline void push(LiftoffAssembler* assm, LiftoffRegister reg, ValueKind kind,
                 int padding = 0) {
  switch (kind) {
    case kI32:
    case kRef:
    case kOptRef:
      assm->AllocateStackSpace(padding);
      assm->push(reg.gp());
      break;
    case kI64:
      assm->AllocateStackSpace(padding);
      assm->push(reg.high_gp());
      assm->push(reg.low_gp());
      break;
    case kF32:
      assm->AllocateStackSpace(sizeof(float) + padding);
      assm->movss(Operand(esp, 0), reg.fp());
      break;
    case kF64:
      assm->AllocateStackSpace(sizeof(double) + padding);
      assm->movsd(Operand(esp, 0), reg.fp());
      break;
    case kS128:
      assm->AllocateStackSpace(sizeof(double) * 2 + padding);
      assm->movdqu(Operand(esp, 0), reg.fp());
      break;
    default:
      UNREACHABLE();
  }
}

inline void SignExtendI32ToI64(Assembler* assm, LiftoffRegister reg) {
  assm->mov(reg.high_gp(), reg.low_gp());
  assm->sar(reg.high_gp(), 31);
}

// Get a temporary byte register, using {candidate} if possible.
// Might spill, but always keeps status flags intact.
inline Register GetTmpByteRegister(LiftoffAssembler* assm, Register candidate) {
  if (candidate.is_byte_register()) return candidate;
  // {GetUnusedRegister()} may insert move instructions to spill registers to
  // the stack. This is OK because {mov} does not change the status flags.
  return assm->GetUnusedRegister(liftoff::kByteRegs).gp();
}

inline void MoveStackValue(LiftoffAssembler* assm, const Operand& src,
                           const Operand& dst) {
  if (assm->cache_state()->has_unused_register(kGpReg)) {
    Register tmp = assm->cache_state()->unused_register(kGpReg).gp();
    assm->mov(tmp, src);
    assm->mov(dst, tmp);
  } else {
    // No free register, move via the stack.
    assm->push(src);
    assm->pop(dst);
  }
}

constexpr DoubleRegister kScratchDoubleReg = xmm7;

constexpr int kSubSpSize = 6;  // 6 bytes for "sub esp, <imm32>"

}  // namespace liftoff

int LiftoffAssembler::PrepareStackFrame() {
  int offset = pc_offset();
  // Next we reserve the memory for the whole stack frame. We do not know yet
  // how big the stack frame will be so we just emit a placeholder instruction.
  // PatchPrepareStackFrame will patch this in order to increase the stack
  // appropriately.
  sub_sp_32(0);
  DCHECK_EQ(liftoff::kSubSpSize, pc_offset() - offset);
  return offset;
}

void LiftoffAssembler::PrepareTailCall(int num_callee_stack_params,
                                       int stack_param_delta) {
  // Push the return address and frame pointer to complete the stack frame.
  push(Operand(ebp, 4));
  push(Operand(ebp, 0));

  // Shift the whole frame upwards.
  Register scratch = eax;
  push(scratch);
  const int slot_count = num_callee_stack_params + 2;
  for (int i = slot_count; i > 0; --i) {
    mov(scratch, Operand(esp, i * 4));
    mov(Operand(ebp, (i - stack_param_delta - 1) * 4), scratch);
  }
  pop(scratch);

  // Set the new stack and frame pointers.
  lea(esp, Operand(ebp, -stack_param_delta * 4));
  pop(ebp);
}

void LiftoffAssembler::AlignFrameSize() {}

void LiftoffAssembler::PatchPrepareStackFrame(int offset) {
  // The frame_size includes the frame marker. The frame marker has already been
  // pushed on the stack though, so we don't need to allocate memory for it
  // anymore.
  int frame_size = GetTotalFrameSize() - kSystemPointerSize;
  DCHECK_EQ(frame_size % kSystemPointerSize, 0);
  // We can't run out of space, just pass anything big enough to not cause the
  // assembler to try to grow the buffer.
  constexpr int kAvailableSpace = 64;
  Assembler patching_assembler(
      AssemblerOptions{},
      ExternalAssemblerBuffer(buffer_start_ + offset, kAvailableSpace));
#if V8_OS_WIN
  if (frame_size > kStackPageSize) {
    // Generate OOL code (at the end of the function, where the current
    // assembler is pointing) to do the explicit stack limit check (see
    // https://docs.microsoft.com/en-us/previous-versions/visualstudio/
    // visual-studio-6.0/aa227153(v=vs.60)).
    // At the function start, emit a jump to that OOL code (from {offset} to
    // {pc_offset()}).
    int ool_offset = pc_offset() - offset;
    patching_assembler.jmp_rel(ool_offset);
    DCHECK_GE(liftoff::kSubSpSize, patching_assembler.pc_offset());
    patching_assembler.Nop(liftoff::kSubSpSize -
                           patching_assembler.pc_offset());

    // Now generate the OOL code.
    AllocateStackSpace(frame_size);
    // Jump back to the start of the function (from {pc_offset()} to {offset +
    // kSubSpSize}).
    int func_start_offset = offset + liftoff::kSubSpSize - pc_offset();
    jmp_rel(func_start_offset);
    return;
  }
#endif
  patching_assembler.sub_sp_32(frame_size);
  DCHECK_EQ(liftoff::kSubSpSize, patching_assembler.pc_offset());
}

void LiftoffAssembler::FinishCode() {}

void LiftoffAssembler::AbortCompilation() {}

// static
constexpr int LiftoffAssembler::StaticStackFrameSize() {
  return liftoff::kInstanceOffset;
}

int LiftoffAssembler::SlotSizeForType(ValueKind kind) {
  return is_reference(kind) ? kSystemPointerSize : element_size_bytes(kind);
}

bool LiftoffAssembler::NeedsAlignment(ValueKind kind) {
  return is_reference(kind);
}

void LiftoffAssembler::LoadConstant(LiftoffRegister reg, WasmValue value,
                                    RelocInfo::Mode rmode) {
  switch (value.type().kind()) {
    case kI32:
      TurboAssembler::Move(reg.gp(), Immediate(value.to_i32(), rmode));
      break;
    case kI64: {
      DCHECK(RelocInfo::IsNone(rmode));
      int32_t low_word = value.to_i64();
      int32_t high_word = value.to_i64() >> 32;
      TurboAssembler::Move(reg.low_gp(), Immediate(low_word));
      TurboAssembler::Move(reg.high_gp(), Immediate(high_word));
      break;
    }
    case kF32:
      TurboAssembler::Move(reg.fp(), value.to_f32_boxed().get_bits());
      break;
    case kF64:
      TurboAssembler::Move(reg.fp(), value.to_f64_boxed().get_bits());
      break;
    default:
      UNREACHABLE();
  }
}

void LiftoffAssembler::LoadInstanceFromFrame(Register dst) {
  mov(dst, liftoff::GetInstanceOperand());
}

void LiftoffAssembler::LoadFromInstance(Register dst, Register instance,
                                        int offset, int size) {
  DCHECK_LE(0, offset);
  Operand src{instance, offset};
  switch (size) {
    case 1:
      movzx_b(dst, src);
      break;
    case 4:
      mov(dst, src);
      break;
    default:
      UNIMPLEMENTED();
  }
}

void LiftoffAssembler::LoadTaggedPointerFromInstance(Register dst,
                                                     Register instance,
                                                     int offset) {
  STATIC_ASSERT(kTaggedSize == kSystemPointerSize);
  mov(dst, Operand{instance, offset});
}

void LiftoffAssembler::SpillInstance(Register instance) {
  mov(liftoff::GetInstanceOperand(), instance);
}

void LiftoffAssembler::ResetOSRTarget() {}

void LiftoffAssembler::FillInstanceInto(Register dst) {
  mov(dst, liftoff::GetInstanceOperand());
}

void LiftoffAssembler::LoadTaggedPointer(Register dst, Register src_addr,
                                         Register offset_reg,
                                         int32_t offset_imm,
                                         LiftoffRegList pinned) {
  DCHECK_GE(offset_imm, 0);
  STATIC_ASSERT(kTaggedSize == kInt32Size);
  Load(LiftoffRegister(dst), src_addr, offset_reg,
       static_cast<uint32_t>(offset_imm), LoadType::kI32Load, pinned);
}

void LiftoffAssembler::LoadFullPointer(Register dst, Register src_addr,
                                       int32_t offset_imm) {
  mov(dst, Operand(src_addr, offset_imm));
}

void LiftoffAssembler::StoreTaggedPointer(Register dst_addr,
                                          Register offset_reg,
                                          int32_t offset_imm,
                                          LiftoffRegister src,
                                          LiftoffRegList pinned,
                                          SkipWriteBarrier skip_write_barrier) {
  DCHECK_GE(offset_imm, 0);
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  STATIC_ASSERT(kTaggedSize == kInt32Size);
  Operand dst_op = offset_reg == no_reg
                       ? Operand(dst_addr, offset_imm)
                       : Operand(dst_addr, offset_reg, times_1, offset_imm);
  mov(dst_op, src.gp());

  if (skip_write_barrier || FLAG_disable_write_barriers) return;

  Register scratch = pinned.set(GetUnusedRegister(kGpReg, pinned)).gp();
  Label write_barrier;
  Label exit;
  CheckPageFlag(dst_addr, scratch,
                MemoryChunk::kPointersFromHereAreInterestingMask, not_zero,
                &write_barrier, Label::kNear);
  jmp(&exit, Label::kNear);
  bind(&write_barrier);
  JumpIfSmi(src.gp(), &exit, Label::kNear);
  CheckPageFlag(src.gp(), scratch,
                MemoryChunk::kPointersToHereAreInterestingMask, zero, &exit,
                Label::kNear);
  lea(scratch, dst_op);
  CallRecordWriteStub(dst_addr, scratch, RememberedSetAction::kEmit,
                      SaveFPRegsMode::kSave,
                      StubCallMode::kCallWasmRuntimeStub);
  bind(&exit);
}

void LiftoffAssembler::Load(LiftoffRegister dst, Register src_addr,
                            Register offset_reg, uint32_t offset_imm,
                            LoadType type, LiftoffRegList pinned,
                            uint32_t* protected_load_pc, bool is_load_mem) {
  // Offsets >=2GB are statically OOB on 32-bit systems.
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  DCHECK_EQ(type.value_type() == kWasmI64, dst.is_gp_pair());
  Operand src_op = offset_reg == no_reg
                       ? Operand(src_addr, offset_imm)
                       : Operand(src_addr, offset_reg, times_1, offset_imm);
  if (protected_load_pc) *protected_load_pc = pc_offset();

  switch (type.value()) {
    case LoadType::kI32Load8U:
      movzx_b(dst.gp(), src_op);
      break;
    case LoadType::kI32Load8S:
      movsx_b(dst.gp(), src_op);
      break;
    case LoadType::kI64Load8U:
      movzx_b(dst.low_gp(), src_op);
      xor_(dst.high_gp(), dst.high_gp());
      break;
    case LoadType::kI64Load8S:
      movsx_b(dst.low_gp(), src_op);
      liftoff::SignExtendI32ToI64(this, dst);
      break;
    case LoadType::kI32Load16U:
      movzx_w(dst.gp(), src_op);
      break;
    case LoadType::kI32Load16S:
      movsx_w(dst.gp(), src_op);
      break;
    case LoadType::kI64Load16U:
      movzx_w(dst.low_gp(), src_op);
      xor_(dst.high_gp(), dst.high_gp());
      break;
    case LoadType::kI64Load16S:
      movsx_w(dst.low_gp(), src_op);
      liftoff::SignExtendI32ToI64(this, dst);
      break;
    case LoadType::kI32Load:
      mov(dst.gp(), src_op);
      break;
    case LoadType::kI64Load32U:
      mov(dst.low_gp(), src_op);
      xor_(dst.high_gp(), dst.high_gp());
      break;
    case LoadType::kI64Load32S:
      mov(dst.low_gp(), src_op);
      liftoff::SignExtendI32ToI64(this, dst);
      break;
    case LoadType::kI64Load: {
      // Compute the operand for the load of the upper half.
      Operand upper_src_op =
          offset_reg == no_reg
              ? Operand(src_addr, bit_cast<int32_t>(offset_imm + 4))
              : Operand(src_addr, offset_reg, times_1, offset_imm + 4);
      // The high word has to be mov'ed first, such that this is the protected
      // instruction. The mov of the low word cannot segfault.
      mov(dst.high_gp(), upper_src_op);
      mov(dst.low_gp(), src_op);
      break;
    }
    case LoadType::kF32Load:
      movss(dst.fp(), src_op);
      break;
    case LoadType::kF64Load:
      movsd(dst.fp(), src_op);
      break;
    case LoadType::kS128Load:
      movdqu(dst.fp(), src_op);
      break;
  }
}

void LiftoffAssembler::Store(Register dst_addr, Register offset_reg,
                             uint32_t offset_imm, LiftoffRegister src,
                             StoreType type, LiftoffRegList pinned,
                             uint32_t* protected_store_pc, bool is_store_mem) {
  DCHECK_EQ(type.value_type() == kWasmI64, src.is_gp_pair());
  // Offsets >=2GB are statically OOB on 32-bit systems.
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  Operand dst_op = offset_reg == no_reg
                       ? Operand(dst_addr, offset_imm)
                       : Operand(dst_addr, offset_reg, times_1, offset_imm);
  if (protected_store_pc) *protected_store_pc = pc_offset();

  switch (type.value()) {
    case StoreType::kI64Store8:
      src = src.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store8:
      // Only the lower 4 registers can be addressed as 8-bit registers.
      if (src.gp().is_byte_register()) {
        mov_b(dst_op, src.gp());
      } else {
        // We know that {src} is not a byte register, so the only pinned byte
        // registers (beside the outer {pinned}) are {dst_addr} and potentially
        // {offset_reg}.
        LiftoffRegList pinned_byte = pinned | LiftoffRegList::ForRegs(dst_addr);
        if (offset_reg != no_reg) pinned_byte.set(offset_reg);
        Register byte_src =
            GetUnusedRegister(liftoff::kByteRegs.MaskOut(pinned_byte)).gp();
        mov(byte_src, src.gp());
        mov_b(dst_op, byte_src);
      }
      break;
    case StoreType::kI64Store16:
      src = src.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store16:
      mov_w(dst_op, src.gp());
      break;
    case StoreType::kI64Store32:
      src = src.low();
      V8_FALLTHROUGH;
    case StoreType::kI32Store:
      mov(dst_op, src.gp());
      break;
    case StoreType::kI64Store: {
      // Compute the operand for the store of the upper half.
      Operand upper_dst_op =
          offset_reg == no_reg
              ? Operand(dst_addr, bit_cast<int32_t>(offset_imm + 4))
              : Operand(dst_addr, offset_reg, times_1, offset_imm + 4);
      // The high word has to be mov'ed first, such that this is the protected
      // instruction. The mov of the low word cannot segfault.
      mov(upper_dst_op, src.high_gp());
      mov(dst_op, src.low_gp());
      break;
    }
    case StoreType::kF32Store:
      movss(dst_op, src.fp());
      break;
    case StoreType::kF64Store:
      movsd(dst_op, src.fp());
      break;
    case StoreType::kS128Store:
      Movdqu(dst_op, src.fp());
      break;
  }
}

void LiftoffAssembler::AtomicLoad(LiftoffRegister dst, Register src_addr,
                                  Register offset_reg, uint32_t offset_imm,
                                  LoadType type, LiftoffRegList pinned) {
  if (type.value() != LoadType::kI64Load) {
    Load(dst, src_addr, offset_reg, offset_imm, type, pinned, nullptr, true);
    return;
  }

  DCHECK_EQ(type.value_type() == kWasmI64, dst.is_gp_pair());
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  Operand src_op = offset_reg == no_reg
                       ? Operand(src_addr, offset_imm)
                       : Operand(src_addr, offset_reg, times_1, offset_imm);

  movsd(liftoff::kScratchDoubleReg, src_op);
  Pextrd(dst.low().gp(), liftoff::kScratchDoubleReg, 0);
  Pextrd(dst.high().gp(), liftoff::kScratchDoubleReg, 1);
}

void LiftoffAssembler::AtomicStore(Register dst_addr, Register offset_reg,
                                   uint32_t offset_imm, LiftoffRegister src,
                                   StoreType type, LiftoffRegList pinned) {
  DCHECK_NE(offset_reg, no_reg);
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  Operand dst_op = Operand(dst_addr, offset_reg, times_1, offset_imm);

  // i64 store uses a totally different approach, hence implement it separately.
  if (type.value() == StoreType::kI64Store) {
    auto scratch2 = GetUnusedRegister(kFpReg, pinned).fp();
    movd(liftoff::kScratchDoubleReg, src.low().gp());
    movd(scratch2, src.high().gp());
    Punpckldq(liftoff::kScratchDoubleReg, scratch2);
    movsd(dst_op, liftoff::kScratchDoubleReg);
    // This lock+or is needed to achieve sequential consistency.
    lock();
    or_(Operand(esp, 0), Immediate(0));
    return;
  }

  // Other i64 stores actually only use the low word.
  if (src.is_pair()) src = src.low();
  Register src_gp = src.gp();

  bool is_byte_store = type.size() == 1;
  LiftoffRegList src_candidates =
      is_byte_store ? liftoff::kByteRegs : kGpCacheRegList;
  pinned = pinned | LiftoffRegList::ForRegs(dst_addr, src, offset_reg);

  // Ensure that {src} is a valid and otherwise unused register.
  if (!src_candidates.has(src) || cache_state_.is_used(src)) {
    // If there are no unused candidate registers, but {src} is a candidate,
    // then spill other uses of {src}. Otherwise spill any candidate register
    // and use that.
    LiftoffRegList unpinned_candidates = src_candidates.MaskOut(pinned);
    if (!cache_state_.has_unused_register(unpinned_candidates) &&
        src_candidates.has(src)) {
      SpillRegister(src);
    } else {
      Register safe_src = GetUnusedRegister(unpinned_candidates).gp();
      mov(safe_src, src_gp);
      src_gp = safe_src;
    }
  }

  switch (type.value()) {
    case StoreType::kI64Store8:
    case StoreType::kI32Store8:
      xchg_b(src_gp, dst_op);
      return;
    case StoreType::kI64Store16:
    case StoreType::kI32Store16:
      xchg_w(src_gp, dst_op);
      return;
    case StoreType::kI64Store32:
    case StoreType::kI32Store:
      xchg(src_gp, dst_op);
      return;
    default:
      UNREACHABLE();
  }
}

namespace liftoff {
#define __ lasm->

enum Binop { kAdd, kSub, kAnd, kOr, kXor, kExchange };

inline void AtomicAddOrSubOrExchange32(LiftoffAssembler* lasm, Binop binop,
                                       Register dst_addr, Register offset_reg,
                                       uint32_t offset_imm,
                                       LiftoffRegister value,
                                       LiftoffRegister result, StoreType type) {
  DCHECK_EQ(value, result);
  DCHECK(!__ cache_state()->is_used(result));
  bool is_64_bit_op = type.value_type() == kWasmI64;

  Register value_reg = is_64_bit_op ? value.low_gp() : value.gp();
  Register result_reg = is_64_bit_op ? result.low_gp() : result.gp();

  bool is_byte_store = type.size() == 1;
  LiftoffRegList pinned =
      LiftoffRegList::ForRegs(dst_addr, value_reg, offset_reg);

  // Ensure that {value_reg} is a valid register.
  if (is_byte_store && !liftoff::kByteRegs.has(value_reg)) {
    Register safe_value_reg =
        __ GetUnusedRegister(liftoff::kByteRegs.MaskOut(pinned)).gp();
    __ mov(safe_value_reg, value_reg);
    value_reg = safe_value_reg;
  }

  Operand dst_op = Operand(dst_addr, offset_reg, times_1, offset_imm);
  if (binop == kSub) {
    __ neg(value_reg);
  }
  if (binop != kExchange) {
    __ lock();
  }
  switch (type.value()) {
    case StoreType::kI64Store8:
    case StoreType::kI32Store8:
      if (binop == kExchange) {
        __ xchg_b(value_reg, dst_op);
      } else {
        __ xadd_b(dst_op, value_reg);
      }
      __ movzx_b(result_reg, value_reg);
      break;
    case StoreType::kI64Store16:
    case StoreType::kI32Store16:
      if (binop == kExchange) {
        __ xchg_w(value_reg, dst_op);
      } else {
        __ xadd_w(dst_op, value_reg);
      }
      __ movzx_w(result_reg, value_reg);
      break;
    case StoreType::kI64Store32:
    case StoreType::kI32Store:
      if (binop == kExchange) {
        __ xchg(value_reg, dst_op);
      } else {
        __ xadd(dst_op, value_reg);
      }
      if (value_reg != result_reg) {
        __ mov(result_reg, value_reg);
      }
      break;
    default:
      UNREACHABLE();
  }
  if (is_64_bit_op) {
    __ xor_(result.high_gp(), result.high_gp());
  }
}

inline void AtomicBinop32(LiftoffAssembler* lasm, Binop op, Register dst_addr,
                          Register offset_reg, uint32_t offset_imm,
                          LiftoffRegister value, LiftoffRegister result,
                          StoreType type) {
  DCHECK_EQ(value, result);
  DCHECK(!__ cache_state()->is_used(result));
  bool is_64_bit_op = type.value_type() == kWasmI64;

  Register value_reg = is_64_bit_op ? value.low_gp() : value.gp();
  Register result_reg = is_64_bit_op ? result.low_gp() : result.gp();

  // The cmpxchg instruction uses eax to store the old value of the
  // compare-exchange primitive. Therefore we have to spill the register and
  // move any use to another register.
  __ ClearRegister(eax, {&dst_addr, &offset_reg, &value_reg},
                   LiftoffRegList::ForRegs(dst_addr, offset_reg, value_reg));

  bool is_byte_store = type.size() == 1;
  Register scratch = no_reg;
  if (is_byte_store) {
    // The scratch register has to be a byte register. As we are already tight
    // on registers, we just use the root register here.
    static_assert((kLiftoffAssemblerGpCacheRegs & kRootRegister.bit()) == 0,
                  "root register is not Liftoff cache register");
    DCHECK(kRootRegister.is_byte_register());
    __ push(kRootRegister);
    scratch = kRootRegister;
  } else {
    scratch = __ GetUnusedRegister(
                  kGpReg,
                  LiftoffRegList::ForRegs(dst_addr, offset_reg, value_reg, eax))
                  .gp();
  }

  Operand dst_op = Operand(dst_addr, offset_reg, times_1, offset_imm);

  switch (type.value()) {
    case StoreType::kI32Store8:
    case StoreType::kI64Store8: {
      __ xor_(eax, eax);
      __ mov_b(eax, dst_op);
      break;
    }
    case StoreType::kI32Store16:
    case StoreType::kI64Store16: {
      __ xor_(eax, eax);
      __ mov_w(eax, dst_op);
      break;
    }
    case StoreType::kI32Store:
    case StoreType::kI64Store32: {
      __ mov(eax, dst_op);
      break;
    }
    default:
      UNREACHABLE();
  }

  Label binop;
  __ bind(&binop);
  __ mov(scratch, eax);

  switch (op) {
    case kAnd: {
      __ and_(scratch, value_reg);
      break;
    }
    case kOr: {
      __ or_(scratch, value_reg);
      break;
    }
    case kXor: {
      __ xor_(scratch, value_reg);
      break;
    }
    default:
      UNREACHABLE();
  }

  __ lock();

  switch (type.value()) {
    case StoreType::kI32Store8:
    case StoreType::kI64Store8: {
      __ cmpxchg_b(dst_op, scratch);
      break;
    }
    case StoreType::kI32Store16:
    case StoreType::kI64Store16: {
      __ cmpxchg_w(dst_op, scratch);
      break;
    }
    case StoreType::kI32Store:
    case StoreType::kI64Store32: {
      __ cmpxchg(dst_op, scratch);
      break;
    }
    default:
      UNREACHABLE();
  }
  __ j(not_equal, &binop);

  if (is_byte_store) {
    __ pop(kRootRegister);
  }
  if (result_reg != eax) {
    __ mov(result_reg, eax);
  }
  if (is_64_bit_op) {
    __ xor_(result.high_gp(), result.high_gp());
  }
}

inline void AtomicBinop64(LiftoffAssembler* lasm, Binop op, Register dst_addr,
                          Register offset_reg, uint32_t offset_imm,
                          LiftoffRegister value, LiftoffRegister result) {
  // We need {ebx} here, which is the root register. As the root register it
  // needs special treatment. As we use {ebx} directly in the code below, we
  // have to make sure here that the root register is actually {ebx}.
  static_assert(kRootRegister == ebx,
                "The following code assumes that kRootRegister == ebx");
  __ push(ebx);

  // Store the value on the stack, so that we can use it for retries.
  __ AllocateStackSpace(8);
  Operand value_op_hi = Operand(esp, 0);
  Operand value_op_lo = Operand(esp, 4);
  __ mov(value_op_lo, value.low_gp());
  __ mov(value_op_hi, value.high_gp());

  // We want to use the compare-exchange instruction here. It uses registers
  // as follows: old-value = EDX:EAX; new-value = ECX:EBX.
  Register old_hi = edx;
  Register old_lo = eax;
  Register new_hi = ecx;
  Register new_lo = ebx;
  // Base and offset need separate registers that do not alias with the
  // ones above.
  Register base = esi;
  Register offset = edi;

  // Swap base and offset register if necessary to avoid unnecessary
  // moves.
  if (dst_addr == offset || offset_reg == base) {
    std::swap(dst_addr, offset_reg);
  }
  // Spill all these registers if they are still holding other values.
  __ SpillRegisters(old_hi, old_lo, new_hi, base, offset);
  __ ParallelRegisterMove(
      {{LiftoffRegister::ForPair(base, offset),
        LiftoffRegister::ForPair(dst_addr, offset_reg), kI64}});

  Operand dst_op_lo = Operand(base, offset, times_1, offset_imm);
  Operand dst_op_hi = Operand(base, offset, times_1, offset_imm + 4);

  // Load the old value from memory.
  __ mov(old_lo, dst_op_lo);
  __ mov(old_hi, dst_op_hi);
  Label retry;
  __ bind(&retry);
  __ mov(new_lo, old_lo);
  __ mov(new_hi, old_hi);
  switch (op) {
    case kAdd:
      __ add(new_lo, value_op_lo);
      __ adc(new_hi, value_op_hi);
      break;
    case kSub:
      __ sub(new_lo, value_op_lo);
      __ sbb(new_hi, value_op_hi);
      break;
    case kAnd:
      __ and_(new_lo, value_op_lo);
      __ and_(new_hi, value_op_hi);
      break;
    case kOr:
      __ or_(new_lo, value_op_lo);
      __ or_(new_hi, value_op_hi);
      break;
    case kXor:
      __ xor_(new_lo, value_op_lo);
      __ xor_(new_hi, value_op_hi);
      break;
    case kExchange:
      __ mov(new_lo, value_op_lo);
      __ mov(new_hi, value_op_hi);
      break;
  }
  __ lock();
  __ cmpxchg8b(dst_op_lo);
  __ j(not_equal, &retry);

  // Deallocate the stack space again.
  __ add(esp, Immediate(8));
  // Restore the root register, and we are done.
  __ pop(kRootRegister);

  // Move the result into the correct registers.
  __ ParallelRegisterMove(
      {{result, LiftoffRegister::ForPair(old_lo, old_hi), kI64}});
}

#undef __
}  // namespace liftoff

void LiftoffAssembler::AtomicAdd(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicBinop64(this, liftoff::kAdd, dst_addr, offset_reg,
                           offset_imm, value, result);
    return;
  }

  liftoff::AtomicAddOrSubOrExchange32(this, liftoff::kAdd, dst_addr, offset_reg,
                                      offset_imm, value, result, type);
}

void LiftoffAssembler::AtomicSub(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicBinop64(this, liftoff::kSub, dst_addr, offset_reg,
                           offset_imm, value, result);
    return;
  }
  liftoff::AtomicAddOrSubOrExchange32(this, liftoff::kSub, dst_addr, offset_reg,
                                      offset_imm, value, result, type);
}

void LiftoffAssembler::AtomicAnd(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicBinop64(this, liftoff::kAnd, dst_addr, offset_reg,
                           offset_imm, value, result);
    return;
  }

  liftoff::AtomicBinop32(this, liftoff::kAnd, dst_addr, offset_reg, offset_imm,
                         value, result, type);
}

void LiftoffAssembler::AtomicOr(Register dst_addr, Register offset_reg,
                                uint32_t offset_imm, LiftoffRegister value,
                                LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicBinop64(this, liftoff::kOr, dst_addr, offset_reg, offset_imm,
                           value, result);
    return;
  }

  liftoff::AtomicBinop32(this, liftoff::kOr, dst_addr, offset_reg, offset_imm,
                         value, result, type);
}

void LiftoffAssembler::AtomicXor(Register dst_addr, Register offset_reg,
                                 uint32_t offset_imm, LiftoffRegister value,
                                 LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicBinop64(this, liftoff::kXor, dst_addr, offset_reg,
                           offset_imm, value, result);
    return;
  }

  liftoff::AtomicBinop32(this, liftoff::kXor, dst_addr, offset_reg, offset_imm,
                         value, result, type);
}

void LiftoffAssembler::AtomicExchange(Register dst_addr, Register offset_reg,
                                      uint32_t offset_imm,
                                      LiftoffRegister value,
                                      LiftoffRegister result, StoreType type) {
  if (type.value() == StoreType::kI64Store) {
    liftoff::AtomicBinop64(this, liftoff::kExchange, dst_addr, offset_reg,
                           offset_imm, value, result);
    return;
  }
  liftoff::AtomicAddOrSubOrExchange32(this, liftoff::kExchange, dst_addr,
                                      offset_reg, offset_imm, value, result,
                                      type);
}

void LiftoffAssembler::AtomicCompareExchange(
    Register dst_addr, Register offset_reg, uint32_t offset_imm,
    LiftoffRegister expected, LiftoffRegister new_value, LiftoffRegister result,
    StoreType type) {
  // We expect that the offset has already been added to {dst_addr}, and no
  // {offset_reg} is provided. This is to save registers.
  DCHECK_EQ(offset_reg, no_reg);

  DCHECK_EQ(result, expected);

  if (type.value() != StoreType::kI64Store) {
    bool is_64_bit_op = type.value_type() == kWasmI64;

    Register value_reg = is_64_bit_op ? new_value.low_gp() : new_value.gp();
    Register expected_reg = is_64_bit_op ? expected.low_gp() : expected.gp();
    Register result_reg = expected_reg;

    // The cmpxchg instruction uses eax to store the old value of the
    // compare-exchange primitive. Therefore we have to spill the register and
    // move any use to another register.
    ClearRegister(eax, {&dst_addr, &value_reg},
                  LiftoffRegList::ForRegs(dst_addr, value_reg, expected_reg));
    if (expected_reg != eax) {
      mov(eax, expected_reg);
      expected_reg = eax;
    }

    bool is_byte_store = type.size() == 1;
    LiftoffRegList pinned =
        LiftoffRegList::ForRegs(dst_addr, value_reg, expected_reg);

    // Ensure that {value_reg} is a valid register.
    if (is_byte_store && !liftoff::kByteRegs.has(value_reg)) {
      Register safe_value_reg =
          pinned.set(GetUnusedRegister(liftoff::kByteRegs.MaskOut(pinned)))
              .gp();
      mov(safe_value_reg, value_reg);
      value_reg = safe_value_reg;
      pinned.clear(LiftoffRegister(value_reg));
    }


    Operand dst_op = Operand(dst_addr, offset_imm);

    lock();
    switch (type.value()) {
      case StoreType::kI32Store8:
      case StoreType::kI64Store8: {
        cmpxchg_b(dst_op, value_reg);
        movzx_b(result_reg, eax);
        break;
      }
      case StoreType::kI32Store16:
      case StoreType::kI64Store16: {
        cmpxchg_w(dst_op, value_reg);
        movzx_w(result_reg, eax);
        break;
      }
      case StoreType::kI32Store:
      case StoreType::kI64Store32: {
        cmpxchg(dst_op, value_reg);
        if (result_reg != eax) {
          mov(result_reg, eax);
        }
        break;
      }
      default:
        UNREACHABLE();
    }
    if (is_64_bit_op) {
      xor_(result.high_gp(), result.high_gp());
    }
    return;
  }

  // The following code handles kExprI64AtomicCompareExchange.

  // We need {ebx} here, which is the root register. The root register it
  // needs special treatment. As we use {ebx} directly in the code below, we
  // have to make sure here that the root register is actually {ebx}.
  static_assert(kRootRegister == ebx,
                "The following code assumes that kRootRegister == ebx");
  push(kRootRegister);

  // The compare-exchange instruction uses registers as follows:
  // old-value = EDX:EAX; new-value = ECX:EBX.
  Register expected_hi = edx;
  Register expected_lo = eax;
  Register new_hi = ecx;
  Register new_lo = ebx;
  // The address needs a separate registers that does not alias with the
  // ones above.
  Register address = esi;

  // Spill all these registers if they are still holding other values.
  SpillRegisters(expected_hi, expected_lo, new_hi, address);

  // We have to set new_lo specially, because it's the root register. We do it
  // before setting all other registers so that the original value does not get
  // overwritten.
  mov(new_lo, new_value.low_gp());

  // Move all other values into the right register.
  ParallelRegisterMove(
      {{LiftoffRegister(address), LiftoffRegister(dst_addr), kI32},
       {LiftoffRegister::ForPair(expected_lo, expected_hi), expected, kI64},
       {LiftoffRegister(new_hi), new_value.high(), kI32}});

  Operand dst_op = Operand(address, offset_imm);

  lock();
  cmpxchg8b(dst_op);

  // Restore the root register, and we are done.
  pop(kRootRegister);

  // Move the result into the correct registers.
  ParallelRegisterMove(
      {{result, LiftoffRegister::ForPair(expected_lo, expected_hi), kI64}});
}

void LiftoffAssembler::AtomicFence() { mfence(); }

void LiftoffAssembler::LoadCallerFrameSlot(LiftoffRegister dst,
                                           uint32_t caller_slot_idx,
                                           ValueKind kind) {
  liftoff::Load(this, dst, ebp, kSystemPointerSize * (caller_slot_idx + 1),
                kind);
}

void LiftoffAssembler::LoadReturnStackSlot(LiftoffRegister reg, int offset,
                                           ValueKind kind) {
  liftoff::Load(this, reg, esp, offset, kind);
}

void LiftoffAssembler::StoreCallerFrameSlot(LiftoffRegister src,
                                            uint32_t caller_slot_idx,
                                            ValueKind kind) {
  liftoff::Store(this, ebp, kSystemPointerSize * (caller_slot_idx + 1), src,
                 kind);
}

void LiftoffAssembler::MoveStackValue(uint32_t dst_offset, uint32_t src_offset,
                                      ValueKind kind) {
  if (needs_gp_reg_pair(kind)) {
    liftoff::MoveStackValue(this,
                            liftoff::GetHalfStackSlot(src_offset, kLowWord),
                            liftoff::GetHalfStackSlot(dst_offset, kLowWord));
    liftoff::MoveStackValue(this,
                            liftoff::GetHalfStackSlot(src_offset, kHighWord),
                            liftoff::GetHalfStackSlot(dst_offset, kHighWord));
  } else {
    liftoff::MoveStackValue(this, liftoff::GetStackSlot(src_offset),
                            liftoff::GetStackSlot(dst_offset));
  }
}

void LiftoffAssembler::Move(Register dst, Register src, ValueKind kind) {
  DCHECK_NE(dst, src);
  DCHECK(kI32 == kind || is_reference(kind));
  mov(dst, src);
}

void LiftoffAssembler::Move(DoubleRegister dst, DoubleRegister src,
                            ValueKind kind) {
  DCHECK_NE(dst, src);
  if (kind == kF32) {
    movss(dst, src);
  } else if (kind == kF64) {
    movsd(dst, src);
  } else {
    DCHECK_EQ(kS128, kind);
    Movaps(dst, src);
  }
}

void LiftoffAssembler::Spill(int offset, LiftoffRegister reg, ValueKind kind) {
  RecordUsedSpillOffset(offset);
  Operand dst = liftoff::GetStackSlot(offset);
  switch (kind) {
    case kI32:
    case kOptRef:
    case kRef:
    case kRtt:
    case kRttWithDepth:
      mov(dst, reg.gp());
      break;
    case kI64:
      mov(liftoff::GetHalfStackSlot(offset, kLowWord), reg.low_gp());
      mov(liftoff::GetHalfStackSlot(offset, kHighWord), reg.high_gp());
      break;
    case kF32:
      movss(dst, reg.fp());
      break;
    case kF64:
      movsd(dst, reg.fp());
      break;
    case kS128:
      movdqu(dst, reg.fp());
      break;
    default:
      UNREACHABLE();
  }
}

void LiftoffAssembler::Spill(int offset, WasmValue value) {
  RecordUsedSpillOffset(offset);
  Operand dst = liftoff::GetStackSlot(offset);
  switch (value.type().kind()) {
    case kI32:
      mov(dst, Immediate(value.to_i32()));
      break;
    case kI64: {
      int32_t low_word = value.to_i64();
      int32_t high_word = value.to_i64() >> 32;
      mov(liftoff::GetHalfStackSlot(offset, kLowWord), Immediate(low_word));
      mov(liftoff::GetHalfStackSlot(offset, kHighWord), Immediate(high_word));
      break;
    }
    default:
      // We do not track f32 and f64 constants, hence they are unreachable.
      UNREACHABLE();
  }
}

void LiftoffAssembler::Fill(LiftoffRegister reg, int offset, ValueKind kind) {
  liftoff::Load(this, reg, ebp, -offset, kind);
}

void LiftoffAssembler::FillI64Half(Register reg, int offset, RegPairHalf half) {
  mov(reg, liftoff::GetHalfStackSlot(offset, half));
}

void LiftoffAssembler::FillStackSlotsWithZero(int start, int size) {
  DCHECK_LT(0, size);
  DCHECK_EQ(0, size % 4);
  RecordUsedSpillOffset(start + size);

  if (size <= 12) {
    // Special straight-line code for up to three words (6-9 bytes per word:
    // C7 <1-4 bytes operand> <4 bytes imm>, makes 18-27 bytes total).
    for (int offset = 4; offset <= size; offset += 4) {
      mov(liftoff::GetHalfStackSlot(start + offset, kLowWord), Immediate(0));
    }
  } else {
    // General case for bigger counts.
    // This sequence takes 19-22 bytes (3 for pushes, 3-6 for lea, 2 for xor, 5
    // for mov, 3 for repstosq, 3 for pops).
    // Note: rep_stos fills ECX doublewords at [EDI] with EAX.
    push(eax);
    push(ecx);
    push(edi);
    lea(edi, liftoff::GetStackSlot(start + size));
    xor_(eax, eax);
    // Size is in bytes, convert to doublewords (4-bytes).
    mov(ecx, Immediate(size / 4));
    rep_stos();
    pop(edi);
    pop(ecx);
    pop(eax);
  }
}

void LiftoffAssembler::emit_i32_add(Register dst, Register lhs, Register rhs) {
  if (lhs != dst) {
    lea(dst, Operand(lhs, rhs, times_1, 0));
  } else {
    add(dst, rhs);
  }
}

void LiftoffAssembler::emit_i32_addi(Register dst, Register lhs, int32_t imm) {
  if (lhs != dst) {
    lea(dst, Operand(lhs, imm));
  } else {
    add(dst, Immediate(imm));
  }
}

void LiftoffAssembler::emit_i32_sub(Register dst, Register lhs, Register rhs) {
  if (dst != rhs) {
    // Default path.
    if (dst != lhs) mov(dst, lhs);
    sub(dst, rhs);
  } else if (lhs == rhs) {
    // Degenerate case.
    xor_(dst, dst);
  } else {
    // Emit {dst = lhs + -rhs} if dst == rhs.
    neg(dst);
    add(dst, lhs);
  }
}

void LiftoffAssembler::emit_i32_subi(Register dst, Register lhs, int32_t imm) {
  if (dst != lhs) {
    // We'll have to implement an UB-safe version if we need this corner case.
    DCHECK_NE(imm, kMinInt);
    lea(dst, Operand(lhs, -imm));
  } else {
    sub(dst, Immediate(imm));
  }
}

namespace liftoff {
template <void (Assembler::*op)(Register, Register)>
void EmitCommutativeBinOp(LiftoffAssembler* assm, Register dst, Register lhs,
                          Register rhs) {
  if (dst == rhs) {
    (assm->*op)(dst, lhs);
  } else {
    if (dst != lhs) assm->mov(dst, lhs);
    (assm->*op)(dst, rhs);
  }
}

template <void (Assembler::*op)(Register, int32_t)>
void EmitCommutativeBinOpImm(LiftoffAssembler* assm, Register dst, Register lhs,
                             int32_t imm) {
  if (dst != lhs) assm->mov(dst, lhs);
  (assm->*op)(dst, imm);
}
}  // namespace liftoff

void LiftoffAssembler::emit_i32_mul(Register dst, Register lhs, Register rhs) {
  liftoff::EmitCommutativeBinOp<&Assembler::imul>(this, dst, lhs, rhs);
}

namespace liftoff {
enum class DivOrRem : uint8_t { kDiv, kRem };
template <bool is_signed, DivOrRem div_or_rem>
void EmitInt32DivOrRem(LiftoffAssembler* assm, Register dst, Register lhs,
                       Register rhs, Label* trap_div_by_zero,
                       Label* trap_div_unrepresentable) {
  constexpr bool needs_unrepresentable_check =
      is_signed && div_or_rem == DivOrRem::kDiv;
  constexpr bool special_case_minus_1 =
      is_signed && div_or_rem == DivOrRem::kRem;
  DCHECK_EQ(needs_unrepresentable_check, trap_div_unrepresentable != nullptr);

  // For division, the lhs is always taken from {edx:eax}. Thus, make sure that
  // these registers are unused. If {rhs} is stored in one of them, move it to
  // another temporary register.
  // Do all this before any branch, such that the code is executed
  // unconditionally, as the cache state will also be modified unconditionally.
  assm->SpillRegisters(eax, edx);
  if (rhs == eax || rhs == edx) {
    LiftoffRegList unavailable = LiftoffRegList::ForRegs(eax, edx, lhs);
    Register tmp = assm->GetUnusedRegister(kGpReg, unavailable).gp();
    assm->mov(tmp, rhs);
    rhs = tmp;
  }

  // Check for division by zero.
  assm->test(rhs, rhs);
  assm->j(zero, trap_div_by_zero);

  Label done;
  if (needs_unrepresentable_check) {
    // Check for {kMinInt / -1}. This is unrepresentable.
    Label do_div;
    assm->cmp(rhs, -1);
    assm->j(not_equal, &do_div);
    assm->cmp(lhs, kMinInt);
    assm->j(equal, trap_div_unrepresentable);
    assm->bind(&do_div);
  } else if (special_case_minus_1) {
    // {lhs % -1} is always 0 (needs to be special cased because {kMinInt / -1}
    // cannot be computed).
    Label do_rem;
    assm->cmp(rhs, -1);
    assm->j(not_equal, &do_rem);
    assm->xor_(dst, dst);
    assm->jmp(&done);
    assm->bind(&do_rem);
  }

  // Now move {lhs} into {eax}, then zero-extend or sign-extend into {edx}, then
  // do the division.
  if (lhs != eax) assm->mov(eax, lhs);
  if (is_signed) {
    assm->cdq();
    assm->idiv(rhs);
  } else {
    assm->xor_(edx, edx);
    assm->div(rhs);
  }

  // Move back the result (in {eax} or {edx}) into the {dst} register.
  constexpr Register kResultReg = div_or_rem == DivOrRem::kDiv ? eax : edx;
  if (dst != kResultReg) assm->mov(dst, kResultReg);
  if (special_case_minus_1) assm->bind(&done);
}
}  // namespace liftoff

void LiftoffAssembler::emit_i32_divs(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero,
                                     Label* trap_div_unrepresentable) {
  liftoff::EmitInt32DivOrRem<true, liftoff::DivOrRem::kDiv>(
      this, dst, lhs, rhs, trap_div_by_zero, trap_div_unrepresentable);
}

void LiftoffAssembler::emit_i32_divu(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero) {
  liftoff::EmitInt32DivOrRem<false, liftoff::DivOrRem::kDiv>(
      this, dst, lhs, rhs, trap_div_by_zero, nullptr);
}

void LiftoffAssembler::emit_i32_rems(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero) {
  liftoff::EmitInt32DivOrRem<true, liftoff::DivOrRem::kRem>(
      this, dst, lhs, rhs, trap_div_by_zero, nullptr);
}

void LiftoffAssembler::emit_i32_remu(Register dst, Register lhs, Register rhs,
                                     Label* trap_div_by_zero) {
  liftoff::EmitInt32DivOrRem<false, liftoff::DivOrRem::kRem>(
      this, dst, lhs, rhs, trap_div_by_zero, nullptr);
}

void LiftoffAssembler::emit_i32_and(Register dst, Register lhs, Register rhs) {
  liftoff::EmitCommutativeBinOp<&Assembler::and_>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32_andi(Register dst, Register lhs, int32_t imm) {
  liftoff::EmitCommutativeBinOpImm<&Assembler::and_>(this, dst, lhs, imm);
}

void LiftoffAssembler::emit_i32_or(Register dst, Register lhs, Register rhs) {
  liftoff::EmitCommutativeBinOp<&Assembler::or_>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32_ori(Register dst, Register lhs, int32_t imm) {
  liftoff::EmitCommutativeBinOpImm<&Assembler::or_>(this, dst, lhs, imm);
}

void LiftoffAssembler::emit_i32_xor(Register dst, Register lhs, Register rhs) {
  liftoff::EmitCommutativeBinOp<&Assembler::xor_>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32_xori(Register dst, Register lhs, int32_t imm) {
  liftoff::EmitCommutativeBinOpImm<&Assembler::xor_>(this, dst, lhs, imm);
}

namespace liftoff {
inline void EmitShiftOperation(LiftoffAssembler* assm, Register dst,
                               Register src, Register amount,
                               void (Assembler::*emit_shift)(Register)) {
  LiftoffRegList pinned = LiftoffRegList::ForRegs(dst, src, amount);
  // If dst is ecx, compute into a tmp register first, then move to ecx.
  if (dst == ecx) {
    Register tmp = assm->GetUnusedRegister(kGpReg, pinned).gp();
    assm->mov(tmp, src);
    if (amount != ecx) assm->mov(ecx, amount);
    (assm->*emit_shift)(tmp);
    assm->mov(ecx, tmp);
    return;
  }

  // Move amount into ecx. If ecx is in use, move its content to a tmp register
  // first. If src is ecx, src is now the tmp register.
  Register tmp_reg = no_reg;
  if (amount != ecx) {
    if (assm->cache_state()->is_used(LiftoffRegister(ecx)) ||
        pinned.has(LiftoffRegister(ecx))) {
      tmp_reg = assm->GetUnusedRegister(kGpReg, pinned).gp();
      assm->mov(tmp_reg, ecx);
      if (src == ecx) src = tmp_reg;
    }
    assm->mov(ecx, amount);
  }

  // Do the actual shift.
  if (dst != src) assm->mov(dst, src);
  (assm->*emit_shift)(dst);

  // Restore ecx if needed.
  if (tmp_reg.is_valid()) assm->mov(ecx, tmp_reg);
}
}  // namespace liftoff

void LiftoffAssembler::emit_i32_shl(Register dst, Register src,
                                    Register amount) {
  liftoff::EmitShiftOperation(this, dst, src, amount, &Assembler::shl_cl);
}

void LiftoffAssembler::emit_i32_shli(Register dst, Register src,
                                     int32_t amount) {
  if (dst != src) mov(dst, src);
  shl(dst, amount & 31);
}

void LiftoffAssembler::emit_i32_sar(Register dst, Register src,
                                    Register amount) {
  liftoff::EmitShiftOperation(this, dst, src, amount, &Assembler::sar_cl);
}

void LiftoffAssembler::emit_i32_sari(Register dst, Register src,
                                     int32_t amount) {
  if (dst != src) mov(dst, src);
  sar(dst, amount & 31);
}

void LiftoffAssembler::emit_i32_shr(Register dst, Register src,
                                    Register amount) {
  liftoff::EmitShiftOperation(this, dst, src, amount, &Assembler::shr_cl);
}

void LiftoffAssembler::emit_i32_shri(Register dst, Register src,
                                     int32_t amount) {
  if (dst != src) mov(dst, src);
  shr(dst, amount & 31);
}

void LiftoffAssembler::emit_i32_clz(Register dst, Register src) {
  Lzcnt(dst, src);
}

void LiftoffAssembler::emit_i32_ctz(Register dst, Register src) {
  Tzcnt(dst, src);
}

bool LiftoffAssembler::emit_i32_popcnt(Register dst, Register src) {
  if (!CpuFeatures::IsSupported(POPCNT)) return false;
  CpuFeatureScope scope(this, POPCNT);
  popcnt(dst, src);
  return true;
}

namespace liftoff {
template <void (Assembler::*op)(Register, Register),
          void (Assembler::*op_with_carry)(Register, Register)>
inline void OpWithCarry(LiftoffAssembler* assm, LiftoffRegister dst,
                        LiftoffRegister lhs, LiftoffRegister rhs) {
  // First, compute the low half of the result, potentially into a temporary dst
  // register if {dst.low_gp()} equals {rhs.low_gp()} or any register we need to
  // keep alive for computing the upper half.
  LiftoffRegList keep_alive = LiftoffRegList::ForRegs(lhs.high_gp(), rhs);
  Register dst_low = keep_alive.has(dst.low_gp())
                         ? assm->GetUnusedRegister(kGpReg, keep_alive).gp()
                         : dst.low_gp();

  if (dst_low != lhs.low_gp()) assm->mov(dst_low, lhs.low_gp());
  (assm->*op)(dst_low, rhs.low_gp());

  // Now compute the upper half, while keeping alive the previous result.
  keep_alive = LiftoffRegList::ForRegs(dst_low, rhs.high_gp());
  Register dst_high = keep_alive.has(dst.high_gp())
                          ? assm->GetUnusedRegister(kGpReg, keep_alive).gp()
                          : dst.high_gp();

  if (dst_high != lhs.high_gp()) assm->mov(dst_high, lhs.high_gp());
  (assm->*op_with_carry)(dst_high, rhs.high_gp());

  // If necessary, move result into the right registers.
  LiftoffRegister tmp_result = LiftoffRegister::ForPair(dst_low, dst_high);
  if (tmp_result != dst) assm->Move(dst, tmp_result, kI64);
}

template <void (Assembler::*op)(Register, const Immediate&),
          void (Assembler::*op_with_carry)(Register, int32_t)>
inline void OpWithCarryI(LiftoffAssembler* assm, LiftoffRegister dst,
                         LiftoffRegister lhs, int64_t imm) {
  // The compiler allocated registers such that either {dst == lhs} or there is
  // no overlap between the two.
  DCHECK_NE(dst.low_gp(), lhs.high_gp());

  int32_t imm_low_word = static_cast<int32_t>(imm);
  int32_t imm_high_word = static_cast<int32_t>(imm >> 32);

  // First, compute the low half of the result.
  if (dst.low_gp() != lhs.low_gp()) assm->mov(dst.low_gp(), lhs.low_gp());
  (assm->*op)(dst.low_gp(), Immediate(imm_low_word));

  // Now compute the upper half.
  if (dst.high_gp() != lhs.high_gp()) assm->mov(dst.high_gp(), lhs.high_gp());
  (assm->*op_with_carry)(dst.high_gp(), imm_high_word);
}
}  // namespace liftoff

void LiftoffAssembler::emit_i64_add(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::OpWithCarry<&Assembler::add, &Assembler::adc>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_addi(LiftoffRegister dst, LiftoffRegister lhs,
                                     int64_t imm) {
  liftoff::OpWithCarryI<&Assembler::add, &Assembler::adc>(this, dst, lhs, imm);
}

void LiftoffAssembler::emit_i64_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::OpWithCarry<&Assembler::sub, &Assembler::sbb>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  // Idea:
  //        [           lhs_hi  |           lhs_lo  ] * [  rhs_hi  |  rhs_lo  ]
  //    =   [  lhs_hi * rhs_lo  |                   ]  (32 bit mul, shift 32)
  //      + [  lhs_lo * rhs_hi  |                   ]  (32 bit mul, shift 32)
  //      + [             lhs_lo * rhs_lo           ]  (32x32->64 mul, shift 0)

  // For simplicity, we move lhs and rhs into fixed registers.
  Register dst_hi = edx;
  Register dst_lo = eax;
  Register lhs_hi = ecx;
  Register lhs_lo = dst_lo;
  Register rhs_hi = dst_hi;
  Register rhs_lo = esi;

  // Spill all these registers if they are still holding other values.
  SpillRegisters(dst_hi, dst_lo, lhs_hi, rhs_lo);

  // Move lhs and rhs into the respective registers.
  ParallelRegisterMove({{LiftoffRegister::ForPair(lhs_lo, lhs_hi), lhs, kI64},
                        {LiftoffRegister::ForPair(rhs_lo, rhs_hi), rhs, kI64}});

  // First mul: lhs_hi' = lhs_hi * rhs_lo.
  imul(lhs_hi, rhs_lo);
  // Second mul: rhi_hi' = rhs_hi * lhs_lo.
  imul(rhs_hi, lhs_lo);
  // Add them: lhs_hi'' = lhs_hi' + rhs_hi' = lhs_hi * rhs_lo + rhs_hi * lhs_lo.
  add(lhs_hi, rhs_hi);
  // Third mul: edx:eax (dst_hi:dst_lo) = eax * esi (lhs_lo * rhs_lo).
  mul(rhs_lo);
  // Add lhs_hi'' to dst_hi.
  add(dst_hi, lhs_hi);

  // Finally, move back the temporary result to the actual dst register pair.
  LiftoffRegister dst_tmp = LiftoffRegister::ForPair(dst_lo, dst_hi);
  if (dst != dst_tmp) Move(dst, dst_tmp, kI64);
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

namespace liftoff {
inline bool PairContains(LiftoffRegister pair, Register reg) {
  return pair.low_gp() == reg || pair.high_gp() == reg;
}

inline LiftoffRegister ReplaceInPair(LiftoffRegister pair, Register old_reg,
                                     Register new_reg) {
  if (pair.low_gp() == old_reg) {
    return LiftoffRegister::ForPair(new_reg, pair.high_gp());
  }
  if (pair.high_gp() == old_reg) {
    return LiftoffRegister::ForPair(pair.low_gp(), new_reg);
  }
  return pair;
}

inline void Emit64BitShiftOperation(
    LiftoffAssembler* assm, LiftoffRegister dst, LiftoffRegister src,
    Register amount, void (TurboAssembler::*emit_shift)(Register, Register)) {
  // Temporary registers cannot overlap with {dst}.
  LiftoffRegList pinned = LiftoffRegList::ForRegs(dst);

  constexpr size_t kMaxRegMoves = 3;
  base::SmallVector<LiftoffAssembler::ParallelRegisterMoveTuple, kMaxRegMoves>
      reg_moves;

  // If {dst} contains {ecx}, replace it by an unused register, which is then
  // moved to {ecx} in the end.
  Register ecx_replace = no_reg;
  if (PairContains(dst, ecx)) {
    ecx_replace = assm->GetUnusedRegister(kGpReg, pinned).gp();
    dst = ReplaceInPair(dst, ecx, ecx_replace);
    // If {amount} needs to be moved to {ecx}, but {ecx} is in use (and not part
    // of {dst}, hence overwritten anyway), move {ecx} to a tmp register and
    // restore it at the end.
  } else if (amount != ecx &&
             (assm->cache_state()->is_used(LiftoffRegister(ecx)) ||
              pinned.has(LiftoffRegister(ecx)))) {
    ecx_replace = assm->GetUnusedRegister(kGpReg, pinned).gp();
    reg_moves.emplace_back(ecx_replace, ecx, kI32);
  }

  reg_moves.emplace_back(dst, src, kI64);
  reg_moves.emplace_back(ecx, amount, kI32);
  assm->ParallelRegisterMove(VectorOf(reg_moves));

  // Do the actual shift.
  (assm->*emit_shift)(dst.high_gp(), dst.low_gp());

  // Restore {ecx} if needed.
  if (ecx_replace != no_reg) assm->mov(ecx, ecx_replace);
}
}  // namespace liftoff

void LiftoffAssembler::emit_i64_shl(LiftoffRegister dst, LiftoffRegister src,
                                    Register amount) {
  liftoff::Emit64BitShiftOperation(this, dst, src, amount,
                                   &TurboAssembler::ShlPair_cl);
}

void LiftoffAssembler::emit_i64_shli(LiftoffRegister dst, LiftoffRegister src,
                                     int32_t amount) {
  amount &= 63;
  if (amount >= 32) {
    if (dst.high_gp() != src.low_gp()) mov(dst.high_gp(), src.low_gp());
    if (amount != 32) shl(dst.high_gp(), amount - 32);
    xor_(dst.low_gp(), dst.low_gp());
  } else {
    if (dst != src) Move(dst, src, kI64);
    ShlPair(dst.high_gp(), dst.low_gp(), amount);
  }
}

void LiftoffAssembler::emit_i64_sar(LiftoffRegister dst, LiftoffRegister src,
                                    Register amount) {
  liftoff::Emit64BitShiftOperation(this, dst, src, amount,
                                   &TurboAssembler::SarPair_cl);
}

void LiftoffAssembler::emit_i64_sari(LiftoffRegister dst, LiftoffRegister src,
                                     int32_t amount) {
  amount &= 63;
  if (amount >= 32) {
    if (dst.low_gp() != src.high_gp()) mov(dst.low_gp(), src.high_gp());
    if (dst.high_gp() != src.high_gp()) mov(dst.high_gp(), src.high_gp());
    if (amount != 32) sar(dst.low_gp(), amount - 32);
    sar(dst.high_gp(), 31);
  } else {
    if (dst != src) Move(dst, src, kI64);
    SarPair(dst.high_gp(), dst.low_gp(), amount);
  }
}
void LiftoffAssembler::emit_i64_shr(LiftoffRegister dst, LiftoffRegister src,
                                    Register amount) {
  liftoff::Emit64BitShiftOperation(this, dst, src, amount,
                                   &TurboAssembler::ShrPair_cl);
}

void LiftoffAssembler::emit_i64_shri(LiftoffRegister dst, LiftoffRegister src,
                                     int32_t amount) {
  amount &= 63;
  if (amount >= 32) {
    if (dst.low_gp() != src.high_gp()) mov(dst.low_gp(), src.high_gp());
    if (amount != 32) shr(dst.low_gp(), amount - 32);
    xor_(dst.high_gp(), dst.high_gp());
  } else {
    if (dst != src) Move(dst, src, kI64);
    ShrPair(dst.high_gp(), dst.low_gp(), amount);
  }
}

void LiftoffAssembler::emit_i64_clz(LiftoffRegister dst, LiftoffRegister src) {
  // return high == 0 ? 32 + CLZ32(low) : CLZ32(high);
  Label done;
  Register safe_dst = dst.low_gp();
  if (src.low_gp() == safe_dst) safe_dst = dst.high_gp();
  if (CpuFeatures::IsSupported(LZCNT)) {
    CpuFeatureScope scope(this, LZCNT);
    lzcnt(safe_dst, src.high_gp());  // Sets CF if high == 0.
    j(not_carry, &done, Label::kNear);
    lzcnt(safe_dst, src.low_gp());
    add(safe_dst, Immediate(32));  // 32 + CLZ32(low)
  } else {
    // CLZ32(x) =^ x == 0 ? 32 : 31 - BSR32(x)
    Label high_is_zero;
    bsr(safe_dst, src.high_gp());  // Sets ZF is high == 0.
    j(zero, &high_is_zero, Label::kNear);
    xor_(safe_dst, Immediate(31));  // for x in [0..31], 31^x == 31-x.
    jmp(&done, Label::kNear);

    bind(&high_is_zero);
    Label low_not_zero;
    bsr(safe_dst, src.low_gp());
    j(not_zero, &low_not_zero, Label::kNear);
    mov(safe_dst, Immediate(64 ^ 63));  // 64, after the xor below.
    bind(&low_not_zero);
    xor_(safe_dst, 63);  // for x in [0..31], 63^x == 63-x.
  }

  bind(&done);
  if (safe_dst != dst.low_gp()) mov(dst.low_gp(), safe_dst);
  xor_(dst.high_gp(), dst.high_gp());  // High word of result is always 0.
}

void LiftoffAssembler::emit_i64_ctz(LiftoffRegister dst, LiftoffRegister src) {
  // return low == 0 ? 32 + CTZ32(high) : CTZ32(low);
  Label done;
  Register safe_dst = dst.low_gp();
  if (src.high_gp() == safe_dst) safe_dst = dst.high_gp();
  if (CpuFeatures::IsSupported(BMI1)) {
    CpuFeatureScope scope(this, BMI1);
    tzcnt(safe_dst, src.low_gp());  // Sets CF if low == 0.
    j(not_carry, &done, Label::kNear);
    tzcnt(safe_dst, src.high_gp());
    add(safe_dst, Immediate(32));  // 32 + CTZ32(high)
  } else {
    // CTZ32(x) =^ x == 0 ? 32 : BSF32(x)
    bsf(safe_dst, src.low_gp());  // Sets ZF is low == 0.
    j(not_zero, &done, Label::kNear);

    Label high_not_zero;
    bsf(safe_dst, src.high_gp());
    j(not_zero, &high_not_zero, Label::kNear);
    mov(safe_dst, 64);  // low == 0 and high == 0
    jmp(&done);
    bind(&high_not_zero);
    add(safe_dst, Immediate(32));  // 32 + CTZ32(high)
  }

  bind(&done);
  if (safe_dst != dst.low_gp()) mov(dst.low_gp(), safe_dst);
  xor_(dst.high_gp(), dst.high_gp());  // High word of result is always 0.
}

bool LiftoffAssembler::emit_i64_popcnt(LiftoffRegister dst,
                                       LiftoffRegister src) {
  if (!CpuFeatures::IsSupported(POPCNT)) return false;
  CpuFeatureScope scope(this, POPCNT);
  // Produce partial popcnts in the two dst registers.
  Register src1 = src.high_gp() == dst.low_gp() ? src.high_gp() : src.low_gp();
  Register src2 = src.high_gp() == dst.low_gp() ? src.low_gp() : src.high_gp();
  popcnt(dst.low_gp(), src1);
  popcnt(dst.high_gp(), src2);
  // Add the two into the lower dst reg, clear the higher dst reg.
  add(dst.low_gp(), dst.high_gp());
  xor_(dst.high_gp(), dst.high_gp());
  return true;
}

void LiftoffAssembler::emit_u32_to_intptr(Register dst, Register src) {
  // This is a nop on ia32.
}

void LiftoffAssembler::emit_f32_add(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vaddss(dst, lhs, rhs);
  } else if (dst == rhs) {
    addss(dst, lhs);
  } else {
    if (dst != lhs) movss(dst, lhs);
    addss(dst, rhs);
  }
}

void LiftoffAssembler::emit_f32_sub(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vsubss(dst, lhs, rhs);
  } else if (dst == rhs) {
    movss(liftoff::kScratchDoubleReg, rhs);
    movss(dst, lhs);
    subss(dst, liftoff::kScratchDoubleReg);
  } else {
    if (dst != lhs) movss(dst, lhs);
    subss(dst, rhs);
  }
}

void LiftoffAssembler::emit_f32_mul(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vmulss(dst, lhs, rhs);
  } else if (dst == rhs) {
    mulss(dst, lhs);
  } else {
    if (dst != lhs) movss(dst, lhs);
    mulss(dst, rhs);
  }
}

void LiftoffAssembler::emit_f32_div(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vdivss(dst, lhs, rhs);
  } else if (dst == rhs) {
    movss(liftoff::kScratchDoubleReg, rhs);
    movss(dst, lhs);
    divss(dst, liftoff::kScratchDoubleReg);
  } else {
    if (dst != lhs) movss(dst, lhs);
    divss(dst, rhs);
  }
}

namespace liftoff {
enum class MinOrMax : uint8_t { kMin, kMax };
template <typename type>
inline void EmitFloatMinOrMax(LiftoffAssembler* assm, DoubleRegister dst,
                              DoubleRegister lhs, DoubleRegister rhs,
                              MinOrMax min_or_max) {
  Label is_nan;
  Label lhs_below_rhs;
  Label lhs_above_rhs;
  Label done;

  // We need one tmp register to extract the sign bit. Get it right at the
  // beginning, such that the spilling code is not accidentially jumped over.
  Register tmp = assm->GetUnusedRegister(kGpReg, {}).gp();

#define dop(name, ...)            \
  do {                            \
    if (sizeof(type) == 4) {      \
      assm->name##s(__VA_ARGS__); \
    } else {                      \
      assm->name##d(__VA_ARGS__); \
    }                             \
  } while (false)

  // Check the easy cases first: nan (e.g. unordered), smaller and greater.
  // NaN has to be checked first, because PF=1 implies CF=1.
  dop(ucomis, lhs, rhs);
  assm->j(parity_even, &is_nan, Label::kNear);   // PF=1
  assm->j(below, &lhs_below_rhs, Label::kNear);  // CF=1
  assm->j(above, &lhs_above_rhs, Label::kNear);  // CF=0 && ZF=0

  // If we get here, then either
  // a) {lhs == rhs},
  // b) {lhs == -0.0} and {rhs == 0.0}, or
  // c) {lhs == 0.0} and {rhs == -0.0}.
  // For a), it does not matter whether we return {lhs} or {rhs}. Check the sign
  // bit of {rhs} to differentiate b) and c).
  dop(movmskp, tmp, rhs);
  assm->test(tmp, Immediate(1));
  assm->j(zero, &lhs_below_rhs, Label::kNear);
  assm->jmp(&lhs_above_rhs, Label::kNear);

  assm->bind(&is_nan);
  // Create a NaN output.
  dop(xorp, dst, dst);
  dop(divs, dst, dst);
  assm->jmp(&done, Label::kNear);

  assm->bind(&lhs_below_rhs);
  DoubleRegister lhs_below_rhs_src = min_or_max == MinOrMax::kMin ? lhs : rhs;
  if (dst != lhs_below_rhs_src) dop(movs, dst, lhs_below_rhs_src);
  assm->jmp(&done, Label::kNear);

  assm->bind(&lhs_above_rhs);
  DoubleRegister lhs_above_rhs_src = min_or_max == MinOrMax::kMin ? rhs : lhs;
  if (dst != lhs_above_rhs_src) dop(movs, dst, lhs_above_rhs_src);

  assm->bind(&done);
}
}  // namespace liftoff

void LiftoffAssembler::emit_f32_min(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax<float>(this, dst, lhs, rhs,
                                    liftoff::MinOrMax::kMin);
}

void LiftoffAssembler::emit_f32_max(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax<float>(this, dst, lhs, rhs,
                                    liftoff::MinOrMax::kMax);
}

void LiftoffAssembler::emit_f32_copysign(DoubleRegister dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  static constexpr int kF32SignBit = 1 << 31;
  LiftoffRegList pinned;
  Register scratch = pinned.set(GetUnusedRegister(kGpReg, pinned)).gp();
  Register scratch2 = GetUnusedRegister(kGpReg, pinned).gp();
  Movd(scratch, lhs);                      // move {lhs} into {scratch}.
  and_(scratch, Immediate(~kF32SignBit));  // clear sign bit in {scratch}.
  Movd(scratch2, rhs);                     // move {rhs} into {scratch2}.
  and_(scratch2, Immediate(kF32SignBit));  // isolate sign bit in {scratch2}.
  or_(scratch, scratch2);                  // combine {scratch2} into {scratch}.
  Movd(dst, scratch);                      // move result into {dst}.
}

void LiftoffAssembler::emit_f32_abs(DoubleRegister dst, DoubleRegister src) {
  static constexpr uint32_t kSignBit = uint32_t{1} << 31;
  if (dst == src) {
    TurboAssembler::Move(liftoff::kScratchDoubleReg, kSignBit - 1);
    Andps(dst, liftoff::kScratchDoubleReg);
  } else {
    TurboAssembler::Move(dst, kSignBit - 1);
    Andps(dst, src);
  }
}

void LiftoffAssembler::emit_f32_neg(DoubleRegister dst, DoubleRegister src) {
  static constexpr uint32_t kSignBit = uint32_t{1} << 31;
  if (dst == src) {
    TurboAssembler::Move(liftoff::kScratchDoubleReg, kSignBit);
    Xorps(dst, liftoff::kScratchDoubleReg);
  } else {
    TurboAssembler::Move(dst, kSignBit);
    Xorps(dst, src);
  }
}

bool LiftoffAssembler::emit_f32_ceil(DoubleRegister dst, DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundss(dst, src, kRoundUp);
  return true;
}

bool LiftoffAssembler::emit_f32_floor(DoubleRegister dst, DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundss(dst, src, kRoundDown);
  return true;
}

bool LiftoffAssembler::emit_f32_trunc(DoubleRegister dst, DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundss(dst, src, kRoundToZero);
  return true;
}

bool LiftoffAssembler::emit_f32_nearest_int(DoubleRegister dst,
                                            DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundss(dst, src, kRoundToNearest);
  return true;
}

void LiftoffAssembler::emit_f32_sqrt(DoubleRegister dst, DoubleRegister src) {
  Sqrtss(dst, src);
}

void LiftoffAssembler::emit_f64_add(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vaddsd(dst, lhs, rhs);
  } else if (dst == rhs) {
    addsd(dst, lhs);
  } else {
    if (dst != lhs) movsd(dst, lhs);
    addsd(dst, rhs);
  }
}

void LiftoffAssembler::emit_f64_sub(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vsubsd(dst, lhs, rhs);
  } else if (dst == rhs) {
    movsd(liftoff::kScratchDoubleReg, rhs);
    movsd(dst, lhs);
    subsd(dst, liftoff::kScratchDoubleReg);
  } else {
    if (dst != lhs) movsd(dst, lhs);
    subsd(dst, rhs);
  }
}

void LiftoffAssembler::emit_f64_mul(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vmulsd(dst, lhs, rhs);
  } else if (dst == rhs) {
    mulsd(dst, lhs);
  } else {
    if (dst != lhs) movsd(dst, lhs);
    mulsd(dst, rhs);
  }
}

void LiftoffAssembler::emit_f64_div(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vdivsd(dst, lhs, rhs);
  } else if (dst == rhs) {
    movsd(liftoff::kScratchDoubleReg, rhs);
    movsd(dst, lhs);
    divsd(dst, liftoff::kScratchDoubleReg);
  } else {
    if (dst != lhs) movsd(dst, lhs);
    divsd(dst, rhs);
  }
}

void LiftoffAssembler::emit_f64_min(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax<double>(this, dst, lhs, rhs,
                                     liftoff::MinOrMax::kMin);
}

void LiftoffAssembler::emit_f64_copysign(DoubleRegister dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  static constexpr int kF32SignBit = 1 << 31;
  // On ia32, we cannot hold the whole f64 value in a gp register, so we just
  // operate on the upper half (UH).
  LiftoffRegList pinned;
  Register scratch = pinned.set(GetUnusedRegister(kGpReg, pinned)).gp();
  Register scratch2 = GetUnusedRegister(kGpReg, pinned).gp();

  Pextrd(scratch, lhs, 1);                 // move UH of {lhs} into {scratch}.
  and_(scratch, Immediate(~kF32SignBit));  // clear sign bit in {scratch}.
  Pextrd(scratch2, rhs, 1);                // move UH of {rhs} into {scratch2}.
  and_(scratch2, Immediate(kF32SignBit));  // isolate sign bit in {scratch2}.
  or_(scratch, scratch2);                  // combine {scratch2} into {scratch}.
  movsd(dst, lhs);                         // move {lhs} into {dst}.
  Pinsrd(dst, scratch, 1);                 // insert {scratch} into UH of {dst}.
}

void LiftoffAssembler::emit_f64_max(DoubleRegister dst, DoubleRegister lhs,
                                    DoubleRegister rhs) {
  liftoff::EmitFloatMinOrMax<double>(this, dst, lhs, rhs,
                                     liftoff::MinOrMax::kMax);
}

void LiftoffAssembler::emit_f64_abs(DoubleRegister dst, DoubleRegister src) {
  static constexpr uint64_t kSignBit = uint64_t{1} << 63;
  if (dst == src) {
    TurboAssembler::Move(liftoff::kScratchDoubleReg, kSignBit - 1);
    Andpd(dst, liftoff::kScratchDoubleReg);
  } else {
    TurboAssembler::Move(dst, kSignBit - 1);
    Andpd(dst, src);
  }
}

void LiftoffAssembler::emit_f64_neg(DoubleRegister dst, DoubleRegister src) {
  static constexpr uint64_t kSignBit = uint64_t{1} << 63;
  if (dst == src) {
    TurboAssembler::Move(liftoff::kScratchDoubleReg, kSignBit);
    Xorpd(dst, liftoff::kScratchDoubleReg);
  } else {
    TurboAssembler::Move(dst, kSignBit);
    Xorpd(dst, src);
  }
}

bool LiftoffAssembler::emit_f64_ceil(DoubleRegister dst, DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundsd(dst, src, kRoundUp);
  return true;
}

bool LiftoffAssembler::emit_f64_floor(DoubleRegister dst, DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundsd(dst, src, kRoundDown);
  return true;
}

bool LiftoffAssembler::emit_f64_trunc(DoubleRegister dst, DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundsd(dst, src, kRoundToZero);
  return true;
}

bool LiftoffAssembler::emit_f64_nearest_int(DoubleRegister dst,
                                            DoubleRegister src) {
  RETURN_FALSE_IF_MISSING_CPU_FEATURE(SSE4_1);
  roundsd(dst, src, kRoundToNearest);
  return true;
}

void LiftoffAssembler::emit_f64_sqrt(DoubleRegister dst, DoubleRegister src) {
  Sqrtsd(dst, src);
}

namespace liftoff {
#define __ assm->
// Used for float to int conversions. If the value in {converted_back} equals
// {src} afterwards, the conversion succeeded.
template <typename dst_type, typename src_type>
inline void ConvertFloatToIntAndBack(LiftoffAssembler* assm, Register dst,
                                     DoubleRegister src,
                                     DoubleRegister converted_back,
                                     LiftoffRegList pinned) {
  if (std::is_same<double, src_type>::value) {  // f64
    if (std::is_signed<dst_type>::value) {      // f64 -> i32
      __ cvttsd2si(dst, src);
      __ Cvtsi2sd(converted_back, dst);
    } else {  // f64 -> u32
      __ Cvttsd2ui(dst, src, liftoff::kScratchDoubleReg);
      __ Cvtui2sd(converted_back, dst,
                  __ GetUnusedRegister(kGpReg, pinned).gp());
    }
  } else {                                  // f32
    if (std::is_signed<dst_type>::value) {  // f32 -> i32
      __ cvttss2si(dst, src);
      __ Cvtsi2ss(converted_back, dst);
    } else {  // f32 -> u32
      __ Cvttss2ui(dst, src, liftoff::kScratchDoubleReg);
      __ Cvtui2ss(converted_back, dst,
                  __ GetUnusedRegister(kGpReg, pinned).gp());
    }
  }
}

template <typename dst_type, typename src_type>
inline bool EmitTruncateFloatToInt(LiftoffAssembler* assm, Register dst,
                                   DoubleRegister src, Label* trap) {
  if (!CpuFeatures::IsSupported(SSE4_1)) {
    __ bailout(kMissingCPUFeature, "no SSE4.1");
    return true;
  }
  CpuFeatureScope feature(assm, SSE4_1);

  LiftoffRegList pinned = LiftoffRegList::ForRegs(src, dst);
  DoubleRegister rounded =
      pinned.set(__ GetUnusedRegister(kFpReg, pinned)).fp();
  DoubleRegister converted_back =
      pinned.set(__ GetUnusedRegister(kFpReg, pinned)).fp();

  if (std::is_same<double, src_type>::value) {  // f64
    __ roundsd(rounded, src, kRoundToZero);
  } else {  // f32
    __ roundss(rounded, src, kRoundToZero);
  }
  ConvertFloatToIntAndBack<dst_type, src_type>(assm, dst, rounded,
                                               converted_back, pinned);
  if (std::is_same<double, src_type>::value) {  // f64
    __ ucomisd(converted_back, rounded);
  } else {  // f32
    __ ucomiss(converted_back, rounded);
  }

  // Jump to trap if PF is 0 (one of the operands was NaN) or they are not
  // equal.
  __ j(parity_even, trap);
  __ j(not_equal, trap);
  return true;
}

template <typename dst_type, typename src_type>
inline bool EmitSatTruncateFloatToInt(LiftoffAssembler* assm, Register dst,
                                      DoubleRegister src) {
  if (!CpuFeatures::IsSupported(SSE4_1)) {
    __ bailout(kMissingCPUFeature, "no SSE4.1");
    return true;
  }
  CpuFeatureScope feature(assm, SSE4_1);

  Label done;
  Label not_nan;
  Label src_positive;

  LiftoffRegList pinned = LiftoffRegList::ForRegs(src, dst);
  DoubleRegister rounded =
      pinned.set(__ GetUnusedRegister(kFpReg, pinned)).fp();
  DoubleRegister converted_back =
      pinned.set(__ GetUnusedRegister(kFpReg, pinned)).fp();
  DoubleRegister zero_reg =
      pinned.set(__ GetUnusedRegister(kFpReg, pinned)).fp();

  if (std::is_same<double, src_type>::value) {  // f64
    __ roundsd(rounded, src, kRoundToZero);
  } else {  // f32
    __ roundss(rounded, src, kRoundToZero);
  }

  ConvertFloatToIntAndBack<dst_type, src_type>(assm, dst, rounded,
                                               converted_back, pinned);
  if (std::is_same<double, src_type>::value) {  // f64
    __ ucomisd(converted_back, rounded);
  } else {  // f32
    __ ucomiss(converted_back, rounded);
  }

  // Return 0 if PF is 0 (one of the operands was NaN)
  __ j(parity_odd, &not_nan);
  __ xor_(dst, dst);
  __ jmp(&done);

  __ bind(&not_nan);
  // If rounding is as expected, return result
  __ j(equal, &done);

  __ Xorpd(zero_reg, zero_reg);

  // if out-of-bounds, check if src is positive
  if (std::is_same<double, src_type>::value) {  // f64
    __ ucomisd(src, zero_reg);
  } else {  // f32
    __ ucomiss(src, zero_reg);
  }
  __ j(above, &src_positive);
  __ mov(dst, Immediate(std::numeric_limits<dst_type>::min()));
  __ jmp(&done);

  __ bind(&src_positive);

  __ mov(dst, Immediate(std::numeric_limits<dst_type>::max()));

  __ bind(&done);
  return true;
}
#undef __
}  // namespace liftoff

bool LiftoffAssembler::emit_type_conversion(WasmOpcode opcode,
                                            LiftoffRegister dst,
                                            LiftoffRegister src, Label* trap) {
  switch (opcode) {
    case kExprI32ConvertI64:
      if (dst.gp() != src.low_gp()) mov(dst.gp(), src.low_gp());
      return true;
    case kExprI32SConvertF32:
      return liftoff::EmitTruncateFloatToInt<int32_t, float>(this, dst.gp(),
                                                             src.fp(), trap);
    case kExprI32UConvertF32:
      return liftoff::EmitTruncateFloatToInt<uint32_t, float>(this, dst.gp(),
                                                              src.fp(), trap);
    case kExprI32SConvertF64:
      return liftoff::EmitTruncateFloatToInt<int32_t, double>(this, dst.gp(),
                                                              src.fp(), trap);
    case kExprI32UConvertF64:
      return liftoff::EmitTruncateFloatToInt<uint32_t, double>(this, dst.gp(),
                                                               src.fp(), trap);
    case kExprI32SConvertSatF32:
      return liftoff::EmitSatTruncateFloatToInt<int32_t, float>(this, dst.gp(),
                                                                src.fp());
    case kExprI32UConvertSatF32:
      return liftoff::EmitSatTruncateFloatToInt<uint32_t, float>(this, dst.gp(),
                                                                 src.fp());
    case kExprI32SConvertSatF64:
      return liftoff::EmitSatTruncateFloatToInt<int32_t, double>(this, dst.gp(),
                                                                 src.fp());
    case kExprI32UConvertSatF64:
      return liftoff::EmitSatTruncateFloatToInt<uint32_t, double>(
          this, dst.gp(), src.fp());
    case kExprI32ReinterpretF32:
      Movd(dst.gp(), src.fp());
      return true;
    case kExprI64SConvertI32:
      if (dst.low_gp() != src.gp()) mov(dst.low_gp(), src.gp());
      if (dst.high_gp() != src.gp()) mov(dst.high_gp(), src.gp());
      sar(dst.high_gp(), 31);
      return true;
    case kExprI64UConvertI32:
      if (dst.low_gp() != src.gp()) mov(dst.low_gp(), src.gp());
      xor_(dst.high_gp(), dst.high_gp());
      return true;
    case kExprI64ReinterpretF64:
      // Push src to the stack.
      AllocateStackSpace(8);
      movsd(Operand(esp, 0), src.fp());
      // Pop to dst.
      pop(dst.low_gp());
      pop(dst.high_gp());
      return true;
    case kExprF32SConvertI32:
      cvtsi2ss(dst.fp(), src.gp());
      return true;
    case kExprF32UConvertI32: {
      LiftoffRegList pinned = LiftoffRegList::ForRegs(dst, src);
      Register scratch = GetUnusedRegister(kGpReg, pinned).gp();
      Cvtui2ss(dst.fp(), src.gp(), scratch);
      return true;
    }
    case kExprF32ConvertF64:
      cvtsd2ss(dst.fp(), src.fp());
      return true;
    case kExprF32ReinterpretI32:
      Movd(dst.fp(), src.gp());
      return true;
    case kExprF64SConvertI32:
      Cvtsi2sd(dst.fp(), src.gp());
      return true;
    case kExprF64UConvertI32: {
      LiftoffRegList pinned = LiftoffRegList::ForRegs(dst, src);
      Register scratch = GetUnusedRegister(kGpReg, pinned).gp();
      Cvtui2sd(dst.fp(), src.gp(), scratch);
      return true;
    }
    case kExprF64ConvertF32:
      cvtss2sd(dst.fp(), src.fp());
      return true;
    case kExprF64ReinterpretI64:
      // Push src to the stack.
      push(src.high_gp());
      push(src.low_gp());
      // Pop to dst.
      movsd(dst.fp(), Operand(esp, 0));
      add(esp, Immediate(8));
      return true;
    default:
      return false;
  }
}

void LiftoffAssembler::emit_i32_signextend_i8(Register dst, Register src) {
  Register byte_reg = liftoff::GetTmpByteRegister(this, src);
  if (byte_reg != src) mov(byte_reg, src);
  movsx_b(dst, byte_reg);
}

void LiftoffAssembler::emit_i32_signextend_i16(Register dst, Register src) {
  movsx_w(dst, src);
}

void LiftoffAssembler::emit_i64_signextend_i8(LiftoffRegister dst,
                                              LiftoffRegister src) {
  Register byte_reg = liftoff::GetTmpByteRegister(this, src.low_gp());
  if (byte_reg != src.low_gp()) mov(byte_reg, src.low_gp());
  movsx_b(dst.low_gp(), byte_reg);
  liftoff::SignExtendI32ToI64(this, dst);
}

void LiftoffAssembler::emit_i64_signextend_i16(LiftoffRegister dst,
                                               LiftoffRegister src) {
  movsx_w(dst.low_gp(), src.low_gp());
  liftoff::SignExtendI32ToI64(this, dst);
}

void LiftoffAssembler::emit_i64_signextend_i32(LiftoffRegister dst,
                                               LiftoffRegister src) {
  if (dst.low_gp() != src.low_gp()) mov(dst.low_gp(), src.low_gp());
  liftoff::SignExtendI32ToI64(this, dst);
}

void LiftoffAssembler::emit_jump(Label* label) { jmp(label); }

void LiftoffAssembler::emit_jump(Register target) { jmp(target); }

void LiftoffAssembler::emit_cond_jump(LiftoffCondition liftoff_cond,
                                      Label* label, ValueKind kind,
                                      Register lhs, Register rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  if (rhs != no_reg) {
    switch (kind) {
      case kRef:
      case kOptRef:
      case kRtt:
      case kRttWithDepth:
        DCHECK(liftoff_cond == kEqual || liftoff_cond == kUnequal);
        V8_FALLTHROUGH;
      case kI32:
        cmp(lhs, rhs);
        break;
      default:
        UNREACHABLE();
    }
  } else {
    DCHECK_EQ(kind, kI32);
    test(lhs, lhs);
  }

  j(cond, label);
}

void LiftoffAssembler::emit_i32_cond_jumpi(LiftoffCondition liftoff_cond,
                                           Label* label, Register lhs,
                                           int imm) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  cmp(lhs, Immediate(imm));
  j(cond, label);
}

namespace liftoff {

// Setcc into dst register, given a scratch byte register (might be the same as
// dst). Never spills.
inline void setcc_32_no_spill(LiftoffAssembler* assm, Condition cond,
                              Register dst, Register tmp_byte_reg) {
  assm->setcc(cond, tmp_byte_reg);
  assm->movzx_b(dst, tmp_byte_reg);
}

// Setcc into dst register (no contraints). Might spill.
inline void setcc_32(LiftoffAssembler* assm, Condition cond, Register dst) {
  Register tmp_byte_reg = GetTmpByteRegister(assm, dst);
  setcc_32_no_spill(assm, cond, dst, tmp_byte_reg);
}

}  // namespace liftoff

void LiftoffAssembler::emit_i32_eqz(Register dst, Register src) {
  test(src, src);
  liftoff::setcc_32(this, equal, dst);
}

void LiftoffAssembler::emit_i32_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, Register lhs,
                                         Register rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  cmp(lhs, rhs);
  liftoff::setcc_32(this, cond, dst);
}

void LiftoffAssembler::emit_i64_eqz(Register dst, LiftoffRegister src) {
  // Compute the OR of both registers in the src pair, using dst as scratch
  // register. Then check whether the result is equal to zero.
  if (src.low_gp() == dst) {
    or_(dst, src.high_gp());
  } else {
    if (src.high_gp() != dst) mov(dst, src.high_gp());
    or_(dst, src.low_gp());
  }
  liftoff::setcc_32(this, equal, dst);
}

namespace liftoff {
inline LiftoffCondition cond_make_unsigned(LiftoffCondition cond) {
  switch (cond) {
    case kSignedLessThan:
      return kUnsignedLessThan;
    case kSignedLessEqual:
      return kUnsignedLessEqual;
    case kSignedGreaterThan:
      return kUnsignedGreaterThan;
    case kSignedGreaterEqual:
      return kUnsignedGreaterEqual;
    default:
      return cond;
  }
}
}  // namespace liftoff

void LiftoffAssembler::emit_i64_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, LiftoffRegister lhs,
                                         LiftoffRegister rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  Condition unsigned_cond =
      liftoff::ToCondition(liftoff::cond_make_unsigned(liftoff_cond));

  // Get the tmp byte register out here, such that we don't conditionally spill
  // (this cannot be reflected in the cache state).
  Register tmp_byte_reg = liftoff::GetTmpByteRegister(this, dst);

  // For signed i64 comparisons, we still need to use unsigned comparison for
  // the low word (the only bit carrying signedness information is the MSB in
  // the high word).
  Label setcc;
  Label cont;
  // Compare high word first. If it differs, use if for the setcc. If it's
  // equal, compare the low word and use that for setcc.
  cmp(lhs.high_gp(), rhs.high_gp());
  j(not_equal, &setcc, Label::kNear);
  cmp(lhs.low_gp(), rhs.low_gp());
  if (unsigned_cond != cond) {
    // If the condition predicate for the low differs from that for the high
    // word, emit a separete setcc sequence for the low word.
    liftoff::setcc_32_no_spill(this, unsigned_cond, dst, tmp_byte_reg);
    jmp(&cont);
  }
  bind(&setcc);
  liftoff::setcc_32_no_spill(this, cond, dst, tmp_byte_reg);
  bind(&cont);
}

namespace liftoff {
template <void (Assembler::*cmp_op)(DoubleRegister, DoubleRegister)>
void EmitFloatSetCond(LiftoffAssembler* assm, Condition cond, Register dst,
                      DoubleRegister lhs, DoubleRegister rhs) {
  Label cont;
  Label not_nan;

  // Get the tmp byte register out here, such that we don't conditionally spill
  // (this cannot be reflected in the cache state).
  Register tmp_byte_reg = GetTmpByteRegister(assm, dst);

  (assm->*cmp_op)(lhs, rhs);
  // If PF is one, one of the operands was Nan. This needs special handling.
  assm->j(parity_odd, &not_nan, Label::kNear);
  // Return 1 for f32.ne, 0 for all other cases.
  if (cond == not_equal) {
    assm->mov(dst, Immediate(1));
  } else {
    assm->xor_(dst, dst);
  }
  assm->jmp(&cont, Label::kNear);
  assm->bind(&not_nan);

  setcc_32_no_spill(assm, cond, dst, tmp_byte_reg);
  assm->bind(&cont);
}
}  // namespace liftoff

void LiftoffAssembler::emit_f32_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  liftoff::EmitFloatSetCond<&Assembler::ucomiss>(this, cond, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64_set_cond(LiftoffCondition liftoff_cond,
                                         Register dst, DoubleRegister lhs,
                                         DoubleRegister rhs) {
  Condition cond = liftoff::ToCondition(liftoff_cond);
  liftoff::EmitFloatSetCond<&Assembler::ucomisd>(this, cond, dst, lhs, rhs);
}

bool LiftoffAssembler::emit_select(LiftoffRegister dst, Register condition,
                                   LiftoffRegister true_value,
                                   LiftoffRegister false_value,
                                   ValueKind kind) {
  return false;
}

void LiftoffAssembler::emit_smi_check(Register obj, Label* target,
                                      SmiCheckMode mode) {
  test_b(obj, Immediate(kSmiTagMask));
  Condition condition = mode == kJumpOnSmi ? zero : not_zero;
  j(condition, target);
}

namespace liftoff {
template <void (Assembler::*avx_op)(XMMRegister, XMMRegister, XMMRegister),
          void (Assembler::*sse_op)(XMMRegister, XMMRegister)>
void EmitSimdCommutativeBinOp(
    LiftoffAssembler* assm, LiftoffRegister dst, LiftoffRegister lhs,
    LiftoffRegister rhs, base::Optional<CpuFeature> feature = base::nullopt) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(assm, AVX);
    (assm->*avx_op)(dst.fp(), lhs.fp(), rhs.fp());
    return;
  }

  base::Optional<CpuFeatureScope> sse_scope;
  if (feature.has_value()) sse_scope.emplace(assm, *feature);

  if (dst.fp() == rhs.fp()) {
    (assm->*sse_op)(dst.fp(), lhs.fp());
  } else {
    if (dst.fp() != lhs.fp()) (assm->movaps)(dst.fp(), lhs.fp());
    (assm->*sse_op)(dst.fp(), rhs.fp());
  }
}

template <void (Assembler::*avx_op)(XMMRegister, XMMRegister, XMMRegister),
          void (Assembler::*sse_op)(XMMRegister, XMMRegister)>
void EmitSimdNonCommutativeBinOp(
    LiftoffAssembler* assm, LiftoffRegister dst, LiftoffRegister lhs,
    LiftoffRegister rhs, base::Optional<CpuFeature> feature = base::nullopt) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(assm, AVX);
    (assm->*avx_op)(dst.fp(), lhs.fp(), rhs.fp());
    return;
  }

  base::Optional<CpuFeatureScope> sse_scope;
  if (feature.has_value()) sse_scope.emplace(assm, *feature);

  if (dst.fp() == rhs.fp()) {
    assm->movaps(kScratchDoubleReg, rhs.fp());
    assm->movaps(dst.fp(), lhs.fp());
    (assm->*sse_op)(dst.fp(), kScratchDoubleReg);
  } else {
    if (dst.fp() != lhs.fp()) assm->movaps(dst.fp(), lhs.fp());
    (assm->*sse_op)(dst.fp(), rhs.fp());
  }
}

template <void (Assembler::*avx_op)(XMMRegister, XMMRegister, XMMRegister),
          void (Assembler::*sse_op)(XMMRegister, XMMRegister), uint8_t width>
void EmitSimdShiftOp(LiftoffAssembler* assm, LiftoffRegister dst,
                     LiftoffRegister operand, LiftoffRegister count) {
  static constexpr RegClass tmp_rc = reg_class_for(kI32);
  LiftoffRegister tmp =
      assm->GetUnusedRegister(tmp_rc, LiftoffRegList::ForRegs(count));
  constexpr int mask = (1 << width) - 1;

  assm->mov(tmp.gp(), count.gp());
  assm->and_(tmp.gp(), Immediate(mask));
  assm->Movd(kScratchDoubleReg, tmp.gp());
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(assm, AVX);
    (assm->*avx_op)(dst.fp(), operand.fp(), kScratchDoubleReg);
  } else {
    if (dst.fp() != operand.fp()) assm->movaps(dst.fp(), operand.fp());
    (assm->*sse_op)(dst.fp(), kScratchDoubleReg);
  }
}

template <void (Assembler::*avx_op)(XMMRegister, XMMRegister, byte),
          void (Assembler::*sse_op)(XMMRegister, byte), uint8_t width>
void EmitSimdShiftOpImm(LiftoffAssembler* assm, LiftoffRegister dst,
                        LiftoffRegister operand, int32_t count) {
  constexpr int mask = (1 << width) - 1;
  byte shift = static_cast<byte>(count & mask);
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(assm, AVX);
    (assm->*avx_op)(dst.fp(), operand.fp(), shift);
  } else {
    if (dst.fp() != operand.fp()) assm->movaps(dst.fp(), operand.fp());
    (assm->*sse_op)(dst.fp(), shift);
  }
}

enum class ShiftSignedness { kSigned, kUnsigned };

template <bool is_signed>
void EmitI8x16Shr(LiftoffAssembler* assm, LiftoffRegister dst,
                  LiftoffRegister lhs, LiftoffRegister rhs) {
  // Same algorithm is used for both signed and unsigned shifts, the only
  // difference is the actual shift and pack in the end. This is the same
  // algorithm as used in code-generator-ia32.cc
  Register tmp =
      assm->GetUnusedRegister(kGpReg, LiftoffRegList::ForRegs(rhs)).gp();
  XMMRegister tmp_simd =
      assm->GetUnusedRegister(kFpReg, LiftoffRegList::ForRegs(dst, lhs)).fp();

  // Unpack the bytes into words, do logical shifts, and repack.
  assm->Punpckhbw(liftoff::kScratchDoubleReg, lhs.fp());
  assm->Punpcklbw(dst.fp(), lhs.fp());
  assm->mov(tmp, rhs.gp());
  // Take shift value modulo 8.
  assm->and_(tmp, 7);
  assm->add(tmp, Immediate(8));
  assm->Movd(tmp_simd, tmp);
  if (is_signed) {
    assm->Psraw(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg,
                tmp_simd);
    assm->Psraw(dst.fp(), dst.fp(), tmp_simd);
    assm->Packsswb(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    assm->Psrlw(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg,
                tmp_simd);
    assm->Psrlw(dst.fp(), dst.fp(), tmp_simd);
    assm->Packuswb(dst.fp(), liftoff::kScratchDoubleReg);
  }
}

inline void EmitAnyTrue(LiftoffAssembler* assm, LiftoffRegister dst,
                        LiftoffRegister src) {
  Register tmp =
      assm->GetUnusedRegister(kGpReg, LiftoffRegList::ForRegs(dst)).gp();
  assm->xor_(tmp, tmp);
  assm->mov(dst.gp(), Immediate(1));
  assm->Ptest(src.fp(), src.fp());
  assm->cmov(zero, dst.gp(), tmp);
}

template <void (TurboAssembler::*pcmp)(XMMRegister, XMMRegister)>
inline void EmitAllTrue(LiftoffAssembler* assm, LiftoffRegister dst,
                        LiftoffRegister src,
                        base::Optional<CpuFeature> feature = base::nullopt) {
  base::Optional<CpuFeatureScope> sse_scope;
  if (feature.has_value()) sse_scope.emplace(assm, *feature);

  Register tmp =
      assm->GetUnusedRegister(kGpReg, LiftoffRegList::ForRegs(dst)).gp();
  XMMRegister tmp_simd = liftoff::kScratchDoubleReg;
  assm->mov(tmp, Immediate(1));
  assm->xor_(dst.gp(), dst.gp());
  assm->Pxor(tmp_simd, tmp_simd);
  (assm->*pcmp)(tmp_simd, src.fp());
  assm->Ptest(tmp_simd, tmp_simd);
  assm->cmov(zero, dst.gp(), tmp);
}

}  // namespace liftoff

void LiftoffAssembler::LoadTransform(LiftoffRegister dst, Register src_addr,
                                     Register offset_reg, uintptr_t offset_imm,
                                     LoadType type,
                                     LoadTransformationKind transform,
                                     uint32_t* protected_load_pc) {
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  Operand src_op{src_addr, offset_reg, times_1,
                 static_cast<int32_t>(offset_imm)};
  *protected_load_pc = pc_offset();

  MachineType memtype = type.mem_type();
  if (transform == LoadTransformationKind::kExtend) {
    if (memtype == MachineType::Int8()) {
      Pmovsxbw(dst.fp(), src_op);
    } else if (memtype == MachineType::Uint8()) {
      Pmovzxbw(dst.fp(), src_op);
    } else if (memtype == MachineType::Int16()) {
      Pmovsxwd(dst.fp(), src_op);
    } else if (memtype == MachineType::Uint16()) {
      Pmovzxwd(dst.fp(), src_op);
    } else if (memtype == MachineType::Int32()) {
      Pmovsxdq(dst.fp(), src_op);
    } else if (memtype == MachineType::Uint32()) {
      Pmovzxdq(dst.fp(), src_op);
    }
  } else if (transform == LoadTransformationKind::kZeroExtend) {
    if (memtype == MachineType::Int32()) {
      movss(dst.fp(), src_op);
    } else {
      DCHECK_EQ(MachineType::Int64(), memtype);
      movsd(dst.fp(), src_op);
    }
  } else {
    DCHECK_EQ(LoadTransformationKind::kSplat, transform);
    if (memtype == MachineType::Int8()) {
      Pinsrb(dst.fp(), src_op, 0);
      Pxor(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
      Pshufb(dst.fp(), liftoff::kScratchDoubleReg);
    } else if (memtype == MachineType::Int16()) {
      Pinsrw(dst.fp(), src_op, 0);
      Pshuflw(dst.fp(), dst.fp(), uint8_t{0});
      Punpcklqdq(dst.fp(), dst.fp());
    } else if (memtype == MachineType::Int32()) {
      Vbroadcastss(dst.fp(), src_op);
    } else if (memtype == MachineType::Int64()) {
      Movddup(dst.fp(), src_op);
    }
  }
}

void LiftoffAssembler::LoadLane(LiftoffRegister dst, LiftoffRegister src,
                                Register addr, Register offset_reg,
                                uintptr_t offset_imm, LoadType type,
                                uint8_t laneidx, uint32_t* protected_load_pc) {
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  Operand src_op{addr, offset_reg, times_1, static_cast<int32_t>(offset_imm)};
  *protected_load_pc = pc_offset();

  MachineType mem_type = type.mem_type();
  if (mem_type == MachineType::Int8()) {
    Pinsrb(dst.fp(), src.fp(), src_op, laneidx);
  } else if (mem_type == MachineType::Int16()) {
    Pinsrw(dst.fp(), src.fp(), src_op, laneidx);
  } else if (mem_type == MachineType::Int32()) {
    Pinsrd(dst.fp(), src.fp(), src_op, laneidx);
  } else {
    DCHECK_EQ(MachineType::Int64(), mem_type);
    if (laneidx == 0) {
      Movlps(dst.fp(), src.fp(), src_op);
    } else {
      DCHECK_EQ(1, laneidx);
      Movhps(dst.fp(), src.fp(), src_op);
    }
  }
}

void LiftoffAssembler::StoreLane(Register dst, Register offset,
                                 uintptr_t offset_imm, LiftoffRegister src,
                                 StoreType type, uint8_t lane,
                                 uint32_t* protected_store_pc) {
  DCHECK_LE(offset_imm, std::numeric_limits<int32_t>::max());
  Operand dst_op = Operand(dst, offset, times_1, offset_imm);
  if (protected_store_pc) *protected_store_pc = pc_offset();

  MachineRepresentation rep = type.mem_rep();
  if (rep == MachineRepresentation::kWord8) {
    Pextrb(dst_op, src.fp(), lane);
  } else if (rep == MachineRepresentation::kWord16) {
    Pextrw(dst_op, src.fp(), lane);
  } else if (rep == MachineRepresentation::kWord32) {
    S128Store32Lane(dst_op, src.fp(), lane);
  } else {
    DCHECK_EQ(MachineRepresentation::kWord64, rep);
    if (lane == 0) {
      Movlps(dst_op, src.fp());
    } else {
      DCHECK_EQ(1, lane);
      Movhps(dst_op, src.fp());
    }
  }
}

void LiftoffAssembler::emit_i8x16_shuffle(LiftoffRegister dst,
                                          LiftoffRegister lhs,
                                          LiftoffRegister rhs,
                                          const uint8_t shuffle[16],
                                          bool is_swizzle) {
  LiftoffRegister tmp = GetUnusedRegister(kGpReg, {});
  // Prepare 16 byte aligned buffer for shuffle control mask.
  mov(tmp.gp(), esp);
  and_(esp, -16);

  if (is_swizzle) {
    uint32_t imms[4];
    // Shuffles that use just 1 operand are called swizzles, rhs can be ignored.
    wasm::SimdShuffle::Pack16Lanes(imms, shuffle);
    for (int i = 3; i >= 0; i--) {
      push_imm32(imms[i]);
    }
    Pshufb(dst.fp(), lhs.fp(), Operand(esp, 0));
    mov(esp, tmp.gp());
    return;
  }

  movups(liftoff::kScratchDoubleReg, lhs.fp());
  for (int i = 3; i >= 0; i--) {
    uint32_t mask = 0;
    for (int j = 3; j >= 0; j--) {
      uint8_t lane = shuffle[i * 4 + j];
      mask <<= 8;
      mask |= lane < kSimd128Size ? lane : 0x80;
    }
    push(Immediate(mask));
  }
  Pshufb(liftoff::kScratchDoubleReg, lhs.fp(), Operand(esp, 0));

  for (int i = 3; i >= 0; i--) {
    uint32_t mask = 0;
    for (int j = 3; j >= 0; j--) {
      uint8_t lane = shuffle[i * 4 + j];
      mask <<= 8;
      mask |= lane >= kSimd128Size ? (lane & 0x0F) : 0x80;
    }
    push(Immediate(mask));
  }
  Pshufb(dst.fp(), rhs.fp(), Operand(esp, 0));
  Por(dst.fp(), liftoff::kScratchDoubleReg);
  mov(esp, tmp.gp());
}

void LiftoffAssembler::emit_i8x16_swizzle(LiftoffRegister dst,
                                          LiftoffRegister lhs,
                                          LiftoffRegister rhs) {
  Register scratch = GetUnusedRegister(RegClass::kGpReg, {}).gp();
  I8x16Swizzle(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg,
               scratch);
}

void LiftoffAssembler::emit_i8x16_popcnt(LiftoffRegister dst,
                                         LiftoffRegister src) {
  Register scratch = GetUnusedRegister(RegClass::kGpReg, {}).gp();
  XMMRegister tmp =
      GetUnusedRegister(RegClass::kFpReg, LiftoffRegList::ForRegs(dst, src))
          .fp();
  I8x16Popcnt(dst.fp(), src.fp(), liftoff::kScratchDoubleReg, tmp, scratch);
}

void LiftoffAssembler::emit_i8x16_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  Movd(dst.fp(), src.gp());
  Pxor(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pshufb(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i16x8_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  Movd(dst.fp(), src.gp());
  Pshuflw(dst.fp(), dst.fp(), uint8_t{0});
  Pshufd(dst.fp(), dst.fp(), uint8_t{0});
}

void LiftoffAssembler::emit_i32x4_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  Movd(dst.fp(), src.gp());
  Pshufd(dst.fp(), dst.fp(), uint8_t{0});
}

void LiftoffAssembler::emit_i64x2_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  Pinsrd(dst.fp(), src.low_gp(), 0);
  Pinsrd(dst.fp(), src.high_gp(), 1);
  Pshufd(dst.fp(), dst.fp(), uint8_t{0x44});
}

void LiftoffAssembler::emit_f32x4_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  F32x4Splat(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_f64x2_splat(LiftoffRegister dst,
                                        LiftoffRegister src) {
  Movddup(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i8x16_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqb, &Assembler::pcmpeqb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqb, &Assembler::pcmpeqb>(
      this, dst, lhs, rhs);
  Pcmpeqb(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i8x16_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpcmpgtb,
                                       &Assembler::pcmpgtb>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i8x16_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxub, &Assembler::pmaxub>(
      this, dst, lhs, rhs);
  Pcmpeqb(dst.fp(), ref);
  Pcmpeqb(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i8x16_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminsb, &Assembler::pminsb>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqb(dst.fp(), ref);
}

void LiftoffAssembler::emit_i8x16_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminub, &Assembler::pminub>(
      this, dst, lhs, rhs);
  Pcmpeqb(dst.fp(), ref);
}

void LiftoffAssembler::emit_i16x8_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqw, &Assembler::pcmpeqw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqw, &Assembler::pcmpeqw>(
      this, dst, lhs, rhs);
  Pcmpeqw(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i16x8_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpcmpgtw,
                                       &Assembler::pcmpgtw>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i16x8_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxuw, &Assembler::pmaxuw>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqw(dst.fp(), ref);
  Pcmpeqw(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i16x8_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminsw, &Assembler::pminsw>(
      this, dst, lhs, rhs);
  Pcmpeqw(dst.fp(), ref);
}

void LiftoffAssembler::emit_i16x8_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminuw, &Assembler::pminuw>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqw(dst.fp(), ref);
}

void LiftoffAssembler::emit_i32x4_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqd, &Assembler::pcmpeqd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqd, &Assembler::pcmpeqd>(
      this, dst, lhs, rhs);
  Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i32x4_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpcmpgtd,
                                       &Assembler::pcmpgtd>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i32x4_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxud, &Assembler::pmaxud>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqd(dst.fp(), ref);
  Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i32x4_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminsd, &Assembler::pminsd>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqd(dst.fp(), ref);
}

void LiftoffAssembler::emit_i32x4_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  DoubleRegister ref = rhs.fp();
  if (dst == rhs) {
    Movaps(liftoff::kScratchDoubleReg, rhs.fp());
    ref = liftoff::kScratchDoubleReg;
  }
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminud, &Assembler::pminud>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqd(dst.fp(), ref);
}

void LiftoffAssembler::emit_i64x2_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqq, &Assembler::pcmpeqq>(
      this, dst, lhs, rhs, SSE4_1);
}

void LiftoffAssembler::emit_i64x2_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpcmpeqq, &Assembler::pcmpeqq>(
      this, dst, lhs, rhs, SSE4_1);
  Pcmpeqq(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i64x2_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  // Different register alias requirements depending on CpuFeatures supported:
  if (CpuFeatures::IsSupported(AVX)) {
    // 1. AVX, no requirements.
    I64x2GtS(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
  } else if (CpuFeatures::IsSupported(SSE4_2)) {
    // 2. SSE4_2, dst == lhs.
    if (dst != lhs) {
      movaps(dst.fp(), lhs.fp());
    }
    I64x2GtS(dst.fp(), dst.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
  } else {
    // 3. Else, dst != lhs && dst != rhs (lhs == rhs is ok).
    if (dst == lhs || dst == rhs) {
      LiftoffRegister tmp = GetUnusedRegister(
          RegClass::kFpReg, LiftoffRegList::ForRegs(lhs, rhs));
      I64x2GtS(tmp.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
      movaps(dst.fp(), tmp.fp());
    } else {
      I64x2GtS(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
    }
  }
}

void LiftoffAssembler::emit_i64x2_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  // Different register alias requirements depending on CpuFeatures supported:
  if (CpuFeatures::IsSupported(AVX)) {
    // 1. AVX, no requirements.
    I64x2GeS(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
  } else if (CpuFeatures::IsSupported(SSE4_2)) {
    // 2. SSE4_2, dst != lhs.
    if (dst == lhs) {
      LiftoffRegister tmp = GetUnusedRegister(RegClass::kFpReg, {rhs},
                                              LiftoffRegList::ForRegs(lhs));
      // macro-assembler uses kScratchDoubleReg, so don't use it.
      I64x2GeS(tmp.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
      movaps(dst.fp(), tmp.fp());
    } else {
      I64x2GeS(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
    }
  } else {
    // 3. Else, dst != lhs && dst != rhs (lhs == rhs is ok).
    if (dst == lhs || dst == rhs) {
      LiftoffRegister tmp = GetUnusedRegister(
          RegClass::kFpReg, LiftoffRegList::ForRegs(lhs, rhs));
      I64x2GeS(tmp.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
      movaps(dst.fp(), tmp.fp());
    } else {
      I64x2GeS(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
    }
  }
}

void LiftoffAssembler::emit_f32x4_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vcmpeqps, &Assembler::cmpeqps>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f32x4_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vcmpneqps,
                                    &Assembler::cmpneqps>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f32x4_lt(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vcmpltps,
                                       &Assembler::cmpltps>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_f32x4_le(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vcmpleps,
                                       &Assembler::cmpleps>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_f64x2_eq(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vcmpeqpd, &Assembler::cmpeqpd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64x2_ne(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vcmpneqpd,
                                    &Assembler::cmpneqpd>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64x2_lt(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vcmpltpd,
                                       &Assembler::cmpltpd>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_f64x2_le(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vcmplepd,
                                       &Assembler::cmplepd>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_s128_const(LiftoffRegister dst,
                                       const uint8_t imms[16]) {
  uint64_t vals[2];
  base::Memcpy(vals, imms, sizeof(vals));
  TurboAssembler::Move(dst.fp(), vals[0]);

  uint64_t high = vals[1];
  Register tmp = GetUnusedRegister(RegClass::kGpReg, {}).gp();
  TurboAssembler::Move(tmp, Immediate(high & 0xffff'ffff));
  Pinsrd(dst.fp(), tmp, 2);

  TurboAssembler::Move(tmp, Immediate(high >> 32));
  Pinsrd(dst.fp(), tmp, 3);
}

void LiftoffAssembler::emit_s128_not(LiftoffRegister dst, LiftoffRegister src) {
  S128Not(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_s128_and(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpand, &Assembler::pand>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_s128_or(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpor, &Assembler::por>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_s128_xor(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpxor, &Assembler::pxor>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_s128_select(LiftoffRegister dst,
                                        LiftoffRegister src1,
                                        LiftoffRegister src2,
                                        LiftoffRegister mask) {
  // Ensure that we don't overwrite any inputs with the movaps below.
  DCHECK_NE(dst, src1);
  DCHECK_NE(dst, src2);
  if (!CpuFeatures::IsSupported(AVX) && dst != mask) {
    movaps(dst.fp(), mask.fp());
    S128Select(dst.fp(), dst.fp(), src1.fp(), src2.fp(),
               liftoff::kScratchDoubleReg);
  } else {
    S128Select(dst.fp(), mask.fp(), src1.fp(), src2.fp(),
               liftoff::kScratchDoubleReg);
  }
}

void LiftoffAssembler::emit_i8x16_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Psignb(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pxor(dst.fp(), dst.fp());
    Psubb(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_v128_anytrue(LiftoffRegister dst,
                                         LiftoffRegister src) {
  liftoff::EmitAnyTrue(this, dst, src);
}

void LiftoffAssembler::emit_i8x16_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  liftoff::EmitAllTrue<&TurboAssembler::Pcmpeqb>(this, dst, src);
}

void LiftoffAssembler::emit_i8x16_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  Pmovmskb(dst.gp(), src.fp());
}

void LiftoffAssembler::emit_i8x16_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  static constexpr RegClass tmp_rc = reg_class_for(kI32);
  static constexpr RegClass tmp_simd_rc = reg_class_for(kS128);
  LiftoffRegister tmp = GetUnusedRegister(tmp_rc, LiftoffRegList::ForRegs(rhs));
  LiftoffRegister tmp_simd =
      GetUnusedRegister(tmp_simd_rc, LiftoffRegList::ForRegs(dst, lhs));
  // Mask off the unwanted bits before word-shifting.
  Pcmpeqw(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  mov(tmp.gp(), rhs.gp());
  and_(tmp.gp(), Immediate(7));
  add(tmp.gp(), Immediate(8));
  Movd(tmp_simd.fp(), tmp.gp());
  Psrlw(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, tmp_simd.fp());
  Packuswb(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);

  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpand(dst.fp(), lhs.fp(), liftoff::kScratchDoubleReg);
  } else {
    if (dst.fp() != lhs.fp()) movaps(dst.fp(), lhs.fp());
    andps(dst.fp(), liftoff::kScratchDoubleReg);
  }
  sub(tmp.gp(), Immediate(8));
  Movd(tmp_simd.fp(), tmp.gp());
  Psllw(dst.fp(), dst.fp(), tmp_simd.fp());
}

void LiftoffAssembler::emit_i8x16_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  static constexpr RegClass tmp_rc = reg_class_for(kI32);
  LiftoffRegister tmp = GetUnusedRegister(tmp_rc, {});
  byte shift = static_cast<byte>(rhs & 0x7);
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpsllw(dst.fp(), lhs.fp(), shift);
  } else {
    if (dst.fp() != lhs.fp()) movaps(dst.fp(), lhs.fp());
    psllw(dst.fp(), shift);
  }

  uint8_t bmask = static_cast<uint8_t>(0xff << shift);
  uint32_t mask = bmask << 24 | bmask << 16 | bmask << 8 | bmask;
  mov(tmp.gp(), mask);
  Movd(liftoff::kScratchDoubleReg, tmp.gp());
  Pshufd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, uint8_t{0});
  Pand(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i8x16_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitI8x16Shr</*is_signed=*/true>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  Punpckhbw(liftoff::kScratchDoubleReg, lhs.fp());
  Punpcklbw(dst.fp(), lhs.fp());
  uint8_t shift = (rhs & 7) + 8;
  Psraw(liftoff::kScratchDoubleReg, shift);
  Psraw(dst.fp(), shift);
  Packsswb(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i8x16_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitI8x16Shr</*is_signed=*/false>(this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  Register tmp = GetUnusedRegister(kGpReg, {}).gp();
  // Perform 16-bit shift, then mask away high bits.
  uint8_t shift = rhs & 7;
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsrlw, &Assembler::psrlw, 3>(
      this, dst, lhs, rhs);

  uint8_t bmask = 0xff >> shift;
  uint32_t mask = bmask << 24 | bmask << 16 | bmask << 8 | bmask;
  mov(tmp, mask);
  Movd(liftoff::kScratchDoubleReg, tmp);
  Pshufd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, uint8_t{0});
  Pand(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i8x16_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddb, &Assembler::paddb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_add_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddsb, &Assembler::paddsb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_add_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddusb, &Assembler::paddusb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubb, &Assembler::psubb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_sub_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubsb, &Assembler::psubsb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_sub_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubusb,
                                       &Assembler::psubusb>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i8x16_min_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminsb, &Assembler::pminsb>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i8x16_min_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminub, &Assembler::pminub>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_max_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxsb, &Assembler::pmaxsb>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i8x16_max_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxub, &Assembler::pmaxub>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Psignw(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pxor(dst.fp(), dst.fp());
    Psubw(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_i16x8_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  liftoff::EmitAllTrue<&TurboAssembler::Pcmpeqw>(this, dst, src);
}

void LiftoffAssembler::emit_i16x8_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  XMMRegister tmp = liftoff::kScratchDoubleReg;
  Packsswb(tmp, src.fp());
  Pmovmskb(dst.gp(), tmp);
  shr(dst.gp(), 8);
}

void LiftoffAssembler::emit_i16x8_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsllw, &Assembler::psllw, 4>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsllw, &Assembler::psllw, 4>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsraw, &Assembler::psraw, 4>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsraw, &Assembler::psraw, 4>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsrlw, &Assembler::psrlw, 4>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsrlw, &Assembler::psrlw, 4>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddw, &Assembler::paddw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_add_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddsw, &Assembler::paddsw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_add_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddusw, &Assembler::paddusw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubw, &Assembler::psubw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_sub_sat_s(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubsw, &Assembler::psubsw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_sub_sat_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubusw,
                                       &Assembler::psubusw>(this, dst, lhs,
                                                            rhs);
}

void LiftoffAssembler::emit_i16x8_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmullw, &Assembler::pmullw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_min_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminsw, &Assembler::pminsw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_min_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminuw, &Assembler::pminuw>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i16x8_max_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxsw, &Assembler::pmaxsw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_max_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxuw, &Assembler::pmaxuw>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i16x8_extadd_pairwise_i8x16_s(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  I16x8ExtAddPairwiseI8x16S(dst.fp(), src.fp(), liftoff::kScratchDoubleReg,
                            GetUnusedRegister(kGpReg, {}).gp());
}

void LiftoffAssembler::emit_i16x8_extadd_pairwise_i8x16_u(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  I16x8ExtAddPairwiseI8x16U(dst.fp(), src.fp(),
                            GetUnusedRegister(kGpReg, {}).gp());
}

void LiftoffAssembler::emit_i16x8_extmul_low_i8x16_s(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  I16x8ExtMulLow(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg,
                 /*is_signed=*/true);
}

void LiftoffAssembler::emit_i16x8_extmul_low_i8x16_u(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  I16x8ExtMulLow(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg,
                 /*is_signed=*/false);
}

void LiftoffAssembler::emit_i16x8_extmul_high_i8x16_s(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  I16x8ExtMulHighS(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i16x8_extmul_high_i8x16_u(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  I16x8ExtMulHighU(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i16x8_q15mulr_sat_s(LiftoffRegister dst,
                                                LiftoffRegister src1,
                                                LiftoffRegister src2) {
  I16x8Q15MulRSatS(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i32x4_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Psignd(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pxor(dst.fp(), dst.fp());
    Psubd(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_i32x4_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  liftoff::EmitAllTrue<&TurboAssembler::Pcmpeqd>(this, dst, src);
}

void LiftoffAssembler::emit_i32x4_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  Movmskps(dst.gp(), src.fp());
}

void LiftoffAssembler::emit_i32x4_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpslld, &Assembler::pslld, 5>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpslld, &Assembler::pslld, 5>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsrad, &Assembler::psrad, 5>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsrad, &Assembler::psrad, 5>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsrld, &Assembler::psrld, 5>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsrld, &Assembler::psrld, 5>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddd, &Assembler::paddd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubd, &Assembler::psubd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmulld, &Assembler::pmulld>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i32x4_min_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminsd, &Assembler::pminsd>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i32x4_min_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpminud, &Assembler::pminud>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i32x4_max_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxsd, &Assembler::pmaxsd>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i32x4_max_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaxud, &Assembler::pmaxud>(
      this, dst, lhs, rhs, base::Optional<CpuFeature>(SSE4_1));
}

void LiftoffAssembler::emit_i32x4_dot_i16x8_s(LiftoffRegister dst,
                                              LiftoffRegister lhs,
                                              LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmaddwd, &Assembler::pmaddwd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i32x4_extadd_pairwise_i16x8_s(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  I32x4ExtAddPairwiseI16x8S(dst.fp(), src.fp(),
                            GetUnusedRegister(kGpReg, {}).gp());
}

void LiftoffAssembler::emit_i32x4_extadd_pairwise_i16x8_u(LiftoffRegister dst,
                                                          LiftoffRegister src) {
  I32x4ExtAddPairwiseI16x8U(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

namespace liftoff {
// Helper function to check for register aliasing, AVX support, and moves
// registers around before calling the actual macro-assembler function.
inline void I32x4ExtMulHelper(LiftoffAssembler* assm, XMMRegister dst,
                              XMMRegister src1, XMMRegister src2, bool low,
                              bool is_signed) {
  // I32x4ExtMul requires dst == src1 if AVX is not supported.
  if (CpuFeatures::IsSupported(AVX) || dst == src1) {
    assm->I32x4ExtMul(dst, src1, src2, liftoff::kScratchDoubleReg, low,
                      is_signed);
  } else if (dst != src2) {
    // dst != src1 && dst != src2
    assm->movaps(dst, src1);
    assm->I32x4ExtMul(dst, dst, src2, liftoff::kScratchDoubleReg, low,
                      is_signed);
  } else {
    // dst == src2
    // Extended multiplication is commutative,
    assm->movaps(dst, src2);
    assm->I32x4ExtMul(dst, dst, src1, liftoff::kScratchDoubleReg, low,
                      is_signed);
  }
}
}  // namespace liftoff

void LiftoffAssembler::emit_i32x4_extmul_low_i16x8_s(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  liftoff::I32x4ExtMulHelper(this, dst.fp(), src1.fp(), src2.fp(), /*low=*/true,
                             /*is_signed=*/true);
}

void LiftoffAssembler::emit_i32x4_extmul_low_i16x8_u(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  liftoff::I32x4ExtMulHelper(this, dst.fp(), src1.fp(), src2.fp(), /*low=*/true,
                             /*is_signed=*/false);
}

void LiftoffAssembler::emit_i32x4_extmul_high_i16x8_s(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  liftoff::I32x4ExtMulHelper(this, dst.fp(), src1.fp(), src2.fp(),
                             /*low=*/false,
                             /*is_signed=*/true);
}

void LiftoffAssembler::emit_i32x4_extmul_high_i16x8_u(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  liftoff::I32x4ExtMulHelper(this, dst.fp(), src1.fp(), src2.fp(),
                             /*low=*/false,
                             /*is_signed=*/false);
}

void LiftoffAssembler::emit_i64x2_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  I64x2Neg(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i64x2_alltrue(LiftoffRegister dst,
                                          LiftoffRegister src) {
  liftoff::EmitAllTrue<&TurboAssembler::Pcmpeqq>(this, dst, src, SSE4_1);
}

void LiftoffAssembler::emit_i64x2_shl(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsllq, &Assembler::psllq, 6>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_shli(LiftoffRegister dst, LiftoffRegister lhs,
                                       int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsllq, &Assembler::psllq, 6>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_shr_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  XMMRegister shift = liftoff::kScratchDoubleReg;
  XMMRegister tmp =
      GetUnusedRegister(RegClass::kFpReg, LiftoffRegList::ForRegs(dst, lhs))
          .fp();

  // Take shift value modulo 64.
  and_(rhs.gp(), Immediate(63));
  Movd(shift, rhs.gp());

  // Set up a mask [0x80000000,0,0x80000000,0].
  Pcmpeqb(tmp, tmp);
  Psllq(tmp, tmp, byte{63});

  Psrlq(tmp, tmp, shift);
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpsrlq(dst.fp(), lhs.fp(), shift);
  } else {
    if (dst != lhs) {
      movaps(dst.fp(), lhs.fp());
    }
    psrlq(dst.fp(), shift);
  }
  Pxor(dst.fp(), tmp);
  Psubq(dst.fp(), tmp);
}

void LiftoffAssembler::emit_i64x2_shri_s(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  XMMRegister tmp = liftoff::kScratchDoubleReg;
  byte shift = rhs & 63;

  // Set up a mask [0x80000000,0,0x80000000,0].
  Pcmpeqb(tmp, tmp);
  Psllq(tmp, tmp, byte{63});

  Psrlq(tmp, tmp, shift);
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsrlq, &Assembler::psrlq, 6>(
      this, dst, lhs, rhs);
  Pxor(dst.fp(), tmp);
  Psubq(dst.fp(), tmp);
}

void LiftoffAssembler::emit_i64x2_shr_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs) {
  liftoff::EmitSimdShiftOp<&Assembler::vpsrlq, &Assembler::psrlq, 6>(this, dst,
                                                                     lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_shri_u(LiftoffRegister dst,
                                         LiftoffRegister lhs, int32_t rhs) {
  liftoff::EmitSimdShiftOpImm<&Assembler::vpsrlq, &Assembler::psrlq, 6>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpaddq, &Assembler::paddq>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpsubq, &Assembler::psubq>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64x2_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  static constexpr RegClass tmp_rc = reg_class_for(kS128);
  LiftoffRegister tmp1 =
      GetUnusedRegister(tmp_rc, LiftoffRegList::ForRegs(dst, lhs, rhs));
  LiftoffRegister tmp2 =
      GetUnusedRegister(tmp_rc, LiftoffRegList::ForRegs(dst, lhs, rhs, tmp1));
  Movaps(tmp1.fp(), lhs.fp());
  Movaps(tmp2.fp(), rhs.fp());
  // Multiply high dword of each qword of left with right.
  Psrlq(tmp1.fp(), byte{32});
  Pmuludq(tmp1.fp(), tmp1.fp(), rhs.fp());
  // Multiply high dword of each qword of right with left.
  Psrlq(tmp2.fp(), byte{32});
  Pmuludq(tmp2.fp(), tmp2.fp(), lhs.fp());
  Paddq(tmp2.fp(), tmp2.fp(), tmp1.fp());
  Psllq(tmp2.fp(), tmp2.fp(), byte{32});
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpmuludq, &Assembler::pmuludq>(
      this, dst, lhs, rhs);
  Paddq(dst.fp(), dst.fp(), tmp2.fp());
}

void LiftoffAssembler::emit_i64x2_extmul_low_i32x4_s(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  I64x2ExtMul(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg,
              /*low=*/true, /*is_signed=*/true);
}

void LiftoffAssembler::emit_i64x2_extmul_low_i32x4_u(LiftoffRegister dst,
                                                     LiftoffRegister src1,
                                                     LiftoffRegister src2) {
  I64x2ExtMul(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg,
              /*low=*/true, /*is_signed=*/false);
}

void LiftoffAssembler::emit_i64x2_extmul_high_i32x4_s(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  I64x2ExtMul(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg,
              /*low=*/false, /*is_signed=*/true);
}

void LiftoffAssembler::emit_i64x2_extmul_high_i32x4_u(LiftoffRegister dst,
                                                      LiftoffRegister src1,
                                                      LiftoffRegister src2) {
  I64x2ExtMul(dst.fp(), src1.fp(), src2.fp(), liftoff::kScratchDoubleReg,
              /*low=*/false, /*is_signed=*/false);
}

void LiftoffAssembler::emit_i64x2_bitmask(LiftoffRegister dst,
                                          LiftoffRegister src) {
  Movmskpd(dst.gp(), src.fp());
}

void LiftoffAssembler::emit_i64x2_sconvert_i32x4_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  Pmovsxdq(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i64x2_sconvert_i32x4_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  I64x2SConvertI32x4High(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i64x2_uconvert_i32x4_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  Pmovzxdq(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i64x2_uconvert_i32x4_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  I64x2UConvertI32x4High(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_f32x4_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Psrld(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, byte{1});
    Andps(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pcmpeqd(dst.fp(), dst.fp());
    Psrld(dst.fp(), dst.fp(), byte{1});
    Andps(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_f32x4_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Pslld(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, byte{31});
    Xorps(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pcmpeqd(dst.fp(), dst.fp());
    Pslld(dst.fp(), dst.fp(), byte{31});
    Xorps(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_f32x4_sqrt(LiftoffRegister dst,
                                       LiftoffRegister src) {
  Sqrtps(dst.fp(), src.fp());
}

bool LiftoffAssembler::emit_f32x4_ceil(LiftoffRegister dst,
                                       LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundps(dst.fp(), src.fp(), kRoundUp);
  return true;
}

bool LiftoffAssembler::emit_f32x4_floor(LiftoffRegister dst,
                                        LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundps(dst.fp(), src.fp(), kRoundDown);
  return true;
}

bool LiftoffAssembler::emit_f32x4_trunc(LiftoffRegister dst,
                                        LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundps(dst.fp(), src.fp(), kRoundToZero);
  return true;
}

bool LiftoffAssembler::emit_f32x4_nearest_int(LiftoffRegister dst,
                                              LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundps(dst.fp(), src.fp(), kRoundToNearest);
  return true;
}

void LiftoffAssembler::emit_f32x4_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vaddps, &Assembler::addps>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f32x4_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vsubps, &Assembler::subps>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f32x4_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vmulps, &Assembler::mulps>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f32x4_div(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vdivps, &Assembler::divps>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f32x4_min(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  // The minps instruction doesn't propagate NaNs and +0's in its first
  // operand. Perform minps in both orders, merge the results, and adjust.
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vminps(liftoff::kScratchDoubleReg, lhs.fp(), rhs.fp());
    vminps(dst.fp(), rhs.fp(), lhs.fp());
  } else if (dst.fp() == lhs.fp() || dst.fp() == rhs.fp()) {
    XMMRegister src = dst.fp() == lhs.fp() ? rhs.fp() : lhs.fp();
    movaps(liftoff::kScratchDoubleReg, src);
    minps(liftoff::kScratchDoubleReg, dst.fp());
    minps(dst.fp(), src);
  } else {
    movaps(liftoff::kScratchDoubleReg, lhs.fp());
    minps(liftoff::kScratchDoubleReg, rhs.fp());
    movaps(dst.fp(), rhs.fp());
    minps(dst.fp(), lhs.fp());
  }
  // propagate -0's and NaNs, which may be non-canonical.
  Orps(liftoff::kScratchDoubleReg, dst.fp());
  // Canonicalize NaNs by quieting and clearing the payload.
  Cmpunordps(dst.fp(), dst.fp(), liftoff::kScratchDoubleReg);
  Orps(liftoff::kScratchDoubleReg, dst.fp());
  Psrld(dst.fp(), dst.fp(), byte{10});
  Andnps(dst.fp(), dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_f32x4_max(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  // The maxps instruction doesn't propagate NaNs and +0's in its first
  // operand. Perform maxps in both orders, merge the results, and adjust.
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vmaxps(liftoff::kScratchDoubleReg, lhs.fp(), rhs.fp());
    vmaxps(dst.fp(), rhs.fp(), lhs.fp());
  } else if (dst.fp() == lhs.fp() || dst.fp() == rhs.fp()) {
    XMMRegister src = dst.fp() == lhs.fp() ? rhs.fp() : lhs.fp();
    movaps(liftoff::kScratchDoubleReg, src);
    maxps(liftoff::kScratchDoubleReg, dst.fp());
    maxps(dst.fp(), src);
  } else {
    movaps(liftoff::kScratchDoubleReg, lhs.fp());
    maxps(liftoff::kScratchDoubleReg, rhs.fp());
    movaps(dst.fp(), rhs.fp());
    maxps(dst.fp(), lhs.fp());
  }
  // Find discrepancies.
  Xorps(dst.fp(), liftoff::kScratchDoubleReg);
  // Propagate NaNs, which may be non-canonical.
  Orps(liftoff::kScratchDoubleReg, dst.fp());
  // Propagate sign discrepancy and (subtle) quiet NaNs.
  Subps(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, dst.fp());
  // Canonicalize NaNs by clearing the payload. Sign is non-deterministic.
  Cmpunordps(dst.fp(), dst.fp(), liftoff::kScratchDoubleReg);
  Psrld(dst.fp(), dst.fp(), byte{10});
  Andnps(dst.fp(), dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_f32x4_pmin(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  // Due to the way minps works, pmin(a, b) = minps(b, a).
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vminps, &Assembler::minps>(
      this, dst, rhs, lhs);
}

void LiftoffAssembler::emit_f32x4_pmax(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  // Due to the way maxps works, pmax(a, b) = maxps(b, a).
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vmaxps, &Assembler::maxps>(
      this, dst, rhs, lhs);
}

void LiftoffAssembler::emit_f64x2_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Psrlq(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, byte{1});
    Andpd(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pcmpeqd(dst.fp(), dst.fp());
    Psrlq(dst.fp(), dst.fp(), byte{1});
    Andpd(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_f64x2_neg(LiftoffRegister dst,
                                      LiftoffRegister src) {
  if (dst.fp() == src.fp()) {
    Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    Psllq(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, byte{63});
    Xorpd(dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    Pcmpeqd(dst.fp(), dst.fp());
    Psllq(dst.fp(), dst.fp(), byte{63});
    Xorpd(dst.fp(), src.fp());
  }
}

void LiftoffAssembler::emit_f64x2_sqrt(LiftoffRegister dst,
                                       LiftoffRegister src) {
  Sqrtpd(dst.fp(), src.fp());
}

bool LiftoffAssembler::emit_f64x2_ceil(LiftoffRegister dst,
                                       LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundpd(dst.fp(), src.fp(), kRoundUp);
  return true;
}

bool LiftoffAssembler::emit_f64x2_floor(LiftoffRegister dst,
                                        LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundpd(dst.fp(), src.fp(), kRoundDown);
  return true;
}

bool LiftoffAssembler::emit_f64x2_trunc(LiftoffRegister dst,
                                        LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundpd(dst.fp(), src.fp(), kRoundToZero);
  return true;
}

bool LiftoffAssembler::emit_f64x2_nearest_int(LiftoffRegister dst,
                                              LiftoffRegister src) {
  DCHECK(CpuFeatures::IsSupported(SSE4_1));
  Roundpd(dst.fp(), src.fp(), kRoundToNearest);
  return true;
}

void LiftoffAssembler::emit_f64x2_add(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vaddpd, &Assembler::addpd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64x2_sub(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vsubpd, &Assembler::subpd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64x2_mul(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vmulpd, &Assembler::mulpd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64x2_div(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vdivpd, &Assembler::divpd>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_f64x2_min(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  F64x2Min(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_f64x2_max(LiftoffRegister dst, LiftoffRegister lhs,
                                      LiftoffRegister rhs) {
  F64x2Max(dst.fp(), lhs.fp(), rhs.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_f64x2_pmin(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  // Due to the way minpd works, pmin(a, b) = minpd(b, a).
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vminpd, &Assembler::minpd>(
      this, dst, rhs, lhs);
}

void LiftoffAssembler::emit_f64x2_pmax(LiftoffRegister dst, LiftoffRegister lhs,
                                       LiftoffRegister rhs) {
  // Due to the way maxpd works, pmax(a, b) = maxpd(b, a).
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vmaxpd, &Assembler::maxpd>(
      this, dst, rhs, lhs);
}

void LiftoffAssembler::emit_f64x2_convert_low_i32x4_s(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  Cvtdq2pd(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_f64x2_convert_low_i32x4_u(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  Register tmp = GetUnusedRegister(kGpReg, {}).gp();
  F64x2ConvertLowI32x4U(dst.fp(), src.fp(), tmp);
}

void LiftoffAssembler::emit_f64x2_promote_low_f32x4(LiftoffRegister dst,
                                                    LiftoffRegister src) {
  Cvtps2pd(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i32x4_sconvert_f32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  // NAN->0
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vcmpeqps(liftoff::kScratchDoubleReg, src.fp(), src.fp());
    vpand(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
  } else {
    movaps(liftoff::kScratchDoubleReg, src.fp());
    cmpeqps(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
    if (dst.fp() != src.fp()) movaps(dst.fp(), src.fp());
    andps(dst.fp(), liftoff::kScratchDoubleReg);
  }
  // Set top bit if >= 0 (but not -0.0!).
  Pxor(liftoff::kScratchDoubleReg, dst.fp());
  // Convert to int.
  Cvttps2dq(dst.fp(), dst.fp());
  // Set top bit if >=0 is now < 0.
  Pand(liftoff::kScratchDoubleReg, dst.fp());
  Psrad(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, byte{31});
  // Set positive overflow lanes to 0x7FFFFFFF.
  Pxor(dst.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i32x4_uconvert_f32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  static constexpr RegClass tmp_rc = reg_class_for(kS128);
  DoubleRegister tmp =
      GetUnusedRegister(tmp_rc, LiftoffRegList::ForRegs(dst, src)).fp();
  // NAN->0, negative->0.
  Pxor(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vmaxps(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
  } else {
    if (dst.fp() != src.fp()) movaps(dst.fp(), src.fp());
    maxps(dst.fp(), liftoff::kScratchDoubleReg);
  }
  // scratch: float representation of max_signed.
  Pcmpeqd(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Psrld(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg,
        uint8_t{1});  // 0x7fffffff
  Cvtdq2ps(liftoff::kScratchDoubleReg,
           liftoff::kScratchDoubleReg);  // 0x4f000000
  // tmp: convert (src-max_signed).
  // Set positive overflow lanes to 0x7FFFFFFF.
  // Set negative lanes to 0.
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vsubps(tmp, dst.fp(), liftoff::kScratchDoubleReg);
  } else {
    movaps(tmp, dst.fp());
    subps(tmp, liftoff::kScratchDoubleReg);
  }
  Cmpleps(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg, tmp);
  Cvttps2dq(tmp, tmp);
  Pxor(tmp, liftoff::kScratchDoubleReg);
  Pxor(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);
  Pmaxsd(tmp, tmp, liftoff::kScratchDoubleReg);
  // Convert to int. Overflow lanes above max_signed will be 0x80000000.
  Cvttps2dq(dst.fp(), dst.fp());
  // Add (src-max_signed) for overflow lanes.
  Paddd(dst.fp(), dst.fp(), tmp);
}

void LiftoffAssembler::emit_f32x4_sconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  Cvtdq2ps(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_f32x4_uconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister src) {
  Pxor(liftoff::kScratchDoubleReg, liftoff::kScratchDoubleReg);  // Zeros.
  Pblendw(liftoff::kScratchDoubleReg, src.fp(),
          uint8_t{0x55});  // Get lo 16 bits.
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpsubd(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);  // Get hi 16 bits.
  } else {
    if (dst.fp() != src.fp()) movaps(dst.fp(), src.fp());
    psubd(dst.fp(), liftoff::kScratchDoubleReg);
  }
  Cvtdq2ps(liftoff::kScratchDoubleReg,
           liftoff::kScratchDoubleReg);  // Convert lo exactly.
  Psrld(dst.fp(), dst.fp(), byte{1});   // Divide by 2 to get in unsigned range.
  Cvtdq2ps(dst.fp(), dst.fp());         // Convert hi, exactly.
  Addps(dst.fp(), dst.fp(), dst.fp());  // Double hi, exactly.
  Addps(dst.fp(), dst.fp(),
        liftoff::kScratchDoubleReg);  // Add hi and lo, may round.
}

void LiftoffAssembler::emit_f32x4_demote_f64x2_zero(LiftoffRegister dst,
                                                    LiftoffRegister src) {
  Cvtpd2ps(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i8x16_sconvert_i16x8(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpacksswb,
                                       &Assembler::packsswb>(this, dst, lhs,
                                                             rhs);
}

void LiftoffAssembler::emit_i8x16_uconvert_i16x8(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpackuswb,
                                       &Assembler::packuswb>(this, dst, lhs,
                                                             rhs);
}

void LiftoffAssembler::emit_i16x8_sconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpackssdw,
                                       &Assembler::packssdw>(this, dst, lhs,
                                                             rhs);
}

void LiftoffAssembler::emit_i16x8_uconvert_i32x4(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vpackusdw,
                                       &Assembler::packusdw>(this, dst, lhs,
                                                             rhs, SSE4_1);
}

void LiftoffAssembler::emit_i16x8_sconvert_i8x16_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  Pmovsxbw(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i16x8_sconvert_i8x16_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  I16x8SConvertI8x16High(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i16x8_uconvert_i8x16_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  Pmovzxbw(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i16x8_uconvert_i8x16_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  I16x8UConvertI8x16High(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i32x4_sconvert_i16x8_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  Pmovsxwd(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i32x4_sconvert_i16x8_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  I32x4SConvertI16x8High(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i32x4_uconvert_i16x8_low(LiftoffRegister dst,
                                                     LiftoffRegister src) {
  Pmovzxwd(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i32x4_uconvert_i16x8_high(LiftoffRegister dst,
                                                      LiftoffRegister src) {
  I32x4UConvertI16x8High(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i32x4_trunc_sat_f64x2_s_zero(LiftoffRegister dst,
                                                         LiftoffRegister src) {
  Register tmp = GetUnusedRegister(kGpReg, {}).gp();
  I32x4TruncSatF64x2SZero(dst.fp(), src.fp(), liftoff::kScratchDoubleReg, tmp);
}

void LiftoffAssembler::emit_i32x4_trunc_sat_f64x2_u_zero(LiftoffRegister dst,
                                                         LiftoffRegister src) {
  Register tmp = GetUnusedRegister(kGpReg, {}).gp();
  I32x4TruncSatF64x2UZero(dst.fp(), src.fp(), liftoff::kScratchDoubleReg, tmp);
}

void LiftoffAssembler::emit_s128_and_not(LiftoffRegister dst,
                                         LiftoffRegister lhs,
                                         LiftoffRegister rhs) {
  liftoff::EmitSimdNonCommutativeBinOp<&Assembler::vandnps, &Assembler::andnps>(
      this, dst, rhs, lhs);
}

void LiftoffAssembler::emit_i8x16_rounding_average_u(LiftoffRegister dst,
                                                     LiftoffRegister lhs,
                                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpavgb, &Assembler::pavgb>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i16x8_rounding_average_u(LiftoffRegister dst,
                                                     LiftoffRegister lhs,
                                                     LiftoffRegister rhs) {
  liftoff::EmitSimdCommutativeBinOp<&Assembler::vpavgw, &Assembler::pavgw>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i8x16_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  Pabsb(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i16x8_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  Pabsw(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i32x4_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  Pabsd(dst.fp(), src.fp());
}

void LiftoffAssembler::emit_i64x2_abs(LiftoffRegister dst,
                                      LiftoffRegister src) {
  I64x2Abs(dst.fp(), src.fp(), liftoff::kScratchDoubleReg);
}

void LiftoffAssembler::emit_i8x16_extract_lane_s(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  Register byte_reg = liftoff::GetTmpByteRegister(this, dst.gp());
  Pextrb(byte_reg, lhs.fp(), imm_lane_idx);
  movsx_b(dst.gp(), byte_reg);
}

void LiftoffAssembler::emit_i8x16_extract_lane_u(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  Pextrb(dst.gp(), lhs.fp(), imm_lane_idx);
}

void LiftoffAssembler::emit_i16x8_extract_lane_s(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  Pextrw(dst.gp(), lhs.fp(), imm_lane_idx);
  movsx_w(dst.gp(), dst.gp());
}

void LiftoffAssembler::emit_i16x8_extract_lane_u(LiftoffRegister dst,
                                                 LiftoffRegister lhs,
                                                 uint8_t imm_lane_idx) {
  Pextrw(dst.gp(), lhs.fp(), imm_lane_idx);
}

void LiftoffAssembler::emit_i32x4_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  Pextrd(dst.gp(), lhs.fp(), imm_lane_idx);
}

void LiftoffAssembler::emit_i64x2_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  Pextrd(dst.low_gp(), lhs.fp(), imm_lane_idx * 2);
  Pextrd(dst.high_gp(), lhs.fp(), imm_lane_idx * 2 + 1);
}

void LiftoffAssembler::emit_f32x4_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  F32x4ExtractLane(dst.fp(), lhs.fp(), imm_lane_idx);
}

void LiftoffAssembler::emit_f64x2_extract_lane(LiftoffRegister dst,
                                               LiftoffRegister lhs,
                                               uint8_t imm_lane_idx) {
  F64x2ExtractLane(dst.fp(), lhs.fp(), imm_lane_idx);
}

void LiftoffAssembler::emit_i8x16_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpinsrb(dst.fp(), src1.fp(), src2.gp(), imm_lane_idx);
  } else {
    CpuFeatureScope scope(this, SSE4_1);
    if (dst.fp() != src1.fp()) movaps(dst.fp(), src1.fp());
    pinsrb(dst.fp(), src2.gp(), imm_lane_idx);
  }
}

void LiftoffAssembler::emit_i16x8_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpinsrw(dst.fp(), src1.fp(), src2.gp(), imm_lane_idx);
  } else {
    if (dst.fp() != src1.fp()) movaps(dst.fp(), src1.fp());
    pinsrw(dst.fp(), src2.gp(), imm_lane_idx);
  }
}

void LiftoffAssembler::emit_i32x4_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpinsrd(dst.fp(), src1.fp(), src2.gp(), imm_lane_idx);
  } else {
    CpuFeatureScope scope(this, SSE4_1);
    if (dst.fp() != src1.fp()) movaps(dst.fp(), src1.fp());
    pinsrd(dst.fp(), src2.gp(), imm_lane_idx);
  }
}

void LiftoffAssembler::emit_i64x2_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpinsrd(dst.fp(), src1.fp(), src2.low_gp(), imm_lane_idx * 2);
    vpinsrd(dst.fp(), dst.fp(), src2.high_gp(), imm_lane_idx * 2 + 1);
  } else {
    CpuFeatureScope scope(this, SSE4_1);
    if (dst.fp() != src1.fp()) movaps(dst.fp(), src1.fp());
    pinsrd(dst.fp(), src2.low_gp(), imm_lane_idx * 2);
    pinsrd(dst.fp(), src2.high_gp(), imm_lane_idx * 2 + 1);
  }
}

void LiftoffAssembler::emit_f32x4_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vinsertps(dst.fp(), src1.fp(), src2.fp(), (imm_lane_idx << 4) & 0x30);
  } else {
    CpuFeatureScope scope(this, SSE4_1);
    if (dst.fp() != src1.fp()) movaps(dst.fp(), src1.fp());
    insertps(dst.fp(), src2.fp(), (imm_lane_idx << 4) & 0x30);
  }
}

void LiftoffAssembler::emit_f64x2_replace_lane(LiftoffRegister dst,
                                               LiftoffRegister src1,
                                               LiftoffRegister src2,
                                               uint8_t imm_lane_idx) {
  F64x2ReplaceLane(dst.fp(), src1.fp(), src2.fp(), imm_lane_idx);
}

void LiftoffAssembler::StackCheck(Label* ool_code, Register limit_address) {
  cmp(esp, Operand(limit_address, 0));
  j(below_equal, ool_code);
}

void LiftoffAssembler::CallTrapCallbackForTesting() {
  PrepareCallCFunction(0, GetUnusedRegister(kGpReg, {}).gp());
  CallCFunction(ExternalReference::wasm_call_trap_callback_for_testing(), 0);
}

void LiftoffAssembler::AssertUnreachable(AbortReason reason) {
  TurboAssembler::AssertUnreachable(reason);
}

void LiftoffAssembler::PushRegisters(LiftoffRegList regs) {
  LiftoffRegList gp_regs = regs & kGpCacheRegList;
  while (!gp_regs.is_empty()) {
    LiftoffRegister reg = gp_regs.GetFirstRegSet();
    push(reg.gp());
    gp_regs.clear(reg);
  }
  LiftoffRegList fp_regs = regs & kFpCacheRegList;
  unsigned num_fp_regs = fp_regs.GetNumRegsSet();
  if (num_fp_regs) {
    AllocateStackSpace(num_fp_regs * kSimd128Size);
    unsigned offset = 0;
    while (!fp_regs.is_empty()) {
      LiftoffRegister reg = fp_regs.GetFirstRegSet();
      Movdqu(Operand(esp, offset), reg.fp());
      fp_regs.clear(reg);
      offset += kSimd128Size;
    }
    DCHECK_EQ(offset, num_fp_regs * kSimd128Size);
  }
}

void LiftoffAssembler::PopRegisters(LiftoffRegList regs) {
  LiftoffRegList fp_regs = regs & kFpCacheRegList;
  unsigned fp_offset = 0;
  while (!fp_regs.is_empty()) {
    LiftoffRegister reg = fp_regs.GetFirstRegSet();
    Movdqu(reg.fp(), Operand(esp, fp_offset));
    fp_regs.clear(reg);
    fp_offset += kSimd128Size;
  }
  if (fp_offset) add(esp, Immediate(fp_offset));
  LiftoffRegList gp_regs = regs & kGpCacheRegList;
  while (!gp_regs.is_empty()) {
    LiftoffRegister reg = gp_regs.GetLastRegSet();
    pop(reg.gp());
    gp_regs.clear(reg);
  }
}

void LiftoffAssembler::RecordSpillsInSafepoint(Safepoint& safepoint,
                                               LiftoffRegList all_spills,
                                               LiftoffRegList ref_spills,
                                               int spill_offset) {
  int spill_space_size = 0;
  while (!all_spills.is_empty()) {
    LiftoffRegister reg = all_spills.GetFirstRegSet();
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
  DCHECK_LT(num_stack_slots,
            (1 << 16) / kSystemPointerSize);  // 16 bit immediate
  ret(static_cast<int>(num_stack_slots * kSystemPointerSize));
}

void LiftoffAssembler::CallC(const ValueKindSig* sig,
                             const LiftoffRegister* args,
                             const LiftoffRegister* rets,
                             ValueKind out_argument_kind, int stack_bytes,
                             ExternalReference ext_ref) {
  AllocateStackSpace(stack_bytes);

  int arg_bytes = 0;
  for (ValueKind param_kind : sig->parameters()) {
    liftoff::Store(this, esp, arg_bytes, *args++, param_kind);
    arg_bytes += element_size_bytes(param_kind);
  }
  DCHECK_LE(arg_bytes, stack_bytes);

  constexpr Register kScratch = eax;
  constexpr Register kArgumentBuffer = ecx;
  constexpr int kNumCCallArgs = 1;
  mov(kArgumentBuffer, esp);
  PrepareCallCFunction(kNumCCallArgs, kScratch);

  // Pass a pointer to the buffer with the arguments to the C function. ia32
  // does not use registers here, so push to the stack.
  mov(Operand(esp, 0), kArgumentBuffer);

  // Now call the C function.
  CallCFunction(ext_ref, kNumCCallArgs);

  // Move return value to the right register.
  const LiftoffRegister* next_result_reg = rets;
  if (sig->return_count() > 0) {
    DCHECK_EQ(1, sig->return_count());
    constexpr Register kReturnReg = eax;
    if (kReturnReg != next_result_reg->gp()) {
      Move(*next_result_reg, LiftoffRegister(kReturnReg), sig->GetReturn(0));
    }
    ++next_result_reg;
  }

  // Load potential output value from the buffer on the stack.
  if (out_argument_kind != kVoid) {
    liftoff::Load(this, *next_result_reg, esp, 0, out_argument_kind);
  }

  add(esp, Immediate(stack_bytes));
}

void LiftoffAssembler::CallNativeWasmCode(Address addr) {
  wasm_call(addr, RelocInfo::WASM_CALL);
}

void LiftoffAssembler::TailCallNativeWasmCode(Address addr) {
  jmp(addr, RelocInfo::WASM_CALL);
}

void LiftoffAssembler::CallIndirect(const ValueKindSig* sig,
                                    compiler::CallDescriptor* call_descriptor,
                                    Register target) {
  // Since we have more cache registers than parameter registers, the
  // {LiftoffCompiler} should always be able to place {target} in a register.
  DCHECK(target.is_valid());
  if (FLAG_untrusted_code_mitigations) {
    RetpolineCall(target);
  } else {
    call(target);
  }
}

void LiftoffAssembler::TailCallIndirect(Register target) {
  // Since we have more cache registers than parameter registers, the
  // {LiftoffCompiler} should always be able to place {target} in a register.
  DCHECK(target.is_valid());
  if (FLAG_untrusted_code_mitigations) {
    RetpolineJump(target);
  } else {
    jmp(target);
  }
}

void LiftoffAssembler::CallRuntimeStub(WasmCode::RuntimeStubId sid) {
  // A direct call to a wasm runtime stub defined in this module.
  // Just encode the stub index. This will be patched at relocation.
  wasm_call(static_cast<Address>(sid), RelocInfo::WASM_STUB_CALL);
}

void LiftoffAssembler::AllocateStackSlot(Register addr, uint32_t size) {
  AllocateStackSpace(size);
  mov(addr, esp);
}

void LiftoffAssembler::DeallocateStackSlot(uint32_t size) {
  add(esp, Immediate(size));
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
      case LiftoffAssembler::VarState::kStack:
        // The combination of AllocateStackSpace and 2 movdqu is usually smaller
        // in code size than doing 4 pushes.
        if (src.kind() == kS128) {
          asm_->AllocateStackSpace(stack_decrement);
          asm_->movdqu(liftoff::kScratchDoubleReg,
                       liftoff::GetStackSlot(slot.src_offset_));
          asm_->movdqu(Operand(esp, 0), liftoff::kScratchDoubleReg);
          break;
        }
        if (src.kind() == kF64) {
          asm_->AllocateStackSpace(stack_decrement - kDoubleSize);
          DCHECK_EQ(kLowWord, slot.half_);
          asm_->push(liftoff::GetHalfStackSlot(slot.src_offset_, kHighWord));
          stack_decrement = kSystemPointerSize;
        }
        asm_->AllocateStackSpace(stack_decrement - kSystemPointerSize);
        asm_->push(liftoff::GetHalfStackSlot(slot.src_offset_, slot.half_));
        break;
      case LiftoffAssembler::VarState::kRegister:
        if (src.kind() == kI64) {
          liftoff::push(
              asm_, slot.half_ == kLowWord ? src.reg().low() : src.reg().high(),
              kI32, stack_decrement - kSystemPointerSize);
        } else {
          int pushed_bytes = SlotSizeInBytes(slot);
          liftoff::push(asm_, src.reg(), src.kind(),
                        stack_decrement - pushed_bytes);
        }
        break;
      case LiftoffAssembler::VarState::kIntConst:
        asm_->AllocateStackSpace(stack_decrement - kSystemPointerSize);
        // The high word is the sign extension of the low word.
        asm_->push(Immediate(slot.half_ == kLowWord ? src.i32_const()
                                                    : src.i32_const() >> 31));
        break;
    }
  }
}

#undef RETURN_FALSE_IF_MISSING_CPU_FEATURE

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_BASELINE_IA32_LIFTOFF_ASSEMBLER_IA32_H_
