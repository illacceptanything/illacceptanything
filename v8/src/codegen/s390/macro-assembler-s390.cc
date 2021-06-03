// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>  // For assert
#include <limits.h>  // For LONG_MIN, LONG_MAX.

#if V8_TARGET_ARCH_S390

#include "src/base/bits.h"
#include "src/base/division-by-constant.h"
#include "src/codegen/callable.h"
#include "src/codegen/code-factory.h"
#include "src/codegen/external-reference-table.h"
#include "src/codegen/interface-descriptors-inl.h"
#include "src/codegen/macro-assembler.h"
#include "src/codegen/register-configuration.h"
#include "src/debug/debug.h"
#include "src/execution/frames-inl.h"
#include "src/heap/memory-chunk.h"
#include "src/init/bootstrapper.h"
#include "src/logging/counters.h"
#include "src/objects/smi.h"
#include "src/runtime/runtime.h"
#include "src/snapshot/embedded/embedded-data.h"
#include "src/snapshot/snapshot.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/wasm-code-manager.h"
#endif  // V8_ENABLE_WEBASSEMBLY

// Satisfy cpplint check, but don't include platform-specific header. It is
// included recursively via macro-assembler.h.
#if 0
#include "src/codegen/s390/macro-assembler-s390.h"
#endif

namespace v8 {
namespace internal {

void TurboAssembler::DoubleMax(DoubleRegister result_reg,
                               DoubleRegister left_reg,
                               DoubleRegister right_reg) {
  if (CpuFeatures::IsSupported(VECTOR_ENHANCE_FACILITY_1)) {
    vfmax(result_reg, left_reg, right_reg, Condition(1), Condition(8),
          Condition(3));
    return;
  }

  Label check_zero, return_left, return_right, return_nan, done;
  cdbr(left_reg, right_reg);
  bunordered(&return_nan, Label::kNear);
  beq(&check_zero);
  bge(&return_left, Label::kNear);
  b(&return_right, Label::kNear);

  bind(&check_zero);
  lzdr(kDoubleRegZero);
  cdbr(left_reg, kDoubleRegZero);
  /* left == right != 0. */
  bne(&return_left, Label::kNear);
  /* At this point, both left and right are either 0 or -0. */
  /* N.B. The following works because +0 + -0 == +0 */
  /* For max we want logical-and of sign bit: (L + R) */
  ldr(result_reg, left_reg);
  adbr(result_reg, right_reg);
  b(&done, Label::kNear);

  bind(&return_nan);
  /* If left or right are NaN, adbr propagates the appropriate one.*/
  adbr(left_reg, right_reg);
  b(&return_left, Label::kNear);

  bind(&return_right);
  if (right_reg != result_reg) {
    ldr(result_reg, right_reg);
  }
  b(&done, Label::kNear);

  bind(&return_left);
  if (left_reg != result_reg) {
    ldr(result_reg, left_reg);
  }
  bind(&done);
}

void TurboAssembler::DoubleMin(DoubleRegister result_reg,
                               DoubleRegister left_reg,
                               DoubleRegister right_reg) {
  if (CpuFeatures::IsSupported(VECTOR_ENHANCE_FACILITY_1)) {
    vfmin(result_reg, left_reg, right_reg, Condition(1), Condition(8),
          Condition(3));
    return;
  }
  Label check_zero, return_left, return_right, return_nan, done;
  cdbr(left_reg, right_reg);
  bunordered(&return_nan, Label::kNear);
  beq(&check_zero);
  ble(&return_left, Label::kNear);
  b(&return_right, Label::kNear);

  bind(&check_zero);
  lzdr(kDoubleRegZero);
  cdbr(left_reg, kDoubleRegZero);
  /* left == right != 0. */
  bne(&return_left, Label::kNear);
  /* At this point, both left and right are either 0 or -0. */
  /* N.B. The following works because +0 + -0 == +0 */
  /* For min we want logical-or of sign bit: -(-L + -R) */
  lcdbr(left_reg, left_reg);
  ldr(result_reg, left_reg);
  if (left_reg == right_reg) {
    adbr(result_reg, right_reg);
  } else {
    sdbr(result_reg, right_reg);
  }
  lcdbr(result_reg, result_reg);
  b(&done, Label::kNear);

  bind(&return_nan);
  /* If left or right are NaN, adbr propagates the appropriate one.*/
  adbr(left_reg, right_reg);
  b(&return_left, Label::kNear);

  bind(&return_right);
  if (right_reg != result_reg) {
    ldr(result_reg, right_reg);
  }
  b(&done, Label::kNear);

  bind(&return_left);
  if (left_reg != result_reg) {
    ldr(result_reg, left_reg);
  }
  bind(&done);
}

void TurboAssembler::FloatMax(DoubleRegister result_reg,
                              DoubleRegister left_reg,
                              DoubleRegister right_reg) {
  if (CpuFeatures::IsSupported(VECTOR_ENHANCE_FACILITY_1)) {
    vfmax(result_reg, left_reg, right_reg, Condition(1), Condition(8),
          Condition(2));
    return;
  }
  Label check_zero, return_left, return_right, return_nan, done;
  cebr(left_reg, right_reg);
  bunordered(&return_nan, Label::kNear);
  beq(&check_zero);
  bge(&return_left, Label::kNear);
  b(&return_right, Label::kNear);

  bind(&check_zero);
  lzdr(kDoubleRegZero);
  cebr(left_reg, kDoubleRegZero);
  /* left == right != 0. */
  bne(&return_left, Label::kNear);
  /* At this point, both left and right are either 0 or -0. */
  /* N.B. The following works because +0 + -0 == +0 */
  /* For max we want logical-and of sign bit: (L + R) */
  ldr(result_reg, left_reg);
  aebr(result_reg, right_reg);
  b(&done, Label::kNear);

  bind(&return_nan);
  /* If left or right are NaN, aebr propagates the appropriate one.*/
  aebr(left_reg, right_reg);
  b(&return_left, Label::kNear);

  bind(&return_right);
  if (right_reg != result_reg) {
    ldr(result_reg, right_reg);
  }
  b(&done, Label::kNear);

  bind(&return_left);
  if (left_reg != result_reg) {
    ldr(result_reg, left_reg);
  }
  bind(&done);
}

void TurboAssembler::FloatMin(DoubleRegister result_reg,
                              DoubleRegister left_reg,
                              DoubleRegister right_reg) {
  if (CpuFeatures::IsSupported(VECTOR_ENHANCE_FACILITY_1)) {
    vfmin(result_reg, left_reg, right_reg, Condition(1), Condition(8),
          Condition(2));
    return;
  }

  Label check_zero, return_left, return_right, return_nan, done;
  cebr(left_reg, right_reg);
  bunordered(&return_nan, Label::kNear);
  beq(&check_zero);
  ble(&return_left, Label::kNear);
  b(&return_right, Label::kNear);

  bind(&check_zero);
  lzdr(kDoubleRegZero);
  cebr(left_reg, kDoubleRegZero);
  /* left == right != 0. */
  bne(&return_left, Label::kNear);
  /* At this point, both left and right are either 0 or -0. */
  /* N.B. The following works because +0 + -0 == +0 */
  /* For min we want logical-or of sign bit: -(-L + -R) */
  lcebr(left_reg, left_reg);
  ldr(result_reg, left_reg);
  if (left_reg == right_reg) {
    aebr(result_reg, right_reg);
  } else {
    sebr(result_reg, right_reg);
  }
  lcebr(result_reg, result_reg);
  b(&done, Label::kNear);

  bind(&return_nan);
  /* If left or right are NaN, aebr propagates the appropriate one.*/
  aebr(left_reg, right_reg);
  b(&return_left, Label::kNear);

  bind(&return_right);
  if (right_reg != result_reg) {
    ldr(result_reg, right_reg);
  }
  b(&done, Label::kNear);

  bind(&return_left);
  if (left_reg != result_reg) {
    ldr(result_reg, left_reg);
  }
  bind(&done);
}

void TurboAssembler::CeilF32(DoubleRegister dst, DoubleRegister src) {
  fiebra(ROUND_TOWARD_POS_INF, dst, src);
}

void TurboAssembler::CeilF64(DoubleRegister dst, DoubleRegister src) {
  fidbra(ROUND_TOWARD_POS_INF, dst, src);
}

void TurboAssembler::FloorF32(DoubleRegister dst, DoubleRegister src) {
  fiebra(ROUND_TOWARD_NEG_INF, dst, src);
}

void TurboAssembler::FloorF64(DoubleRegister dst, DoubleRegister src) {
  fidbra(ROUND_TOWARD_NEG_INF, dst, src);
}

void TurboAssembler::TruncF32(DoubleRegister dst, DoubleRegister src) {
  fiebra(ROUND_TOWARD_0, dst, src);
}

void TurboAssembler::TruncF64(DoubleRegister dst, DoubleRegister src) {
  fidbra(ROUND_TOWARD_0, dst, src);
}

void TurboAssembler::NearestIntF32(DoubleRegister dst, DoubleRegister src) {
  fiebra(ROUND_TO_NEAREST_TO_EVEN, dst, src);
}

void TurboAssembler::NearestIntF64(DoubleRegister dst, DoubleRegister src) {
  fidbra(ROUND_TO_NEAREST_TO_EVEN, dst, src);
}

int TurboAssembler::RequiredStackSizeForCallerSaved(SaveFPRegsMode fp_mode,
                                                    Register exclusion1,
                                                    Register exclusion2,
                                                    Register exclusion3) const {
  int bytes = 0;
  RegList exclusions = 0;
  if (exclusion1 != no_reg) {
    exclusions |= exclusion1.bit();
    if (exclusion2 != no_reg) {
      exclusions |= exclusion2.bit();
      if (exclusion3 != no_reg) {
        exclusions |= exclusion3.bit();
      }
    }
  }

  RegList list = kJSCallerSaved & ~exclusions;
  bytes += NumRegs(list) * kSystemPointerSize;

  if (fp_mode == SaveFPRegsMode::kSave) {
    bytes += NumRegs(kCallerSavedDoubles) * kDoubleSize;
  }

  return bytes;
}

int TurboAssembler::PushCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1,
                                    Register exclusion2, Register exclusion3) {
  int bytes = 0;
  RegList exclusions = 0;
  if (exclusion1 != no_reg) {
    exclusions |= exclusion1.bit();
    if (exclusion2 != no_reg) {
      exclusions |= exclusion2.bit();
      if (exclusion3 != no_reg) {
        exclusions |= exclusion3.bit();
      }
    }
  }

  RegList list = kJSCallerSaved & ~exclusions;
  MultiPush(list);
  bytes += NumRegs(list) * kSystemPointerSize;

  if (fp_mode == SaveFPRegsMode::kSave) {
    MultiPushDoubles(kCallerSavedDoubles);
    bytes += NumRegs(kCallerSavedDoubles) * kDoubleSize;
  }

  return bytes;
}

int TurboAssembler::PopCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1,
                                   Register exclusion2, Register exclusion3) {
  int bytes = 0;
  if (fp_mode == SaveFPRegsMode::kSave) {
    MultiPopDoubles(kCallerSavedDoubles);
    bytes += NumRegs(kCallerSavedDoubles) * kDoubleSize;
  }

  RegList exclusions = 0;
  if (exclusion1 != no_reg) {
    exclusions |= exclusion1.bit();
    if (exclusion2 != no_reg) {
      exclusions |= exclusion2.bit();
      if (exclusion3 != no_reg) {
        exclusions |= exclusion3.bit();
      }
    }
  }

  RegList list = kJSCallerSaved & ~exclusions;
  MultiPop(list);
  bytes += NumRegs(list) * kSystemPointerSize;

  return bytes;
}

void TurboAssembler::LoadFromConstantsTable(Register destination,
                                            int constant_index) {
  DCHECK(RootsTable::IsImmortalImmovable(RootIndex::kBuiltinsConstantsTable));

  const uint32_t offset = FixedArray::kHeaderSize +
                          constant_index * kSystemPointerSize - kHeapObjectTag;

  CHECK(is_uint19(offset));
  DCHECK_NE(destination, r0);
  LoadRoot(destination, RootIndex::kBuiltinsConstantsTable);
  LoadTaggedPointerField(
      destination,
      FieldMemOperand(destination,
                      FixedArray::OffsetOfElementAt(constant_index)),
      r1);
}

void TurboAssembler::LoadRootRelative(Register destination, int32_t offset) {
  LoadU64(destination, MemOperand(kRootRegister, offset));
}

void TurboAssembler::LoadRootRegisterOffset(Register destination,
                                            intptr_t offset) {
  if (offset == 0) {
    mov(destination, kRootRegister);
  } else if (is_uint12(offset)) {
    la(destination, MemOperand(kRootRegister, offset));
  } else {
    DCHECK(is_int20(offset));
    lay(destination, MemOperand(kRootRegister, offset));
  }
}

void TurboAssembler::Jump(Register target, Condition cond) { b(cond, target); }

void TurboAssembler::Jump(intptr_t target, RelocInfo::Mode rmode,
                          Condition cond) {
  Label skip;

  if (cond != al) b(NegateCondition(cond), &skip);

  mov(ip, Operand(target, rmode));
  b(ip);

  bind(&skip);
}

void TurboAssembler::Jump(Address target, RelocInfo::Mode rmode,
                          Condition cond) {
  DCHECK(!RelocInfo::IsCodeTarget(rmode));
  Jump(static_cast<intptr_t>(target), rmode, cond);
}

void TurboAssembler::Jump(Handle<Code> code, RelocInfo::Mode rmode,
                          Condition cond) {
  DCHECK(RelocInfo::IsCodeTarget(rmode));
  DCHECK_IMPLIES(options().isolate_independent_code,
                 Builtins::IsIsolateIndependentBuiltin(*code));

  int builtin_index = Builtins::kNoBuiltinId;
  bool target_is_builtin =
      isolate()->builtins()->IsBuiltinHandle(code, &builtin_index);

  if (options().inline_offheap_trampolines && target_is_builtin) {
    // Inline the trampoline.
    RecordCommentForOffHeapTrampoline(builtin_index);
    CHECK_NE(builtin_index, Builtins::kNoBuiltinId);
    EmbeddedData d = EmbeddedData::FromBlob();
    Address entry = d.InstructionStartOfBuiltin(builtin_index);
    mov(ip, Operand(entry, RelocInfo::OFF_HEAP_TARGET));
    b(cond, ip);
    return;
  }
  jump(code, RelocInfo::RELATIVE_CODE_TARGET, cond);
}

void TurboAssembler::Jump(const ExternalReference& reference) {
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  Move(scratch, reference);
  Jump(scratch);
}

void TurboAssembler::Call(Register target) {
  // Branch to target via indirect branch
  basr(r14, target);
}

void MacroAssembler::CallJSEntry(Register target) {
  DCHECK(target == r4);
  Call(target);
}

int MacroAssembler::CallSizeNotPredictableCodeSize(Address target,
                                                   RelocInfo::Mode rmode,
                                                   Condition cond) {
  // S390 Assembler::move sequence is IILF / IIHF
  int size;
#if V8_TARGET_ARCH_S390X
  size = 14;  // IILF + IIHF + BASR
#else
  size = 8;  // IILF + BASR
#endif
  return size;
}

void TurboAssembler::Call(Address target, RelocInfo::Mode rmode,
                          Condition cond) {
  DCHECK(cond == al);

  mov(ip, Operand(target, rmode));
  basr(r14, ip);
}

void TurboAssembler::Call(Handle<Code> code, RelocInfo::Mode rmode,
                          Condition cond) {
  DCHECK(RelocInfo::IsCodeTarget(rmode) && cond == al);

  DCHECK_IMPLIES(options().isolate_independent_code,
                 Builtins::IsIsolateIndependentBuiltin(*code));
  int builtin_index = Builtins::kNoBuiltinId;
  bool target_is_builtin =
      isolate()->builtins()->IsBuiltinHandle(code, &builtin_index);

  if (target_is_builtin && options().inline_offheap_trampolines) {
    // Inline the trampoline.
    RecordCommentForOffHeapTrampoline(builtin_index);
    CHECK_NE(builtin_index, Builtins::kNoBuiltinId);
    EmbeddedData d = EmbeddedData::FromBlob();
    Address entry = d.InstructionStartOfBuiltin(builtin_index);
    mov(ip, Operand(entry, RelocInfo::OFF_HEAP_TARGET));
    Call(ip);
    return;
  }
  DCHECK(code->IsExecutable());
  call(code, rmode);
}

void TurboAssembler::Drop(int count) {
  if (count > 0) {
    int total = count * kSystemPointerSize;
    if (is_uint12(total)) {
      la(sp, MemOperand(sp, total));
    } else if (is_int20(total)) {
      lay(sp, MemOperand(sp, total));
    } else {
      AddS64(sp, Operand(total));
    }
  }
}

void TurboAssembler::Drop(Register count, Register scratch) {
  ShiftLeftU64(scratch, count, Operand(kSystemPointerSizeLog2));
  AddS64(sp, sp, scratch);
}

void TurboAssembler::Call(Label* target) { b(r14, target); }

void TurboAssembler::Push(Handle<HeapObject> handle) {
  mov(r0, Operand(handle));
  push(r0);
}

void TurboAssembler::Push(Smi smi) {
  mov(r0, Operand(smi));
  push(r0);
}

void TurboAssembler::Move(Register dst, Handle<HeapObject> value,
                          RelocInfo::Mode rmode) {
  // TODO(jgruber,v8:8887): Also consider a root-relative load when generating
  // non-isolate-independent code. In many cases it might be cheaper than
  // embedding the relocatable value.
  if (root_array_available_ && options().isolate_independent_code) {
    IndirectLoadConstant(dst, value);
    return;
  } else if (RelocInfo::IsCompressedEmbeddedObject(rmode)) {
    EmbeddedObjectIndex index = AddEmbeddedObject(value);
    DCHECK(is_uint32(index));
    mov(dst, Operand(static_cast<int>(index), rmode));
  } else {
    DCHECK(RelocInfo::IsFullEmbeddedObject(rmode));
    mov(dst, Operand(value.address(), rmode));
  }
}

void TurboAssembler::Move(Register dst, ExternalReference reference) {
  // TODO(jgruber,v8:8887): Also consider a root-relative load when generating
  // non-isolate-independent code. In many cases it might be cheaper than
  // embedding the relocatable value.
  if (root_array_available_ && options().isolate_independent_code) {
    IndirectLoadExternalReference(dst, reference);
    return;
  }
  mov(dst, Operand(reference));
}

void TurboAssembler::Move(Register dst, Register src, Condition cond) {
  if (dst != src) {
    if (cond == al) {
      mov(dst, src);
    } else {
      LoadOnConditionP(cond, dst, src);
    }
  }
}

void TurboAssembler::Move(DoubleRegister dst, DoubleRegister src) {
  if (dst != src) {
    ldr(dst, src);
  }
}

// Wrapper around Assembler::mvc (SS-a format)
void TurboAssembler::MoveChar(const MemOperand& opnd1, const MemOperand& opnd2,
                              const Operand& length) {
  mvc(opnd1, opnd2, Operand(static_cast<intptr_t>(length.immediate() - 1)));
}

// Wrapper around Assembler::clc (SS-a format)
void TurboAssembler::CompareLogicalChar(const MemOperand& opnd1,
                                        const MemOperand& opnd2,
                                        const Operand& length) {
  clc(opnd1, opnd2, Operand(static_cast<intptr_t>(length.immediate() - 1)));
}

// Wrapper around Assembler::xc (SS-a format)
void TurboAssembler::ExclusiveOrChar(const MemOperand& opnd1,
                                     const MemOperand& opnd2,
                                     const Operand& length) {
  xc(opnd1, opnd2, Operand(static_cast<intptr_t>(length.immediate() - 1)));
}

// Wrapper around Assembler::risbg(n) (RIE-f)
void TurboAssembler::RotateInsertSelectBits(Register dst, Register src,
                                            const Operand& startBit,
                                            const Operand& endBit,
                                            const Operand& shiftAmt,
                                            bool zeroBits) {
  if (zeroBits)
    // High tag the top bit of I4/EndBit to zero out any unselected bits
    risbg(dst, src, startBit,
          Operand(static_cast<intptr_t>(endBit.immediate() | 0x80)), shiftAmt);
  else
    risbg(dst, src, startBit, endBit, shiftAmt);
}

