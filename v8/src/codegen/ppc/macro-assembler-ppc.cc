// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>  // For assert
#include <limits.h>  // For LONG_MIN, LONG_MAX.

#if V8_TARGET_ARCH_PPC || V8_TARGET_ARCH_PPC64

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
#include "src/runtime/runtime.h"
#include "src/snapshot/embedded/embedded-data.h"
#include "src/snapshot/snapshot.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/wasm-code-manager.h"
#endif  // V8_ENABLE_WEBASSEMBLY

// Satisfy cpplint check, but don't include platform-specific header. It is
// included recursively via macro-assembler.h.
#if 0
#include "src/codegen/ppc/macro-assembler-ppc.h"
#endif

namespace v8 {
namespace internal {

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
    bytes += kNumCallerSavedDoubles * kDoubleSize;
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
    bytes += kNumCallerSavedDoubles * kDoubleSize;
  }

  return bytes;
}

int TurboAssembler::PopCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1,
                                   Register exclusion2, Register exclusion3) {
  int bytes = 0;
  if (fp_mode == SaveFPRegsMode::kSave) {
    MultiPopDoubles(kCallerSavedDoubles);
    bytes += kNumCallerSavedDoubles * kDoubleSize;
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

void TurboAssembler::Jump(Register target) {
  mtctr(target);
  bctr();
}

void TurboAssembler::LoadFromConstantsTable(Register destination,
                                            int constant_index) {
  DCHECK(RootsTable::IsImmortalImmovable(RootIndex::kBuiltinsConstantsTable));

  DCHECK_NE(destination, r0);
  LoadRoot(destination, RootIndex::kBuiltinsConstantsTable);
  LoadTaggedPointerField(
      destination,
      FieldMemOperand(destination,
                      FixedArray::OffsetOfElementAt(constant_index)),
      r0);
}

void TurboAssembler::LoadRootRelative(Register destination, int32_t offset) {
  LoadU64(destination, MemOperand(kRootRegister, offset), r0);
}

void TurboAssembler::LoadRootRegisterOffset(Register destination,
                                            intptr_t offset) {
  if (offset == 0) {
    mr(destination, kRootRegister);
  } else if (is_int16(offset)) {
    addi(destination, kRootRegister, Operand(offset));
  } else {
    mov(destination, Operand(offset));
    add(destination, kRootRegister, destination);
  }
}

void TurboAssembler::Jump(intptr_t target, RelocInfo::Mode rmode,
                          Condition cond, CRegister cr) {
  Label skip;

  if (cond != al) b(NegateCondition(cond), &skip, cr);

  DCHECK(rmode == RelocInfo::CODE_TARGET || rmode == RelocInfo::RUNTIME_ENTRY);

  mov(ip, Operand(target, rmode));
  mtctr(ip);
  bctr();

  bind(&skip);
}

void TurboAssembler::Jump(Address target, RelocInfo::Mode rmode, Condition cond,
                          CRegister cr) {
  DCHECK(!RelocInfo::IsCodeTarget(rmode));
  Jump(static_cast<intptr_t>(target), rmode, cond, cr);
}

void TurboAssembler::Jump(Handle<Code> code, RelocInfo::Mode rmode,
                          Condition cond, CRegister cr) {
  DCHECK(RelocInfo::IsCodeTarget(rmode));
  DCHECK_IMPLIES(options().isolate_independent_code,
                 Builtins::IsIsolateIndependentBuiltin(*code));

  int builtin_index = Builtins::kNoBuiltinId;
  bool target_is_builtin =
      isolate()->builtins()->IsBuiltinHandle(code, &builtin_index);

  if (root_array_available_ && options().isolate_independent_code) {
    Label skip;
    Register scratch = ip;
    int offset = code->builtin_index() * kSystemPointerSize +
                 IsolateData::builtin_entry_table_offset();
    LoadU64(scratch, MemOperand(kRootRegister, offset), r0);
    if (cond != al) b(NegateCondition(cond), &skip, cr);
    Jump(scratch);
    bind(&skip);
    return;
  } else if (options().inline_offheap_trampolines && target_is_builtin) {
    // Inline the trampoline.
    Label skip;
    RecordCommentForOffHeapTrampoline(builtin_index);
    EmbeddedData d = EmbeddedData::FromBlob();
    Address entry = d.InstructionStartOfBuiltin(builtin_index);
    // Use ip directly instead of using UseScratchRegisterScope, as we do
    // not preserve scratch registers across calls.
    mov(ip, Operand(entry, RelocInfo::OFF_HEAP_TARGET));
    if (cond != al) b(NegateCondition(cond), &skip, cr);
    Jump(ip);
    bind(&skip);
    return;
  }
  int32_t target_index = AddCodeTarget(code);
  Jump(static_cast<intptr_t>(target_index), rmode, cond, cr);
}

void TurboAssembler::Jump(const ExternalReference& reference) {
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  Move(scratch, reference);
  if (ABI_USES_FUNCTION_DESCRIPTORS) {
    // AIX uses a function descriptor. When calling C code be
    // aware of this descriptor and pick up values from it.
    LoadU64(ToRegister(ABI_TOC_REGISTER),
            MemOperand(scratch, kSystemPointerSize));
    LoadU64(scratch, MemOperand(scratch, 0));
  }
  Jump(scratch);
}

void TurboAssembler::Call(Register target) {
  BlockTrampolinePoolScope block_trampoline_pool(this);
  // branch via link register and set LK bit for return point
  mtctr(target);
  bctrl();
}

void MacroAssembler::CallJSEntry(Register target) {
  CHECK(target == r5);
  Call(target);
}

int MacroAssembler::CallSizeNotPredictableCodeSize(Address target,
                                                   RelocInfo::Mode rmode,
                                                   Condition cond) {
  return (2 + kMovInstructionsNoConstantPool) * kInstrSize;
}

void TurboAssembler::Call(Address target, RelocInfo::Mode rmode,
                          Condition cond) {
  BlockTrampolinePoolScope block_trampoline_pool(this);
  DCHECK(cond == al);

  // This can likely be optimized to make use of bc() with 24bit relative
  //
  // RecordRelocInfo(x.rmode_, x.immediate);
  // bc( BA, .... offset, LKset);
  //

  mov(ip, Operand(target, rmode));
  mtctr(ip);
  bctrl();
}

void TurboAssembler::Call(Handle<Code> code, RelocInfo::Mode rmode,
                          Condition cond) {
  BlockTrampolinePoolScope block_trampoline_pool(this);
  DCHECK(RelocInfo::IsCodeTarget(rmode));
  DCHECK_IMPLIES(options().isolate_independent_code,
                 Builtins::IsIsolateIndependentBuiltin(*code));
  DCHECK_IMPLIES(options().use_pc_relative_calls_and_jumps,
                 Builtins::IsIsolateIndependentBuiltin(*code));

  int builtin_index = Builtins::kNoBuiltinId;
  bool target_is_builtin =
      isolate()->builtins()->IsBuiltinHandle(code, &builtin_index);

  if (root_array_available_ && options().isolate_independent_code) {
    Label skip;
    int offset = code->builtin_index() * kSystemPointerSize +
                 IsolateData::builtin_entry_table_offset();
    LoadU64(ip, MemOperand(kRootRegister, offset));
    if (cond != al) b(NegateCondition(cond), &skip);
    Call(ip);
    bind(&skip);
    return;
  } else if (options().inline_offheap_trampolines && target_is_builtin) {
    // Inline the trampoline.
    RecordCommentForOffHeapTrampoline(builtin_index);
    EmbeddedData d = EmbeddedData::FromBlob();
    Address entry = d.InstructionStartOfBuiltin(builtin_index);
    // Use ip directly instead of using UseScratchRegisterScope, as we do
    // not preserve scratch registers across calls.
    mov(ip, Operand(entry, RelocInfo::OFF_HEAP_TARGET));
    Label skip;
    if (cond != al) b(NegateCondition(cond), &skip);
    Call(ip);
    bind(&skip);
    return;
  }
  DCHECK(code->IsExecutable());
  int32_t target_index = AddCodeTarget(code);
  Call(static_cast<Address>(target_index), rmode, cond);
}

void TurboAssembler::Drop(int count) {
  if (count > 0) {
    Add(sp, sp, count * kSystemPointerSize, r0);
  }
}

void TurboAssembler::Drop(Register count, Register scratch) {
  ShiftLeftImm(scratch, count, Operand(kSystemPointerSizeLog2));
  add(sp, sp, scratch);
}

void TurboAssembler::Call(Label* target) { b(target, SetLK); }

void TurboAssembler::Push(Handle<HeapObject> handle) {
  mov(r0, Operand(handle));
  push(r0);
}

void TurboAssembler::Push(Smi smi) {
  mov(r0, Operand(smi));
  push(r0);
}

void TurboAssembler::PushArray(Register array, Register size, Register scratch,
                               Register scratch2, PushArrayOrder order) {
  Label loop, done;

  if (order == kNormal) {
    cmpi(size, Operand::Zero());
    beq(&done);
    ShiftLeftImm(scratch, size, Operand(kSystemPointerSizeLog2));
    add(scratch, array, scratch);
    mtctr(size);

    bind(&loop);
    LoadPU(scratch2, MemOperand(scratch, -kSystemPointerSize));
    StorePU(scratch2, MemOperand(sp, -kSystemPointerSize));
    bdnz(&loop);

    bind(&done);
  } else {
    cmpi(size, Operand::Zero());
    beq(&done);

    mtctr(size);
    subi(scratch, array, Operand(kSystemPointerSize));

    bind(&loop);
    LoadPU(scratch2, MemOperand(scratch, kSystemPointerSize));
    StorePU(scratch2, MemOperand(sp, -kSystemPointerSize));
    bdnz(&loop);
    bind(&done);
  }
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
  DCHECK(cond == al);
  if (dst != src) {
    mr(dst, src);
  }
}

void TurboAssembler::Move(DoubleRegister dst, DoubleRegister src) {
  if (dst != src) {
    fmr(dst, src);
  }
}

void TurboAssembler::MultiPush(RegList regs, Register location) {
  int16_t num_to_push = base::bits::CountPopulation(regs);
  int16_t stack_offset = num_to_push * kSystemPointerSize;

  subi(location, location, Operand(stack_offset));
  for (int16_t i = Register::kNumRegisters - 1; i >= 0; i--) {
    if ((regs & (1 << i)) != 0) {
      stack_offset -= kSystemPointerSize;
      StoreP(ToRegister(i), MemOperand(location, stack_offset));
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
  addi(location, location, Operand(stack_offset));
}

void TurboAssembler::MultiPushDoubles(RegList dregs, Register location) {
  int16_t num_to_push = base::bits::CountPopulation(dregs);
  int16_t stack_offset = num_to_push * kDoubleSize;

  subi(location, location, Operand(stack_offset));
  for (int16_t i = DoubleRegister::kNumRegisters - 1; i >= 0; i--) {
    if ((dregs & (1 << i)) != 0) {
      DoubleRegister dreg = DoubleRegister::from_code(i);
      stack_offset -= kDoubleSize;
      stfd(dreg, MemOperand(location, stack_offset));
    }
  }
}

void TurboAssembler::MultiPushV128(RegList dregs, Register location) {
  int16_t num_to_push = base::bits::CountPopulation(dregs);
  int16_t stack_offset = num_to_push * kSimd128Size;

  subi(location, location, Operand(stack_offset));
  for (int16_t i = Simd128Register::kNumRegisters - 1; i >= 0; i--) {
    if ((dregs & (1 << i)) != 0) {
      Simd128Register dreg = Simd128Register::from_code(i);
      stack_offset -= kSimd128Size;
      li(ip, Operand(stack_offset));
      StoreSimd128(dreg, MemOperand(location, ip));
    }
  }
}

void TurboAssembler::MultiPopDoubles(RegList dregs, Register location) {
  int16_t stack_offset = 0;

  for (int16_t i = 0; i < DoubleRegister::kNumRegisters; i++) {
    if ((dregs & (1 << i)) != 0) {
      DoubleRegister dreg = DoubleRegister::from_code(i);
      lfd(dreg, MemOperand(location, stack_offset));
      stack_offset += kDoubleSize;
    }
  }
  addi(location, location, Operand(stack_offset));
}

void TurboAssembler::MultiPopV128(RegList dregs, Register location) {
  int16_t stack_offset = 0;

  for (int16_t i = 0; i < Simd128Register::kNumRegisters; i++) {
    if ((dregs & (1 << i)) != 0) {
      Simd128Register dreg = Simd128Register::from_code(i);
      li(ip, Operand(stack_offset));
      LoadSimd128(dreg, MemOperand(location, ip));
      stack_offset += kSimd128Size;
    }
  }
  addi(location, location, Operand(stack_offset));
}

void TurboAssembler::LoadRoot(Register destination, RootIndex index,
                              Condition cond) {
  DCHECK(cond == al);
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

void TurboAssembler::SmiUntag(Register dst, const MemOperand& src, RCBit rc) {
  if (SmiValuesAre31Bits()) {
    lwz(dst, src);
  } else {
    LoadU64(dst, src);
  }

  SmiUntag(dst, rc);
}

void TurboAssembler::SmiUntagField(Register dst, const MemOperand& src,
                                   RCBit rc) {
  SmiUntag(dst, src, rc);
}

void TurboAssembler::StoreTaggedFieldX(const Register& value,
                                       const MemOperand& dst_field_operand,
                                       const Register& scratch) {
  if (COMPRESS_POINTERS_BOOL) {
    RecordComment("[ StoreTagged");
    stwx(value, dst_field_operand);
    RecordComment("]");
  } else {
    StorePX(value, dst_field_operand);
  }
}

void TurboAssembler::StoreTaggedField(const Register& value,
                                      const MemOperand& dst_field_operand,
                                      const Register& scratch) {
  if (COMPRESS_POINTERS_BOOL) {
    RecordComment("[ StoreTagged");
    StoreWord(value, dst_field_operand, scratch);
    RecordComment("]");
  } else {
    StoreP(value, dst_field_operand, scratch);
  }
}

void TurboAssembler::DecompressTaggedSigned(Register destination,
                                            Register src) {
  RecordComment("[ DecompressTaggedSigned");
  ZeroExtWord32(destination, src);
  RecordComment("]");
}

void TurboAssembler::DecompressTaggedSigned(Register destination,
                                            MemOperand field_operand) {
  RecordComment("[ DecompressTaggedSigned");
  LoadU32(destination, field_operand, r0);
  RecordComment("]");
}

void TurboAssembler::DecompressTaggedPointer(Register destination,
                                             Register source) {
  RecordComment("[ DecompressTaggedPointer");
  ZeroExtWord32(destination, source);
  add(destination, destination, kRootRegister);
  RecordComment("]");
}

void TurboAssembler::DecompressTaggedPointer(Register destination,
                                             MemOperand field_operand) {
  RecordComment("[ DecompressTaggedPointer");
  LoadU32(destination, field_operand, r0);
  add(destination, destination, kRootRegister);
  RecordComment("]");
}

void TurboAssembler::DecompressAnyTagged(Register destination,
                                         MemOperand field_operand) {
  RecordComment("[ DecompressAnyTagged");
  LoadU32(destination, field_operand, r0);
  add(destination, destination, kRootRegister);
  RecordComment("]");
}

void TurboAssembler::DecompressAnyTagged(Register destination,
                                         Register source) {
  RecordComment("[ DecompressAnyTagged");
  ZeroExtWord32(destination, source);
  add(destination, destination, kRootRegister);
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

  Add(dst, object, offset - kHeapObjectTag, r0);
  if (FLAG_debug_code) {
    Label ok;
    andi(r0, dst, Operand(kTaggedSize - 1));
    beq(&ok, cr0);
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

  push(object);
  push(address);

  pop(slot_parameter);
  pop(object_parameter);

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

  push(object);
  push(address);

  pop(slot_parameter);
  pop(object_parameter);

  if (mode == StubCallMode::kCallWasmRuntimeStub) {
    // Use {near_call} for direct Wasm call within a module.
    auto wasm_target =
        wasm::WasmCode::GetRecordWriteStub(remembered_set_action, fp_mode);
    Call(wasm_target, RelocInfo::WASM_STUB_CALL);
  } else {
    auto builtin_index =
        Builtins::GetRecordWriteStub(remembered_set_action, fp_mode);
    if (options().inline_offheap_trampolines) {
      RecordCommentForOffHeapTrampoline(builtin_index);
      EmbeddedData d = EmbeddedData::FromBlob();
      Address entry = d.InstructionStartOfBuiltin(builtin_index);
      // Use ip directly instead of using UseScratchRegisterScope, as we do
      // not preserve scratch registers across calls.
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
    cmp(r0, value);
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
    mflr(r0);
    push(r0);
  }
  CallRecordWriteStub(object, address, remembered_set_action, fp_mode);
  if (lr_status == kLRHasNotBeenSaved) {
    pop(r0);
    mtlr(r0);
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
  mflr(r0);
  if (FLAG_enable_embedded_constant_pool) {
    if (marker_reg.is_valid()) {
      Push(r0, fp, kConstantPoolRegister, marker_reg);
      fp_delta = 2;
    } else {
      Push(r0, fp, kConstantPoolRegister);
      fp_delta = 1;
    }
  } else {
    if (marker_reg.is_valid()) {
      Push(r0, fp, marker_reg);
      fp_delta = 1;
    } else {
      Push(r0, fp);
      fp_delta = 0;
    }
  }
  addi(fp, sp, Operand(fp_delta * kSystemPointerSize));
}

void TurboAssembler::PushStandardFrame(Register function_reg) {
  int fp_delta = 0;
  mflr(r0);
  if (FLAG_enable_embedded_constant_pool) {
    if (function_reg.is_valid()) {
      Push(r0, fp, kConstantPoolRegister, cp, function_reg);
      fp_delta = 3;
    } else {
      Push(r0, fp, kConstantPoolRegister, cp);
      fp_delta = 2;
    }
  } else {
    if (function_reg.is_valid()) {
      Push(r0, fp, cp, function_reg);
      fp_delta = 2;
    } else {
      Push(r0, fp, cp);
      fp_delta = 1;
    }
  }
  addi(fp, sp, Operand(fp_delta * kSystemPointerSize));
  Push(kJavaScriptCallArgCountRegister);
}

void TurboAssembler::RestoreFrameStateForTailCall() {
  if (FLAG_enable_embedded_constant_pool) {
    LoadU64(kConstantPoolRegister,
            MemOperand(fp, StandardFrameConstants::kConstantPoolOffset));
    set_constant_pool_available(false);
  }
  LoadU64(r0, MemOperand(fp, StandardFrameConstants::kCallerPCOffset));
  LoadU64(fp, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  mtlr(r0);
}

void TurboAssembler::CanonicalizeNaN(const DoubleRegister dst,
                                     const DoubleRegister src) {
  // Turn potential sNaN into qNaN.
  fsub(dst, src, kDoubleRegZero);
}

void TurboAssembler::ConvertIntToDouble(Register src, DoubleRegister dst) {
  MovIntToDouble(dst, src, r0);
  fcfid(dst, dst);
}

void TurboAssembler::ConvertUnsignedIntToDouble(Register src,
                                                DoubleRegister dst) {
  MovUnsignedIntToDouble(dst, src, r0);
  fcfid(dst, dst);
}

void TurboAssembler::ConvertIntToFloat(Register src, DoubleRegister dst) {
  MovIntToDouble(dst, src, r0);
  fcfids(dst, dst);
}

void TurboAssembler::ConvertUnsignedIntToFloat(Register src,
                                               DoubleRegister dst) {
  MovUnsignedIntToDouble(dst, src, r0);
  fcfids(dst, dst);
}

#if V8_TARGET_ARCH_PPC64
void TurboAssembler::ConvertInt64ToDouble(Register src,
                                          DoubleRegister double_dst) {
  MovInt64ToDouble(double_dst, src);
  fcfid(double_dst, double_dst);
}

void TurboAssembler::ConvertUnsignedInt64ToFloat(Register src,
                                                 DoubleRegister double_dst) {
  MovInt64ToDouble(double_dst, src);
  fcfidus(double_dst, double_dst);
}

void TurboAssembler::ConvertUnsignedInt64ToDouble(Register src,
                                                  DoubleRegister double_dst) {
  MovInt64ToDouble(double_dst, src);
  fcfidu(double_dst, double_dst);
}

void TurboAssembler::ConvertInt64ToFloat(Register src,
                                         DoubleRegister double_dst) {
  MovInt64ToDouble(double_dst, src);
  fcfids(double_dst, double_dst);
}
#endif

void TurboAssembler::ConvertDoubleToInt64(const DoubleRegister double_input,
#if !V8_TARGET_ARCH_PPC64
                                          const Register dst_hi,
#endif
                                          const Register dst,
                                          const DoubleRegister double_dst,
                                          FPRoundingMode rounding_mode) {
  if (rounding_mode == kRoundToZero) {
    fctidz(double_dst, double_input);
  } else {
    SetRoundingMode(rounding_mode);
    fctid(double_dst, double_input);
    ResetRoundingMode();
  }

  MovDoubleToInt64(
#if !V8_TARGET_ARCH_PPC64
      dst_hi,
#endif
      dst, double_dst);
}

#if V8_TARGET_ARCH_PPC64
void TurboAssembler::ConvertDoubleToUnsignedInt64(
    const DoubleRegister double_input, const Register dst,
    const DoubleRegister double_dst, FPRoundingMode rounding_mode) {
  if (rounding_mode == kRoundToZero) {
    fctiduz(double_dst, double_input);
  } else {
    SetRoundingMode(rounding_mode);
    fctidu(double_dst, double_input);
    ResetRoundingMode();
  }

  MovDoubleToInt64(dst, double_dst);
}
#endif

#if !V8_TARGET_ARCH_PPC64
void TurboAssembler::ShiftLeftPair(Register dst_low, Register dst_high,
                                   Register src_low, Register src_high,
                                   Register scratch, Register shift) {
  DCHECK(!AreAliased(dst_low, src_high));
  DCHECK(!AreAliased(dst_high, src_low));
  DCHECK(!AreAliased(dst_low, dst_high, shift));
  Label less_than_32;
  Label done;
  cmpi(shift, Operand(32));
  blt(&less_than_32);
  // If shift >= 32
  andi(scratch, shift, Operand(0x1F));
  slw(dst_high, src_low, scratch);
  li(dst_low, Operand::Zero());
  b(&done);
  bind(&less_than_32);
  // If shift < 32
  subfic(scratch, shift, Operand(32));
  slw(dst_high, src_high, shift);
  srw(scratch, src_low, scratch);
  orx(dst_high, dst_high, scratch);
  slw(dst_low, src_low, shift);
  bind(&done);
}

void TurboAssembler::ShiftLeftPair(Register dst_low, Register dst_high,
                                   Register src_low, Register src_high,
                                   uint32_t shift) {
  DCHECK(!AreAliased(dst_low, src_high));
  DCHECK(!AreAliased(dst_high, src_low));
  if (shift == 32) {
    Move(dst_high, src_low);
    li(dst_low, Operand::Zero());
  } else if (shift > 32) {
    shift &= 0x1F;
    slwi(dst_high, src_low, Operand(shift));
    li(dst_low, Operand::Zero());
  } else if (shift == 0) {
    Move(dst_low, src_low);
    Move(dst_high, src_high);
  } else {
    slwi(dst_high, src_high, Operand(shift));
    rlwimi(dst_high, src_low, shift, 32 - shift, 31);
    slwi(dst_low, src_low, Operand(shift));
  }
}

void TurboAssembler::ShiftRightPair(Register dst_low, Register dst_high,
                                    Register src_low, Register src_high,
                                    Register scratch, Register shift) {
  DCHECK(!AreAliased(dst_low, src_high));
  DCHECK(!AreAliased(dst_high, src_low));
  DCHECK(!AreAliased(dst_low, dst_high, shift));
  Label less_than_32;
  Label done;
  cmpi(shift, Operand(32));
  blt(&less_than_32);
  // If shift >= 32
  andi(scratch, shift, Operand(0x1F));
  srw(dst_low, src_high, scratch);
  li(dst_high, Operand::Zero());
  b(&done);
  bind(&less_than_32);
  // If shift < 32
  subfic(scratch, shift, Operand(32));
  srw(dst_low, src_low, shift);
  slw(scratch, src_high, scratch);
  orx(dst_low, dst_low, scratch);
  srw(dst_high, src_high, shift);
  bind(&done);
}

void TurboAssembler::ShiftRightPair(Register dst_low, Register dst_high,
                                    Register src_low, Register src_high,
                                    uint32_t shift) {
  DCHECK(!AreAliased(dst_low, src_high));
  DCHECK(!AreAliased(dst_high, src_low));
  if (shift == 32) {
    Move(dst_low, src_high);
    li(dst_high, Operand::Zero());
  } else if (shift > 32) {
    shift &= 0x1F;
    srwi(dst_low, src_high, Operand(shift));
    li(dst_high, Operand::Zero());
  } else if (shift == 0) {
    Move(dst_low, src_low);
    Move(dst_high, src_high);
  } else {
    srwi(dst_low, src_low, Operand(shift));
    rlwimi(dst_low, src_high, 32 - shift, 0, shift - 1);
    srwi(dst_high, src_high, Operand(shift));
  }
}

void TurboAssembler::ShiftRightAlgPair(Register dst_low, Register dst_high,
                                       Register src_low, Register src_high,
                                       Register scratch, Register shift) {
  DCHECK(!AreAliased(dst_low, src_high, shift));
  DCHECK(!AreAliased(dst_high, src_low, shift));
  Label less_than_32;
  Label done;
  cmpi(shift, Operand(32));
  blt(&less_than_32);
  // If shift >= 32
  andi(scratch, shift, Operand(0x1F));
  sraw(dst_low, src_high, scratch);
  srawi(dst_high, src_high, 31);
  b(&done);
  bind(&less_than_32);
  // If shift < 32
  subfic(scratch, shift, Operand(32));
  srw(dst_low, src_low, shift);
  slw(scratch, src_high, scratch);
  orx(dst_low, dst_low, scratch);
  sraw(dst_high, src_high, shift);
  bind(&done);
}

void TurboAssembler::ShiftRightAlgPair(Register dst_low, Register dst_high,
                                       Register src_low, Register src_high,
                                       uint32_t shift) {
  DCHECK(!AreAliased(dst_low, src_high));
  DCHECK(!AreAliased(dst_high, src_low));
  if (shift == 32) {
    Move(dst_low, src_high);
    srawi(dst_high, src_high, 31);
  } else if (shift > 32) {
    shift &= 0x1F;
    srawi(dst_low, src_high, shift);
    srawi(dst_high, src_high, 31);
  } else if (shift == 0) {
    Move(dst_low, src_low);
    Move(dst_high, src_high);
  } else {
    srwi(dst_low, src_low, Operand(shift));
    rlwimi(dst_low, src_high, 32 - shift, 0, shift - 1);
    srawi(dst_high, src_high, shift);
  }
}
#endif

void TurboAssembler::LoadConstantPoolPointerRegisterFromCodeTargetAddress(
    Register code_target_address) {
  // Builtins do not use the constant pool (see is_constant_pool_available).
  STATIC_ASSERT(Code::kOnHeapBodyIsContiguous);

  lwz(r0, MemOperand(code_target_address,
                     Code::kInstructionSizeOffset - Code::kHeaderSize));
  lwz(kConstantPoolRegister,
      MemOperand(code_target_address,
                 Code::kConstantPoolOffsetOffset - Code::kHeaderSize));
  add(kConstantPoolRegister, kConstantPoolRegister, code_target_address);
  add(kConstantPoolRegister, kConstantPoolRegister, r0);
}

void TurboAssembler::LoadPC(Register dst) {
  b(4, SetLK);
  mflr(dst);
}

void TurboAssembler::ComputeCodeStartAddress(Register dst) {
  mflr(r0);
  LoadPC(dst);
  subi(dst, dst, Operand(pc_offset() - kInstrSize));
  mtlr(r0);
}

void TurboAssembler::LoadConstantPoolPointerRegister() {
  //
  // Builtins do not use the constant pool (see is_constant_pool_available).
  STATIC_ASSERT(Code::kOnHeapBodyIsContiguous);

  LoadPC(kConstantPoolRegister);
  int32_t delta = -pc_offset() + 4;
  add_label_offset(kConstantPoolRegister, kConstantPoolRegister,
                   ConstantPoolPosition(), delta);
}

void TurboAssembler::StubPrologue(StackFrame::Type type) {
  {
    ConstantPoolUnavailableScope constant_pool_unavailable(this);
    mov(r11, Operand(StackFrame::TypeToMarker(type)));
    PushCommonFrame(r11);
  }
  if (FLAG_enable_embedded_constant_pool) {
    LoadConstantPoolPointerRegister();
    set_constant_pool_available(true);
  }
}

void TurboAssembler::Prologue() {
  PushStandardFrame(r4);
  if (FLAG_enable_embedded_constant_pool) {
    // base contains prologue address
    LoadConstantPoolPointerRegister();
    set_constant_pool_available(true);
  }
}

void TurboAssembler::EnterFrame(StackFrame::Type type,
                                bool load_constant_pool_pointer_reg) {
  if (FLAG_enable_embedded_constant_pool && load_constant_pool_pointer_reg) {
    // Push type explicitly so we can leverage the constant pool.
    // This path cannot rely on ip containing code entry.
    PushCommonFrame();
    LoadConstantPoolPointerRegister();
    mov(ip, Operand(StackFrame::TypeToMarker(type)));
    push(ip);
  } else {
    mov(ip, Operand(StackFrame::TypeToMarker(type)));
    PushCommonFrame(ip);
  }
}

int TurboAssembler::LeaveFrame(StackFrame::Type type, int stack_adjustment) {
  ConstantPoolUnavailableScope constant_pool_unavailable(this);
  // r3: preserved
  // r4: preserved
  // r5: preserved

  // Drop the execution stack down to the frame pointer and restore
  // the caller's state.
  int frame_ends;
  LoadU64(r0, MemOperand(fp, StandardFrameConstants::kCallerPCOffset));
  LoadU64(ip, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  if (FLAG_enable_embedded_constant_pool) {
    LoadU64(kConstantPoolRegister,
            MemOperand(fp, StandardFrameConstants::kConstantPoolOffset));
  }
  mtlr(r0);
  frame_ends = pc_offset();
  Add(sp, fp, StandardFrameConstants::kCallerSPOffset + stack_adjustment, r0);
  mr(fp, ip);
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
// in the fp register (r31)
// Then - we buy a new frame

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

  mov(ip, Operand(StackFrame::TypeToMarker(frame_type)));
  PushCommonFrame(ip);
  // Reserve room for saved entry sp.
  subi(sp, fp, Operand(ExitFrameConstants::kFixedFrameSizeFromFp));

  if (FLAG_debug_code) {
    li(r8, Operand::Zero());
    StoreP(r8, MemOperand(fp, ExitFrameConstants::kSPOffset));
  }
  if (FLAG_enable_embedded_constant_pool) {
    StoreP(kConstantPoolRegister,
           MemOperand(fp, ExitFrameConstants::kConstantPoolOffset));
  }

  // Save the frame pointer and the context in top.
  Move(r8, ExternalReference::Create(IsolateAddressId::kCEntryFPAddress,
                                     isolate()));
  StoreP(fp, MemOperand(r8));
  Move(r8,
       ExternalReference::Create(IsolateAddressId::kContextAddress, isolate()));
  StoreP(cp, MemOperand(r8));

  // Optionally save all volatile double registers.
  if (save_doubles) {
    MultiPushDoubles(kCallerSavedDoubles);
    // Note that d0 will be accessible at
    //   fp - ExitFrameConstants::kFrameSize -
    //   kNumCallerSavedDoubles * kDoubleSize,
    // since the sp slot and code slot were pushed after the fp.
  }

  addi(sp, sp, Operand(-stack_space * kSystemPointerSize));

  // Allocate and align the frame preparing for calling the runtime
  // function.
  const int frame_alignment = ActivationFrameAlignment();
  if (frame_alignment > kSystemPointerSize) {
    DCHECK(base::bits::IsPowerOfTwo(frame_alignment));
    ClearRightImm(sp, sp,
                  Operand(base::bits::WhichPowerOfTwo(frame_alignment)));
  }
  li(r0, Operand::Zero());
  StorePU(r0,
          MemOperand(sp, -kNumRequiredStackFrameSlots * kSystemPointerSize));

  // Set the exit frame sp value to point just before the return address
  // location.
  addi(r8, sp, Operand((kStackFrameExtraParamSlot + 1) * kSystemPointerSize));
  StoreP(r8, MemOperand(fp, ExitFrameConstants::kSPOffset));
}

int TurboAssembler::ActivationFrameAlignment() {
#if !defined(USE_SIMULATOR)
  // Running on the real platform. Use the alignment as mandated by the local
  // environment.
  // Note: This will break if we ever start generating snapshots on one PPC
  // platform for another PPC platform with a different alignment.
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
  ConstantPoolUnavailableScope constant_pool_unavailable(this);
  // Optionally restore all double registers.
  if (save_doubles) {
    // Calculate the stack location of the saved doubles and restore them.
    const int kNumRegs = kNumCallerSavedDoubles;
    const int offset =
        (ExitFrameConstants::kFixedFrameSizeFromFp + kNumRegs * kDoubleSize);
    addi(r6, fp, Operand(-offset));
    MultiPopDoubles(kCallerSavedDoubles, r6);
  }

  // Clear top frame.
  li(r6, Operand::Zero());
  Move(ip, ExternalReference::Create(IsolateAddressId::kCEntryFPAddress,
                                     isolate()));
  StoreP(r6, MemOperand(ip));

  // Restore current context from top and clear it in debug mode.
  Move(ip,
       ExternalReference::Create(IsolateAddressId::kContextAddress, isolate()));
  LoadU64(cp, MemOperand(ip));

#ifdef DEBUG
  mov(r6, Operand(Context::kInvalidContext));
  Move(ip,
       ExternalReference::Create(IsolateAddressId::kContextAddress, isolate()));
  StoreP(r6, MemOperand(ip));
#endif

  // Tear down the exit frame, pop the arguments, and return.
  LeaveFrame(StackFrame::EXIT);

  if (argument_count.is_valid()) {
    if (!argument_count_is_length) {
      ShiftLeftImm(argument_count, argument_count,
                   Operand(kSystemPointerSizeLog2));
    }
    add(sp, sp, argument_count);
  }
}

void TurboAssembler::MovFromFloatResult(const DoubleRegister dst) {
  Move(dst, d1);
}

void TurboAssembler::MovFromFloatParameter(const DoubleRegister dst) {
  Move(dst, d1);
}

void TurboAssembler::PrepareForTailCall(Register callee_args_count,
                                        Register caller_args_count,
                                        Register scratch0, Register scratch1) {
  DCHECK(!AreAliased(callee_args_count, caller_args_count, scratch0, scratch1));

  // Calculate the end of destination area where we will put the arguments
  // after we drop current frame. We add kSystemPointerSize to count the
  // receiver argument which is not included into formal parameters count.
  Register dst_reg = scratch0;
  ShiftLeftImm(dst_reg, caller_args_count, Operand(kSystemPointerSizeLog2));
  add(dst_reg, fp, dst_reg);
  addi(dst_reg, dst_reg,
       Operand(StandardFrameConstants::kCallerSPOffset + kSystemPointerSize));

  Register src_reg = caller_args_count;
  // Calculate the end of source area. +kSystemPointerSize is for the receiver.
  ShiftLeftImm(src_reg, callee_args_count, Operand(kSystemPointerSizeLog2));
  add(src_reg, sp, src_reg);
  addi(src_reg, src_reg, Operand(kSystemPointerSize));

  if (FLAG_debug_code) {
    cmpl(src_reg, dst_reg);
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
  addi(tmp_reg, callee_args_count, Operand(1));  // +1 for receiver
  mtctr(tmp_reg);
  bind(&loop);
  LoadPU(tmp_reg, MemOperand(src_reg, -kSystemPointerSize));
  StorePU(tmp_reg, MemOperand(dst_reg, -kSystemPointerSize));
  bdnz(&loop);

  // Leave current frame.
  mr(sp, dst_reg);
}

void MacroAssembler::LoadStackLimit(Register destination, StackLimitKind kind) {
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
  LoadU64(destination, MemOperand(kRootRegister, offset), r0);
}

void MacroAssembler::StackOverflowCheck(Register num_args, Register scratch,
                                        Label* stack_overflow) {
  // Check the stack for overflow. We are not trying to catch
  // interruptions (e.g. debug break and preemption) here, so the "real stack
  // limit" is checked.
  LoadStackLimit(scratch, StackLimitKind::kRealStackLimit);
  // Make scratch the space we have left. The stack might already be overflowed
  // here which will cause scratch to become negative.
  sub(scratch, sp, scratch);
  // Check if the arguments will overflow the stack.
  ShiftLeftImm(r0, num_args, Operand(kSystemPointerSizeLog2));
  cmp(scratch, r0);
  ble(stack_overflow);  // Signed comparison.
}

void MacroAssembler::InvokePrologue(Register expected_parameter_count,
                                    Register actual_parameter_count,
                                    Label* done, InvokeType type) {
  Label regular_invoke;

  //  r3: actual arguments count
  //  r4: function (passed through to callee)
  //  r5: expected arguments count

  DCHECK_EQ(actual_parameter_count, r3);
  DCHECK_EQ(expected_parameter_count, r5);

  // If the expected parameter count is equal to the adaptor sentinel, no need
  // to push undefined value as arguments.
  mov(r0, Operand(kDontAdaptArgumentsSentinel));
  cmp(expected_parameter_count, r0);
  beq(&regular_invoke);

  // If overapplication or if the actual argument count is equal to the
  // formal parameter count, no need to push extra undefined values.
  sub(expected_parameter_count, expected_parameter_count,
      actual_parameter_count, LeaveOE, SetRC);
  ble(&regular_invoke, cr0);

  Label stack_overflow;
  Register scratch = r7;
  StackOverflowCheck(expected_parameter_count, scratch, &stack_overflow);

  // Underapplication. Move the arguments already in the stack, including the
  // receiver and the return address.
  {
    Label copy;
    Register src = r9, dest = r8;
    addi(src, sp, Operand(-kSystemPointerSize));
    ShiftLeftImm(r0, expected_parameter_count, Operand(kSystemPointerSizeLog2));
    sub(sp, sp, r0);
    // Update stack pointer.
    addi(dest, sp, Operand(-kSystemPointerSize));
    addi(r0, actual_parameter_count, Operand(1));
    mtctr(r0);

    bind(&copy);
    LoadPU(r0, MemOperand(src, kSystemPointerSize));
    StorePU(r0, MemOperand(dest, kSystemPointerSize));
    bdnz(&copy);
  }

  // Fill remaining expected arguments with undefined values.
  LoadRoot(scratch, RootIndex::kUndefinedValue);
  {
    mtctr(expected_parameter_count);

    Label loop;
    bind(&loop);
    StorePU(scratch, MemOperand(r8, kSystemPointerSize));
    bdnz(&loop);
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
  Move(r7, debug_hook_active);
  LoadU8(r7, MemOperand(r7), r0);
  extsb(r7, r7);
  CmpSmiLiteral(r7, Smi::zero(), r0);
  beq(&skip_hook);

  {
    // Load receiver to pass it later to DebugOnFunctionCall hook.
    LoadReceiver(r7, actual_parameter_count);
    FrameScope frame(this,
                     has_frame() ? StackFrame::NONE : StackFrame::INTERNAL);

    SmiTag(expected_parameter_count);
    Push(expected_parameter_count);

    SmiTag(actual_parameter_count);
    Push(actual_parameter_count);

    if (new_target.is_valid()) {
      Push(new_target);
    }
    Push(fun, fun, r7);
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
  DCHECK_EQ(function, r4);
  DCHECK_IMPLIES(new_target.is_valid(), new_target == r6);

  // On function call, call into the debugger if necessary.
  CheckDebugHook(function, new_target, expected_parameter_count,
                 actual_parameter_count);

  // Clear the new.target register if not given.
  if (!new_target.is_valid()) {
    LoadRoot(r6, RootIndex::kUndefinedValue);
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

  // Contract with called JS functions requires that function is passed in r4.
  DCHECK_EQ(fun, r4);

  Register expected_reg = r5;
  Register temp_reg = r7;

  LoadTaggedPointerField(
      temp_reg, FieldMemOperand(r4, JSFunction::kSharedFunctionInfoOffset));
  LoadTaggedPointerField(cp, FieldMemOperand(r4, JSFunction::kContextOffset));
  LoadU16(expected_reg,
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

  // Contract with called JS functions requires that function is passed in r4.
  DCHECK_EQ(function, r4);

  // Get the function and setup the context.
  LoadTaggedPointerField(cp, FieldMemOperand(r4, JSFunction::kContextOffset));

  InvokeFunctionCode(r4, no_reg, expected_parameter_count,
                     actual_parameter_count, type);
}

void MacroAssembler::PushStackHandler() {
  // Adjust this code if not the case.
  STATIC_ASSERT(StackHandlerConstants::kSize == 2 * kSystemPointerSize);
  STATIC_ASSERT(StackHandlerConstants::kNextOffset == 0 * kSystemPointerSize);

  Push(Smi::zero());  // Padding.

  // Link the current handler as the next handler.
  // Preserve r4-r8.
  Move(r3,
       ExternalReference::Create(IsolateAddressId::kHandlerAddress, isolate()));
  LoadU64(r0, MemOperand(r3));
  push(r0);

  // Set this new handler as the current one.
  StoreP(sp, MemOperand(r3));
}

void MacroAssembler::PopStackHandler() {
  STATIC_ASSERT(StackHandlerConstants::kSize == 2 * kSystemPointerSize);
  STATIC_ASSERT(StackHandlerConstants::kNextOffset == 0);

  pop(r4);
  Move(ip,
       ExternalReference::Create(IsolateAddressId::kHandlerAddress, isolate()));
  StoreP(r4, MemOperand(ip));

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
  lhz(type_reg, FieldMemOperand(map, Map::kInstanceTypeOffset));
  cmpi(type_reg, Operand(type));
}

void MacroAssembler::CompareInstanceTypeRange(Register map, Register type_reg,
                                              InstanceType lower_limit,
                                              InstanceType higher_limit) {
  DCHECK_LT(lower_limit, higher_limit);
  UseScratchRegisterScope temps(this);
  Register scratch = temps.Acquire();
  LoadU16(type_reg, FieldMemOperand(map, Map::kInstanceTypeOffset));
  mov(scratch, Operand(lower_limit));
  sub(scratch, type_reg, scratch);
  cmpli(scratch, Operand(higher_limit - lower_limit));
}

void MacroAssembler::CompareRoot(Register obj, RootIndex index) {
  DCHECK(obj != r0);
  LoadRoot(r0, index);
  cmp(obj, r0);
}

void TurboAssembler::AddAndCheckForOverflow(Register dst, Register left,
                                            Register right,
                                            Register overflow_dst,
                                            Register scratch) {
  DCHECK(dst != overflow_dst);
  DCHECK(dst != scratch);
  DCHECK(overflow_dst != scratch);
  DCHECK(overflow_dst != left);
  DCHECK(overflow_dst != right);

  bool left_is_right = left == right;
  RCBit xorRC = left_is_right ? SetRC : LeaveRC;

  // C = A+B; C overflows if A/B have same sign and C has diff sign than A
  if (dst == left) {
    mr(scratch, left);                        // Preserve left.
    add(dst, left, right);                    // Left is overwritten.
    xor_(overflow_dst, dst, scratch, xorRC);  // Original left.
    if (!left_is_right) xor_(scratch, dst, right);
  } else if (dst == right) {
    mr(scratch, right);     // Preserve right.
    add(dst, left, right);  // Right is overwritten.
    xor_(overflow_dst, dst, left, xorRC);
    if (!left_is_right) xor_(scratch, dst, scratch);  // Original right.
  } else {
    add(dst, left, right);
    xor_(overflow_dst, dst, left, xorRC);
    if (!left_is_right) xor_(scratch, dst, right);
  }
  if (!left_is_right) and_(overflow_dst, scratch, overflow_dst, SetRC);
}

void TurboAssembler::AddAndCheckForOverflow(Register dst, Register left,
                                            intptr_t right,
                                            Register overflow_dst,
                                            Register scratch) {
  Register original_left = left;
  DCHECK(dst != overflow_dst);
  DCHECK(dst != scratch);
  DCHECK(overflow_dst != scratch);
  DCHECK(overflow_dst != left);

  // C = A+B; C overflows if A/B have same sign and C has diff sign than A
  if (dst == left) {
    // Preserve left.
    original_left = overflow_dst;
    mr(original_left, left);
  }
  Add(dst, left, right, scratch);
  xor_(overflow_dst, dst, original_left);
  if (right >= 0) {
    and_(overflow_dst, overflow_dst, dst, SetRC);
  } else {
    andc(overflow_dst, overflow_dst, dst, SetRC);
  }
}

void TurboAssembler::SubAndCheckForOverflow(Register dst, Register left,
                                            Register right,
                                            Register overflow_dst,
                                            Register scratch) {
  DCHECK(dst != overflow_dst);
  DCHECK(dst != scratch);
  DCHECK(overflow_dst != scratch);
  DCHECK(overflow_dst != left);
  DCHECK(overflow_dst != right);

  // C = A-B; C overflows if A/B have diff signs and C has diff sign than A
  if (dst == left) {
    mr(scratch, left);      // Preserve left.
    sub(dst, left, right);  // Left is overwritten.
    xor_(overflow_dst, dst, scratch);
    xor_(scratch, scratch, right);
    and_(overflow_dst, overflow_dst, scratch, SetRC);
  } else if (dst == right) {
    mr(scratch, right);     // Preserve right.
    sub(dst, left, right);  // Right is overwritten.
    xor_(overflow_dst, dst, left);
    xor_(scratch, left, scratch);
    and_(overflow_dst, overflow_dst, scratch, SetRC);
  } else {
    sub(dst, left, right);
    xor_(overflow_dst, dst, left);
    xor_(scratch, left, right);
    and_(overflow_dst, scratch, overflow_dst, SetRC);
  }
}

void MacroAssembler::JumpIfIsInRange(Register value, unsigned lower_limit,
                                     unsigned higher_limit,
                                     Label* on_in_range) {
  Register scratch = r0;
  if (lower_limit != 0) {
    mov(scratch, Operand(lower_limit));
    sub(scratch, value, scratch);
    cmpli(scratch, Operand(higher_limit - lower_limit));
  } else {
    mov(scratch, Operand(higher_limit));
    cmpl(value, scratch);
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
  mflr(r0);
  push(r0);
  // Put input on stack.
  stfdu(double_input, MemOperand(sp, -kDoubleSize));

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

  LoadU64(result, MemOperand(sp));
  addi(sp, sp, Operand(kDoubleSize));
  pop(r0);
  mtlr(r0);

  bind(&done);
}

void TurboAssembler::TryInlineTruncateDoubleToI(Register result,
                                                DoubleRegister double_input,
                                                Label* done) {
  DoubleRegister double_scratch = kScratchDoubleReg;
#if !V8_TARGET_ARCH_PPC64
  Register scratch = ip;
#endif

  ConvertDoubleToInt64(double_input,
#if !V8_TARGET_ARCH_PPC64
                       scratch,
#endif
                       result, double_scratch);

// Test for overflow
#if V8_TARGET_ARCH_PPC64
  TestIfInt32(result, r0);
#else
  TestIfInt32(scratch, result, r0);
#endif
  beq(done);
}

void MacroAssembler::CallRuntime(const Runtime::Function* f, int num_arguments,
                                 SaveFPRegsMode save_doubles) {
  // All parameters are on the stack.  r3 has the return value after call.

  // If the expected number of arguments of the runtime function is
  // constant, we check that the actual number of arguments match the
  // expectation.
  CHECK(f->nargs < 0 || f->nargs == num_arguments);

  // TODO(1236192): Most runtime routines don't need the number of
  // arguments passed in because it is constant. At some point we
  // should remove this need and make the runtime routine entry code
  // smarter.
  mov(r3, Operand(num_arguments));
  Move(r4, ExternalReference::Create(f));
#if V8_TARGET_ARCH_PPC64
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
    mov(r3, Operand(function->nargs));
  }
  JumpToExternalReference(ExternalReference::Create(fid));
}

void MacroAssembler::JumpToExternalReference(const ExternalReference& builtin,
                                             bool builtin_exit_frame) {
  Move(r4, builtin);
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
  cmpwi(in, Operand(kClearedWeakHeapObjectLower32));
  beq(target_if_cleared);

  mov(r0, Operand(~kWeakHeapObjectMask));
  and_(out, in, r0);
}

void MacroAssembler::IncrementCounter(StatsCounter* counter, int value,
                                      Register scratch1, Register scratch2) {
  DCHECK_GT(value, 0);
  if (FLAG_native_code_counters && counter->Enabled()) {
    // This operation has to be exactly 32-bit wide in case the external
    // reference table redirects the counter to a uint32_t dummy_stats_counter_
    // field.
    Move(scratch2, ExternalReference::Create(counter));
    lwz(scratch1, MemOperand(scratch2));
    addi(scratch1, scratch1, Operand(value));
    stw(scratch1, MemOperand(scratch2));
  }
}

void MacroAssembler::DecrementCounter(StatsCounter* counter, int value,
                                      Register scratch1, Register scratch2) {
  DCHECK_GT(value, 0);
  if (FLAG_native_code_counters && counter->Enabled()) {
    // This operation has to be exactly 32-bit wide in case the external
    // reference table redirects the counter to a uint32_t dummy_stats_counter_
    // field.
    Move(scratch2, ExternalReference::Create(counter));
    lwz(scratch1, MemOperand(scratch2));
    subi(scratch1, scratch1, Operand(value));
    stw(scratch1, MemOperand(scratch2));
  }
}

void TurboAssembler::Assert(Condition cond, AbortReason reason, CRegister cr) {
  if (FLAG_debug_code) Check(cond, reason, cr);
}

void TurboAssembler::Check(Condition cond, AbortReason reason, CRegister cr) {
  Label L;
  b(cond, &L, cr);
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
    mov(r3, Operand(static_cast<int>(reason)));
    PrepareCallCFunction(1, r4);
    CallCFunction(ExternalReference::abort_with_reason(), 1);
    return;
  }

  LoadSmiLiteral(r4, Smi::FromInt(static_cast<int>(reason)));

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
    TestIfSmi(object, r0);
    Check(ne, AbortReason::kOperandIsASmi, cr0);
  }
}

void MacroAssembler::AssertSmi(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object, r0);
    Check(eq, AbortReason::kOperandIsNotASmi, cr0);
  }
}

void MacroAssembler::AssertConstructor(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object, r0);
    Check(ne, AbortReason::kOperandIsASmiAndNotAConstructor, cr0);
    push(object);
    LoadMap(object, object);
    lbz(object, FieldMemOperand(object, Map::kBitFieldOffset));
    andi(object, object, Operand(Map::Bits1::IsConstructorBit::kMask));
    pop(object);
    Check(ne, AbortReason::kOperandIsNotAConstructor, cr0);
  }
}

void MacroAssembler::AssertFunction(Register object) {
  if (FLAG_debug_code) {
    STATIC_ASSERT(kSmiTag == 0);
    TestIfSmi(object, r0);
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
    TestIfSmi(object, r0);
    Check(ne, AbortReason::kOperandIsASmiAndNotABoundFunction, cr0);
    push(object);
    CompareObjectType(object, object, object, JS_BOUND_FUNCTION_TYPE);
    pop(object);
    Check(eq, AbortReason::kOperandIsNotABoundFunction);
  }
}

void MacroAssembler::AssertGeneratorObject(Register object) {
  if (!FLAG_debug_code) return;
  TestIfSmi(object, r0);
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
  cmpi(instance_type, Operand(JS_ASYNC_FUNCTION_OBJECT_TYPE));
  beq(&do_check);

  // Check if JSAsyncGeneratorObject (See MacroAssembler::CompareInstanceType)
  cmpi(instance_type, Operand(JS_ASYNC_GENERATOR_OBJECT_TYPE));

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
    beq(&done_checking);
    LoadMap(scratch, object);
    CompareInstanceType(scratch, scratch, ALLOCATION_SITE_TYPE);
    Assert(eq, AbortReason::kExpectedUndefinedOrCell);
    bind(&done_checking);
  }
}

static const int kRegisterPassedArguments = 8;

int TurboAssembler::CalculateStackPassedWords(int num_reg_arguments,
                                              int num_double_arguments) {
  int stack_passed_words = 0;
  if (num_double_arguments > DoubleRegister::kNumRegisters) {
    stack_passed_words +=
        2 * (num_double_arguments - DoubleRegister::kNumRegisters);
  }
  // Up to 8 simple arguments are passed in registers r3..r10.
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
    mr(scratch, sp);
    addi(sp, sp, Operand(-(stack_passed_arguments + 1) * kSystemPointerSize));
    DCHECK(base::bits::IsPowerOfTwo(frame_alignment));
    ClearRightImm(sp, sp,
                  Operand(base::bits::WhichPowerOfTwo(frame_alignment)));
    StoreP(scratch,
           MemOperand(sp, stack_passed_arguments * kSystemPointerSize));
  } else {
    // Make room for stack arguments
    stack_space += stack_passed_arguments;
  }

  // Allocate frame with required slots to make ABI work.
  li(r0, Operand::Zero());
  StorePU(r0, MemOperand(sp, -stack_space * kSystemPointerSize));
}

void TurboAssembler::PrepareCallCFunction(int num_reg_arguments,
                                          Register scratch) {
  PrepareCallCFunction(num_reg_arguments, 0, scratch);
}

void TurboAssembler::MovToFloatParameter(DoubleRegister src) { Move(d1, src); }

void TurboAssembler::MovToFloatResult(DoubleRegister src) { Move(d1, src); }

void TurboAssembler::MovToFloatParameters(DoubleRegister src1,
                                          DoubleRegister src2) {
  if (src2 == d1) {
    DCHECK(src1 != d2);
    Move(d2, src2);
    Move(d1, src1);
  } else {
    Move(d1, src1);
    Move(d2, src2);
  }
}

void TurboAssembler::CallCFunction(ExternalReference function,
                                   int num_reg_arguments,
                                   int num_double_arguments,
                                   bool has_function_descriptor) {
  Move(ip, function);
  CallCFunctionHelper(ip, num_reg_arguments, num_double_arguments,
                      has_function_descriptor);
}

void TurboAssembler::CallCFunction(Register function, int num_reg_arguments,
                                   int num_double_arguments,
                                   bool has_function_descriptor) {
  CallCFunctionHelper(function, num_reg_arguments, num_double_arguments,
                      has_function_descriptor);
}

void TurboAssembler::CallCFunction(ExternalReference function,
                                   int num_arguments,
                                   bool has_function_descriptor) {
  CallCFunction(function, num_arguments, 0, has_function_descriptor);
}

void TurboAssembler::CallCFunction(Register function, int num_arguments,
                                   bool has_function_descriptor) {
  CallCFunction(function, num_arguments, 0, has_function_descriptor);
}

void TurboAssembler::CallCFunctionHelper(Register function,
                                         int num_reg_arguments,
                                         int num_double_arguments,
                                         bool has_function_descriptor) {
  DCHECK_LE(num_reg_arguments + num_double_arguments, kMaxCParameters);
  DCHECK(has_frame());

  // Save the frame pointer and PC so that the stack layout remains iterable,
  // even without an ExitFrame which normally exists between JS and C frames.
  Register addr_scratch = r7;
  Register scratch = r8;
  Push(scratch);
  mflr(scratch);
  // See x64 code for reasoning about how to address the isolate data fields.
  if (root_array_available()) {
    LoadPC(r0);
    StoreP(r0, MemOperand(kRootRegister,
                          IsolateData::fast_c_call_caller_pc_offset()));
    StoreP(fp, MemOperand(kRootRegister,
                          IsolateData::fast_c_call_caller_fp_offset()));
  } else {
    DCHECK_NOT_NULL(isolate());
    Push(addr_scratch);

    Move(addr_scratch,
         ExternalReference::fast_c_call_caller_pc_address(isolate()));
    LoadPC(r0);
    StoreP(r0, MemOperand(addr_scratch));
    Move(addr_scratch,
         ExternalReference::fast_c_call_caller_fp_address(isolate()));
    StoreP(fp, MemOperand(addr_scratch));
    Pop(addr_scratch);
  }
  mtlr(scratch);
  Pop(scratch);

  // Just call directly. The function called cannot cause a GC, or
  // allow preemption, so the return address in the link register
  // stays correct.
  Register dest = function;
  if (ABI_USES_FUNCTION_DESCRIPTORS && has_function_descriptor) {
    // AIX/PPC64BE Linux uses a function descriptor. When calling C code be
    // aware of this descriptor and pick up values from it
    LoadU64(ToRegister(ABI_TOC_REGISTER),
            MemOperand(function, kSystemPointerSize));
    LoadU64(ip, MemOperand(function, 0));
    dest = ip;
  } else if (ABI_CALL_VIA_IP) {
    // pLinux and Simualtor, not AIX
    Move(ip, function);
    dest = ip;
  }

  Call(dest);

  // We don't unset the PC; the FP is the source of truth.
  Register zero_scratch = r0;
  mov(zero_scratch, Operand::Zero());

  if (root_array_available()) {
    StoreP(
        zero_scratch,
        MemOperand(kRootRegister, IsolateData::fast_c_call_caller_fp_offset()));
  } else {
    DCHECK_NOT_NULL(isolate());
    Push(addr_scratch);
    Move(addr_scratch,
         ExternalReference::fast_c_call_caller_fp_address(isolate()));
    StoreP(zero_scratch, MemOperand(addr_scratch));
    Pop(addr_scratch);
  }

  // Remove frame bought in PrepareCallCFunction
  int stack_passed_arguments =
      CalculateStackPassedWords(num_reg_arguments, num_double_arguments);
  int stack_space = kNumRequiredStackFrameSlots + stack_passed_arguments;
  if (ActivationFrameAlignment() > kSystemPointerSize) {
    LoadU64(sp, MemOperand(sp, stack_space * kSystemPointerSize));
  } else {
    addi(sp, sp, Operand(stack_space * kSystemPointerSize));
  }
}

void TurboAssembler::CheckPageFlag(
    Register object,
    Register scratch,  // scratch may be same register as object
    int mask, Condition cc, Label* condition_met) {
  DCHECK(cc == ne || cc == eq);
  ClearRightImm(scratch, object, Operand(kPageSizeBits));
  LoadU64(scratch, MemOperand(scratch, BasicMemoryChunk::kFlagsOffset));

  mov(r0, Operand(mask));
  and_(r0, scratch, r0, SetRC);

  if (cc == ne) {
    bne(condition_met, cr0);
  }
  if (cc == eq) {
    beq(condition_met, cr0);
  }
}

void TurboAssembler::SetRoundingMode(FPRoundingMode RN) { mtfsfi(7, RN); }

void TurboAssembler::ResetRoundingMode() {
  mtfsfi(7, kRoundToNearest);  // reset (default is kRoundToNearest)
}

////////////////////////////////////////////////////////////////////////////////
//
// New MacroAssembler Interfaces added for PPC
//
////////////////////////////////////////////////////////////////////////////////
void TurboAssembler::LoadIntLiteral(Register dst, int value) {
  mov(dst, Operand(value));
}

void TurboAssembler::LoadSmiLiteral(Register dst, Smi smi) {
  mov(dst, Operand(smi));
}

void TurboAssembler::LoadDoubleLiteral(DoubleRegister result, Double value,
                                       Register scratch) {
  if (FLAG_enable_embedded_constant_pool && is_constant_pool_available() &&
      !(scratch == r0 && ConstantPoolAccessIsInOverflow())) {
    ConstantPoolEntry::Access access = ConstantPoolAddEntry(value);
    if (access == ConstantPoolEntry::OVERFLOWED) {
      addis(scratch, kConstantPoolRegister, Operand::Zero());
      lfd(result, MemOperand(scratch, 0));
    } else {
      lfd(result, MemOperand(kConstantPoolRegister, 0));
    }
    return;
  }

  // avoid gcc strict aliasing error using union cast
  union {
    uint64_t dval;
#if V8_TARGET_ARCH_PPC64
    intptr_t ival;
#else
    intptr_t ival[2];
#endif
  } litVal;

  litVal.dval = value.AsUint64();

#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mov(scratch, Operand(litVal.ival));
    mtfprd(result, scratch);
    return;
  }
#endif

  addi(sp, sp, Operand(-kDoubleSize));
#if V8_TARGET_ARCH_PPC64
  mov(scratch, Operand(litVal.ival));
  std(scratch, MemOperand(sp));
#else
  LoadIntLiteral(scratch, litVal.ival[0]);
  stw(scratch, MemOperand(sp, 0));
  LoadIntLiteral(scratch, litVal.ival[1]);
  stw(scratch, MemOperand(sp, 4));
#endif
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(result, MemOperand(sp, 0));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovIntToDouble(DoubleRegister dst, Register src,
                                    Register scratch) {
// sign-extend src to 64-bit
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mtfprwa(dst, src);
    return;
  }
#endif

  DCHECK(src != scratch);
  subi(sp, sp, Operand(kDoubleSize));
#if V8_TARGET_ARCH_PPC64
  extsw(scratch, src);
  std(scratch, MemOperand(sp, 0));
#else
  srawi(scratch, src, 31);
  stw(scratch, MemOperand(sp, Register::kExponentOffset));
  stw(src, MemOperand(sp, Register::kMantissaOffset));
#endif
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(dst, MemOperand(sp, 0));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovUnsignedIntToDouble(DoubleRegister dst, Register src,
                                            Register scratch) {
// zero-extend src to 64-bit
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mtfprwz(dst, src);
    return;
  }
#endif

  DCHECK(src != scratch);
  subi(sp, sp, Operand(kDoubleSize));
#if V8_TARGET_ARCH_PPC64
  clrldi(scratch, src, Operand(32));
  std(scratch, MemOperand(sp, 0));
#else
  li(scratch, Operand::Zero());
  stw(scratch, MemOperand(sp, Register::kExponentOffset));
  stw(src, MemOperand(sp, Register::kMantissaOffset));
#endif
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(dst, MemOperand(sp, 0));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovInt64ToDouble(DoubleRegister dst,
#if !V8_TARGET_ARCH_PPC64
                                      Register src_hi,
#endif
                                      Register src) {
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mtfprd(dst, src);
    return;
  }
#endif

  subi(sp, sp, Operand(kDoubleSize));
#if V8_TARGET_ARCH_PPC64
  std(src, MemOperand(sp, 0));
#else
  stw(src_hi, MemOperand(sp, Register::kExponentOffset));
  stw(src, MemOperand(sp, Register::kMantissaOffset));
#endif
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(dst, MemOperand(sp, 0));
  addi(sp, sp, Operand(kDoubleSize));
}

#if V8_TARGET_ARCH_PPC64
void TurboAssembler::MovInt64ComponentsToDouble(DoubleRegister dst,
                                                Register src_hi,
                                                Register src_lo,
                                                Register scratch) {
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    sldi(scratch, src_hi, Operand(32));
    rldimi(scratch, src_lo, 0, 32);
    mtfprd(dst, scratch);
    return;
  }

  subi(sp, sp, Operand(kDoubleSize));
  stw(src_hi, MemOperand(sp, Register::kExponentOffset));
  stw(src_lo, MemOperand(sp, Register::kMantissaOffset));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(dst, MemOperand(sp));
  addi(sp, sp, Operand(kDoubleSize));
}
#endif

void TurboAssembler::InsertDoubleLow(DoubleRegister dst, Register src,
                                     Register scratch) {
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mffprd(scratch, dst);
    rldimi(scratch, src, 0, 32);
    mtfprd(dst, scratch);
    return;
  }
#endif

  subi(sp, sp, Operand(kDoubleSize));
  stfd(dst, MemOperand(sp));
  stw(src, MemOperand(sp, Register::kMantissaOffset));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(dst, MemOperand(sp));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::InsertDoubleHigh(DoubleRegister dst, Register src,
                                      Register scratch) {
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mffprd(scratch, dst);
    rldimi(scratch, src, 32, 0);
    mtfprd(dst, scratch);
    return;
  }
#endif

  subi(sp, sp, Operand(kDoubleSize));
  stfd(dst, MemOperand(sp));
  stw(src, MemOperand(sp, Register::kExponentOffset));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfd(dst, MemOperand(sp));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovDoubleLowToInt(Register dst, DoubleRegister src) {
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mffprwz(dst, src);
    return;
  }
#endif

  subi(sp, sp, Operand(kDoubleSize));
  stfd(src, MemOperand(sp));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lwz(dst, MemOperand(sp, Register::kMantissaOffset));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovDoubleHighToInt(Register dst, DoubleRegister src) {
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mffprd(dst, src);
    srdi(dst, dst, Operand(32));
    return;
  }
#endif

  subi(sp, sp, Operand(kDoubleSize));
  stfd(src, MemOperand(sp));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lwz(dst, MemOperand(sp, Register::kExponentOffset));
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovDoubleToInt64(
#if !V8_TARGET_ARCH_PPC64
    Register dst_hi,
#endif
    Register dst, DoubleRegister src) {
#if V8_TARGET_ARCH_PPC64
  if (CpuFeatures::IsSupported(FPR_GPR_MOV)) {
    mffprd(dst, src);
    return;
  }
#endif

  subi(sp, sp, Operand(kDoubleSize));
  stfd(src, MemOperand(sp));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
#if V8_TARGET_ARCH_PPC64
  ld(dst, MemOperand(sp, 0));
#else
  lwz(dst_hi, MemOperand(sp, Register::kExponentOffset));
  lwz(dst, MemOperand(sp, Register::kMantissaOffset));
#endif
  addi(sp, sp, Operand(kDoubleSize));
}

void TurboAssembler::MovIntToFloat(DoubleRegister dst, Register src) {
  subi(sp, sp, Operand(kFloatSize));
  stw(src, MemOperand(sp, 0));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lfs(dst, MemOperand(sp, 0));
  addi(sp, sp, Operand(kFloatSize));
}

void TurboAssembler::MovFloatToInt(Register dst, DoubleRegister src) {
  subi(sp, sp, Operand(kFloatSize));
  stfs(src, MemOperand(sp, 0));
  nop(GROUP_ENDING_NOP);  // LHS/RAW optimization
  lwz(dst, MemOperand(sp, 0));
  addi(sp, sp, Operand(kFloatSize));
}

void TurboAssembler::Add(Register dst, Register src, intptr_t value,
                         Register scratch) {
  if (is_int16(value)) {
    addi(dst, src, Operand(value));
  } else {
    mov(scratch, Operand(value));
    add(dst, src, scratch);
  }
}

void TurboAssembler::Cmpi(Register src1, const Operand& src2, Register scratch,
                          CRegister cr) {
  intptr_t value = src2.immediate();
  if (is_int16(value)) {
    cmpi(src1, src2, cr);
  } else {
    mov(scratch, src2);
    cmp(src1, scratch, cr);
  }
}

void TurboAssembler::Cmpli(Register src1, const Operand& src2, Register scratch,
                           CRegister cr) {
  intptr_t value = src2.immediate();
  if (is_uint16(value)) {
    cmpli(src1, src2, cr);
  } else {
    mov(scratch, src2);
    cmpl(src1, scratch, cr);
  }
}

void TurboAssembler::Cmpwi(Register src1, const Operand& src2, Register scratch,
                           CRegister cr) {
  intptr_t value = src2.immediate();
  if (is_int16(value)) {
    cmpwi(src1, src2, cr);
  } else {
    mov(scratch, src2);
    cmpw(src1, scratch, cr);
  }
}

void MacroAssembler::Cmplwi(Register src1, const Operand& src2,
                            Register scratch, CRegister cr) {
  intptr_t value = src2.immediate();
  if (is_uint16(value)) {
    cmplwi(src1, src2, cr);
  } else {
    mov(scratch, src2);
    cmplw(src1, scratch, cr);
  }
}

void MacroAssembler::And(Register ra, Register rs, const Operand& rb,
                         RCBit rc) {
  if (rb.is_reg()) {
    and_(ra, rs, rb.rm(), rc);
  } else {
    if (is_uint16(rb.immediate()) && RelocInfo::IsNone(rb.rmode_) &&
        rc == SetRC) {
      andi(ra, rs, rb);
    } else {
      // mov handles the relocation.
      DCHECK(rs != r0);
      mov(r0, rb);
      and_(ra, rs, r0, rc);
    }
  }
}

void MacroAssembler::Or(Register ra, Register rs, const Operand& rb, RCBit rc) {
  if (rb.is_reg()) {
    orx(ra, rs, rb.rm(), rc);
  } else {
    if (is_uint16(rb.immediate()) && RelocInfo::IsNone(rb.rmode_) &&
        rc == LeaveRC) {
      ori(ra, rs, rb);
    } else {
      // mov handles the relocation.
      DCHECK(rs != r0);
      mov(r0, rb);
      orx(ra, rs, r0, rc);
    }
  }
}

void MacroAssembler::Xor(Register ra, Register rs, const Operand& rb,
                         RCBit rc) {
  if (rb.is_reg()) {
    xor_(ra, rs, rb.rm(), rc);
  } else {
    if (is_uint16(rb.immediate()) && RelocInfo::IsNone(rb.rmode_) &&
        rc == LeaveRC) {
      xori(ra, rs, rb);
    } else {
      // mov handles the relocation.
      DCHECK(rs != r0);
      mov(r0, rb);
      xor_(ra, rs, r0, rc);
    }
  }
}

void MacroAssembler::CmpSmiLiteral(Register src1, Smi smi, Register scratch,
                                   CRegister cr) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  Cmpwi(src1, Operand(smi), scratch, cr);
#else
  LoadSmiLiteral(scratch, smi);
  cmp(src1, scratch, cr);
#endif
}

void MacroAssembler::CmplSmiLiteral(Register src1, Smi smi, Register scratch,
                                    CRegister cr) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  Cmpli(src1, Operand(smi), scratch, cr);
#else
  LoadSmiLiteral(scratch, smi);
  cmpl(src1, scratch, cr);
#endif
}

void MacroAssembler::AddSmiLiteral(Register dst, Register src, Smi smi,
                                   Register scratch) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  Add(dst, src, static_cast<intptr_t>(smi.ptr()), scratch);
#else
  LoadSmiLiteral(scratch, smi);
  add(dst, src, scratch);
#endif
}

void MacroAssembler::SubSmiLiteral(Register dst, Register src, Smi smi,
                                   Register scratch) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  Add(dst, src, -(static_cast<intptr_t>(smi.ptr())), scratch);
#else
  LoadSmiLiteral(scratch, smi);
  sub(dst, src, scratch);
#endif
}

void MacroAssembler::AndSmiLiteral(Register dst, Register src, Smi smi,
                                   Register scratch, RCBit rc) {
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
  And(dst, src, Operand(smi), rc);
#else
  LoadSmiLiteral(scratch, smi);
  and_(dst, src, scratch, rc);
#endif
}

// Load a "pointer" sized value from the memory location
void TurboAssembler::LoadU64(Register dst, const MemOperand& mem,
                             Register scratch) {
  DCHECK_EQ(mem.rb(), no_reg);
  int offset = mem.offset();
  int misaligned = (offset & 3);
  int adj = (offset & 3) - 4;
  int alignedOffset = (offset & ~3) + 4;

  if (!is_int16(offset) || (misaligned && !is_int16(alignedOffset))) {
    /* cannot use d-form */
    mov(scratch, Operand(offset));
    LoadPX(dst, MemOperand(mem.ra(), scratch));
  } else {
    if (misaligned) {
      // adjust base to conform to offset alignment requirements
      // Todo: enhance to use scratch if dst is unsuitable
      DCHECK_NE(dst, r0);
      addi(dst, mem.ra(), Operand(adj));
      ld(dst, MemOperand(dst, alignedOffset));
    } else {
      ld(dst, mem);
    }
  }
}

void TurboAssembler::LoadPU(Register dst, const MemOperand& mem,
                            Register scratch) {
  int offset = mem.offset();

  if (!is_int16(offset)) {
    /* cannot use d-form */
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    LoadPUX(dst, MemOperand(mem.ra(), scratch));
  } else {
#if V8_TARGET_ARCH_PPC64
    ldu(dst, mem);
#else
    lwzu(dst, mem);
#endif
  }
}

// Store a "pointer" sized value to the memory location
void TurboAssembler::StoreP(Register src, const MemOperand& mem,
                            Register scratch) {
  int offset = mem.offset();

  if (!is_int16(offset)) {
    /* cannot use d-form */
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    StorePX(src, MemOperand(mem.ra(), scratch));
  } else {
#if V8_TARGET_ARCH_PPC64
    int misaligned = (offset & 3);
    if (misaligned) {
      // adjust base to conform to offset alignment requirements
      // a suitable scratch is required here
      DCHECK(scratch != no_reg);
      if (scratch == r0) {
        LoadIntLiteral(scratch, offset);
        stdx(src, MemOperand(mem.ra(), scratch));
      } else {
        addi(scratch, mem.ra(), Operand((offset & 3) - 4));
        std(src, MemOperand(scratch, (offset & ~3) + 4));
      }
    } else {
      std(src, mem);
    }
#else
    stw(src, mem);
#endif
  }
}

void TurboAssembler::StorePU(Register src, const MemOperand& mem,
                             Register scratch) {
  int offset = mem.offset();

  if (!is_int16(offset)) {
    /* cannot use d-form */
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    StorePUX(src, MemOperand(mem.ra(), scratch));
  } else {
#if V8_TARGET_ARCH_PPC64
    stdu(src, mem);
#else
    stwu(src, mem);
#endif
  }
}

void TurboAssembler::LoadS32(Register dst, const MemOperand& mem,
                             Register scratch) {
  int offset = mem.offset();

  if (!is_int16(offset)) {
    CHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    lwax(dst, MemOperand(mem.ra(), scratch));
  } else {
    int misaligned = (offset & 3);
    if (misaligned) {
      // adjust base to conform to offset alignment requirements
      // Todo: enhance to use scratch if dst is unsuitable
      CHECK(dst != r0);
      addi(dst, mem.ra(), Operand((offset & 3) - 4));
      lwa(dst, MemOperand(dst, (offset & ~3) + 4));
    } else {
      lwa(dst, mem);
    }
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand currently only supports d-form
void TurboAssembler::LoadU32(Register dst, const MemOperand& mem,
                             Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    CHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    lwzx(dst, MemOperand(base, scratch));
  } else {
    // lwz can handle offset misalign
    lwz(dst, mem);
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand current only supports d-form
void TurboAssembler::StoreWord(Register src, const MemOperand& mem,
                               Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    LoadIntLiteral(scratch, offset);
    stwx(src, MemOperand(base, scratch));
  } else {
    stw(src, mem);
  }
}

void TurboAssembler::LoadS16(Register dst, const MemOperand& mem,
                             Register scratch) {
  int offset = mem.offset();

  if (!is_int16(offset)) {
    DCHECK(scratch != no_reg);
    mov(scratch, Operand(offset));
    lhax(dst, MemOperand(mem.ra(), scratch));
  } else {
    lha(dst, mem);
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand currently only supports d-form
void TurboAssembler::LoadU16(Register dst, const MemOperand& mem,
                             Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    DCHECK_NE(scratch, no_reg);
    mov(scratch, Operand(offset));
    lhzx(dst, MemOperand(base, scratch));
  } else {
    lhz(dst, mem);
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand current only supports d-form
void MacroAssembler::StoreHalfWord(Register src, const MemOperand& mem,
                                   Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    LoadIntLiteral(scratch, offset);
    sthx(src, MemOperand(base, scratch));
  } else {
    sth(src, mem);
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand currently only supports d-form
void TurboAssembler::LoadU8(Register dst, const MemOperand& mem,
                            Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    lbzx(dst, MemOperand(base, scratch));
  } else {
    lbz(dst, mem);
  }
}

// Variable length depending on whether offset fits into immediate field
// MemOperand current only supports d-form
void MacroAssembler::StoreByte(Register src, const MemOperand& mem,
                               Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    LoadIntLiteral(scratch, offset);
    stbx(src, MemOperand(base, scratch));
  } else {
    stb(src, mem);
  }
}

void TurboAssembler::LoadDouble(DoubleRegister dst, const MemOperand& mem,
                                Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    lfdx(dst, MemOperand(base, scratch));
  } else {
    lfd(dst, mem);
  }
}

void TurboAssembler::LoadFloat32(DoubleRegister dst, const MemOperand& mem,
                                 Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    lfsx(dst, MemOperand(base, scratch));
  } else {
    lfs(dst, mem);
  }
}

void MacroAssembler::LoadDoubleU(DoubleRegister dst, const MemOperand& mem,
                                 Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    lfdux(dst, MemOperand(base, scratch));
  } else {
    lfdu(dst, mem);
  }
}

void TurboAssembler::LoadSingle(DoubleRegister dst, const MemOperand& mem,
                                Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    lfsx(dst, MemOperand(base, scratch));
  } else {
    lfs(dst, mem);
  }
}

void TurboAssembler::LoadSingleU(DoubleRegister dst, const MemOperand& mem,
                                 Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    lfsux(dst, MemOperand(base, scratch));
  } else {
    lfsu(dst, mem);
  }
}