void TurboAssembler::BranchRelativeOnIdxHighP(Register dst, Register inc,
                                              Label* L) {
#if V8_TARGET_ARCH_S390X
  brxhg(dst, inc, L);
#else
  brxh(dst, inc, L);
#endif  // V8_TARGET_ARCH_S390X
}

void TurboAssembler::PushArray(Register array, Register size, Register scratch,
                               Register scratch2, PushArrayOrder order) {
  Label loop, done;

  if (order == kNormal) {
    ShiftLeftU64(scratch, size, Operand(kSystemPointerSizeLog2));
    lay(scratch, MemOperand(array, scratch));
    bind(&loop);
    CmpS64(array, scratch);
    bge(&done);
    lay(scratch, MemOperand(scratch, -kSystemPointerSize));
    lay(sp, MemOperand(sp, -kSystemPointerSize));
    MoveChar(MemOperand(sp), MemOperand(scratch), Operand(kSystemPointerSize));
    b(&loop);
    bind(&done);
  } else {
    DCHECK_NE(scratch2, r0);
    ShiftLeftU64(scratch, size, Operand(kSystemPointerSizeLog2));
    lay(scratch, MemOperand(array, scratch));
    mov(scratch2, array);
    bind(&loop);
    CmpS64(scratch2, scratch);
    bge(&done);
    lay(sp, MemOperand(sp, -kSystemPointerSize));
    MoveChar(MemOperand(sp), MemOperand(scratch2), Operand(kSystemPointerSize));
    lay(scratch2, MemOperand(scratch2, kSystemPointerSize));
    b(&loop);
    bind(&done);
  }
}

void TurboAssembler::MultiPush(RegList regs, Register location) {
  int16_t num_to_push = base::bits::CountPopulation(regs);
  int16_t stack_offset = num_to_push * kSystemPointerSize;

  SubS64(location, location, Operand(stack_offset));
  for (int16_t i = Register::kNumRegisters - 1; i >= 0; i--) {
    if ((regs & (1 << i)) != 0) {
      stack_offset -= kSystemPointerSize;
      StoreU64(ToRegister(i), MemOperand(location, stack_offset));
    }
  }
}

void TurboAssembler::MultiPop(RegList regs, Register location) {
  int16_t stack_offset = 0;

  for (int16_t i = 0; i < Register::kNumRegisters; i++) {
    if ((regs & (1 << i)) != 0) {
      LoadU64(ToRegister(i), MemOperand(location, stack_offset));
      stack_offset += kSystemPointerSize;
    }
  }
  AddS64(location, location, Operand(stack_offset));
}

void TurboAssembler::MultiPushDoubles(RegList dregs, Register location) {
  int16_t num_to_push = base::bits::CountPopulation(dregs);
  int16_t stack_offset = num_to_push * kDoubleSize;

  SubS64(location, location, Operand(stack_offset));
  for (int16_t i = DoubleRegister::kNumRegisters - 1; i >= 0; i--) {
    if ((dregs & (1 << i)) != 0) {
      DoubleRegister dreg = DoubleRegister::from_code(i);
      stack_offset -= kDoubleSize;
      StoreF64(dreg, MemOperand(location, stack_offset));
    }
  }
}

void TurboAssembler::MultiPushV128(RegList dregs, Register location) {
  int16_t num_to_push = base::bits::CountPopulation(dregs);
  int16_t stack_offset = num_to_push * kSimd128Size;

  SubS64(location, location, Operand(stack_offset));
  for (int16_t i = Simd128Register::kNumRegisters - 1; i >= 0; i--) {
    if ((dregs & (1 << i)) != 0) {
      Simd128Register dreg = Simd128Register::from_code(i);
      stack_offset -= kSimd128Size;
      StoreV128(dreg, MemOperand(location, stack_offset), r0);
    }
  }
}

void TurboAssembler::MultiPopDoubles(RegList dregs, Register location) {
  int16_t stack_offset = 0;

  for (int16_t i = 0; i < DoubleRegister::kNumRegisters; i++) {
    if ((dregs & (1 << i)) != 0) {
      DoubleRegister dreg = DoubleRegister::from_code(i);
      LoadF64(dreg, MemOperand(location, stack_offset));
      stack_offset += kDoubleSize;
    }
  }
  AddS64(location, location, Operand(stack_offset));
}

void TurboAssembler::MultiPopV128(RegList dregs, Register location) {
  int16_t stack_offset = 0;

  for (int16_t i = 0; i < Simd128Register::kNumRegisters; i++) {
    if ((dregs & (1 << i)) != 0) {
      Simd128Register dreg = Simd128Register::from_code(i);
      LoadV128(dreg, MemOperand(location, stack_offset), r0);
      stack_offset += kSimd128Size;
    }
  }
  AddS64(location, location, Operand(stack_offset));
}

void TurboAssembler::LoadRoot(Register destination, RootIndex index,
                              Condition) {
  LoadU64(destination,
          MemOperand(kRootRegister, RootRegisterOffsetForRootIndex(index)), r0);
}

void TurboAssembler::LoadTaggedPointerField(const Register& destination,
                                            const MemOperand& field_operand,
                                            const Register& scratch) {
  if (COMPRESS_POINTERS_BOOL) {
    DecompressTaggedPointer(destination, field_operand);
  } else {
    LoadU64(destination, field_operand, scratch);
  }
}

void TurboAssembler::LoadAnyTaggedField(const Register& destination,
                                        const MemOperand& field_operand,
                                        const Register& scratch) {
  if (COMPRESS_POINTERS_BOOL) {
    DecompressAnyTagged(destination, field_operand);
  } else {
    LoadU64(destination, field_operand, scratch);
  }
}

void TurboAssembler::SmiUntag(Register dst, const MemOperand& src) {
  if (SmiValuesAre31Bits()) {
    LoadS32(dst, src);
  } else {
    LoadU64(dst, src);
  }
  SmiUntag(dst);
}

void TurboAssembler::SmiUntagField(Register dst, const MemOperand& src) {
  SmiUntag(dst, src);
}

void TurboAssembler::StoreTaggedField(const Register& value,
                                      const MemOperand& dst_field_operand,
                                      const Register& scratch) {
  if (COMPRESS_POINTERS_BOOL) {
    RecordComment("[ StoreTagged");
    StoreU32(value, dst_field_operand);
    RecordComment("]");
  } else {
    StoreU64(value, dst_field_operand, scratch);
  }
}

void TurboAssembler::DecompressTaggedSigned(Register destination,
                                            Register src) {
  RecordComment("[ DecompressTaggedSigned");
  llgfr(destination, src);
  RecordComment("]");
}

void TurboAssembler::DecompressTaggedSigned(Register destination,
                                            MemOperand field_operand) {
  RecordComment("[ DecompressTaggedSigned");
  llgf(destination, field_operand);
  RecordComment("]");
}

void TurboAssembler::DecompressTaggedPointer(Register destination,
                                             Register source) {
  RecordComment("[ DecompressTaggedPointer");
  llgfr(destination, source);
  agr(destination, kRootRegister);
  RecordComment("]");
}

void TurboAssembler::DecompressTaggedPointer(Register destination,
                                             MemOperand field_operand) {
  RecordComment("[ DecompressTaggedPointer");
  llgf(destination, field_operand);
  agr(destination, kRootRegister);
  RecordComment("]");
}

void TurboAssembler::DecompressAnyTagged(Register destination,
                                         MemOperand field_operand) {
  RecordComment("[ DecompressAnyTagged");
  llgf(destination, field_operand);
  agr(destination, kRootRegister);
  RecordComment("]");
}

void TurboAssembler::DecompressAnyTagged(Register destination,
                                         Register source) {
  RecordComment("[ DecompressAnyTagged");
  llgfr(destination, source);
  agr(destination, kRootRegister);
  RecordComment("]");
}
void MacroAssembler::RecordWriteField(Register object, int offset,
                                      Register value, Register dst,
                                      LinkRegisterStatus lr_status,
                                      SaveFPRegsMode save_fp,
                                      RememberedSetAction remembered_set_action,
                                      SmiCheck smi_check) {
  // First, check if a write barrier is even needed. The tests below
  // catch stores of Smis.
  Label done;

  // Skip barrier if writing a smi.
  if (smi_check == SmiCheck::kInline) {
    JumpIfSmi(value, &done);
  }

  // Although the object register is tagged, the offset is relative to the start
  // of the object, so so offset must be a multiple of kSystemPointerSize.
  DCHECK(IsAligned(offset, kTaggedSize));

  lay(dst, MemOperand(object, offset - kHeapObjectTag));
  if (FLAG_debug_code) {
    Label ok;
    AndP(r0, dst, Operand(kTaggedSize - 1));
    beq(&ok, Label::kNear);
    stop();
    bind(&ok);
  }

  RecordWrite(object, dst, value, lr_status, save_fp, remembered_set_action,
              SmiCheck::kOmit);

  bind(&done);

  // Clobber clobbered input registers when running with the debug-code flag
  // turned on to provoke errors.
  if (FLAG_debug_code) {
    mov(value, Operand(bit_cast<intptr_t>(kZapValue + 4)));
    mov(dst, Operand(bit_cast<intptr_t>(kZapValue + 8)));
  }
}

void TurboAssembler::SaveRegisters(RegList registers) {
  DCHECK_GT(NumRegs(registers), 0);
  RegList regs = 0;
  for (int i = 0; i < Register::kNumRegisters; ++i) {
    if ((registers >> i) & 1u) {
      regs |= Register::from_code(i).bit();
    }
  }
  MultiPush(regs);
}

void TurboAssembler::RestoreRegisters(RegList registers) {
  DCHECK_GT(NumRegs(registers), 0);
  RegList regs = 0;
  for (int i = 0; i < Register::kNumRegisters; ++i) {
    if ((registers >> i) & 1u) {
      regs |= Register::from_code(i).bit();
    }
  }
  MultiPop(regs);
}

void TurboAssembler::CallEphemeronKeyBarrier(Register object, Register address,
                                             SaveFPRegsMode fp_mode) {
  WriteBarrierDescriptor descriptor;
  RegList registers = descriptor.allocatable_registers();

  SaveRegisters(registers);

  Register object_parameter(
      descriptor.GetRegisterParameter(WriteBarrierDescriptor::kObject));
  Register slot_parameter(
      descriptor.GetRegisterParameter(WriteBarrierDescriptor::kSlotAddress));

  Push(object);
  Push(address);

  Pop(slot_parameter);
  Pop(object_parameter);

  Call(isolate()->builtins()->builtin_handle(
           Builtins::GetEphemeronKeyBarrierStub(fp_mode)),
       RelocInfo::CODE_TARGET);
  RestoreRegisters(registers);
}

void TurboAssembler::CallRecordWriteStub(
    Register object, Register address,
    RememberedSetAction remembered_set_action, SaveFPRegsMode fp_mode,
    StubCallMode mode) {
  WriteBarrierDescriptor descriptor;
  RegList registers = descriptor.allocatable_registers();
  SaveRegisters(registers);

  Register object_parameter(
      descriptor.GetRegisterParameter(WriteBarrierDescriptor::kObject));
  Register slot_parameter(
      descriptor.GetRegisterParameter(WriteBarrierDescriptor::kSlotAddress));

  Push(object);
  Push(address);

  Pop(slot_parameter);
  Pop(object_parameter);

  if (mode == StubCallMode::kCallWasmRuntimeStub) {
    auto wasm_target =
        wasm::WasmCode::GetRecordWriteStub(remembered_set_action, fp_mode);
    Call(wasm_target, RelocInfo::WASM_STUB_CALL);
  } else {
    auto builtin_index =
        Builtins::GetRecordWriteStub(remembered_set_action, fp_mode);
    if (options().inline_offheap_trampolines) {
      RecordCommentForOffHeapTrampoline(builtin_index);
      CHECK_NE(builtin_index, Builtins::kNoBuiltinId);
      EmbeddedData d = EmbeddedData::FromBlob();
      Address entry = d.InstructionStartOfBuiltin(builtin_index);
      mov(ip, Operand(entry, RelocInfo::OFF_HEAP_TARGET));
      Call(ip);
    } else {
      Handle<Code> code_target =
          isolate()->builtins()->builtin_handle(builtin_index);
      Call(code_target, RelocInfo::CODE_TARGET);
    }
  }

  RestoreRegisters(registers);
}

// Will clobber 4 registers: object, address, scratch, ip.  The
// register 'object' contains a heap object pointer.  The heap object
// tag is shifted away.
void MacroAssembler::RecordWrite(Register object, Register address,
                                 Register value, LinkRegisterStatus lr_status,
                                 SaveFPRegsMode fp_mode,
                                 RememberedSetAction remembered_set_action,
                                 SmiCheck smi_check) {
  DCHECK(object != value);
  if (FLAG_debug_code) {
    LoadTaggedPointerField(r0, MemOperand(address));
    CmpS64(value, r0);
    Check(eq, AbortReason::kWrongAddressOrValuePassedToRecordWrite);
  }

  if ((remembered_set_action == RememberedSetAction::kOmit &&
       !FLAG_incremental_marking) ||
      FLAG_disable_write_barriers) {
    return;
  }
  // First, check if a write barrier is even needed. The tests below
  // catch stores of smis and stores into the young generation.
  Label done;

  if (smi_check == SmiCheck::kInline) {
    JumpIfSmi(value, &done);
  }

  CheckPageFlag(value,
                value,  // Used as scratch.
                MemoryChunk::kPointersToHereAreInterestingMask, eq, &done);
  CheckPageFlag(object,
                value,  // Used as scratch.
                MemoryChunk::kPointersFromHereAreInterestingMask, eq, &done);

  // Record the actual write.
  if (lr_status == kLRHasNotBeenSaved) {
    push(r14);
  }
  CallRecordWriteStub(object, address, remembered_set_action, fp_mode);
  if (lr_status == kLRHasNotBeenSaved) {
    pop(r14);
  }

  bind(&done);

  // Clobber clobbered registers when running with the debug-code flag
  // turned on to provoke errors.
  if (FLAG_debug_code) {
    mov(address, Operand(bit_cast<intptr_t>(kZapValue + 12)));
    mov(value, Operand(bit_cast<intptr_t>(kZapValue + 16)));
  }
}

void TurboAssembler::PushCommonFrame(Register marker_reg) {
  int fp_delta = 0;
  CleanseP(r14);
  if (marker_reg.is_valid()) {
    Push(r14, fp, marker_reg);
    fp_delta = 1;
  } else {
    Push(r14, fp);
    fp_delta = 0;
  }
  la(fp, MemOperand(sp, fp_delta * kSystemPointerSize));
}

void TurboAssembler::PopCommonFrame(Register marker_reg) {
  if (marker_reg.is_valid()) {
    Pop(r14, fp, marker_reg);
  } else {
    Pop(r14, fp);
  }
}

void TurboAssembler::PushStandardFrame(Register function_reg) {
  int fp_delta = 0;
  CleanseP(r14);
  if (function_reg.is_valid()) {
    Push(r14, fp, cp, function_reg);
    fp_delta = 2;
  } else {
    Push(r14, fp, cp);
    fp_delta = 1;
  }
  la(fp, MemOperand(sp, fp_delta * kSystemPointerSize));
  Push(kJavaScriptCallArgCountRegister);
}

void TurboAssembler::RestoreFrameStateForTailCall() {
  // if (FLAG_enable_embedded_constant_pool) {
  //   LoadU64(kConstantPoolRegister,
  //         MemOperand(fp, StandardFrameConstants::kConstantPoolOffset));
  //   set_constant_pool_available(false);
  // }
  DCHECK(!FLAG_enable_embedded_constant_pool);
  LoadU64(r14, MemOperand(fp, StandardFrameConstants::kCallerPCOffset));
  LoadU64(fp, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
}

void TurboAssembler::CanonicalizeNaN(const DoubleRegister dst,
                                     const DoubleRegister src) {
  // Turn potential sNaN into qNaN
  if (dst != src) ldr(dst, src);
  lzdr(kDoubleRegZero);
  sdbr(dst, kDoubleRegZero);
}

void TurboAssembler::ConvertIntToDouble(DoubleRegister dst, Register src) {
  cdfbr(dst, src);
}

void TurboAssembler::ConvertUnsignedIntToDouble(DoubleRegister dst,
                                                Register src) {
  if (CpuFeatures::IsSupported(FLOATING_POINT_EXT)) {
    cdlfbr(Condition(5), Condition(0), dst, src);
  } else {
    // zero-extend src
    llgfr(src, src);
    // convert to double
    cdgbr(dst, src);
  }
}

void TurboAssembler::ConvertIntToFloat(DoubleRegister dst, Register src) {
  cefbra(Condition(4), dst, src);
}

void TurboAssembler::ConvertUnsignedIntToFloat(DoubleRegister dst,
                                               Register src) {
  celfbr(Condition(4), Condition(0), dst, src);
}

void TurboAssembler::ConvertInt64ToFloat(DoubleRegister double_dst,
                                         Register src) {
  cegbr(double_dst, src);
}

void TurboAssembler::ConvertInt64ToDouble(DoubleRegister double_dst,
                                          Register src) {
  cdgbr(double_dst, src);
}

void TurboAssembler::ConvertUnsignedInt64ToFloat(DoubleRegister double_dst,
                                                 Register src) {
  celgbr(Condition(0), Condition(0), double_dst, src);
}

void TurboAssembler::ConvertUnsignedInt64ToDouble(DoubleRegister double_dst,
                                                  Register src) {
  cdlgbr(Condition(0), Condition(0), double_dst, src);
}

void TurboAssembler::ConvertFloat32ToInt64(const Register dst,
                                           const DoubleRegister double_input,
                                           FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      UNIMPLEMENTED();
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
  cgebr(m, dst, double_input);
}

void TurboAssembler::ConvertDoubleToInt64(const Register dst,
                                          const DoubleRegister double_input,
                                          FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      UNIMPLEMENTED();
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
  cgdbr(m, dst, double_input);
}

void TurboAssembler::ConvertDoubleToInt32(const Register dst,
                                          const DoubleRegister double_input,
                                          FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      m = Condition(4);
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
#ifdef V8_TARGET_ARCH_S390X
  lghi(dst, Operand::Zero());
#endif
  cfdbr(m, dst, double_input);
}

void TurboAssembler::ConvertFloat32ToInt32(const Register result,
                                           const DoubleRegister double_input,
                                           FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      m = Condition(4);
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
#ifdef V8_TARGET_ARCH_S390X
  lghi(result, Operand::Zero());
#endif
  cfebr(m, result, double_input);
}

void TurboAssembler::ConvertFloat32ToUnsignedInt32(
    const Register result, const DoubleRegister double_input,
    FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      UNIMPLEMENTED();
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
#ifdef V8_TARGET_ARCH_S390X
  lghi(result, Operand::Zero());
#endif
  clfebr(m, Condition(0), result, double_input);
}

void TurboAssembler::ConvertFloat32ToUnsignedInt64(
    const Register result, const DoubleRegister double_input,
    FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      UNIMPLEMENTED();
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
  clgebr(m, Condition(0), result, double_input);
}

void TurboAssembler::ConvertDoubleToUnsignedInt64(
    const Register dst, const DoubleRegister double_input,
    FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      UNIMPLEMENTED();
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
  clgdbr(m, Condition(0), dst, double_input);
}

void TurboAssembler::ConvertDoubleToUnsignedInt32(
    const Register dst, const DoubleRegister double_input,
    FPRoundingMode rounding_mode) {
  Condition m = Condition(0);
  switch (rounding_mode) {
    case kRoundToZero:
      m = Condition(5);
      break;
    case kRoundToNearest:
      UNIMPLEMENTED();
      break;
    case kRoundToPlusInf:
      m = Condition(6);
      break;
    case kRoundToMinusInf:
      m = Condition(7);
      break;
    default:
      UNIMPLEMENTED();
      break;
  }
#ifdef V8_TARGET_ARCH_S390X
  lghi(dst, Operand::Zero());
#endif
  clfdbr(m, Condition(0), dst, double_input);
}

void TurboAssembler::MovDoubleToInt64(Register dst, DoubleRegister src) {
  lgdr(dst, src);
}

void TurboAssembler::MovInt64ToDouble(DoubleRegister dst, Register src) {
  ldgr(dst, src);
}

void TurboAssembler::StubPrologue(StackFrame::Type type, Register base,
                                  int prologue_offset) {
  {
    ConstantPoolUnavailableScope constant_pool_unavailable(this);
    mov(r1, Operand(StackFrame::TypeToMarker(type)));
    PushCommonFrame(r1);
  }
}

void TurboAssembler::Prologue(Register base, int prologue_offset) {
  DCHECK(base != no_reg);
  PushStandardFrame(r3);
}

void TurboAssembler::EnterFrame(StackFrame::Type type,
                                bool load_constant_pool_pointer_reg) {
  // We create a stack frame with:
  //    Return Addr <-- old sp
  //    Old FP      <-- new fp
  //    CP
  //    type
  //    CodeObject  <-- new sp

  mov(ip, Operand(StackFrame::TypeToMarker(type)));
  PushCommonFrame(ip);
}

int TurboAssembler::LeaveFrame(StackFrame::Type type, int stack_adjustment) {
  // Drop the execution stack down to the frame pointer and restore
  // the caller frame pointer, return address and constant pool pointer.
  LoadU64(r14, MemOperand(fp, StandardFrameConstants::kCallerPCOffset));
  if (is_int20(StandardFrameConstants::kCallerSPOffset + stack_adjustment)) {
    lay(r1, MemOperand(fp, StandardFrameConstants::kCallerSPOffset +
                               stack_adjustment));
  } else {
    AddS64(r1, fp,
           Operand(StandardFrameConstants::kCallerSPOffset + stack_adjustment));
  }
  LoadU64(fp, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  mov(sp, r1);
  int frame_ends = pc_offset();
  return frame_ends;
}

// ExitFrame layout (probably wrongish.. needs updating)
//
//  SP -> previousSP
//        LK reserved
//        sp_on_exit (for debug?)
// oldSP->prev SP
//        LK
//        <parameters on stack>

// Prior to calling EnterExitFrame, we've got a bunch of parameters
// on the stack that we need to wrap a real frame around.. so first
// we reserve a slot for LK and push the previous SP which is captured
// in the fp register (r11)
// Then - we buy a new frame

// r14
// oldFP <- newFP
// SP
// Floats
// gaps
// Args
// ABIRes <- newSP
void MacroAssembler::EnterExitFrame(bool save_doubles, int stack_space,
                                    StackFrame::Type frame_type) {
  DCHECK(frame_type == StackFrame::EXIT ||
         frame_type == StackFrame::BUILTIN_EXIT);
  // Set up the frame structure on the stack.
  DCHECK_EQ(2 * kSystemPointerSize, ExitFrameConstants::kCallerSPDisplacement);
  DCHECK_EQ(1 * kSystemPointerSize, ExitFrameConstants::kCallerPCOffset);
  DCHECK_EQ(0 * kSystemPointerSize, ExitFrameConstants::kCallerFPOffset);
  DCHECK_GT(stack_space, 0);

  // This is an opportunity to build a frame to wrap
  // all of the pushes that have happened inside of V8
  // since we were called from C code
  CleanseP(r14);
  mov(r1, Operand(StackFrame::TypeToMarker(frame_type)));
  PushCommonFrame(r1);
  // Reserve room for saved entry sp.
  lay(sp, MemOperand(fp, -ExitFrameConstants::kFixedFrameSizeFromFp));

  if (FLAG_debug_code) {
    StoreU64(MemOperand(fp, ExitFrameConstants::kSPOffset), Operand::Zero(),
             r1);
  }

  // Save the frame pointer and the context in top.
  Move(r1, ExternalReference::Create(IsolateAddressId::kCEntryFPAddress,
                                     isolate()));
  StoreU64(fp, MemOperand(r1));
  Move(r1,
       ExternalReference::Create(IsolateAddressId::kContextAddress, isolate()));
  StoreU64(cp, MemOperand(r1));

  // Optionally save all volatile double registers.
  if (save_doubles) {
    MultiPushDoubles(kCallerSavedDoubles);
    // Note that d0 will be accessible at
    //   fp - ExitFrameConstants::kFrameSize -
    //   kNumCallerSavedDoubles * kDoubleSize,
    // since the sp slot and code slot were pushed after the fp.
  }

  lay(sp, MemOperand(sp, -stack_space * kSystemPointerSize));

  // Allocate and align the frame preparing for calling the runtime
  // function.
  const int frame_alignment = TurboAssembler::ActivationFrameAlignment();
  if (frame_alignment > 0) {
    DCHECK_EQ(frame_alignment, 8);
    ClearRightImm(sp, sp, Operand(3));  // equivalent to &= -8
  }

  lay(sp, MemOperand(sp, -kNumRequiredStackFrameSlots * kSystemPointerSize));
  StoreU64(MemOperand(sp), Operand::Zero(), r0);
  // Set the exit frame sp value to point just before the return address
  // location.
  lay(r1, MemOperand(sp, kStackFrameSPSlot * kSystemPointerSize));
  StoreU64(r1, MemOperand(fp, ExitFrameConstants::kSPOffset));
}

int TurboAssembler::ActivationFrameAlignment() {
#if !defined(USE_SIMULATOR)
  // Running on the real platform. Use the alignment as mandated by the local
  // environment.
  // Note: This will break if we ever start generating snapshots on one S390
  // platform for another S390 platform with a different alignment.
  return base::OS::ActivationFrameAlignment();
#else  // Simulated
  // If we are using the simulator then we should always align to the expected
  // alignment. As the simulator is used to generate snapshots we do not know
  // if the target platform will need alignment, so this is controlled from a
  // flag.
  return FLAG_sim_stack_alignment;
#endif
}

void MacroAssembler::LeaveExitFrame(bool save_doubles, Register argument_count,
                                    bool argument_count_is_length) {
  // Optionally restore all double registers.
  if (save_doubles) {
    // Calculate the stack location of the saved doubles and restore them.
    const int kNumRegs = kNumCallerSavedDoubles;
    lay(r5, MemOperand(fp, -(ExitFrameConstants::kFixedFrameSizeFromFp +
                             kNumRegs * kDoubleSize)));
    MultiPopDoubles(kCallerSavedDoubles, r5);
  }

  // Clear top frame.
  Move(ip, ExternalReference::Create(IsolateAddressId::kCEntryFPAddress,
                                     isolate()));
  StoreU64(MemOperand(ip), Operand(0, RelocInfo::NONE), r0);

  // Restore current context from top and clear it in debug mode.
  Move(ip,
       ExternalReference::Create(IsolateAddressId::kContextAddress, isolate()));
  LoadU64(cp, MemOperand(ip));

#ifdef DEBUG
  mov(r1, Operand(Context::kInvalidContext));
  Move(ip,
       ExternalReference::Create(IsolateAddressId::kContextAddress, isolate()));
  StoreU64(r1, MemOperand(ip));
#endif

  // Tear down the exit frame, pop the arguments, and return.
  LeaveFrame(StackFrame::EXIT);

  if (argument_count.is_valid()) {
    if (!argument_count_is_length) {
      ShiftLeftU64(argument_count, argument_count,
                   Operand(kSystemPointerSizeLog2));
    }
    la(sp, MemOperand(sp, argument_count));
  }
}

void TurboAssembler::MovFromFloatResult(const DoubleRegister dst) {
  Move(dst, d0);
}

void TurboAssembler::MovFromFloatParameter(const DoubleRegister dst) {
  Move(dst, d0);
}

void TurboAssembler::PrepareForTailCall(Register callee_args_count,
                                        Register caller_args_count,
                                        Register scratch0, Register scratch1) {
  DCHECK(!AreAliased(callee_args_count, caller_args_count, scratch0, scratch1));

  // Calculate the end of destination area where we will put the arguments
  // after we drop current frame. We AddS64 kSystemPointerSize to count the
  // receiver argument which is not included into formal parameters count.
  Register dst_reg = scratch0;
  ShiftLeftU64(dst_reg, caller_args_count, Operand(kSystemPointerSizeLog2));
  AddS64(dst_reg, fp, dst_reg);
  AddS64(dst_reg, dst_reg,
         Operand(StandardFrameConstants::kCallerSPOffset + kSystemPointerSize));

  Register src_reg = caller_args_count;
  // Calculate the end of source area. +kSystemPointerSize is for the receiver.
  ShiftLeftU64(src_reg, callee_args_count, Operand(kSystemPointerSizeLog2));
  AddS64(src_reg, sp, src_reg);
  AddS64(src_reg, src_reg, Operand(kSystemPointerSize));

  if (FLAG_debug_code) {
    CmpU64(src_reg, dst_reg);
    Check(lt, AbortReason::kStackAccessBelowStackPointer);
  }

  // Restore caller's frame pointer and return address now as they will be
  // overwritten by the copying loop.
  RestoreFrameStateForTailCall();

  // Now copy callee arguments to the caller frame going backwards to avoid
  // callee arguments corruption (source and destination areas could overlap).

  // Both src_reg and dst_reg are pointing to the word after the one to copy,
  // so they must be pre-decremented in the loop.
  Register tmp_reg = scratch1;
  Label loop;
  AddS64(tmp_reg, callee_args_count, Operand(1));  // +1 for receiver
  mov(r1, tmp_reg);
  bind(&loop);
  LoadU64(tmp_reg, MemOperand(src_reg, -kSystemPointerSize));
  StoreU64(tmp_reg, MemOperand(dst_reg, -kSystemPointerSize));
  lay(src_reg, MemOperand(src_reg, -kSystemPointerSize));
  lay(dst_reg, MemOperand(dst_reg, -kSystemPointerSize));
  BranchOnCount(r1, &loop);

  // Leave current frame.
  mov(sp, dst_reg);
}

MemOperand MacroAssembler::StackLimitAsMemOperand(StackLimitKind kind) {
  DCHECK(root_array_available());
  Isolate* isolate = this->isolate();
  ExternalReference limit =
      kind == StackLimitKind::kRealStackLimit
          ? ExternalReference::address_of_real_jslimit(isolate)
          : ExternalReference::address_of_jslimit(isolate);
  DCHECK(TurboAssembler::IsAddressableThroughRootRegister(isolate, limit));

  intptr_t offset =
      TurboAssembler::RootRegisterOffsetForExternalReference(isolate, limit);
  CHECK(is_int32(offset));
  return MemOperand(kRootRegister, offset);
}

void MacroAssembler::StackOverflowCheck(Register num_args, Register scratch,
                                        Label* stack_overflow) {
  // Check the stack for overflow. We are not trying to catch
  // interruptions (e.g. debug break and preemption) here, so the "real stack
  // limit" is checked.
  LoadU64(scratch, StackLimitAsMemOperand(StackLimitKind::kRealStackLimit));
  // Make scratch the space we have left. The stack might already be overflowed
  // here which will cause scratch to become negative.
  SubS64(scratch, sp, scratch);
  // Check if the arguments will overflow the stack.
  ShiftLeftU64(r0, num_args, Operand(kSystemPointerSizeLog2));
  CmpS64(scratch, r0);
  ble(stack_overflow);  // Signed comparison.
}

void MacroAssembler::InvokePrologue(Register expected_parameter_count,
                                    Register actual_parameter_count,
                                    Label* done, InvokeType type) {
  Label regular_invoke;

  //  r2: actual arguments count
  //  r3: function (passed through to callee)
  //  r4: expected arguments count

  DCHECK_EQ(actual_parameter_count, r2);
  DCHECK_EQ(expected_parameter_count, r4);

  // If the expected parameter count is equal to the adaptor sentinel, no need
  // to push undefined value as arguments.
  CmpS64(expected_parameter_count, Operand(kDontAdaptArgumentsSentinel));
  beq(&regular_invoke);

  // If overapplication or if the actual argument count is equal to the
  // formal parameter count, no need to push extra undefined values.
  SubS64(expected_parameter_count, expected_parameter_count,
         actual_parameter_count);
  ble(&regular_invoke);

  Label stack_overflow;
  Register scratch = r6;
  StackOverflowCheck(expected_parameter_count, scratch, &stack_overflow);

  // Underapplication. Move the arguments already in the stack, including the
  // receiver and the return address.
  {
    Label copy, check;
    Register num = r7, src = r8, dest = ip;  // r7 and r8 are context and root.
    mov(src, sp);
    // Update stack pointer.
    ShiftLeftU64(scratch, expected_parameter_count,
                 Operand(kSystemPointerSizeLog2));
    SubS64(sp, sp, scratch);
    mov(dest, sp);
    ltgr(num, actual_parameter_count);
    b(&check);
    bind(&copy);
    LoadU64(r0, MemOperand(src));
    lay(src, MemOperand(src, kSystemPointerSize));
    StoreU64(r0, MemOperand(dest));
    lay(dest, MemOperand(dest, kSystemPointerSize));
    SubS64(num, num, Operand(1));
    bind(&check);
    b(ge, &copy);
  }

  // Fill remaining expected arguments with undefined values.
  LoadRoot(scratch, RootIndex::kUndefinedValue);
  {
    Label loop;
    bind(&loop);
    StoreU64(scratch, MemOperand(ip));
    lay(ip, MemOperand(ip, kSystemPointerSize));
    SubS64(expected_parameter_count, expected_parameter_count, Operand(1));
    bgt(&loop);
  }
  b(&regular_invoke);

  bind(&stack_overflow);
  {
    FrameScope frame(this,
                     has_frame() ? StackFrame::NONE : StackFrame::INTERNAL);
    CallRuntime(Runtime::kThrowStackOverflow);
    bkpt(0);
  }

  bind(&regular_invoke);
}

void MacroAssembler::CheckDebugHook(Register fun, Register new_target,
                                    Register expected_parameter_count,
                                    Register actual_parameter_count) {
  Label skip_hook;

  ExternalReference debug_hook_active =
      ExternalReference::debug_hook_on_function_call_address(isolate());
  Move(r6, debug_hook_active);
  tm(MemOperand(r6), Operand(0xFF));
  beq(&skip_hook);

  {
    // Load receiver to pass it later to DebugOnFunctionCall hook.
    LoadReceiver(r6, actual_parameter_count);
    FrameScope frame(this,
                     has_frame() ? StackFrame::NONE : StackFrame::INTERNAL);

    SmiTag(expected_parameter_count);
    Push(expected_parameter_count);

    SmiTag(actual_parameter_count);
    Push(actual_parameter_count);

    if (new_target.is_valid()) {
      Push(new_target);
    }
    Push(fun, fun, r6);
    CallRuntime(Runtime::kDebugOnFunctionCall);
    Pop(fun);
    if (new_target.is_valid()) {
      Pop(new_target);
    }

    Pop(actual_parameter_count);
    SmiUntag(actual_parameter_count);

    Pop(expected_parameter_count);
    SmiUntag(expected_parameter_count);
  }
  bind(&skip_hook);
}

void MacroAssembler::InvokeFunctionCode(Register function, Register new_target,
                                        Register expected_parameter_count,
                                        Register actual_parameter_count,
                                        InvokeType type) {
  // You can't call a function without a valid frame.
  DCHECK_IMPLIES(type == InvokeType::kCall, has_frame());
  DCHECK_EQ(function, r3);
  DCHECK_IMPLIES(new_target.is_valid(), new_target == r5);

  // On function call, call into the debugger if necessary.
  CheckDebugHook(function, new_target, expected_parameter_count,
                 actual_parameter_count);

  // Clear the new.target register if not given.
  if (!new_target.is_valid()) {
    LoadRoot(r5, RootIndex::kUndefinedValue);
  }

  Label done;
  InvokePrologue(expected_parameter_count, actual_parameter_count, &done, type);
  // We call indirectly through the code field in the function to
  // allow recompilation to take effect without changing any of the
  // call sites.
  Register code = kJavaScriptCallCodeStartRegister;
  LoadTaggedPointerField(code,
                         FieldMemOperand(function, JSFunction::kCodeOffset));
  switch (type) {
    case InvokeType::kCall:
      CallCodeObject(code);
      break;
    case InvokeType::kJump:
      JumpCodeObject(code);
      break;
  }
  // Continue here if InvokePrologue does handle the invocation due to
  // mismatched parameter counts.
  bind(&done);
}

void MacroAssembler::InvokeFunctionWithNewTarget(
    Register fun, Register new_target, Register actual_parameter_count,
    InvokeType type) {
  // You can't call a function without a valid frame.
  DCHECK_IMPLIES(type == InvokeType::kCall, has_frame());

  // Contract with called JS functions requires that function is passed in r3.
  DCHECK_EQ(fun, r3);

  Register expected_reg = r4;
  Register temp_reg = r6;
  LoadTaggedPointerField(cp, FieldMemOperand(fun, JSFunction::kContextOffset));
  LoadTaggedPointerField(
      temp_reg, FieldMemOperand(fun, JSFunction::kSharedFunctionInfoOffset));
  LoadU16(
      expected_reg,
      FieldMemOperand(temp_reg,
                      SharedFunctionInfo::kFormalParameterCountOffset));

  InvokeFunctionCode(fun, new_target, expected_reg, actual_parameter_count,
                     type);
}

void MacroAssembler::InvokeFunction(Register function,
                                    Register expected_parameter_count,
                                    Register actual_parameter_count,
                                    InvokeType type) {
  // You can't call a function without a valid frame.
  DCHECK_IMPLIES(type == InvokeType::kCall, has_frame());

  // Contract with called JS functions requires that function is passed in r3.
  DCHECK_EQ(function, r3);

  // Get the function and setup the context.
  LoadTaggedPointerField(cp,
                         FieldMemOperand(function, JSFunction::kContextOffset));

  InvokeFunctionCode(r3, no_reg, expected_parameter_count,
                     actual_parameter_count, type);
}

void MacroAssembler::PushStackHandler() {
  // Adjust this code if not the case.
  STATIC_ASSERT(StackHandlerConstants::kSize == 2 * kSystemPointerSize);
  STATIC_ASSERT(StackHandlerConstants::kNextOffset == 0 * kSystemPointerSize);

  // Link the current handler as the next handler.
  Move(r7,
       ExternalReference::Create(IsolateAddressId::kHandlerAddress, isolate()));

  // Buy the full stack frame for 5 slots.
  lay(sp, MemOperand(sp, -StackHandlerConstants::kSize));

  // Store padding.
  lghi(r0, Operand::Zero());
  StoreU64(r0, MemOperand(sp));  // Padding.

  // Copy the old handler into the next handler slot.
  MoveChar(MemOperand(sp, StackHandlerConstants::kNextOffset), MemOperand(r7),
           Operand(kSystemPointerSize));
  // Set this new handler as the current one.
  StoreU64(sp, MemOperand(r7));
}

void MacroAssembler::PopStackHandler() {
  STATIC_ASSERT(StackHandlerConstants::kSize == 2 * kSystemPointerSize);
  STATIC_ASSERT(StackHandlerConstants::kNextOffset == 0);

  // Pop the Next Handler into r3 and store it into Handler Address reference.
  Pop(r3);
  Move(ip,
       ExternalReference::Create(IsolateAddressId::kHandlerAddress, isolate()));
  StoreU64(r3, MemOperand(ip));

  Drop(1);  // Drop padding.
}

void MacroAssembler::CompareObjectType(Register object, Register map,
                                       Register type_reg, InstanceType type) {
  const Register temp = type_reg == no_reg ? r0 : type_reg;

  LoadMap(map, object);
  CompareInstanceType(map, temp, type);
}

void MacroAssembler::CompareInstanceType(Register map, Register type_reg,
                                         InstanceType type) {
  STATIC_ASSERT(Map::kInstanceTypeOffset < 4096);
  STATIC_ASSERT(LAST_TYPE <= 0xFFFF);
  LoadS16(type_reg, FieldMemOperand(map, Map::kInstanceTypeOffset));
  CmpS64(type_reg, Operand(type));
}

void MacroAssembler::CompareInstanceTypeRange(Register map, Register type_reg,
                                              InstanceType lower_limit,
                                              InstanceType higher_limit) {
  DCHECK_LT(lower_limit, higher_limit);
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  LoadU16(type_reg, FieldMemOperand(map, Map::kInstanceTypeOffset));
  mov(scratch, type_reg);
  slgfi(scratch, Operand(lower_limit));
  CmpU64(scratch, Operand(higher_limit - lower_limit));
}

void MacroAssembler::CompareRoot(Register obj, RootIndex index) {
  int32_t offset = RootRegisterOffsetForRootIndex(index);
#ifdef V8_TARGET_BIG_ENDIAN
  offset += (COMPRESS_POINTERS_BOOL ? kTaggedSize : 0);
#endif
  CompareTagged(obj, MemOperand(kRootRegister, offset));
}

void MacroAssembler::JumpIfIsInRange(Register value, unsigned lower_limit,
                                     unsigned higher_limit,
                                     Label* on_in_range) {
  if (lower_limit != 0) {
    Register scratch = r0;
    mov(scratch, value);
    slgfi(scratch, Operand(lower_limit));
    CmpU64(scratch, Operand(higher_limit - lower_limit));
  } else {
    CmpU64(value, Operand(higher_limit));
  }
  ble(on_in_range);
}

void TurboAssembler::TruncateDoubleToI(Isolate* isolate, Zone* zone,
                                       Register result,
                                       DoubleRegister double_input,
                                       StubCallMode stub_mode) {
  Label done;

  TryInlineTruncateDoubleToI(result, double_input, &done);

  // If we fell through then inline version didn't succeed - call stub instead.
  push(r14);
  // Put input on stack.
  lay(sp, MemOperand(sp, -kDoubleSize));
  StoreF64(double_input, MemOperand(sp));

#if V8_ENABLE_WEBASSEMBLY
  if (stub_mode == StubCallMode::kCallWasmRuntimeStub) {
    Call(wasm::WasmCode::kDoubleToI, RelocInfo::WASM_STUB_CALL);
#else
  // For balance.
  if (false) {
#endif  // V8_ENABLE_WEBASSEMBLY
  } else {
    Call(BUILTIN_CODE(isolate, DoubleToI), RelocInfo::CODE_TARGET);
  }

  LoadU64(result, MemOperand(sp, 0));
  la(sp, MemOperand(sp, kDoubleSize));
  pop(r14);

  bind(&done);
}

void TurboAssembler::TryInlineTruncateDoubleToI(Register result,
                                                DoubleRegister double_input,
                                                Label* done) {
  ConvertDoubleToInt64(result, double_input);

  // Test for overflow
  TestIfInt32(result);
  beq(done);
}

void MacroAssembler::CallRuntime(const Runtime::Function* f, int num_arguments,
                                 SaveFPRegsMode save_doubles) {
  // All parameters are on the stack.  r2 has the return value after call.

  // If the expected number of arguments of the runtime function is
  // constant, we check that the actual number of arguments match the
  // expectation.
  CHECK(f->nargs < 0 || f->nargs == num_arguments);

  // TODO(1236192): Most runtime routines don't need the number of
  // arguments passed in because it is constant. At some point we
  // should remove this need and make the runtime routine entry code
  // smarter.
  mov(r2, Operand(num_arguments));
  Move(r3, ExternalReference::Create(f));
#if V8_TARGET_ARCH_S390X
  Handle<Code> code =
      CodeFactory::CEntry(isolate(), f->result_size, save_doubles);
#else
  Handle<Code> code = CodeFactory::CEntry(isolate(), 1, save_doubles);
#endif

  Call(code, RelocInfo::CODE_TARGET);
}

void MacroAssembler::TailCallRuntime(Runtime::FunctionId fid) {
  const Runtime::Function* function = Runtime::FunctionForId(fid);
  DCHECK_EQ(1, function->result_size);
  if (function->nargs >= 0) {
    mov(r2, Operand(function->nargs));
  }
  JumpToExternalReference(ExternalReference::Create(fid));
}

void MacroAssembler::JumpToExternalReference(const ExternalReference& builtin,
                                             bool builtin_exit_frame) {
  Move(r3, builtin);
  Handle<Code> code = CodeFactory::CEntry(isolate(), 1, SaveFPRegsMode::kIgnore,
                                          ArgvMode::kStack, builtin_exit_frame);
  Jump(code, RelocInfo::CODE_TARGET);
}

void MacroAssembler::JumpToInstructionStream(Address entry) {
  mov(kOffHeapTrampolineRegister, Operand(entry, RelocInfo::OFF_HEAP_TARGET));
  Jump(kOffHeapTrampolineRegister);
}

void MacroAssembler::LoadWeakValue(Register out, Register in,
                                   Label* target_if_cleared) {
  CmpS32(in, Operand(kClearedWeakHeapObjectLower32));
  beq(target_if_cleared);

  AndP(out, in, Operand(~kWeakHeapObjectMask));
}

void MacroAssembler::IncrementCounter(StatsCounter* counter, int value,
                                      Register scratch1, Register scratch2) {
  DCHECK(value > 0 && is_int8(value));
  if (FLAG_native_code_counters && counter->Enabled()) {
    Move(scratch2, ExternalReference::Create(counter));
    // @TODO(john.yan): can be optimized by asi()
    LoadS32(scratch1, MemOperand(scratch2));
    AddS64(scratch1, Operand(value));
    StoreU32(scratch1, MemOperand(scratch2));
  }
}

void MacroAssembler::DecrementCounter(StatsCounter* counter, int value,
                                      Register scratch1, Register scratch2) {
  DCHECK(value > 0 && is_int8(value));
  if (FLAG_native_code_counters && counter->Enabled()) {
    Move(scratch2, ExternalReference::Create(counter));
    // @TODO(john.yan): can be optimized by asi()
    LoadS32(scratch1, MemOperand(scratch2));
    AddS64(scratch1, Operand(-value));
    StoreU32(scratch1, MemOperand(scratch2));
  }
}

void TurboAssembler::Assert(Condition cond, AbortReason reason, CRegister cr) {
  if (FLAG_debug_code) Check(cond, reason, cr);
}

void TurboAssembler::AssertUnreachable(AbortReason reason) {
  if (FLAG_debug_code) Abort(reason);
}

void TurboAssembler::Check(Condition cond, AbortReason reason, CRegister cr) {
  Label L;
  b(cond, &L);
  Abort(reason);
  // will not return here
  bind(&L);
}

void TurboAssembler::Abort(AbortReason reason) {
  Label abort_start;
  bind(&abort_start);
  if (FLAG_code_comments) {
    const char* msg = GetAbortReason(reason);
    RecordComment("Abort message: ");
    RecordComment(msg);
  }

  // Avoid emitting call to builtin if requested.
  if (trap_on_abort()) {
    stop();
    return;
  }

  if (should_abort_hard()) {
    // We don't care if we constructed a frame. Just pretend we did.
    FrameScope assume_frame(this, StackFrame::NONE);
    lgfi(r2, Operand(static_cast<int>(reason)));
    PrepareCallCFunction(1, 0, r3);
    Move(r3, ExternalReference::abort_with_reason());
    // Use Call directly to avoid any unneeded overhead. The function won't
    // return anyway.
    Call(r3);
    return;
  }

  LoadSmiLiteral(r3, Smi::FromInt(static_cast<int>(reason)));

  // Disable stub call restrictions to always allow calls to abort.
  if (!has_frame_) {
    // We don't actually want to generate a pile of code for this, so just
    // claim there is a stack frame, without generating one.
    FrameScope scope(this, StackFrame::NONE);
    Call(BUILTIN_CODE(isolate(), Abort), RelocInfo::CODE_TARGET);
  } else {
    Call(BUILTIN_CODE(isolate(), Abort), RelocInfo::CODE_TARGET);
  }
  // will not return here
}

void TurboAssembler::LoadMap(Register destination, Register object) {
  LoadTaggedPointerField(destination,
                         FieldMemOperand(object, HeapObject::kMapOffset));
}

void MacroAssembler::LoadNativeContextSlot(Register dst, int index) {
  LoadMap(dst, cp);
  LoadTaggedPointerField(
      dst, FieldMemOperand(
               dst, Map::kConstructorOrBackPointerOrNativeContextOffset));
  LoadTaggedPointerField(dst, MemOperand(dst, Context::SlotOffset(index)));
}

void MacroAssembler::AssertNotSmi(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object);
    Check(ne, AbortReason::kOperandIsASmi, cr0);
  }
}

void MacroAssembler::AssertSmi(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object);
    Check(eq, AbortReason::kOperandIsNotASmi, cr0);
  }
}