void TurboAssembler::LoadSimd128(Simd128Register dst, const MemOperand& mem) {
  lxvx(dst, mem);
}

void TurboAssembler::StoreDouble(DoubleRegister src, const MemOperand& mem,
                                 Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    stfdx(src, MemOperand(base, scratch));
  } else {
    stfd(src, mem);
  }
}

void TurboAssembler::StoreDoubleU(DoubleRegister src, const MemOperand& mem,
                                  Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    stfdux(src, MemOperand(base, scratch));
  } else {
    stfdu(src, mem);
  }
}

void TurboAssembler::StoreSingle(DoubleRegister src, const MemOperand& mem,
                                 Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    stfsx(src, MemOperand(base, scratch));
  } else {
    stfs(src, mem);
  }
}

void TurboAssembler::StoreSingleU(DoubleRegister src, const MemOperand& mem,
                                  Register scratch) {
  Register base = mem.ra();
  int offset = mem.offset();

  if (!is_int16(offset)) {
    mov(scratch, Operand(offset));
    stfsux(src, MemOperand(base, scratch));
  } else {
    stfsu(src, mem);
  }
}

void TurboAssembler::StoreSimd128(Simd128Register src, const MemOperand& mem) {
  stxvx(src, mem);
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

void TurboAssembler::SwapP(Register src, Register dst, Register scratch) {
  if (src == dst) return;
  DCHECK(!AreAliased(src, dst, scratch));
  mr(scratch, src);
  mr(src, dst);
  mr(dst, scratch);
}

void TurboAssembler::SwapP(Register src, MemOperand dst, Register scratch) {
  if (dst.ra() != r0 && dst.ra().is_valid())
    DCHECK(!AreAliased(src, dst.ra(), scratch));
  if (dst.rb() != r0 && dst.rb().is_valid())
    DCHECK(!AreAliased(src, dst.rb(), scratch));
  DCHECK(!AreAliased(src, scratch));
  mr(scratch, src);
  LoadU64(src, dst, r0);
  StoreP(scratch, dst, r0);
}

void TurboAssembler::SwapP(MemOperand src, MemOperand dst, Register scratch_0,
                           Register scratch_1) {
  if (src.ra() != r0 && src.ra().is_valid())
    DCHECK(!AreAliased(src.ra(), scratch_0, scratch_1));
  if (src.rb() != r0 && src.rb().is_valid())
    DCHECK(!AreAliased(src.rb(), scratch_0, scratch_1));
  if (dst.ra() != r0 && dst.ra().is_valid())
    DCHECK(!AreAliased(dst.ra(), scratch_0, scratch_1));
  if (dst.rb() != r0 && dst.rb().is_valid())
    DCHECK(!AreAliased(dst.rb(), scratch_0, scratch_1));
  DCHECK(!AreAliased(scratch_0, scratch_1));
  if (is_int16(src.offset()) || is_int16(dst.offset())) {
    if (!is_int16(src.offset())) {
      // swap operand
      MemOperand temp = src;
      src = dst;
      dst = temp;
    }
    LoadU64(scratch_1, dst, scratch_0);
    LoadU64(scratch_0, src);
    StoreP(scratch_1, src);
    StoreP(scratch_0, dst, scratch_1);
  } else {
    LoadU64(scratch_1, dst, scratch_0);
    push(scratch_1);
    LoadU64(scratch_0, src, scratch_1);
    StoreP(scratch_0, dst, scratch_1);
    pop(scratch_1);
    StoreP(scratch_1, src, scratch_0);
  }
}

void TurboAssembler::SwapFloat32(DoubleRegister src, DoubleRegister dst,
                                 DoubleRegister scratch) {
  if (src == dst) return;
  DCHECK(!AreAliased(src, dst, scratch));
  fmr(scratch, src);
  fmr(src, dst);
  fmr(dst, scratch);
}

void TurboAssembler::SwapFloat32(DoubleRegister src, MemOperand dst,
                                 DoubleRegister scratch) {
  DCHECK(!AreAliased(src, scratch));
  fmr(scratch, src);
  LoadSingle(src, dst, r0);
  StoreSingle(scratch, dst, r0);
}

void TurboAssembler::SwapFloat32(MemOperand src, MemOperand dst,
                                 DoubleRegister scratch_0,
                                 DoubleRegister scratch_1) {
  DCHECK(!AreAliased(scratch_0, scratch_1));
  LoadSingle(scratch_0, src, r0);
  LoadSingle(scratch_1, dst, r0);
  StoreSingle(scratch_0, dst, r0);
  StoreSingle(scratch_1, src, r0);
}

void TurboAssembler::SwapDouble(DoubleRegister src, DoubleRegister dst,
                                DoubleRegister scratch) {
  if (src == dst) return;
  DCHECK(!AreAliased(src, dst, scratch));
  fmr(scratch, src);
  fmr(src, dst);
  fmr(dst, scratch);
}

void TurboAssembler::SwapDouble(DoubleRegister src, MemOperand dst,
                                DoubleRegister scratch) {
  DCHECK(!AreAliased(src, scratch));
  fmr(scratch, src);
  LoadDouble(src, dst, r0);
  StoreDouble(scratch, dst, r0);
}

void TurboAssembler::SwapDouble(MemOperand src, MemOperand dst,
                                DoubleRegister scratch_0,
                                DoubleRegister scratch_1) {
  DCHECK(!AreAliased(scratch_0, scratch_1));
  LoadDouble(scratch_0, src, r0);
  LoadDouble(scratch_1, dst, r0);
  StoreDouble(scratch_0, dst, r0);
  StoreDouble(scratch_1, src, r0);
}

void TurboAssembler::SwapSimd128(Simd128Register src, Simd128Register dst,
                                 Simd128Register scratch) {
  if (src == dst) return;
  vor(scratch, src, src);
  vor(src, dst, dst);
  vor(dst, scratch, scratch);
}

void TurboAssembler::SwapSimd128(Simd128Register src, MemOperand dst,
                                 Simd128Register scratch) {
  DCHECK(src != scratch);
  // push v0, to be used as scratch
  addi(sp, sp, Operand(-kSimd128Size));
  StoreSimd128(v0, MemOperand(r0, sp));
  mov(ip, Operand(dst.offset()));
  LoadSimd128(v0, MemOperand(dst.ra(), ip));
  StoreSimd128(src, MemOperand(dst.ra(), ip));
  vor(src, v0, v0);
  // restore v0
  LoadSimd128(v0, MemOperand(r0, sp));
  addi(sp, sp, Operand(kSimd128Size));
}

void TurboAssembler::SwapSimd128(MemOperand src, MemOperand dst,
                                 Simd128Register scratch) {
  // push v0 and v1, to be used as scratch
  addi(sp, sp, Operand(2 * -kSimd128Size));
  StoreSimd128(v0, MemOperand(r0, sp));
  li(ip, Operand(kSimd128Size));
  StoreSimd128(v1, MemOperand(ip, sp));

  mov(ip, Operand(src.offset()));
  LoadSimd128(v0, MemOperand(src.ra(), ip));
  mov(ip, Operand(dst.offset()));
  LoadSimd128(v1, MemOperand(dst.ra(), ip));

  StoreSimd128(v0, MemOperand(dst.ra(), ip));
  mov(ip, Operand(src.offset()));
  StoreSimd128(v1, MemOperand(src.ra(), ip));

  // restore v0 and v1
  LoadSimd128(v0, MemOperand(r0, sp));
  li(ip, Operand(kSimd128Size));
  LoadSimd128(v1, MemOperand(ip, sp));
  addi(sp, sp, Operand(2 * kSimd128Size));
}

void TurboAssembler::ResetSpeculationPoisonRegister() {
  mov(kSpeculationPoisonRegister, Operand(-1));
}

void TurboAssembler::JumpIfEqual(Register x, int32_t y, Label* dest) {
  Cmpi(x, Operand(y), r0);
  beq(dest);
}

void TurboAssembler::JumpIfLessThan(Register x, int32_t y, Label* dest) {
  Cmpi(x, Operand(y), r0);
  blt(dest);
}

void TurboAssembler::LoadEntryFromBuiltinIndex(Register builtin_index) {
  STATIC_ASSERT(kSystemPointerSize == 8);
  STATIC_ASSERT(kSmiTagSize == 1);
  STATIC_ASSERT(kSmiTag == 0);

  // The builtin_index register contains the builtin index as a Smi.
  if (SmiValuesAre32Bits()) {
    ShiftRightArithImm(builtin_index, builtin_index,
                       kSmiShift - kSystemPointerSizeLog2);
  } else {
    DCHECK(SmiValuesAre31Bits());
    ShiftLeftImm(builtin_index, builtin_index,
                 Operand(kSystemPointerSizeLog2 - kSmiShift));
  }
  addi(builtin_index, builtin_index,
       Operand(IsolateData::builtin_entry_table_offset()));
  LoadPX(builtin_index, MemOperand(kRootRegister, builtin_index));
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

    Register scratch = r11;

    DCHECK(!AreAliased(destination, scratch));
    DCHECK(!AreAliased(code_object, scratch));

    // Check whether the Code object is an off-heap trampoline. If so, call its
    // (off-heap) entry point directly without going through the (on-heap)
    // trampoline.  Otherwise, just call the Code object as always.
    LoadS32(scratch, FieldMemOperand(code_object, Code::kFlagsOffset));
    mov(r0, Operand(Code::IsOffHeapTrampoline::kMask));
    and_(r0, scratch, r0, SetRC);
    bne(&if_code_is_off_heap, cr0);

    // Not an off-heap trampoline, the entry point is at
    // Code::raw_instruction_start().
    addi(destination, code_object, Operand(Code::kHeaderSize - kHeapObjectTag));
    b(&out);

    // An off-heap trampoline, the entry point is loaded from the builtin entry
    // table.
    bind(&if_code_is_off_heap);
    LoadS32(scratch, FieldMemOperand(code_object, Code::kBuiltinIndexOffset));
    ShiftLeftImm(destination, scratch, Operand(kSystemPointerSizeLog2));
    add(destination, destination, kRootRegister);
    LoadU64(destination,
            MemOperand(destination, IsolateData::builtin_entry_table_offset()),
            r0);

    bind(&out);
  } else {
    addi(destination, code_object, Operand(Code::kHeaderSize - kHeapObjectTag));
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

  static constexpr int after_call_offset = 5 * kInstrSize;
  Label start_call;
  Register dest = target;

  if (ABI_USES_FUNCTION_DESCRIPTORS) {
    // AIX/PPC64BE Linux uses a function descriptor. When calling C code be
    // aware of this descriptor and pick up values from it
    LoadU64(ToRegister(ABI_TOC_REGISTER),
            MemOperand(target, kSystemPointerSize));
    LoadU64(ip, MemOperand(target, 0));
    dest = ip;
  } else if (ABI_CALL_VIA_IP && dest != ip) {
    Move(ip, target);
    dest = ip;
  }

  LoadPC(r7);
  bind(&start_call);
  addi(r7, r7, Operand(after_call_offset));
  StoreP(r7, MemOperand(sp, kStackFrameExtraParamSlot * kSystemPointerSize));
  Call(dest);

  DCHECK_EQ(after_call_offset - kInstrSize,
            SizeOfCodeGeneratedSince(&start_call));
}

void TurboAssembler::CallForDeoptimization(Builtins::Name target, int,
                                           Label* exit, DeoptimizeKind kind,
                                           Label* ret, Label*) {
  BlockTrampolinePoolScope block_trampoline_pool(this);
  LoadU64(ip, MemOperand(kRootRegister,
                         IsolateData::builtin_entry_slot_offset(target)));
  Call(ip);
  DCHECK_EQ(SizeOfCodeGeneratedSince(exit),
            (kind == DeoptimizeKind::kLazy)
                ? Deoptimizer::kLazyDeoptExitSize
                : Deoptimizer::kNonLazyDeoptExitSize);
  if (kind == DeoptimizeKind::kEagerWithResume) {
    b(ret);
    DCHECK_EQ(SizeOfCodeGeneratedSince(exit),
              Deoptimizer::kEagerWithResumeBeforeArgsSize);
  }
}

void TurboAssembler::ZeroExtByte(Register dst, Register src) {
  clrldi(dst, src, Operand(56));
}

void TurboAssembler::ZeroExtHalfWord(Register dst, Register src) {
  clrldi(dst, src, Operand(48));
}

void TurboAssembler::ZeroExtWord32(Register dst, Register src) {
  clrldi(dst, src, Operand(32));
}

void TurboAssembler::Trap() { stop(); }
void TurboAssembler::DebugBreak() { stop(); }

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_PPC || V8_TARGET_ARCH_PPC64