void MacroAssembler::AssertConstructor(Register object, Register scratch) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object);
    Check(ne, AbortReason::kOperandIsASmiAndNotAConstructor);
    LoadMap(scratch, object);
    tm(FieldMemOperand(scratch, Map::kBitFieldOffset),
       Operand(Map::Bits1::IsConstructorBit::kMask));
    Check(ne, AbortReason::kOperandIsNotAConstructor);
  }
}

void MacroAssembler::AssertFunction(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object);
    Check(ne, AbortReason::kOperandIsASmiAndNotAFunction, cr0);
    push(object);
    LoadMap(object, object);
    CompareInstanceTypeRange(object, object, FIRST_JS_FUNCTION_TYPE,
                             LAST_JS_FUNCTION_TYPE);
    pop(object);
    Check(le, AbortReason::kOperandIsNotAFunction);
  }
}

void MacroAssembler::AssertBoundFunction(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object);
    Check(ne, AbortReason::kOperandIsASmiAndNotABoundFunction, cr0);
    push(object);
    CompareObjectType(object, object, object, JS_BOUND_FUNCTION_TYPE);
    pop(object);
    Check(eq, AbortReason::kOperandIsNotABoundFunction);
  }
}

void MacroAssembler::AssertGeneratorObject(Register object) {
  if (!FLAG_debug_code) return;
  TestIfSmi(object);
  Check(ne, AbortReason::kOperandIsASmiAndNotAGeneratorObject, cr0);

  // Load map
  Register map = object;
  push(object);
  LoadMap(map, object);

  // Check if JSGeneratorObject
  Label do_check;
  Register instance_type = object;
  CompareInstanceType(map, instance_type, JS_GENERATOR_OBJECT_TYPE);
  beq(&do_check);

  // Check if JSAsyncFunctionObject (See MacroAssembler::CompareInstanceType)
  CmpS64(instance_type, Operand(JS_ASYNC_FUNCTION_OBJECT_TYPE));
  beq(&do_check);

  // Check if JSAsyncGeneratorObject (See MacroAssembler::CompareInstanceType)
  CmpS64(instance_type, Operand(JS_ASYNC_GENERATOR_OBJECT_TYPE));

  bind(&do_check);
  // Restore generator object to register and perform assertion
  pop(object);
  Check(eq, AbortReason::kOperandIsNotAGeneratorObject);
}

void MacroAssembler::AssertUndefinedOrAllocationSite(Register object,
                                                     Register scratch) {
  if (FLAG_debug_code) {
    Label done_checking;
    AssertNotSmi(object);
    CompareRoot(object, RootIndex::kUndefinedValue);
    beq(&done_checking, Label::kNear);
    LoadMap(scratch, object);
    CompareInstanceType(scratch, scratch, ALLOCATION_SITE_TYPE);
    Assert(eq, AbortReason::kExpectedUndefinedOrCell);
    bind(&done_checking);
  }
}

static const int kRegisterPassedArguments = 5;

int TurboAssembler::CalculateStackPassedWords(int num_reg_arguments,
                                              int num_double_arguments) {
  int stack_passed_words = 0;
  if (num_double_arguments > DoubleRegister::kNumRegisters) {
    stack_passed_words +=
        2 * (num_double_arguments - DoubleRegister::kNumRegisters);
  }
  // Up to five simple arguments are passed in registers r2..r6
  if (num_reg_arguments > kRegisterPassedArguments) {
    stack_passed_words += num_reg_arguments - kRegisterPassedArguments;
  }
  return stack_passed_words;
}

void TurboAssembler::PrepareCallCFunction(int num_reg_arguments,
                                          int num_double_arguments,
                                          Register scratch) {
  int frame_alignment = ActivationFrameAlignment();
  int stack_passed_arguments =
      CalculateStackPassedWords(num_reg_arguments, num_double_arguments);
  int stack_space = kNumRequiredStackFrameSlots;
  if (frame_alignment > kSystemPointerSize) {
    // Make stack end at alignment and make room for stack arguments
    // -- preserving original value of sp.
    mov(scratch, sp);
    lay(sp, MemOperand(sp, -(stack_passed_arguments + 1) * kSystemPointerSize));
    DCHECK(base::bits::IsPowerOfTwo(frame_alignment));
    ClearRightImm(sp, sp,
                  Operand(base::bits::WhichPowerOfTwo(frame_alignment)));
    StoreU64(scratch,
             MemOperand(sp, (stack_passed_arguments)*kSystemPointerSize));
  } else {
    stack_space += stack_passed_arguments;
  }
  lay(sp, MemOperand(sp, (-stack_space) * kSystemPointerSize));
}

void TurboAssembler::PrepareCallCFunction(int num_reg_arguments,
                                          Register scratch) {
  PrepareCallCFunction(num_reg_arguments, 0, scratch);
}

void TurboAssembler::MovToFloatParameter(DoubleRegister src) { Move(d0, src); }

void TurboAssembler::MovToFloatResult(DoubleRegister src) { Move(d0, src); }

void TurboAssembler::MovToFloatParameters(DoubleRegister src1,
                                          DoubleRegister src2) {
  if (src2 == d0) {
    DCHECK(src1 != d2);
    Move(d2, src2);
    Move(d0, src1);
  } else {
    Move(d0, src1);
    Move(d2, src2);
  }
}

void TurboAssembler::CallCFunction(ExternalReference function,
                                   int num_reg_arguments,
                                   int num_double_arguments) {
  Move(ip, function);
  CallCFunctionHelper(ip, num_reg_arguments, num_double_arguments);
}

void TurboAssembler::CallCFunction(Register function, int num_reg_arguments,
                                   int num_double_arguments) {
  CallCFunctionHelper(function, num_reg_arguments, num_double_arguments);
}

void TurboAssembler::CallCFunction(ExternalReference function,
                                   int num_arguments) {
  CallCFunction(function, num_arguments, 0);
}

void TurboAssembler::CallCFunction(Register function, int num_arguments) {
  CallCFunction(function, num_arguments, 0);
}

void TurboAssembler::CallCFunctionHelper(Register function,
                                         int num_reg_arguments,
                                         int num_double_arguments) {
  DCHECK_LE(num_reg_arguments + num_double_arguments, kMaxCParameters);
  DCHECK(has_frame());

  // Save the frame pointer and PC so that the stack layout remains iterable,
  // even without an ExitFrame which normally exists between JS and C frames.
  Register addr_scratch = r1;
  // See x64 code for reasoning about how to address the isolate data fields.
  if (root_array_available()) {
    LoadPC(r0);
    StoreU64(r0, MemOperand(kRootRegister,
                            IsolateData::fast_c_call_caller_pc_offset()));
    StoreU64(fp, MemOperand(kRootRegister,
                            IsolateData::fast_c_call_caller_fp_offset()));
  } else {
    DCHECK_NOT_NULL(isolate());

    Move(addr_scratch,
         ExternalReference::fast_c_call_caller_pc_address(isolate()));
    LoadPC(r0);
    StoreU64(r0, MemOperand(addr_scratch));
    Move(addr_scratch,
         ExternalReference::fast_c_call_caller_fp_address(isolate()));
    StoreU64(fp, MemOperand(addr_scratch));
  }

  // Just call directly. The function called cannot cause a GC, or
  // allow preemption, so the return address in the link register
  // stays correct.
  Register dest = function;
  if (ABI_CALL_VIA_IP) {
    Move(ip, function);
    dest = ip;
  }

  Call(dest);

  // We don't unset the PC; the FP is the source of truth.
  Register zero_scratch = r0;
  lghi(zero_scratch, Operand::Zero());

  if (root_array_available()) {
    StoreU64(
        zero_scratch,
        MemOperand(kRootRegister, IsolateData::fast_c_call_caller_fp_offset()));
  } else {
    DCHECK_NOT_NULL(isolate());
    Move(addr_scratch,
         ExternalReference::fast_c_call_caller_fp_address(isolate()));
    StoreU64(zero_scratch, MemOperand(addr_scratch));
  }

  int stack_passed_arguments =
      CalculateStackPassedWords(num_reg_arguments, num_double_arguments);
  int stack_space = kNumRequiredStackFrameSlots + stack_passed_arguments;
  if (ActivationFrameAlignment() > kSystemPointerSize) {
    // Load the original stack pointer (pre-alignment) from the stack
    LoadU64(sp, MemOperand(sp, stack_space * kSystemPointerSize));
  } else {
    la(sp, MemOperand(sp, stack_space * kSystemPointerSize));
  }
}

void TurboAssembler::CheckPageFlag(
    Register object,
    Register scratch,  // scratch may be same register as object
    int mask, Condition cc, Label* condition_met) {
  DCHECK(cc == ne || cc == eq);
  ClearRightImm(scratch, object, Operand(kPageSizeBits));

  if (base::bits::IsPowerOfTwo(mask)) {
    // If it's a power of two, we can use Test-Under-Mask Memory-Imm form
    // which allows testing of a single byte in memory.
    int32_t byte_offset = 4;
    uint32_t shifted_mask = mask;
    // Determine the byte offset to be tested
    if (mask <= 0x80) {
      byte_offset = kSystemPointerSize - 1;
    } else if (mask < 0x8000) {
      byte_offset = kSystemPointerSize - 2;
      shifted_mask = mask >> 8;
    } else if (mask < 0x800000) {
      byte_offset = kSystemPointerSize - 3;
      shifted_mask = mask >> 16;
    } else {
      byte_offset = kSystemPointerSize - 4;
      shifted_mask = mask >> 24;
    }
#if V8_TARGET_LITTLE_ENDIAN
    // Reverse the byte_offset if emulating on little endian platform
    byte_offset = kSystemPointerSize - byte_offset - 1;
#endif
    tm(MemOperand(scratch, BasicMemoryChunk::kFlagsOffset + byte_offset),
       Operand(shifted_mask));
  } else {
    LoadU64(scratch, MemOperand(scratch, BasicMemoryChunk::kFlagsOffset));
    AndP(r0, scratch, Operand(mask));
  }
  // Should be okay to remove rc

  if (cc == ne) {
    bne(condition_met);
  }
  if (cc == eq) {
    beq(condition_met);
  }
}

Register GetRegisterThatIsNotOneOf(Register reg1, Register reg2, Register reg3,
                                   Register reg4, Register reg5,
                                   Register reg6) {
  RegList regs = 0;
  if (reg1.is_valid()) regs |= reg1.bit();
  if (reg2.is_valid()) regs |= reg2.bit();
  if (reg3.is_valid()) regs |= reg3.bit();
  if (reg4.is_valid()) regs |= reg4.bit();
  if (reg5.is_valid()) regs |= reg5.bit();
  if (reg6.is_valid()) regs |= reg6.bit();

  const RegisterConfiguration* config = RegisterConfiguration::Default();
  for (int i = 0; i < config->num_allocatable_general_registers(); ++i) {
    int code = config->GetAllocatableGeneralCode(i);
    Register candidate = Register::from_code(code);
    if (regs & candidate.bit()) continue;
    return candidate;
  }
  UNREACHABLE();
}

void TurboAssembler::mov(Register dst, Register src) { lgr(dst, src); }

void TurboAssembler::mov(Register dst, const Operand& src) {
  int64_t value = 0;

  if (src.is_heap_object_request()) {
    RequestHeapObject(src.heap_object_request());
  } else {
    value = src.immediate();
  }

  if (src.rmode() != RelocInfo::NONE) {
    // some form of relocation needed
    RecordRelocInfo(src.rmode(), value);
  }

  int32_t hi_32 = static_cast<int32_t>(value >> 32);
  int32_t lo_32 = static_cast<int32_t>(value);

  if (src.rmode() == RelocInfo::NONE) {
    if (hi_32 == 0) {
      if (is_uint16(lo_32)) {
        llill(dst, Operand(lo_32));
        return;
      }
      llilf(dst, Operand(lo_32));
      return;
    } else if (lo_32 == 0) {
      if (is_uint16(hi_32)) {
        llihl(dst, Operand(hi_32));
        return;
      }
      llihf(dst, Operand(hi_32));
      return;
    } else if (is_int16(value)) {
      lghi(dst, Operand(value));
      return;
    } else if (is_int32(value)) {
      lgfi(dst, Operand(value));
      return;
    }
  }

  iihf(dst, Operand(hi_32));
  iilf(dst, Operand(lo_32));
}

void TurboAssembler::MulS32(Register dst, const MemOperand& src1) {
  if (is_uint12(src1.offset())) {
    ms(dst, src1);
  } else if (is_int20(src1.offset())) {
    msy(dst, src1);
  } else {
    UNIMPLEMENTED();
  }
}

void TurboAssembler::MulS32(Register dst, Register src1) { msr(dst, src1); }

void TurboAssembler::MulS32(Register dst, const Operand& src1) {
  msfi(dst, src1);
}

#define Generate_MulHigh32(instr) \
  {                               \
    lgfr(dst, src1);              \
    instr(dst, src2);             \
    srlg(dst, dst, Operand(32));  \
  }

void TurboAssembler::MulHighS32(Register dst, Register src1,
                                const MemOperand& src2) {
  Generate_MulHigh32(msgf);
}

void TurboAssembler::MulHighS32(Register dst, Register src1, Register src2) {
  if (dst == src2) {
    std::swap(src1, src2);
  }
  Generate_MulHigh32(msgfr);
}

void TurboAssembler::MulHighS32(Register dst, Register src1,
                                const Operand& src2) {
  Generate_MulHigh32(msgfi);
}

#undef Generate_MulHigh32

#define Generate_MulHighU32(instr) \
  {                                \
    lr(r1, src1);                  \
    instr(r0, src2);               \
    LoadU32(dst, r0);               \
  }

void TurboAssembler::MulHighU32(Register dst, Register src1,
                                const MemOperand& src2) {
  Generate_MulHighU32(ml);
}

void TurboAssembler::MulHighU32(Register dst, Register src1, Register src2) {
  Generate_MulHighU32(mlr);
}

void TurboAssembler::MulHighU32(Register dst, Register src1,
                                const Operand& src2) {
  USE(dst);
  USE(src1);
  USE(src2);
  UNREACHABLE();
}

#undef Generate_MulHighU32

#define Generate_Mul32WithOverflowIfCCUnequal(instr) \
  {                                                  \
    lgfr(dst, src1);                                 \
    instr(dst, src2);                                \
    cgfr(dst, dst);                                  \
  }

void TurboAssembler::Mul32WithOverflowIfCCUnequal(Register dst, Register src1,
                                                  const MemOperand& src2) {
  Register result = dst;
  if (src2.rx() == dst || src2.rb() == dst) dst = r0;
  Generate_Mul32WithOverflowIfCCUnequal(msgf);
  if (result != dst) llgfr(result, dst);
}

void TurboAssembler::Mul32WithOverflowIfCCUnequal(Register dst, Register src1,
                                                  Register src2) {
  if (dst == src2) {
    std::swap(src1, src2);
  }
  Generate_Mul32WithOverflowIfCCUnequal(msgfr);
}

void TurboAssembler::Mul32WithOverflowIfCCUnequal(Register dst, Register src1,
                                                  const Operand& src2) {
  Generate_Mul32WithOverflowIfCCUnequal(msgfi);
}

#undef Generate_Mul32WithOverflowIfCCUnequal

#define Generate_Div32(instr) \
  {                           \
    lgfr(r1, src1);           \
    instr(r0, src2);          \
    LoadU32(dst, r1);          \
  }

void TurboAssembler::DivS32(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_Div32(dsgf);
}

void TurboAssembler::DivS32(Register dst, Register src1, Register src2) {
  Generate_Div32(dsgfr);
}

#undef Generate_Div32

#define Generate_DivU32(instr) \
  {                            \
    lr(r0, src1);              \
    srdl(r0, Operand(32));     \
    instr(r0, src2);           \
    LoadU32(dst, r1);           \
  }

void TurboAssembler::DivU32(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_DivU32(dl);
}

void TurboAssembler::DivU32(Register dst, Register src1, Register src2) {
  Generate_DivU32(dlr);
}

#undef Generate_DivU32

#define Generate_Div64(instr) \
  {                           \
    lgr(r1, src1);            \
    instr(r0, src2);          \
    lgr(dst, r1);             \
  }

void TurboAssembler::DivS64(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_Div64(dsg);
}

void TurboAssembler::DivS64(Register dst, Register src1, Register src2) {
  Generate_Div64(dsgr);
}

#undef Generate_Div64

#define Generate_DivU64(instr) \
  {                            \
    lgr(r1, src1);             \
    lghi(r0, Operand::Zero()); \
    instr(r0, src2);           \
    lgr(dst, r1);              \
  }

void TurboAssembler::DivU64(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_DivU64(dlg);
}

void TurboAssembler::DivU64(Register dst, Register src1, Register src2) {
  Generate_DivU64(dlgr);
}

#undef Generate_DivU64

#define Generate_Mod32(instr) \
  {                           \
    lgfr(r1, src1);           \
    instr(r0, src2);          \
    LoadU32(dst, r0);          \
  }

void TurboAssembler::ModS32(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_Mod32(dsgf);
}

void TurboAssembler::ModS32(Register dst, Register src1, Register src2) {
  Generate_Mod32(dsgfr);
}

#undef Generate_Mod32

#define Generate_ModU32(instr) \
  {                            \
    lr(r0, src1);              \
    srdl(r0, Operand(32));     \
    instr(r0, src2);           \
    LoadU32(dst, r0);           \
  }

void TurboAssembler::ModU32(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_ModU32(dl);
}

void TurboAssembler::ModU32(Register dst, Register src1, Register src2) {
  Generate_ModU32(dlr);
}

#undef Generate_ModU32

#define Generate_Mod64(instr) \
  {                           \
    lgr(r1, src1);            \
    instr(r0, src2);          \
    lgr(dst, r0);             \
  }

void TurboAssembler::ModS64(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_Mod64(dsg);
}

void TurboAssembler::ModS64(Register dst, Register src1, Register src2) {
  Generate_Mod64(dsgr);
}

#undef Generate_Mod64

#define Generate_ModU64(instr) \
  {                            \
    lgr(r1, src1);             \
    lghi(r0, Operand::Zero()); \
    instr(r0, src2);           \
    lgr(dst, r0);              \
  }

void TurboAssembler::ModU64(Register dst, Register src1,
                            const MemOperand& src2) {
  Generate_ModU64(dlg);
}

void TurboAssembler::ModU64(Register dst, Register src1, Register src2) {
  Generate_ModU64(dlgr);
}

#undef Generate_ModU64

void TurboAssembler::MulS64(Register dst, const Operand& opnd) {
  msgfi(dst, opnd);
}

void TurboAssembler::MulS64(Register dst, Register src) { msgr(dst, src); }

void TurboAssembler::MulS64(Register dst, const MemOperand& opnd) {
  msg(dst, opnd);
}

void TurboAssembler::Sqrt(DoubleRegister result, DoubleRegister input) {
  sqdbr(result, input);
}
void TurboAssembler::Sqrt(DoubleRegister result, const MemOperand& input) {
  if (is_uint12(input.offset())) {
    sqdb(result, input);
  } else {
    ldy(result, input);
    sqdbr(result, result);
  }
}
//----------------------------------------------------------------------------
//  Add Instructions
//----------------------------------------------------------------------------

// Add 32-bit (Register dst = Register dst + Immediate opnd)
void TurboAssembler::AddS32(Register dst, const Operand& opnd) {
  if (is_int16(opnd.immediate()))
    ahi(dst, opnd);
  else
    afi(dst, opnd);
}

// Add Pointer Size (Register dst = Register dst + Immediate opnd)
void TurboAssembler::AddS64(Register dst, const Operand& opnd) {
  if (is_int16(opnd.immediate()))
    aghi(dst, opnd);
  else
    agfi(dst, opnd);
}

void TurboAssembler::AddS32(Register dst, Register src, int32_t opnd) {
  AddS32(dst, src, Operand(opnd));
}

// Add 32-bit (Register dst = Register src + Immediate opnd)
void TurboAssembler::AddS32(Register dst, Register src, const Operand& opnd) {
  if (dst != src) {
    if (CpuFeatures::IsSupported(DISTINCT_OPS) && is_int16(opnd.immediate())) {
      ahik(dst, src, opnd);
      return;
    }
    lr(dst, src);
  }
  AddS32(dst, opnd);
}

void TurboAssembler::AddS64(Register dst, Register src, int32_t opnd) {
  AddS64(dst, src, Operand(opnd));
}

// Add Pointer Size (Register dst = Register src + Immediate opnd)
void TurboAssembler::AddS64(Register dst, Register src, const Operand& opnd) {
  if (dst != src) {
    if (CpuFeatures::IsSupported(DISTINCT_OPS) && is_int16(opnd.immediate())) {
      aghik(dst, src, opnd);
      return;
    }
    mov(dst, src);
  }
  AddS64(dst, opnd);
}

// Add 32-bit (Register dst = Register dst + Register src)
void TurboAssembler::AddS32(Register dst, Register src) { ar(dst, src); }

// Add Pointer Size (Register dst = Register dst + Register src)
void TurboAssembler::AddS64(Register dst, Register src) { agr(dst, src); }

// Add 32-bit (Register dst = Register src1 + Register src2)
void TurboAssembler::AddS32(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate AR/AGR, over the non clobbering ARK/AGRK
    // as AR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      ark(dst, src1, src2);
      return;
    } else {
      lr(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  ar(dst, src2);
}

// Add Pointer Size (Register dst = Register src1 + Register src2)
void TurboAssembler::AddS64(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate AR/AGR, over the non clobbering ARK/AGRK
    // as AR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      agrk(dst, src1, src2);
      return;
    } else {
      mov(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  agr(dst, src2);
}

// Add 32-bit (Register-Memory)
void TurboAssembler::AddS32(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    a(dst, opnd);
  else
    ay(dst, opnd);
}

// Add Pointer Size (Register-Memory)
void TurboAssembler::AddS64(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  ag(dst, opnd);
}

// Add 32-bit (Memory - Immediate)
void TurboAssembler::AddS32(const MemOperand& opnd, const Operand& imm) {
  DCHECK(is_int8(imm.immediate()));
  DCHECK(is_int20(opnd.offset()));
  DCHECK(CpuFeatures::IsSupported(GENERAL_INSTR_EXT));
  asi(opnd, imm);
}

// Add Pointer-sized (Memory - Immediate)
void TurboAssembler::AddS64(const MemOperand& opnd, const Operand& imm) {
  DCHECK(is_int8(imm.immediate()));
  DCHECK(is_int20(opnd.offset()));
  DCHECK(CpuFeatures::IsSupported(GENERAL_INSTR_EXT));
  agsi(opnd, imm);
}

//----------------------------------------------------------------------------
//  Add Logical Instructions
//----------------------------------------------------------------------------

// Add Logical 32-bit (Register dst = Register src1 + Register src2)
void TurboAssembler::AddU32(Register dst, Register src1, Register src2) {
  if (dst != src2 && dst != src1) {
    lr(dst, src1);
    alr(dst, src2);
  } else if (dst != src2) {
    // dst == src1
    DCHECK(dst == src1);
    alr(dst, src2);
  } else {
    // dst == src2
    DCHECK(dst == src2);
    alr(dst, src1);
  }
}

// Add Logical 32-bit (Register dst = Register dst + Immediate opnd)
void TurboAssembler::AddU32(Register dst, const Operand& imm) {
  alfi(dst, imm);
}

// Add Logical Pointer Size (Register dst = Register dst + Immediate opnd)
void TurboAssembler::AddU64(Register dst, const Operand& imm) {
  algfi(dst, imm);
}

// Add Logical 32-bit (Register-Memory)
void TurboAssembler::AddU32(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    al_z(dst, opnd);
  else
    aly(dst, opnd);
}

// Add Logical Pointer Size (Register-Memory)
void TurboAssembler::AddU64(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  alg(dst, opnd);
}

//----------------------------------------------------------------------------
//  Subtract Instructions
//----------------------------------------------------------------------------

// Subtract Logical 32-bit (Register dst = Register src1 - Register src2)
void TurboAssembler::SubU32(Register dst, Register src1, Register src2) {
  if (dst != src2 && dst != src1) {
    lr(dst, src1);
    slr(dst, src2);
  } else if (dst != src2) {
    // dst == src1
    DCHECK(dst == src1);
    slr(dst, src2);
  } else {
    // dst == src2
    DCHECK(dst == src2);
    lr(r0, dst);
    SubU32(dst, src1, r0);
  }
}

// Subtract 32-bit (Register dst = Register dst - Immediate opnd)
void TurboAssembler::SubS32(Register dst, const Operand& imm) {
  AddS32(dst, Operand(-(imm.immediate())));
}

// Subtract Pointer Size (Register dst = Register dst - Immediate opnd)
void TurboAssembler::SubS64(Register dst, const Operand& imm) {
  AddS64(dst, Operand(-(imm.immediate())));
}

void TurboAssembler::SubS32(Register dst, Register src, int32_t imm) {
  SubS32(dst, src, Operand(imm));
}

// Subtract 32-bit (Register dst = Register src - Immediate opnd)
void TurboAssembler::SubS32(Register dst, Register src, const Operand& imm) {
  AddS32(dst, src, Operand(-(imm.immediate())));
}

void TurboAssembler::SubS64(Register dst, Register src, int32_t imm) {
  SubS64(dst, src, Operand(imm));
}

// Subtract Pointer Sized (Register dst = Register src - Immediate opnd)
void TurboAssembler::SubS64(Register dst, Register src, const Operand& imm) {
  AddS64(dst, src, Operand(-(imm.immediate())));
}

// Subtract 32-bit (Register dst = Register dst - Register src)
void TurboAssembler::SubS32(Register dst, Register src) { sr(dst, src); }

// Subtract Pointer Size (Register dst = Register dst - Register src)
void TurboAssembler::SubS64(Register dst, Register src) { sgr(dst, src); }

// Subtract 32-bit (Register = Register - Register)
void TurboAssembler::SubS32(Register dst, Register src1, Register src2) {
  // Use non-clobbering version if possible
  if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
    srk(dst, src1, src2);
    return;
  }
  if (dst != src1 && dst != src2) lr(dst, src1);
  // In scenario where we have dst = src - dst, we need to swap and negate
  if (dst != src1 && dst == src2) {
    Label done;
    lcr(dst, dst);  // dst = -dst
    b(overflow, &done);
    ar(dst, src1);  // dst = dst + src
    bind(&done);
  } else {
    sr(dst, src2);
  }
}

// Subtract Pointer Sized (Register = Register - Register)
void TurboAssembler::SubS64(Register dst, Register src1, Register src2) {
  // Use non-clobbering version if possible
  if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
    sgrk(dst, src1, src2);
    return;
  }
  if (dst != src1 && dst != src2) mov(dst, src1);
  // In scenario where we have dst = src - dst, we need to swap and negate
  if (dst != src1 && dst == src2) {
    Label done;
    lcgr(dst, dst);  // dst = -dst
    b(overflow, &done);
    AddS64(dst, src1);  // dst = dst + src
    bind(&done);
  } else {
    SubS64(dst, src2);
  }
}

// Subtract 32-bit (Register-Memory)
void TurboAssembler::SubS32(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    s(dst, opnd);
  else
    sy(dst, opnd);
}

// Subtract Pointer Sized (Register - Memory)
void TurboAssembler::SubS64(Register dst, const MemOperand& opnd) {
#if V8_TARGET_ARCH_S390X
  sg(dst, opnd);
#else
  SubS32(dst, opnd);
#endif
}

void TurboAssembler::MovIntToFloat(DoubleRegister dst, Register src) {
  sllg(r0, src, Operand(32));
  ldgr(dst, r0);
}

void TurboAssembler::MovFloatToInt(Register dst, DoubleRegister src) {
  lgdr(dst, src);
  srlg(dst, dst, Operand(32));
}

// Load And Subtract 32-bit (similar to laa/lan/lao/lax)
void TurboAssembler::LoadAndSub32(Register dst, Register src,
                                  const MemOperand& opnd) {
  lcr(dst, src);
  laa(dst, dst, opnd);
}

void TurboAssembler::LoadAndSub64(Register dst, Register src,
                                  const MemOperand& opnd) {
  lcgr(dst, src);
  laag(dst, dst, opnd);
}

//----------------------------------------------------------------------------
//  Subtract Logical Instructions
//----------------------------------------------------------------------------

// Subtract Logical 32-bit (Register - Memory)
void TurboAssembler::SubU32(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    sl(dst, opnd);
  else
    sly(dst, opnd);
}

// Subtract Logical Pointer Sized (Register - Memory)
void TurboAssembler::SubU64(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
#if V8_TARGET_ARCH_S390X
  slgf(dst, opnd);
#else
  SubU32(dst, opnd);
#endif
}

//----------------------------------------------------------------------------
//  Bitwise Operations
//----------------------------------------------------------------------------

// AND 32-bit - dst = dst & src
void TurboAssembler::And(Register dst, Register src) { nr(dst, src); }

// AND Pointer Size - dst = dst & src
void TurboAssembler::AndP(Register dst, Register src) { ngr(dst, src); }

// Non-clobbering AND 32-bit - dst = src1 & src1
void TurboAssembler::And(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate XR/XGR, over the non clobbering XRK/XRK
    // as XR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      nrk(dst, src1, src2);
      return;
    } else {
      lr(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  And(dst, src2);
}

// Non-clobbering AND pointer size - dst = src1 & src1
void TurboAssembler::AndP(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate XR/XGR, over the non clobbering XRK/XRK
    // as XR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      ngrk(dst, src1, src2);
      return;
    } else {
      mov(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  AndP(dst, src2);
}

// AND 32-bit (Reg - Mem)
void TurboAssembler::And(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    n(dst, opnd);
  else
    ny(dst, opnd);
}

// AND Pointer Size (Reg - Mem)
void TurboAssembler::AndP(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
#if V8_TARGET_ARCH_S390X
  ng(dst, opnd);
#else
  And(dst, opnd);
#endif
}

// AND 32-bit - dst = dst & imm
void TurboAssembler::And(Register dst, const Operand& opnd) { nilf(dst, opnd); }

// AND Pointer Size - dst = dst & imm
void TurboAssembler::AndP(Register dst, const Operand& opnd) {
#if V8_TARGET_ARCH_S390X
  intptr_t value = opnd.immediate();
  if (value >> 32 != -1) {
    // this may not work b/c condition code won't be set correctly
    nihf(dst, Operand(value >> 32));
  }
  nilf(dst, Operand(value & 0xFFFFFFFF));
#else
  And(dst, opnd);
#endif
}

// AND 32-bit - dst = src & imm
void TurboAssembler::And(Register dst, Register src, const Operand& opnd) {
  if (dst != src) lr(dst, src);
  nilf(dst, opnd);
}

// AND Pointer Size - dst = src & imm
void TurboAssembler::AndP(Register dst, Register src, const Operand& opnd) {
  // Try to exploit RISBG first
  intptr_t value = opnd.immediate();
  if (CpuFeatures::IsSupported(GENERAL_INSTR_EXT)) {
    intptr_t shifted_value = value;
    int trailing_zeros = 0;

    // We start checking how many trailing zeros are left at the end.
    while ((0 != shifted_value) && (0 == (shifted_value & 1))) {
      trailing_zeros++;
      shifted_value >>= 1;
    }

    // If temp (value with right-most set of zeros shifted out) is 1 less
    // than power of 2, we have consecutive bits of 1.
    // Special case: If shift_value is zero, we cannot use RISBG, as it requires
    //               selection of at least 1 bit.
    if ((0 != shifted_value) && base::bits::IsPowerOfTwo(shifted_value + 1)) {
      int startBit =
          base::bits::CountLeadingZeros64(shifted_value) - trailing_zeros;
      int endBit = 63 - trailing_zeros;
      // Start: startBit, End: endBit, Shift = 0, true = zero unselected bits.
      RotateInsertSelectBits(dst, src, Operand(startBit), Operand(endBit),
                             Operand::Zero(), true);
      return;
    } else if (-1 == shifted_value) {
      // A Special case in which all top bits up to MSB are 1's.  In this case,
      // we can set startBit to be 0.
      int endBit = 63 - trailing_zeros;
      RotateInsertSelectBits(dst, src, Operand::Zero(), Operand(endBit),
                             Operand::Zero(), true);
      return;
    }
  }

  // If we are &'ing zero, we can just whack the dst register and skip copy
  if (dst != src && (0 != value)) mov(dst, src);
  AndP(dst, opnd);
}

// OR 32-bit - dst = dst & src
void TurboAssembler::Or(Register dst, Register src) { or_z(dst, src); }

// OR Pointer Size - dst = dst & src
void TurboAssembler::OrP(Register dst, Register src) { ogr(dst, src); }

// Non-clobbering OR 32-bit - dst = src1 & src1
void TurboAssembler::Or(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate XR/XGR, over the non clobbering XRK/XRK
    // as XR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      ork(dst, src1, src2);
      return;
    } else {
      lr(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  Or(dst, src2);
}

// Non-clobbering OR pointer size - dst = src1 & src1
void TurboAssembler::OrP(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate XR/XGR, over the non clobbering XRK/XRK
    // as XR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      ogrk(dst, src1, src2);
      return;
    } else {
      mov(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  OrP(dst, src2);
}

// OR 32-bit (Reg - Mem)
void TurboAssembler::Or(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    o(dst, opnd);
  else
    oy(dst, opnd);
}

// OR Pointer Size (Reg - Mem)
void TurboAssembler::OrP(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
#if V8_TARGET_ARCH_S390X
  og(dst, opnd);
#else
  Or(dst, opnd);
#endif
}

// OR 32-bit - dst = dst & imm
void TurboAssembler::Or(Register dst, const Operand& opnd) { oilf(dst, opnd); }

// OR Pointer Size - dst = dst & imm
void TurboAssembler::OrP(Register dst, const Operand& opnd) {
#if V8_TARGET_ARCH_S390X
  intptr_t value = opnd.immediate();
  if (value >> 32 != 0) {
    // this may not work b/c condition code won't be set correctly
    oihf(dst, Operand(value >> 32));
  }
  oilf(dst, Operand(value & 0xFFFFFFFF));
#else
  Or(dst, opnd);
#endif
}

// OR 32-bit - dst = src & imm
void TurboAssembler::Or(Register dst, Register src, const Operand& opnd) {
  if (dst != src) lr(dst, src);
  oilf(dst, opnd);
}

// OR Pointer Size - dst = src & imm
void TurboAssembler::OrP(Register dst, Register src, const Operand& opnd) {
  if (dst != src) mov(dst, src);
  OrP(dst, opnd);
}

// XOR 32-bit - dst = dst & src
void TurboAssembler::Xor(Register dst, Register src) { xr(dst, src); }

// XOR Pointer Size - dst = dst & src
void TurboAssembler::XorP(Register dst, Register src) { xgr(dst, src); }

// Non-clobbering XOR 32-bit - dst = src1 & src1
void TurboAssembler::Xor(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate XR/XGR, over the non clobbering XRK/XRK
    // as XR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      xrk(dst, src1, src2);
      return;
    } else {
      lr(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  Xor(dst, src2);
}

// Non-clobbering XOR pointer size - dst = src1 & src1
void TurboAssembler::XorP(Register dst, Register src1, Register src2) {
  if (dst != src1 && dst != src2) {
    // We prefer to generate XR/XGR, over the non clobbering XRK/XRK
    // as XR is a smaller instruction
    if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
      xgrk(dst, src1, src2);
      return;
    } else {
      mov(dst, src1);
    }
  } else if (dst == src2) {
    src2 = src1;
  }
  XorP(dst, src2);
}

// XOR 32-bit (Reg - Mem)
void TurboAssembler::Xor(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    x(dst, opnd);
  else
    xy(dst, opnd);
}

// XOR Pointer Size (Reg - Mem)
void TurboAssembler::XorP(Register dst, const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
#if V8_TARGET_ARCH_S390X
  xg(dst, opnd);
#else
  Xor(dst, opnd);
#endif
}

// XOR 32-bit - dst = dst & imm
void TurboAssembler::Xor(Register dst, const Operand& opnd) { xilf(dst, opnd); }

// XOR Pointer Size - dst = dst & imm
void TurboAssembler::XorP(Register dst, const Operand& opnd) {
#if V8_TARGET_ARCH_S390X
  intptr_t value = opnd.immediate();
  xihf(dst, Operand(value >> 32));
  xilf(dst, Operand(value & 0xFFFFFFFF));
#else
  Xor(dst, opnd);
#endif
}

// XOR 32-bit - dst = src & imm
void TurboAssembler::Xor(Register dst, Register src, const Operand& opnd) {
  if (dst != src) lr(dst, src);
  xilf(dst, opnd);
}

// XOR Pointer Size - dst = src & imm
void TurboAssembler::XorP(Register dst, Register src, const Operand& opnd) {
  if (dst != src) mov(dst, src);
  XorP(dst, opnd);
}

void TurboAssembler::Not32(Register dst, Register src) {
  if (src != no_reg && src != dst) lr(dst, src);
  xilf(dst, Operand(0xFFFFFFFF));
}

void TurboAssembler::Not64(Register dst, Register src) {
  if (src != no_reg && src != dst) lgr(dst, src);
  xihf(dst, Operand(0xFFFFFFFF));
  xilf(dst, Operand(0xFFFFFFFF));
}

void TurboAssembler::NotP(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  Not64(dst, src);
#else
  Not32(dst, src);
#endif
}

void TurboAssembler::LoadPositiveP(Register result, Register input) {
#if V8_TARGET_ARCH_S390X
  lpgr(result, input);
#else
  lpr(result, input);
#endif
}

void TurboAssembler::LoadPositive32(Register result, Register input) {
  lpr(result, input);
  lgfr(result, result);
}

//-----------------------------------------------------------------------------
//  Compare Helpers
//-----------------------------------------------------------------------------

// Compare 32-bit Register vs Register
void TurboAssembler::CmpS32(Register src1, Register src2) { cr_z(src1, src2); }

// Compare Pointer Sized Register vs Register
void TurboAssembler::CmpS64(Register src1, Register src2) { cgr(src1, src2); }

// Compare 32-bit Register vs Immediate
// This helper will set up proper relocation entries if required.
void TurboAssembler::CmpS32(Register dst, const Operand& opnd) {
  if (opnd.rmode() == RelocInfo::NONE) {
    intptr_t value = opnd.immediate();
    if (is_int16(value))
      chi(dst, opnd);
    else
      cfi(dst, opnd);
  } else {
    // Need to generate relocation record here
    RecordRelocInfo(opnd.rmode(), opnd.immediate());
    cfi(dst, opnd);
  }
}

// Compare Pointer Sized  Register vs Immediate
// This helper will set up proper relocation entries if required.
void TurboAssembler::CmpS64(Register dst, const Operand& opnd) {
  if (opnd.rmode() == RelocInfo::NONE) {
    cgfi(dst, opnd);
  } else {
    mov(r0, opnd);  // Need to generate 64-bit relocation
    cgr(dst, r0);
  }
}

// Compare 32-bit Register vs Memory
void TurboAssembler::CmpS32(Register dst, const MemOperand& opnd) {
  // make sure offset is within 20 bit range
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    c(dst, opnd);
  else
    cy(dst, opnd);
}

// Compare Pointer Size Register vs Memory
void TurboAssembler::CmpS64(Register dst, const MemOperand& opnd) {
  // make sure offset is within 20 bit range
  DCHECK(is_int20(opnd.offset()));
  cg(dst, opnd);
}

// Using cs or scy based on the offset
void TurboAssembler::CmpAndSwap(Register old_val, Register new_val,
                                const MemOperand& opnd) {
  if (is_uint12(opnd.offset())) {
    cs(old_val, new_val, opnd);
  } else {
    csy(old_val, new_val, opnd);
  }
}

void TurboAssembler::CmpAndSwap64(Register old_val, Register new_val,
                                  const MemOperand& opnd) {
  DCHECK(is_int20(opnd.offset()));
  csg(old_val, new_val, opnd);
}

//-----------------------------------------------------------------------------
// Compare Logical Helpers
//-----------------------------------------------------------------------------

// Compare Logical 32-bit Register vs Register
void TurboAssembler::CmpU32(Register dst, Register src) { clr(dst, src); }

// Compare Logical Pointer Sized Register vs Register
void TurboAssembler::CmpU64(Register dst, Register src) {
#ifdef V8_TARGET_ARCH_S390X
  clgr(dst, src);
#else
  CmpU32(dst, src);
#endif
}

// Compare Logical 32-bit Register vs Immediate
void TurboAssembler::CmpU32(Register dst, const Operand& opnd) {
  clfi(dst, opnd);
}

// Compare Logical Pointer Sized Register vs Immediate
void TurboAssembler::CmpU64(Register dst, const Operand& opnd) {
#if V8_TARGET_ARCH_S390X
  DCHECK_EQ(static_cast<uint32_t>(opnd.immediate() >> 32), 0);
  clgfi(dst, opnd);
#else
  CmpU32(dst, opnd);
#endif
}

// Compare Logical 32-bit Register vs Memory
void TurboAssembler::CmpU32(Register dst, const MemOperand& opnd) {
  // make sure offset is within 20 bit range
  DCHECK(is_int20(opnd.offset()));
  if (is_uint12(opnd.offset()))
    cl(dst, opnd);
  else
    cly(dst, opnd);
}

// Compare Logical Pointer Sized Register vs Memory
void TurboAssembler::CmpU64(Register dst, const MemOperand& opnd) {
  // make sure offset is within 20 bit range
  DCHECK(is_int20(opnd.offset()));
#if V8_TARGET_ARCH_S390X
  clg(dst, opnd);
#else
  CmpU32(dst, opnd);
#endif
}

void TurboAssembler::Branch(Condition c, const Operand& opnd) {
  intptr_t value = opnd.immediate();
  if (is_int16(value))
    brc(c, opnd);
  else
    brcl(c, opnd);
}

// Branch On Count.  Decrement R1, and branch if R1 != 0.
void TurboAssembler::BranchOnCount(Register r1, Label* l) {
  int32_t offset = branch_offset(l);
  if (is_int16(offset)) {
#if V8_TARGET_ARCH_S390X
    brctg(r1, Operand(offset));
#else
    brct(r1, Operand(offset));
#endif
  } else {
    AddS64(r1, Operand(-1));
    Branch(ne, Operand(offset));
  }
}

void TurboAssembler::LoadSmiLiteral(Register dst, Smi smi) {
  intptr_t value = static_cast<intptr_t>(smi.ptr());
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  llilf(dst, Operand(value));
#else
  DCHECK_EQ(value & 0xFFFFFFFF, 0);
  // The smi value is loaded in upper 32-bits.  Lower 32-bit are zeros.
  llihf(dst, Operand(value >> 32));
#endif
}

void TurboAssembler::CmpSmiLiteral(Register src1, Smi smi, Register scratch) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  // CFI takes 32-bit immediate.
  cfi(src1, Operand(smi));
#else
  if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
    cih(src1, Operand(static_cast<intptr_t>(smi.ptr()) >> 32));
  } else {
    LoadSmiLiteral(scratch, smi);
    cgr(src1, scratch);
  }
#endif
}

void TurboAssembler::LoadU64(Register dst, const MemOperand& mem,
                             Register scratch) {
  int offset = mem.offset();

  MemOperand src = mem;
  if (!is_int20(offset)) {
    DCHECK(scratch != no_reg && scratch != r0 && mem.rx() == r0);
    DCHECK(scratch != mem.rb());
    mov(scratch, Operand(offset));
    src = MemOperand(mem.rb(), scratch);
  }
  lg(dst, src);
}

// Store a "pointer" sized value to the memory location
void TurboAssembler::StoreU64(Register src, const MemOperand& mem,
                              Register scratch) {
  if (!is_int20(mem.offset())) {
    DCHECK(scratch != no_reg);
    DCHECK(scratch != r0);
    mov(scratch, Operand(mem.offset()));
    stg(src, MemOperand(mem.rb(), scratch));
  } else {
    stg(src, mem);
  }
}

// Store a "pointer" sized constant to the memory location
void TurboAssembler::StoreU64(const MemOperand& mem, const Operand& opnd,
                              Register scratch) {
  // Relocations not supported
  DCHECK_EQ(opnd.rmode(), RelocInfo::NONE);

  // Try to use MVGHI/MVHI
  if (CpuFeatures::IsSupported(GENERAL_INSTR_EXT) && is_uint12(mem.offset()) &&
      mem.getIndexRegister() == r0 && is_int16(opnd.immediate())) {
    mvghi(mem, opnd);
  } else {
    mov(scratch, opnd);
    StoreU64(scratch, mem);
  }
}

void TurboAssembler::LoadMultipleP(Register dst1, Register dst2,
                                   const MemOperand& mem) {
#if V8_TARGET_ARCH_S390X
  DCHECK(is_int20(mem.offset()));
  lmg(dst1, dst2, mem);
#else
  if (is_uint12(mem.offset())) {
    lm(dst1, dst2, mem);
  } else {
    DCHECK(is_int20(mem.offset()));
    lmy(dst1, dst2, mem);
  }
#endif
}

void TurboAssembler::StoreMultipleP(Register src1, Register src2,
                                    const MemOperand& mem) {
#if V8_TARGET_ARCH_S390X
  DCHECK(is_int20(mem.offset()));
  stmg(src1, src2, mem);
#else
  if (is_uint12(mem.offset())) {
    stm(src1, src2, mem);
  } else {
    DCHECK(is_int20(mem.offset()));
    stmy(src1, src2, mem);
  }
#endif
}

void TurboAssembler::LoadMultipleW(Register dst1, Register dst2,
                                   const MemOperand& mem) {
  if (is_uint12(mem.offset())) {
    lm(dst1, dst2, mem);
  } else {
    DCHECK(is_int20(mem.offset()));
    lmy(dst1, dst2, mem);
  }
}

void TurboAssembler::StoreMultipleW(Register src1, Register src2,
                                    const MemOperand& mem) {
  if (is_uint12(mem.offset())) {
    stm(src1, src2, mem);
  } else {
    DCHECK(is_int20(mem.offset()));
    stmy(src1, src2, mem);
  }
}

// Load 32-bits and sign extend if necessary.
void TurboAssembler::LoadS32(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  lgfr(dst, src);
#else
  if (dst != src) lr(dst, src);
#endif
}

// Load 32-bits and sign extend if necessary.
void TurboAssembler::LoadS32(Register dst, const MemOperand& mem,
                           Register scratch) {
  int offset = mem.offset();

  if (!is_int20(offset)) {
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
#if V8_TARGET_ARCH_S390X
    lgf(dst, MemOperand(mem.rb(), scratch));
#else
    l(dst, MemOperand(mem.rb(), scratch));
#endif
  } else {
#if V8_TARGET_ARCH_S390X
    lgf(dst, mem);
#else
    if (is_uint12(offset)) {
      l(dst, mem);
    } else {
      ly(dst, mem);
    }
#endif
  }
}

// Load 32-bits and zero extend if necessary.
void TurboAssembler::LoadU32(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  llgfr(dst, src);
#else
  if (dst != src) lr(dst, src);
#endif
}

// Variable length depending on whether offset fits into immediate field
// MemOperand of RX or RXY format
void TurboAssembler::LoadU32(Register dst, const MemOperand& mem,
                            Register scratch) {
  Register base = mem.rb();
  int offset = mem.offset();

#if V8_TARGET_ARCH_S390X
  if (is_int20(offset)) {
    llgf(dst, mem);
  } else if (scratch != no_reg) {
    // Materialize offset into scratch register.
    mov(scratch, Operand(offset));
    llgf(dst, MemOperand(base, scratch));
  } else {
    DCHECK(false);
  }
#else
  bool use_RXform = false;
  bool use_RXYform = false;
  if (is_uint12(offset)) {
    // RX-format supports unsigned 12-bits offset.
    use_RXform = true;
  } else if (is_int20(offset)) {
    // RXY-format supports signed 20-bits offset.
    use_RXYform = true;
  } else if (scratch != no_reg) {
    // Materialize offset into scratch register.
    mov(scratch, Operand(offset));
  } else {
    DCHECK(false);
  }

  if (use_RXform) {
    l(dst, mem);
  } else if (use_RXYform) {
    ly(dst, mem);
  } else {
    ly(dst, MemOperand(base, scratch));
  }
#endif
}

void TurboAssembler::LoadU16(Register dst, const MemOperand& mem) {
#if V8_TARGET_ARCH_S390X
  llgh(dst, mem);
#else
  llh(dst, mem);
#endif
}

void TurboAssembler::LoadU16(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  llghr(dst, src);
#else
  llhr(dst, src);
#endif
}

void TurboAssembler::LoadS8(Register dst, const MemOperand& mem) {
#if V8_TARGET_ARCH_S390X
  lgb(dst, mem);
#else
  lb(dst, mem);
#endif
}

void TurboAssembler::LoadS8(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  lgbr(dst, src);
#else
  lbr(dst, src);
#endif
}

void TurboAssembler::LoadU8(Register dst, const MemOperand& mem) {
#if V8_TARGET_ARCH_S390X
  llgc(dst, mem);
#else
  llc(dst, mem);
#endif
}

void TurboAssembler::LoadU8(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  llgcr(dst, src);
#else
  llcr(dst, src);
#endif
}

#ifdef V8_TARGET_BIG_ENDIAN
void TurboAssembler::LoadU64LE(Register dst, const MemOperand& mem,
                               Register scratch) {
  lrvg(dst, mem);
}

void TurboAssembler::LoadS32LE(Register dst, const MemOperand& opnd,
                               Register scratch) {
  lrv(dst, opnd);
  LoadS32(dst, dst);
}

void TurboAssembler::LoadU32LE(Register dst, const MemOperand& opnd,
                               Register scratch) {
  lrv(dst, opnd);
  LoadU32(dst, dst);
}

void TurboAssembler::LoadU16LE(Register dst, const MemOperand& opnd) {
  lrvh(dst, opnd);
  LoadU16(dst, dst);
}

void TurboAssembler::LoadS16LE(Register dst, const MemOperand& opnd) {
  lrvh(dst, opnd);
  LoadS16(dst, dst);
}

void TurboAssembler::LoadV128LE(DoubleRegister dst, const MemOperand& opnd,
                                Register scratch0, Register scratch1) {
  bool use_vlbr = CpuFeatures::IsSupported(VECTOR_ENHANCE_FACILITY_2) &&
                  is_uint12(opnd.offset());
  if (use_vlbr) {
    vlbr(dst, opnd, Condition(4));
  } else {
    lrvg(scratch0, opnd);
    lrvg(scratch1,
         MemOperand(opnd.rx(), opnd.rb(), opnd.offset() + kSystemPointerSize));
    vlvgp(dst, scratch1, scratch0);
  }
}

void TurboAssembler::LoadF64LE(DoubleRegister dst, const MemOperand& opnd,
                               Register scratch) {
  lrvg(scratch, opnd);
  ldgr(dst, scratch);
}

void TurboAssembler::LoadF32LE(DoubleRegister dst, const MemOperand& opnd,
                               Register scratch) {
  lrv(scratch, opnd);
  ShiftLeftU64(scratch, scratch, Operand(32));
  ldgr(dst, scratch);
}

void TurboAssembler::StoreU64LE(Register src, const MemOperand& mem,
                                Register scratch) {
  if (!is_int20(mem.offset())) {
    DCHECK(scratch != no_reg);
    DCHECK(scratch != r0);
    mov(scratch, Operand(mem.offset()));
    strvg(src, MemOperand(mem.rb(), scratch));
  } else {
    strvg(src, mem);
  }
}

void TurboAssembler::StoreU32LE(Register src, const MemOperand& mem,
                                Register scratch) {
  if (!is_int20(mem.offset())) {
    DCHECK(scratch != no_reg);
    DCHECK(scratch != r0);
    mov(scratch, Operand(mem.offset()));
    strv(src, MemOperand(mem.rb(), scratch));
  } else {
    strv(src, mem);
  }
}

void TurboAssembler::StoreU16LE(Register src, const MemOperand& mem,
                                Register scratch) {
  if (!is_int20(mem.offset())) {
    DCHECK(scratch != no_reg);
    DCHECK(scratch != r0);
    mov(scratch, Operand(mem.offset()));
    strvh(src, MemOperand(mem.rb(), scratch));
  } else {
    strvh(src, mem);
  }
}

void TurboAssembler::StoreF64LE(DoubleRegister src, const MemOperand& opnd,
                                Register scratch) {
  DCHECK(is_uint12(opnd.offset()));
  lgdr(scratch, src);
  strvg(scratch, opnd);
}

void TurboAssembler::StoreF32LE(DoubleRegister src, const MemOperand& opnd,
                                Register scratch) {
  DCHECK(is_uint12(opnd.offset()));
  lgdr(scratch, src);
  ShiftRightU64(scratch, scratch, Operand(32));
  strv(scratch, opnd);
}

void TurboAssembler::StoreV128LE(Simd128Register src, const MemOperand& mem,
                                 Register scratch1, Register scratch2) {
  if (CpuFeatures::IsSupported(VECTOR_ENHANCE_FACILITY_2)) {
    vstbr(src, mem, Condition(4));
  } else {
    vlgv(scratch1, src, MemOperand(r0, 1), Condition(3));
    vlgv(scratch2, src, MemOperand(r0, 0), Condition(3));
    strvg(scratch1, mem);
    strvg(scratch2,
          MemOperand(mem.rx(), mem.rb(), mem.offset() + kSystemPointerSize));
  }
}

#else
void TurboAssembler::LoadU64LE(Register dst, const MemOperand& mem,
                               Register scratch) {
  LoadU64(dst, mem, scratch);
}

void TurboAssembler::LoadS32LE(Register dst, const MemOperand& opnd,
                               Register scratch) {
  LoadS32(dst, opnd, scratch);
}

void TurboAssembler::LoadU32LE(Register dst, const MemOperand& opnd,
                               Register scratch) {
  LoadU32(dst, opnd, scratch);
}

void TurboAssembler::LoadU16LE(Register dst, const MemOperand& opnd) {
  LoadU16(dst, opnd);
}

void TurboAssembler::LoadS16LE(Register dst, const MemOperand& opnd) {
  LoadS16(dst, opnd);
}

void TurboAssembler::LoadV128LE(DoubleRegister dst, const MemOperand& opnd,
                                Register scratch0, Register scratch1) {
  USE(scratch1);
  LoadV128(dst, opnd, scratch0);
}

void TurboAssembler::LoadF64LE(DoubleRegister dst, const MemOperand& opnd,
                               Register scratch) {
  USE(scratch);
  LoadF64(dst, opnd);
}

void TurboAssembler::LoadF32LE(DoubleRegister dst, const MemOperand& opnd,
                               Register scratch) {
  USE(scratch);
  LoadF32(dst, opnd);
}

void TurboAssembler::StoreU64LE(Register src, const MemOperand& mem,
                                Register scratch) {
  StoreU64(src, mem, scratch);
}

void TurboAssembler::StoreU32LE(Register src, const MemOperand& mem,
                                Register scratch) {
  StoreU32(src, mem, scratch);
}

void TurboAssembler::StoreU16LE(Register src, const MemOperand& mem,
                                Register scratch) {
  StoreU16(src, mem, scratch);
}

void TurboAssembler::StoreF64LE(DoubleRegister src, const MemOperand& opnd,
                                Register scratch) {
  StoreF64(src, opnd);
}

void TurboAssembler::StoreF32LE(DoubleRegister src, const MemOperand& opnd,
                                Register scratch) {
  StoreF32(src, opnd);
}

void TurboAssembler::StoreV128LE(Simd128Register src, const MemOperand& mem,
                                 Register scratch1, Register scratch2) {
  StoreV128(src, mem, scratch1);
}

#endif

// Load And Test (Reg <- Reg)
void TurboAssembler::LoadAndTest32(Register dst, Register src) {
  ltr(dst, src);
}

// Load And Test Pointer Sized (Reg <- Reg)
void TurboAssembler::LoadAndTestP(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  ltgr(dst, src);
#else
  ltr(dst, src);
#endif
}

// Load And Test 32-bit (Reg <- Mem)
void TurboAssembler::LoadAndTest32(Register dst, const MemOperand& mem) {
  lt_z(dst, mem);
}

// Load And Test Pointer Sized (Reg <- Mem)
void TurboAssembler::LoadAndTestP(Register dst, const MemOperand& mem) {
#if V8_TARGET_ARCH_S390X
  ltg(dst, mem);
#else
  lt_z(dst, mem);
#endif
}

// Load On Condition Pointer Sized (Reg <- Reg)
void TurboAssembler::LoadOnConditionP(Condition cond, Register dst,
                                      Register src) {
#if V8_TARGET_ARCH_S390X
  locgr(cond, dst, src);
#else
  locr(cond, dst, src);
#endif
}

// Load Double Precision (64-bit) Floating Point number from memory
void TurboAssembler::LoadF64(DoubleRegister dst, const MemOperand& mem) {
  // for 32bit and 64bit we all use 64bit floating point regs
  if (is_uint12(mem.offset())) {
    ld(dst, mem);
  } else {
    ldy(dst, mem);
  }
}

// Load Single Precision (32-bit) Floating Point number from memory
void TurboAssembler::LoadF32(DoubleRegister dst, const MemOperand& mem) {
  if (is_uint12(mem.offset())) {
    le_z(dst, mem);
  } else {
    DCHECK(is_int20(mem.offset()));
    ley(dst, mem);
  }
}

void TurboAssembler::LoadV128(Simd128Register dst, const MemOperand& mem,
                              Register scratch) {
  if (is_uint12(mem.offset())) {
    vl(dst, mem, Condition(0));
  } else {
    DCHECK(is_int20(mem.offset()));
    lay(scratch, mem);
    vl(dst, MemOperand(scratch), Condition(0));
  }
}

// Store Double Precision (64-bit) Floating Point number to memory
void TurboAssembler::StoreF64(DoubleRegister dst, const MemOperand& mem) {
  if (is_uint12(mem.offset())) {
    std(dst, mem);
  } else {
    stdy(dst, mem);
  }
}

// Store Single Precision (32-bit) Floating Point number to memory
void TurboAssembler::StoreF32(DoubleRegister src, const MemOperand& mem) {
  if (is_uint12(mem.offset())) {
    ste(src, mem);
  } else {
    stey(src, mem);
  }
}

void TurboAssembler::StoreV128(Simd128Register src, const MemOperand& mem,
                               Register scratch) {
  if (is_uint12(mem.offset())) {
    vst(src, mem, Condition(0));
  } else {
    DCHECK(is_int20(mem.offset()));
    lay(scratch, mem);
    vst(src, MemOperand(scratch), Condition(0));
  }
}

void TurboAssembler::AddF32(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    aebr(dst, rhs);
  } else if (dst == rhs) {
    aebr(dst, lhs);
  } else {
    ler(dst, lhs);
    aebr(dst, rhs);
  }
}

void TurboAssembler::SubF32(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    sebr(dst, rhs);
  } else if (dst == rhs) {
    sebr(dst, lhs);
    lcebr(dst, dst);
  } else {
    ler(dst, lhs);
    sebr(dst, rhs);
  }
}

void TurboAssembler::MulF32(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    meebr(dst, rhs);
  } else if (dst == rhs) {
    meebr(dst, lhs);
  } else {
    ler(dst, lhs);
    meebr(dst, rhs);
  }
}

void TurboAssembler::DivF32(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    debr(dst, rhs);
  } else if (dst == rhs) {
    lay(sp, MemOperand(sp, -kSystemPointerSize));
    StoreF32(dst, MemOperand(sp));
    ler(dst, lhs);
    deb(dst, MemOperand(sp));
    la(sp, MemOperand(sp, kSystemPointerSize));
  } else {
    ler(dst, lhs);
    debr(dst, rhs);
  }
}

void TurboAssembler::AddF64(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    adbr(dst, rhs);
  } else if (dst == rhs) {
    adbr(dst, lhs);
  } else {
    ldr(dst, lhs);
    adbr(dst, rhs);
  }
}

void TurboAssembler::SubF64(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    sdbr(dst, rhs);
  } else if (dst == rhs) {
    sdbr(dst, lhs);
    lcdbr(dst, dst);
  } else {
    ldr(dst, lhs);
    sdbr(dst, rhs);
  }
}

void TurboAssembler::MulF64(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    mdbr(dst, rhs);
  } else if (dst == rhs) {
    mdbr(dst, lhs);
  } else {
    ldr(dst, lhs);
    mdbr(dst, rhs);
  }
}

void TurboAssembler::DivF64(DoubleRegister dst, DoubleRegister lhs,
                            DoubleRegister rhs) {
  if (dst == lhs) {
    ddbr(dst, rhs);
  } else if (dst == rhs) {
    lay(sp, MemOperand(sp, -kSystemPointerSize));
    StoreF64(dst, MemOperand(sp));
    ldr(dst, lhs);
    ddb(dst, MemOperand(sp));
    la(sp, MemOperand(sp, kSystemPointerSize));
  } else {
    ldr(dst, lhs);
    ddbr(dst, rhs);
  }
}

void TurboAssembler::AddFloat32(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    aeb(dst, opnd);
  } else {
    ley(scratch, opnd);
    aebr(dst, scratch);
  }
}

void TurboAssembler::AddFloat64(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    adb(dst, opnd);
  } else {
    ldy(scratch, opnd);
    adbr(dst, scratch);
  }
}

void TurboAssembler::SubFloat32(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    seb(dst, opnd);
  } else {
    ley(scratch, opnd);
    sebr(dst, scratch);
  }
}

void TurboAssembler::SubFloat64(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    sdb(dst, opnd);
  } else {
    ldy(scratch, opnd);
    sdbr(dst, scratch);
  }
}

void TurboAssembler::MulFloat32(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    meeb(dst, opnd);
  } else {
    ley(scratch, opnd);
    meebr(dst, scratch);
  }
}

void TurboAssembler::MulFloat64(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    mdb(dst, opnd);
  } else {
    ldy(scratch, opnd);
    mdbr(dst, scratch);
  }
}

void TurboAssembler::DivFloat32(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    deb(dst, opnd);
  } else {
    ley(scratch, opnd);
    debr(dst, scratch);
  }
}

void TurboAssembler::DivFloat64(DoubleRegister dst, const MemOperand& opnd,
                                DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    ddb(dst, opnd);
  } else {
    ldy(scratch, opnd);
    ddbr(dst, scratch);
  }
}

void TurboAssembler::LoadF32AsF64(DoubleRegister dst, const MemOperand& opnd,
                                  DoubleRegister scratch) {
  if (is_uint12(opnd.offset())) {
    ldeb(dst, opnd);
  } else {
    ley(scratch, opnd);
    ldebr(dst, scratch);
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand of RX or RXY format
void TurboAssembler::StoreU32(Register src, const MemOperand& mem,
                              Register scratch) {
  Register base = mem.rb();
  int offset = mem.offset();

  bool use_RXform = false;
  bool use_RXYform = false;

  if (is_uint12(offset)) {
    // RX-format supports unsigned 12-bits offset.
    use_RXform = true;
  } else if (is_int20(offset)) {
    // RXY-format supports signed 20-bits offset.
    use_RXYform = true;
  } else if (scratch != no_reg) {
    // Materialize offset into scratch register.
    mov(scratch, Operand(offset));
  } else {
    // scratch is no_reg
    DCHECK(false);
  }

  if (use_RXform) {
    st(src, mem);
  } else if (use_RXYform) {
    sty(src, mem);
  } else {
    StoreU32(src, MemOperand(base, scratch));
  }
}

void TurboAssembler::LoadS16(Register dst, Register src) {
#if V8_TARGET_ARCH_S390X
  lghr(dst, src);
#else
  lhr(dst, src);
#endif
}

// Loads 16-bits half-word value from memory and sign extends to pointer
// sized register
void TurboAssembler::LoadS16(Register dst, const MemOperand& mem,
                                   Register scratch) {
  Register base = mem.rb();
  int offset = mem.offset();

  if (!is_int20(offset)) {
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
#if V8_TARGET_ARCH_S390X
    lgh(dst, MemOperand(base, scratch));
#else
    lh(dst, MemOperand(base, scratch));
#endif
  } else {
#if V8_TARGET_ARCH_S390X
    lgh(dst, mem);
#else
    if (is_uint12(offset)) {
      lh(dst, mem);
    } else {
      lhy(dst, mem);
    }
#endif
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand current only supports d-form
void TurboAssembler::StoreU16(Register src, const MemOperand& mem,
                              Register scratch) {
  Register base = mem.rb();
  int offset = mem.offset();

  if (is_uint12(offset)) {
    sth(src, mem);
  } else if (is_int20(offset)) {
    sthy(src, mem);
  } else {
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    sth(src, MemOperand(base, scratch));
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand current only supports d-form
void TurboAssembler::StoreU8(Register src, const MemOperand& mem,
                             Register scratch) {
  Register base = mem.rb();
  int offset = mem.offset();

  if (is_uint12(offset)) {
    stc(src, mem);
  } else if (is_int20(offset)) {
    stcy(src, mem);
  } else {
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    stc(src, MemOperand(base, scratch));
  }
}

// Shift left logical for 32-bit integer types.
void TurboAssembler::ShiftLeftU32(Register dst, Register src,
                                  const Operand& val) {
  ShiftLeftU32(dst, src, r0, val);
}

// Shift left logical for 32-bit integer types.
void TurboAssembler::ShiftLeftU32(Register dst, Register src, Register val,
                                  const Operand& val2) {
  if (dst == src) {
    sll(dst, val, val2);
  } else if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
    sllk(dst, src, val, val2);
  } else {
    DCHECK(dst != val || val == r0);  // The lr/sll path clobbers val.
    lr(dst, src);
    sll(dst, val, val2);
  }
}

// Shift left logical for 32-bit integer types.
void TurboAssembler::ShiftLeftU64(Register dst, Register src,
                                  const Operand& val) {
  ShiftLeftU64(dst, src, r0, val);
}

// Shift left logical for 32-bit integer types.
void TurboAssembler::ShiftLeftU64(Register dst, Register src, Register val,
                                  const Operand& val2) {
  sllg(dst, src, val, val2);
}

// Shift right logical for 32-bit integer types.
void TurboAssembler::ShiftRightU32(Register dst, Register src,
                                   const Operand& val) {
  ShiftRightU32(dst, src, r0, val);
}

// Shift right logical for 32-bit integer types.
void TurboAssembler::ShiftRightU32(Register dst, Register src, Register val,
                                   const Operand& val2) {
  if (dst == src) {
    srl(dst, val, val2);
  } else if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
    srlk(dst, src, val, val2);
  } else {
    DCHECK(dst != val || val == r0);  // The lr/srl path clobbers val.
    lr(dst, src);
    srl(dst, val, val2);
  }
}

void TurboAssembler::ShiftRightU64(Register dst, Register src, Register val,
                                   const Operand& val2) {
  srlg(dst, src, val, val2);
}

// Shift right logical for 64-bit integer types.
void TurboAssembler::ShiftRightU64(Register dst, Register src,
                                   const Operand& val) {
  ShiftRightU64(dst, src, r0, val);
}

// Shift right arithmetic for 32-bit integer types.
void TurboAssembler::ShiftRightS32(Register dst, Register src,
                                   const Operand& val) {
  ShiftRightS32(dst, src, r0, val);
}

// Shift right arithmetic for 32-bit integer types.
void TurboAssembler::ShiftRightS32(Register dst, Register src, Register val,
                                   const Operand& val2) {
  if (dst == src) {
    sra(dst, val, val2);
  } else if (CpuFeatures::IsSupported(DISTINCT_OPS)) {
    srak(dst, src, val, val2);
  } else {
    DCHECK(dst != val || val == r0);  // The lr/sra path clobbers val.
    lr(dst, src);
    sra(dst, val, val2);
  }
}

// Shift right arithmetic for 64-bit integer types.
void TurboAssembler::ShiftRightS64(Register dst, Register src,
                                   const Operand& val) {
  ShiftRightS64(dst, src, r0, val);
}

// Shift right arithmetic for 64-bit integer types.
void TurboAssembler::ShiftRightS64(Register dst, Register src, Register val,
                                   const Operand& val2) {
  srag(dst, src, val, val2);
}

// Clear right most # of bits
void TurboAssembler::ClearRightImm(Register dst, Register src,
                                   const Operand& val) {
  int numBitsToClear = val.immediate() % (kSystemPointerSize * 8);

  // Try to use RISBG if possible
  if (CpuFeatures::IsSupported(GENERAL_INSTR_EXT)) {
    int endBit = 63 - numBitsToClear;
    RotateInsertSelectBits(dst, src, Operand::Zero(), Operand(endBit),
                           Operand::Zero(), true);
    return;
  }

  uint64_t hexMask = ~((1L << numBitsToClear) - 1);

  // S390 AND instr clobbers source.  Make a copy if necessary
  if (dst != src) mov(dst, src);

  if (numBitsToClear <= 16) {
    nill(dst, Operand(static_cast<uint16_t>(hexMask)));
  } else if (numBitsToClear <= 32) {
    nilf(dst, Operand(static_cast<uint32_t>(hexMask)));
  } else if (numBitsToClear <= 64) {
    nilf(dst, Operand(static_cast<intptr_t>(0)));
    nihf(dst, Operand(hexMask >> 32));
  }
}

void TurboAssembler::Popcnt32(Register dst, Register src) {
  DCHECK(src != r0);
  DCHECK(dst != r0);

  popcnt(dst, src);
  ShiftRightU32(r0, dst, Operand(16));
  ar(dst, r0);
  ShiftRightU32(r0, dst, Operand(8));
  ar(dst, r0);
  llgcr(dst, dst);
}

#ifdef V8_TARGET_ARCH_S390X
void TurboAssembler::Popcnt64(Register dst, Register src) {
  DCHECK(src != r0);
  DCHECK(dst != r0);

  popcnt(dst, src);
  ShiftRightU64(r0, dst, Operand(32));
  AddS64(dst, r0);
  ShiftRightU64(r0, dst, Operand(16));
  AddS64(dst, r0);
  ShiftRightU64(r0, dst, Operand(8));
  AddS64(dst, r0);
  LoadU8(dst, dst);
}
#endif

void TurboAssembler::SwapP(Register src, Register dst, Register scratch) {
  if (src == dst) return;
  DCHECK(!AreAliased(src, dst, scratch));
  mov(scratch, src);
  mov(src, dst);
  mov(dst, scratch);
}

void TurboAssembler::SwapP(Register src, MemOperand dst, Register scratch) {
  if (dst.rx() != r0) DCHECK(!AreAliased(src, dst.rx(), scratch));
  if (dst.rb() != r0) DCHECK(!AreAliased(src, dst.rb(), scratch));
  DCHECK(!AreAliased(src, scratch));
  mov(scratch, src);
  LoadU64(src, dst);
  StoreU64(scratch, dst);
}

void TurboAssembler::SwapP(MemOperand src, MemOperand dst, Register scratch_0,
                           Register scratch_1) {
  if (src.rx() != r0) DCHECK(!AreAliased(src.rx(), scratch_0, scratch_1));
  if (src.rb() != r0) DCHECK(!AreAliased(src.rb(), scratch_0, scratch_1));
  if (dst.rx() != r0) DCHECK(!AreAliased(dst.rx(), scratch_0, scratch_1));
  if (dst.rb() != r0) DCHECK(!AreAliased(dst.rb(), scratch_0, scratch_1));
  DCHECK(!AreAliased(scratch_0, scratch_1));
  LoadU64(scratch_0, src);
  LoadU64(scratch_1, dst);
  StoreU64(scratch_0, dst);
  StoreU64(scratch_1, src);
}

void TurboAssembler::SwapFloat32(DoubleRegister src, DoubleRegister dst,
                                 DoubleRegister scratch) {
  if (src == dst) return;
  DCHECK(!AreAliased(src, dst, scratch));
  ldr(scratch, src);
  ldr(src, dst);
  ldr(dst, scratch);
}

void TurboAssembler::SwapFloat32(DoubleRegister src, MemOperand dst,
                                 DoubleRegister scratch) {
  DCHECK(!AreAliased(src, scratch));
  ldr(scratch, src);
  LoadF32(src, dst);
  StoreF32(scratch, dst);
}

void TurboAssembler::SwapFloat32(MemOperand src, MemOperand dst,
                                 DoubleRegister scratch) {
  // push d0, to be used as scratch
  lay(sp, MemOperand(sp, -kDoubleSize));
  StoreF64(d0, MemOperand(sp));
  LoadF32(scratch, src);
  LoadF32(d0, dst);
  StoreF32(scratch, dst);
  StoreF32(d0, src);
  // restore d0
  LoadF64(d0, MemOperand(sp));
  lay(sp, MemOperand(sp, kDoubleSize));
}

void TurboAssembler::SwapDouble(DoubleRegister src, DoubleRegister dst,
                                DoubleRegister scratch) {
  if (src == dst) return;
  DCHECK(!AreAliased(src, dst, scratch));
  ldr(scratch, src);
  ldr(src, dst);
  ldr(dst, scratch);
}

void TurboAssembler::SwapDouble(DoubleRegister src, MemOperand dst,
                                DoubleRegister scratch) {
  DCHECK(!AreAliased(src, scratch));
  ldr(scratch, src);
  LoadF64(src, dst);
  StoreF64(scratch, dst);
}

void TurboAssembler::SwapDouble(MemOperand src, MemOperand dst,
                                DoubleRegister scratch) {
  // push d0, to be used as scratch
  lay(sp, MemOperand(sp, -kDoubleSize));
  StoreF64(d0, MemOperand(sp));
  LoadF64(scratch, src);
  LoadF64(d0, dst);
  StoreF64(scratch, dst);
  StoreF64(d0, src);
  // restore d0
  LoadF64(d0, MemOperand(sp));
  lay(sp, MemOperand(sp, kDoubleSize));
}

void TurboAssembler::SwapSimd128(Simd128Register src, Simd128Register dst,
                                 Simd128Register scratch) {
  if (src == dst) return;
  vlr(scratch, src, Condition(0), Condition(0), Condition(0));
  vlr(src, dst, Condition(0), Condition(0), Condition(0));
  vlr(dst, scratch, Condition(0), Condition(0), Condition(0));
}

void TurboAssembler::SwapSimd128(Simd128Register src, MemOperand dst,
                                 Simd128Register scratch) {
  DCHECK(!AreAliased(src, scratch));
  vlr(scratch, src, Condition(0), Condition(0), Condition(0));
  LoadV128(src, dst, ip);
  StoreV128(scratch, dst, ip);
}

void TurboAssembler::SwapSimd128(MemOperand src, MemOperand dst,
                                 Simd128Register scratch) {
  // push d0, to be used as scratch
  lay(sp, MemOperand(sp, -kSimd128Size));
  StoreV128(d0, MemOperand(sp), ip);
  LoadV128(scratch, src, ip);
  LoadV128(d0, dst, ip);
  StoreV128(scratch, dst, ip);
  StoreV128(d0, src, ip);
  // restore d0
  LoadV128(d0, MemOperand(sp), ip);
  lay(sp, MemOperand(sp, kSimd128Size));
}

void TurboAssembler::ResetSpeculationPoisonRegister() {
  mov(kSpeculationPoisonRegister, Operand(-1));
}

void TurboAssembler::ComputeCodeStartAddress(Register dst) {
  larl(dst, Operand(-pc_offset() / 2));
}

void TurboAssembler::LoadPC(Register dst) {
  Label current_pc;
  larl(dst, &current_pc);
  bind(&current_pc);
}

void TurboAssembler::JumpIfEqual(Register x, int32_t y, Label* dest) {
  CmpS32(x, Operand(y));
  beq(dest);
}

void TurboAssembler::JumpIfLessThan(Register x, int32_t y, Label* dest) {
  CmpS32(x, Operand(y));
  blt(dest);
}

void TurboAssembler::LoadEntryFromBuiltinIndex(Register builtin_index) {
  STATIC_ASSERT(kSystemPointerSize == 8);
  STATIC_ASSERT(kSmiTagSize == 1);
  STATIC_ASSERT(kSmiTag == 0);
  // The builtin_index register contains the builtin index as a Smi.
  if (SmiValuesAre32Bits()) {
    ShiftRightS64(builtin_index, builtin_index,
                  Operand(kSmiShift - kSystemPointerSizeLog2));
  } else {
    DCHECK(SmiValuesAre31Bits());
    ShiftLeftU64(builtin_index, builtin_index,
                 Operand(kSystemPointerSizeLog2 - kSmiShift));
  }
  LoadU64(builtin_index, MemOperand(kRootRegister, builtin_index,
                                    IsolateData::builtin_entry_table_offset()));
}

void TurboAssembler::CallBuiltinByIndex(Register builtin_index) {
  LoadEntryFromBuiltinIndex(builtin_index);
  Call(builtin_index);
}

void TurboAssembler::LoadCodeObjectEntry(Register destination,
                                         Register code_object) {
  // Code objects are called differently depending on whether we are generating
  // builtin code (which will later be embedded into the binary) or compiling
  // user JS code at runtime.
  // * Builtin code runs in --jitless mode and thus must not call into on-heap
  //   Code targets. Instead, we dispatch through the builtins entry table.
  // * Codegen at runtime does not have this restriction and we can use the
  //   shorter, branchless instruction sequence. The assumption here is that
  //   targets are usually generated code and not builtin Code objects.

  if (options().isolate_independent_code) {
    DCHECK(root_array_available());
    Label if_code_is_off_heap, out;

    Register scratch = r1;

    DCHECK(!AreAliased(destination, scratch));
    DCHECK(!AreAliased(code_object, scratch));

    // Check whether the Code object is an off-heap trampoline. If so, call its
    // (off-heap) entry point directly without going through the (on-heap)
    // trampoline.  Otherwise, just call the Code object as always.
    LoadS32(scratch, FieldMemOperand(code_object, Code::kFlagsOffset));
    tmlh(scratch, Operand(Code::IsOffHeapTrampoline::kMask >> 16));
    bne(&if_code_is_off_heap);

    // Not an off-heap trampoline, the entry point is at
    // Code::raw_instruction_start().
    AddS64(destination, code_object,
           Operand(Code::kHeaderSize - kHeapObjectTag));
    b(&out);

    // An off-heap trampoline, the entry point is loaded from the builtin entry
    // table.
    bind(&if_code_is_off_heap);
    LoadS32(scratch, FieldMemOperand(code_object, Code::kBuiltinIndexOffset));
    ShiftLeftU64(destination, scratch, Operand(kSystemPointerSizeLog2));
    AddS64(destination, destination, kRootRegister);
    LoadU64(destination,
            MemOperand(destination, IsolateData::builtin_entry_table_offset()));

    bind(&out);
  } else {
    AddS64(destination, code_object,
           Operand(Code::kHeaderSize - kHeapObjectTag));
  }
}

void TurboAssembler::CallCodeObject(Register code_object) {
  LoadCodeObjectEntry(code_object, code_object);
  Call(code_object);
}

void TurboAssembler::JumpCodeObject(Register code_object, JumpMode jump_mode) {
  DCHECK_EQ(JumpMode::kJump, jump_mode);
  LoadCodeObjectEntry(code_object, code_object);
  Jump(code_object);
}

void TurboAssembler::StoreReturnAddressAndCall(Register target) {
  // This generates the final instruction sequence for calls to C functions
  // once an exit frame has been constructed.
  //
  // Note that this assumes the caller code (i.e. the Code object currently
  // being generated) is immovable or that the callee function cannot trigger
  // GC, since the callee function will return to it.

  Label return_label;
  larl(r14, &return_label);  // Generate the return addr of call later.
  StoreU64(r14, MemOperand(sp, kStackFrameRASlot * kSystemPointerSize));

  // zLinux ABI requires caller's frame to have sufficient space for callee
  // preserved regsiter save area.
  b(target);
  bind(&return_label);
}

void TurboAssembler::CallForDeoptimization(Builtins::Name target, int,
                                           Label* exit, DeoptimizeKind kind,
                                           Label* ret, Label*) {
  LoadU64(ip, MemOperand(kRootRegister,
                         IsolateData::builtin_entry_slot_offset(target)));
  Call(ip);
  DCHECK_EQ(SizeOfCodeGeneratedSince(exit),
            (kind == DeoptimizeKind::kLazy)
                ? Deoptimizer::kLazyDeoptExitSize
                : Deoptimizer::kNonLazyDeoptExitSize);
  if (kind == DeoptimizeKind::kEagerWithResume) {
    bc_long(Condition::al, ret);
    DCHECK_EQ(SizeOfCodeGeneratedSince(exit),
              Deoptimizer::kEagerWithResumeBeforeArgsSize);
  }
}

void TurboAssembler::Trap() { stop(); }
void TurboAssembler::DebugBreak() { stop(); }

void TurboAssembler::CountLeadingZerosU32(Register dst, Register src,
                                          Register scratch_pair) {
  llgfr(dst, src);
  flogr(scratch_pair,
        dst);  // will modify a register pair scratch and scratch + 1
  AddS32(dst, scratch_pair, Operand(-32));
}

void TurboAssembler::CountLeadingZerosU64(Register dst, Register src,
                                          Register scratch_pair) {
  flogr(scratch_pair,
        src);  // will modify a register pair scratch and scratch + 1
  mov(dst, scratch_pair);
}

void TurboAssembler::CountTrailingZerosU32(Register dst, Register src,
                                           Register scratch_pair) {
  Register scratch0 = scratch_pair;
  Register scratch1 = Register::from_code(scratch_pair.code() + 1);
  DCHECK(!AreAliased(dst, scratch0, scratch1));
  DCHECK(!AreAliased(src, scratch0, scratch1));

  Label done;
  // Check if src is all zeros.
  ltr(scratch1, src);
  mov(dst, Operand(32));
  beq(&done);
  llgfr(scratch1, scratch1);
  lcgr(scratch0, scratch1);
  ngr(scratch1, scratch0);
  flogr(scratch0, scratch1);
  mov(dst, Operand(63));
  SubS64(dst, scratch0);
  bind(&done);
}

void TurboAssembler::CountTrailingZerosU64(Register dst, Register src,
                                           Register scratch_pair) {
  Register scratch0 = scratch_pair;
  Register scratch1 = Register::from_code(scratch_pair.code() + 1);
  DCHECK(!AreAliased(dst, scratch0, scratch1));
  DCHECK(!AreAliased(src, scratch0, scratch1));

  Label done;
  // Check if src is all zeros.
  ltgr(scratch1, src);
  mov(dst, Operand(64));
  beq(&done);
  lcgr(scratch0, scratch1);
  ngr(scratch0, scratch1);
  flogr(scratch0, scratch0);
  mov(dst, Operand(63));
  SubS64(dst, scratch0);
  bind(&done);
}

void TurboAssembler::AtomicCmpExchangeHelper(Register addr, Register output,
                                             Register old_value,
                                             Register new_value, int start,
                                             int end, int shift_amount,
                                             int offset, Register temp0,
                                             Register temp1) {
  LoadU32(temp0, MemOperand(addr, offset));
  llgfr(temp1, temp0);
  RotateInsertSelectBits(temp0, old_value, Operand(start), Operand(end),
                         Operand(shift_amount), false);
  RotateInsertSelectBits(temp1, new_value, Operand(start), Operand(end),
                         Operand(shift_amount), false);
  CmpAndSwap(temp0, temp1, MemOperand(addr, offset));
  RotateInsertSelectBits(output, temp0, Operand(start + shift_amount),
                         Operand(end + shift_amount),
                         Operand(64 - shift_amount), true);
}

void TurboAssembler::AtomicCmpExchangeU8(Register addr, Register output,
                                         Register old_value, Register new_value,
                                         Register temp0, Register temp1) {
#ifdef V8_TARGET_BIG_ENDIAN
#define ATOMIC_COMP_EXCHANGE_BYTE(i)                                        \
  {                                                                         \
    constexpr int idx = (i);                                                \
    static_assert(idx <= 3 && idx >= 0, "idx is out of range!");            \
    constexpr int start = 32 + 8 * idx;                                     \
    constexpr int end = start + 7;                                          \
    constexpr int shift_amount = (3 - idx) * 8;                             \
    AtomicCmpExchangeHelper(addr, output, old_value, new_value, start, end, \
                            shift_amount, -idx, temp0, temp1);              \
  }
#else
#define ATOMIC_COMP_EXCHANGE_BYTE(i)                                        \
  {                                                                         \
    constexpr int idx = (i);                                                \
    static_assert(idx <= 3 && idx >= 0, "idx is out of range!");            \
    constexpr int start = 32 + 8 * (3 - idx);                               \
    constexpr int end = start + 7;                                          \
    constexpr int shift_amount = idx * 8;                                   \
    AtomicCmpExchangeHelper(addr, output, old_value, new_value, start, end, \
                            shift_amount, -idx, temp0, temp1);              \
  }
#endif

  Label one, two, three, done;
  tmll(addr, Operand(3));
  b(Condition(1), &three);
  b(Condition(2), &two);
  b(Condition(4), &one);
  /* ending with 0b00 */
  ATOMIC_COMP_EXCHANGE_BYTE(0);
  b(&done);
  /* ending with 0b01 */
  bind(&one);
  ATOMIC_COMP_EXCHANGE_BYTE(1);
  b(&done);
  /* ending with 0b10 */
  bind(&two);
  ATOMIC_COMP_EXCHANGE_BYTE(2);
  b(&done);
  /* ending with 0b11 */
  bind(&three);
  ATOMIC_COMP_EXCHANGE_BYTE(3);
  bind(&done);
}

void TurboAssembler::AtomicCmpExchangeU16(Register addr, Register output,
                                          Register old_value,
                                          Register new_value, Register temp0,
                                          Register temp1) {
#ifdef V8_TARGET_BIG_ENDIAN
#define ATOMIC_COMP_EXCHANGE_HALFWORD(i)                                    \
  {                                                                         \
    constexpr int idx = (i);                                                \
    static_assert(idx <= 1 && idx >= 0, "idx is out of range!");            \
    constexpr int start = 32 + 16 * idx;                                    \
    constexpr int end = start + 15;                                         \
    constexpr int shift_amount = (1 - idx) * 16;                            \
    AtomicCmpExchangeHelper(addr, output, old_value, new_value, start, end, \
                            shift_amount, -idx * 2, temp0, temp1);          \
  }
#else
#define ATOMIC_COMP_EXCHANGE_HALFWORD(i)                                    \
  {                                                                         \
    constexpr int idx = (i);                                                \
    static_assert(idx <= 1 && idx >= 0, "idx is out of range!");            \
    constexpr int start = 32 + 16 * (1 - idx);                              \
    constexpr int end = start + 15;                                         \
    constexpr int shift_amount = idx * 16;                                  \
    AtomicCmpExchangeHelper(addr, output, old_value, new_value, start, end, \
                            shift_amount, -idx * 2, temp0, temp1);          \
  }
#endif

  Label two, done;
  tmll(addr, Operand(3));
  b(Condition(2), &two);
  ATOMIC_COMP_EXCHANGE_HALFWORD(0);
  b(&done);
  bind(&two);
  ATOMIC_COMP_EXCHANGE_HALFWORD(1);
  bind(&done);
}

void TurboAssembler::AtomicExchangeHelper(Register addr, Register value,
                                          Register output, int start, int end,
                                          int shift_amount, int offset,
                                          Register scratch) {
  Label do_cs;
  LoadU32(output, MemOperand(addr, offset));
  bind(&do_cs);
  llgfr(scratch, output);
  RotateInsertSelectBits(scratch, value, Operand(start), Operand(end),
                         Operand(shift_amount), false);
  csy(output, scratch, MemOperand(addr, offset));
  bne(&do_cs, Label::kNear);
  srl(output, Operand(shift_amount));
}

void TurboAssembler::AtomicExchangeU8(Register addr, Register value,
                                      Register output, Register scratch) {
#ifdef V8_TARGET_BIG_ENDIAN
#define ATOMIC_EXCHANGE_BYTE(i)                                               \
  {                                                                           \
    constexpr int idx = (i);                                                  \
    static_assert(idx <= 3 && idx >= 0, "idx is out of range!");              \
    constexpr int start = 32 + 8 * idx;                                       \
    constexpr int end = start + 7;                                            \
    constexpr int shift_amount = (3 - idx) * 8;                               \
    AtomicExchangeHelper(addr, value, output, start, end, shift_amount, -idx, \
                         scratch);                                            \
  }
#else
#define ATOMIC_EXCHANGE_BYTE(i)                                               \
  {                                                                           \
    constexpr int idx = (i);                                                  \
    static_assert(idx <= 3 && idx >= 0, "idx is out of range!");              \
    constexpr int start = 32 + 8 * (3 - idx);                                 \
    constexpr int end = start + 7;                                            \
    constexpr int shift_amount = idx * 8;                                     \
    AtomicExchangeHelper(addr, value, output, start, end, shift_amount, -idx, \
                         scratch);                                            \
  }
#endif
  Label three, two, one, done;
  tmll(addr, Operand(3));
  b(Condition(1), &three);
  b(Condition(2), &two);
  b(Condition(4), &one);

  // end with 0b00
  ATOMIC_EXCHANGE_BYTE(0);
  b(&done);

  // ending with 0b01
  bind(&one);
  ATOMIC_EXCHANGE_BYTE(1);
  b(&done);

  // ending with 0b10
  bind(&two);
  ATOMIC_EXCHANGE_BYTE(2);
  b(&done);

  // ending with 0b11
  bind(&three);
  ATOMIC_EXCHANGE_BYTE(3);

  bind(&done);
}

void TurboAssembler::AtomicExchangeU16(Register addr, Register value,
                                       Register output, Register scratch) {
#ifdef V8_TARGET_BIG_ENDIAN
#define ATOMIC_EXCHANGE_HALFWORD(i)                                     \
  {                                                                     \
    constexpr int idx = (i);                                            \
    static_assert(idx <= 1 && idx >= 0, "idx is out of range!");        \
    constexpr int start = 32 + 16 * idx;                                \
    constexpr int end = start + 15;                                     \
    constexpr int shift_amount = (1 - idx) * 16;                        \
    AtomicExchangeHelper(addr, value, output, start, end, shift_amount, \
                         -idx * 2, scratch);                            \
  }
#else
#define ATOMIC_EXCHANGE_HALFWORD(i)                                     \
  {                                                                     \
    constexpr int idx = (i);                                            \
    static_assert(idx <= 1 && idx >= 0, "idx is out of range!");        \
    constexpr int start = 32 + 16 * (1 - idx);                          \
    constexpr int end = start + 15;                                     \
    constexpr int shift_amount = idx * 16;                              \
    AtomicExchangeHelper(addr, value, output, start, end, shift_amount, \
                         -idx * 2, scratch);                            \
  }
#endif
  Label two, done;
  tmll(addr, Operand(3));
  b(Condition(2), &two);

  // end with 0b00
  ATOMIC_EXCHANGE_HALFWORD(0);
  b(&done);

  // ending with 0b10
  bind(&two);
  ATOMIC_EXCHANGE_HALFWORD(1);

  bind(&done);
}

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_S390
