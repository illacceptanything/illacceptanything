// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INCLUDED_FROM_MACRO_ASSEMBLER_H
#error This header must be included via macro-assembler.h
#endif

#ifndef V8_CODEGEN_X64_MACRO_ASSEMBLER_X64_H_
#define V8_CODEGEN_X64_MACRO_ASSEMBLER_X64_H_

#include "src/base/flags.h"
#include "src/codegen/bailout-reason.h"
#include "src/codegen/shared-ia32-x64/macro-assembler-shared-ia32-x64.h"
#include "src/codegen/x64/assembler-x64.h"
#include "src/common/globals.h"
#include "src/execution/isolate-data.h"
#include "src/objects/contexts.h"
#include "src/objects/tagged-index.h"

namespace v8 {
namespace internal {

// Convenience for platform-independent signatures.
using MemOperand = Operand;

class StringConstantBase;

struct SmiIndex {
  SmiIndex(Register index_register, ScaleFactor scale)
      : reg(index_register), scale(scale) {}
  Register reg;
  ScaleFactor scale;
};

// TODO(victorgomes): Move definition to macro-assembler.h, once all other
// platforms are updated.
enum class StackLimitKind { kInterruptStackLimit, kRealStackLimit };

// Convenient class to access arguments below the stack pointer.
class StackArgumentsAccessor {
 public:
  // argc = the number of arguments not including the receiver.
  explicit StackArgumentsAccessor(Register argc) : argc_(argc) {
    DCHECK_NE(argc_, no_reg);
  }

  // Argument 0 is the receiver (despite argc not including the receiver).
  Operand operator[](int index) const { return GetArgumentOperand(index); }

  Operand GetArgumentOperand(int index) const;
  Operand GetReceiverOperand() const { return GetArgumentOperand(0); }

 private:
  const Register argc_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StackArgumentsAccessor);
};

class V8_EXPORT_PRIVATE TurboAssembler : public SharedTurboAssembler {
 public:
  using SharedTurboAssembler::SharedTurboAssembler;
  AVX_OP(Subsd, subsd)
  AVX_OP(Divss, divss)
  AVX_OP(Divsd, divsd)
  AVX_OP(Pcmpgtw, pcmpgtw)
  AVX_OP(Pmaxsw, pmaxsw)
  AVX_OP(Pminsw, pminsw)
  AVX_OP(Addss, addss)
  AVX_OP(Addsd, addsd)
  AVX_OP(Mulsd, mulsd)
  AVX_OP(Cmpeqps, cmpeqps)
  AVX_OP(Cmpltps, cmpltps)
  AVX_OP(Cmpneqps, cmpneqps)
  AVX_OP(Cmpnltps, cmpnltps)
  AVX_OP(Cmpnleps, cmpnleps)
  AVX_OP(Cmpnltpd, cmpnltpd)
  AVX_OP(Cmpnlepd, cmpnlepd)
  AVX_OP(Cvttpd2dq, cvttpd2dq)
  AVX_OP(Ucomiss, ucomiss)
  AVX_OP(Ucomisd, ucomisd)
  AVX_OP(Psubsw, psubsw)
  AVX_OP(Psubusw, psubusw)
  AVX_OP(Paddsw, paddsw)
  AVX_OP(Pcmpgtd, pcmpgtd)
  AVX_OP(Pcmpeqb, pcmpeqb)
  AVX_OP(Pcmpeqw, pcmpeqw)
  AVX_OP(Pcmpeqd, pcmpeqd)
  AVX_OP(Movlhps, movlhps)
  AVX_OP_SSSE3(Phaddd, phaddd)
  AVX_OP_SSSE3(Phaddw, phaddw)
  AVX_OP_SSSE3(Pshufb, pshufb)
  AVX_OP_SSE4_1(Pcmpeqq, pcmpeqq)
  AVX_OP_SSE4_1(Packusdw, packusdw)
  AVX_OP_SSE4_1(Pminsd, pminsd)
  AVX_OP_SSE4_1(Pminuw, pminuw)
  AVX_OP_SSE4_1(Pminud, pminud)
  AVX_OP_SSE4_1(Pmaxuw, pmaxuw)
  AVX_OP_SSE4_1(Pmaxud, pmaxud)
  AVX_OP_SSE4_1(Pmulld, pmulld)
  AVX_OP_SSE4_1(Insertps, insertps)
  AVX_OP_SSE4_1(Pinsrq, pinsrq)
  AVX_OP_SSE4_1(Pextrq, pextrq)
  AVX_OP_SSE4_1(Roundss, roundss)
  AVX_OP_SSE4_1(Roundsd, roundsd)
  AVX_OP_SSE4_2(Pcmpgtq, pcmpgtq)

#undef AVX_OP

  // Define movq here instead of using AVX_OP. movq is defined using templates
  // and there is a function template `void movq(P1)`, while technically
  // impossible, will be selected when deducing the arguments for AvxHelper.
  void Movq(XMMRegister dst, Register src);
  void Movq(Register dst, XMMRegister src);

  void PushReturnAddressFrom(Register src) { pushq(src); }
  void PopReturnAddressTo(Register dst) { popq(dst); }

  void Ret();

  // Return and drop arguments from stack, where the number of arguments
  // may be bigger than 2^16 - 1.  Requires a scratch register.
  void Ret(int bytes_dropped, Register scratch);

  // Operations on roots in the root-array.
  void LoadRoot(Register destination, RootIndex index) override;
  void LoadRoot(Operand destination, RootIndex index) {
    LoadRoot(kScratchRegister, index);
    movq(destination, kScratchRegister);
  }

  void Push(Register src);
  void Push(Operand src);
  void Push(Immediate value);
  void Push(Smi smi);
  void Push(TaggedIndex index) {
    Push(Immediate(static_cast<uint32_t>(index.ptr())));
  }
  void Push(Handle<HeapObject> source);

  enum class PushArrayOrder { kNormal, kReverse };
  // `array` points to the first element (the lowest address).
  // `array` and `size` are not modified.
  void PushArray(Register array, Register size, Register scratch,
                 PushArrayOrder order = PushArrayOrder::kNormal);

  // Before calling a C-function from generated code, align arguments on stack.
  // After aligning the frame, arguments must be stored in rsp[0], rsp[8],
  // etc., not pushed. The argument count assumes all arguments are word sized.
  // The number of slots reserved for arguments depends on platform. On Windows
  // stack slots are reserved for the arguments passed in registers. On other
  // platforms stack slots are only reserved for the arguments actually passed
  // on the stack.
  void PrepareCallCFunction(int num_arguments);

  // Calls a C function and cleans up the space for arguments allocated
  // by PrepareCallCFunction. The called function is not allowed to trigger a
  // garbage collection, since that might move the code and invalidate the
  // return address (unless this is somehow accounted for by the called
  // function).
  void CallCFunction(ExternalReference function, int num_arguments);
  void CallCFunction(Register function, int num_arguments);

  // Calculate the number of stack slots to reserve for arguments when calling a
  // C function.
  int ArgumentStackSlotsForCFunctionCall(int num_arguments);

  void CheckPageFlag(Register object, Register scratch, int mask, Condition cc,
                     Label* condition_met,
                     Label::Distance condition_met_distance = Label::kFar);

  void Movdqa(XMMRegister dst, Operand src);
  void Movdqa(XMMRegister dst, XMMRegister src);

  void Cvtss2sd(XMMRegister dst, XMMRegister src);
  void Cvtss2sd(XMMRegister dst, Operand src);
  void Cvtsd2ss(XMMRegister dst, XMMRegister src);
  void Cvtsd2ss(XMMRegister dst, Operand src);
  void Cvttsd2si(Register dst, XMMRegister src);
  void Cvttsd2si(Register dst, Operand src);
  void Cvttsd2siq(Register dst, XMMRegister src);
  void Cvttsd2siq(Register dst, Operand src);
  void Cvttss2si(Register dst, XMMRegister src);
  void Cvttss2si(Register dst, Operand src);
  void Cvttss2siq(Register dst, XMMRegister src);
  void Cvttss2siq(Register dst, Operand src);
  void Cvtlui2ss(XMMRegister dst, Register src);
  void Cvtlui2ss(XMMRegister dst, Operand src);
  void Cvtlui2sd(XMMRegister dst, Register src);
  void Cvtlui2sd(XMMRegister dst, Operand src);
  void Cvtqui2ss(XMMRegister dst, Register src);
  void Cvtqui2ss(XMMRegister dst, Operand src);
  void Cvtqui2sd(XMMRegister dst, Register src);
  void Cvtqui2sd(XMMRegister dst, Operand src);
  void Cvttsd2uiq(Register dst, Operand src, Label* fail = nullptr);
  void Cvttsd2uiq(Register dst, XMMRegister src, Label* fail = nullptr);
  void Cvttss2uiq(Register dst, Operand src, Label* fail = nullptr);
  void Cvttss2uiq(Register dst, XMMRegister src, Label* fail = nullptr);

  // cvtsi2sd and cvtsi2ss instructions only write to the low 64/32-bit of dst
  // register, which hinders register renaming and makes dependence chains
  // longer. So we use xorpd to clear the dst register before cvtsi2sd for
  // non-AVX and a scratch XMM register as first src for AVX to solve this
  // issue.
  void Cvtqsi2ss(XMMRegister dst, Register src);
  void Cvtqsi2ss(XMMRegister dst, Operand src);
  void Cvtqsi2sd(XMMRegister dst, Register src);
  void Cvtqsi2sd(XMMRegister dst, Operand src);
  void Cvtlsi2ss(XMMRegister dst, Register src);
  void Cvtlsi2ss(XMMRegister dst, Operand src);
  void Cvtlsi2sd(XMMRegister dst, Register src);
  void Cvtlsi2sd(XMMRegister dst, Operand src);

  void Lzcntq(Register dst, Register src);
  void Lzcntq(Register dst, Operand src);
  void Lzcntl(Register dst, Register src);
  void Lzcntl(Register dst, Operand src);
  void Tzcntq(Register dst, Register src);
  void Tzcntq(Register dst, Operand src);
  void Tzcntl(Register dst, Register src);
  void Tzcntl(Register dst, Operand src);
  void Popcntl(Register dst, Register src);
  void Popcntl(Register dst, Operand src);
  void Popcntq(Register dst, Register src);
  void Popcntq(Register dst, Operand src);

  // Is the value a tagged smi.
  Condition CheckSmi(Register src);
  Condition CheckSmi(Operand src);

  // Jump to label if the value is a tagged smi.
  void JumpIfSmi(Register src, Label* on_smi,
                 Label::Distance near_jump = Label::kFar);

  void JumpIfEqual(Register a, int32_t b, Label* dest) {
    cmpl(a, Immediate(b));
    j(equal, dest);
  }

  void JumpIfLessThan(Register a, int32_t b, Label* dest) {
    cmpl(a, Immediate(b));
    j(less, dest);
  }

#ifdef V8_MAP_PACKING
  void UnpackMapWord(Register r);
#endif

  void LoadMap(Register destination, Register object);

  void Move(Register dst, intptr_t x) {
    if (x == 0) {
      xorl(dst, dst);
    } else if (is_uint8(x)) {
      xorl(dst, dst);
      movb(dst, Immediate(static_cast<uint32_t>(x)));
    } else if (is_uint32(x)) {
      movl(dst, Immediate(static_cast<uint32_t>(x)));
    } else if (is_int32(x)) {
      // "movq reg64, imm32" is sign extending.
      movq(dst, Immediate(static_cast<int32_t>(x)));
    } else {
      movq(dst, Immediate64(x));
    }
  }
  void Move(Operand dst, intptr_t x);
  void Move(Register dst, Smi source);

  void Move(Operand dst, Smi source) {
    Register constant = GetSmiConstant(source);
    movq(dst, constant);
  }

  void Move(Register dst, TaggedIndex source) { Move(dst, source.ptr()); }

  void Move(Operand dst, TaggedIndex source) { Move(dst, source.ptr()); }

  void Move(Register dst, ExternalReference ext);

  void Move(XMMRegister dst, uint32_t src);
  void Move(XMMRegister dst, uint64_t src);
  void Move(XMMRegister dst, float src) { Move(dst, bit_cast<uint32_t>(src)); }
  void Move(XMMRegister dst, double src) { Move(dst, bit_cast<uint64_t>(src)); }
  void Move(XMMRegister dst, uint64_t high, uint64_t low);

  // Move if the registers are not identical.
  void Move(Register target, Register source);
  void Move(XMMRegister target, XMMRegister source);

  void Move(Register target, Operand source);
  void Move(Register target, Immediate source);

  void Move(Register dst, Handle<HeapObject> source,
            RelocInfo::Mode rmode = RelocInfo::FULL_EMBEDDED_OBJECT);
  void Move(Operand dst, Handle<HeapObject> source,
            RelocInfo::Mode rmode = RelocInfo::FULL_EMBEDDED_OBJECT);

  // Loads a pointer into a register with a relocation mode.
  void Move(Register dst, Address ptr, RelocInfo::Mode rmode) {
    // This method must not be used with heap object references. The stored
    // address is not GC safe. Use the handle version instead.
    DCHECK(rmode == RelocInfo::NONE || rmode > RelocInfo::LAST_GCED_ENUM);
    movq(dst, Immediate64(ptr, rmode));
  }

  // Move src0 to dst0 and src1 to dst1, handling possible overlaps.
  void MovePair(Register dst0, Register src0, Register dst1, Register src1);

  void MoveStringConstant(
      Register result, const StringConstantBase* string,
      RelocInfo::Mode rmode = RelocInfo::FULL_EMBEDDED_OBJECT);

  // Convert smi to word-size sign-extended value.
  void SmiUntag(Register reg);
  // Requires dst != src
  void SmiUntag(Register dst, Register src);
  void SmiUntag(Register dst, Operand src);

  // Loads the address of the external reference into the destination
  // register.
  void LoadAddress(Register destination, ExternalReference source);

  void LoadFromConstantsTable(Register destination,
                              int constant_index) override;
  void LoadRootRegisterOffset(Register destination, intptr_t offset) override;
  void LoadRootRelative(Register destination, int32_t offset) override;

  // Operand pointing to an external reference.
  // May emit code to set up the scratch register. The operand is
  // only guaranteed to be correct as long as the scratch register
  // isn't changed.
  // If the operand is used more than once, use a scratch register
  // that is guaranteed not to be clobbered.
  Operand ExternalReferenceAsOperand(ExternalReference reference,
                                     Register scratch = kScratchRegister);

  void Call(Register reg) { call(reg); }
  void Call(Operand op);
  void Call(Handle<Code> code_object, RelocInfo::Mode rmode);
  void Call(Address destination, RelocInfo::Mode rmode);
  void Call(ExternalReference ext);
  void Call(Label* target) { call(target); }

  Operand EntryFromBuiltinIndexAsOperand(Builtins::Name builtin_index);
  Operand EntryFromBuiltinIndexAsOperand(Register builtin_index);
  void CallBuiltinByIndex(Register builtin_index) override;
  void CallBuiltin(Builtins::Name builtin) {
    // TODO(11527): drop the int overload in favour of the Builtins::Name one.
    return CallBuiltin(static_cast<int>(builtin));
  }
  void CallBuiltin(int builtin_index);
  void TailCallBuiltin(Builtins::Name builtin) {
    // TODO(11527): drop the int overload in favour of the Builtins::Name one.
    return TailCallBuiltin(static_cast<int>(builtin));
  }
  void TailCallBuiltin(int builtin_index);

  void LoadCodeObjectEntry(Register destination, Register code_object) override;
  void CallCodeObject(Register code_object) override;
  void JumpCodeObject(Register code_object,
                      JumpMode jump_mode = JumpMode::kJump) override;

  void RetpolineCall(Register reg);
  void RetpolineCall(Address destination, RelocInfo::Mode rmode);

  void Jump(Address destination, RelocInfo::Mode rmode);
  void Jump(const ExternalReference& reference) override;
  void Jump(Operand op);
  void Jump(Handle<Code> code_object, RelocInfo::Mode rmode,
            Condition cc = always);

  void RetpolineJump(Register reg);

  void CallForDeoptimization(Builtins::Name target, int deopt_id, Label* exit,
                             DeoptimizeKind kind, Label* ret,
                             Label* jump_deoptimization_entry_label);

  void Trap() override;
  void DebugBreak() override;

  // Will move src1 to dst if dst != src1.
  void Pmaddwd(XMMRegister dst, XMMRegister src1, Operand src2);
  void Pmaddwd(XMMRegister dst, XMMRegister src1, XMMRegister src2);
  void Pmaddubsw(XMMRegister dst, XMMRegister src1, Operand src2);
  void Pmaddubsw(XMMRegister dst, XMMRegister src1, XMMRegister src2);

  // Non-SSE2 instructions.
  void Pextrd(Register dst, XMMRegister src, uint8_t imm8);

  void Pinsrb(XMMRegister dst, XMMRegister src1, Register src2, uint8_t imm8);
  void Pinsrb(XMMRegister dst, XMMRegister src1, Operand src2, uint8_t imm8);
  void Pinsrw(XMMRegister dst, XMMRegister src1, Register src2, uint8_t imm8);
  void Pinsrw(XMMRegister dst, XMMRegister src1, Operand src2, uint8_t imm8);
  void Pinsrd(XMMRegister dst, XMMRegister src1, Register src2, uint8_t imm8);
  void Pinsrd(XMMRegister dst, XMMRegister src1, Operand src2, uint8_t imm8);
  void Pinsrd(XMMRegister dst, Register src2, uint8_t imm8);
  void Pinsrd(XMMRegister dst, Operand src2, uint8_t imm8);
  void Pinsrq(XMMRegister dst, XMMRegister src1, Register src2, uint8_t imm8);
  void Pinsrq(XMMRegister dst, XMMRegister src1, Operand src2, uint8_t imm8);

  void Pblendvb(XMMRegister dst, XMMRegister src1, XMMRegister src2,
                XMMRegister mask);
  void Blendvps(XMMRegister dst, XMMRegister src1, XMMRegister src2,
                XMMRegister mask);
  void Blendvpd(XMMRegister dst, XMMRegister src1, XMMRegister src2,
                XMMRegister mask);

  // Supports both SSE and AVX. Move src1 to dst if they are not equal on SSE.
  void Pshufb(XMMRegister dst, XMMRegister src1, XMMRegister src2);
  void Pmulhrsw(XMMRegister dst, XMMRegister src1, XMMRegister src2);

  // These Wasm SIMD ops do not have direct lowerings on x64. These
  // helpers are optimized to produce the fastest and smallest codegen.
  // Defined here to allow usage on both TurboFan and Liftoff.
  void I16x8Q15MulRSatS(XMMRegister dst, XMMRegister src1, XMMRegister src2);

  void S128Store64Lane(Operand dst, XMMRegister src, uint8_t laneidx);

  void I8x16Popcnt(XMMRegister dst, XMMRegister src, XMMRegister tmp);

  void F64x2ConvertLowI32x4U(XMMRegister dst, XMMRegister src);
  void I32x4TruncSatF64x2SZero(XMMRegister dst, XMMRegister src);
  void I32x4TruncSatF64x2UZero(XMMRegister dst, XMMRegister src);

  void I16x8ExtAddPairwiseI8x16S(XMMRegister dst, XMMRegister src);
  void I32x4ExtAddPairwiseI16x8U(XMMRegister dst, XMMRegister src);

  void I8x16Swizzle(XMMRegister dst, XMMRegister src, XMMRegister mask,
                    bool omit_add = false);

  void Abspd(XMMRegister dst);
  void Negpd(XMMRegister dst);

  void CompareRoot(Register with, RootIndex index);
  void CompareRoot(Operand with, RootIndex index);

  // Generates function and stub prologue code.
  void StubPrologue(StackFrame::Type type);
  void Prologue();

  // Calls Abort(msg) if the condition cc is not satisfied.
  // Use --debug_code to enable.
  void Assert(Condition cc, AbortReason reason);

  // Like Assert(), but without condition.
  // Use --debug_code to enable.
  void AssertUnreachable(AbortReason reason);

  // Abort execution if a 64 bit register containing a 32 bit payload does not
  // have zeros in the top 32 bits, enabled via --debug-code.
  void AssertZeroExtended(Register reg);

  // Like Assert(), but always enabled.
  void Check(Condition cc, AbortReason reason);

  // Print a message to stdout and abort execution.
  void Abort(AbortReason msg);

  // Check that the stack is aligned.
  void CheckStackAlignment();

  // Activation support.
  void EnterFrame(StackFrame::Type type);
  void EnterFrame(StackFrame::Type type, bool load_constant_pool_pointer_reg) {
    // Out-of-line constant pool not implemented on x64.
    UNREACHABLE();
  }
  void LeaveFrame(StackFrame::Type type);

// Allocate stack space of given size (i.e. decrement {rsp} by the value
// stored in the given register, or by a constant). If you need to perform a
// stack check, do it before calling this function because this function may
// write into the newly allocated space. It may also overwrite the given
// register's value, in the version that takes a register.
#if defined(V8_TARGET_OS_WIN) || defined(V8_TARGET_OS_MACOSX)
  void AllocateStackSpace(Register bytes_scratch);
  void AllocateStackSpace(int bytes);
#else
  void AllocateStackSpace(Register bytes) { subq(rsp, bytes); }
  void AllocateStackSpace(int bytes) {
    DCHECK_GE(bytes, 0);
    if (bytes == 0) return;
    subq(rsp, Immediate(bytes));
  }
#endif

  // Removes current frame and its arguments from the stack preserving the
  // arguments and a return address pushed to the stack for the next call.  Both
  // |callee_args_count| and |caller_args_count| do not include receiver.
  // |callee_args_count| is not modified. |caller_args_count| is trashed.
  void PrepareForTailCall(Register callee_args_count,
                          Register caller_args_count, Register scratch0,
                          Register scratch1);

  void InitializeRootRegister() {
    ExternalReference isolate_root = ExternalReference::isolate_root(isolate());
    Move(kRootRegister, isolate_root);
#ifdef V8_COMPRESS_POINTERS_IN_SHARED_CAGE
    LoadRootRelative(kPtrComprCageBaseRegister,
                     IsolateData::cage_base_offset());
#endif
  }

  void SaveRegisters(RegList registers);
  void RestoreRegisters(RegList registers);

  void CallEphemeronKeyBarrier(Register object, Register address,
                               SaveFPRegsMode fp_mode);

  void CallRecordWriteStub(
      Register object, Register address,
      RememberedSetAction remembered_set_action, SaveFPRegsMode fp_mode,
      StubCallMode mode = StubCallMode::kCallBuiltinPointer);

  void MoveNumber(Register dst, double value);
  void MoveNonSmi(Register dst, double value);

  // Calculate how much stack space (in bytes) are required to store caller
  // registers excluding those specified in the arguments.
  int RequiredStackSizeForCallerSaved(SaveFPRegsMode fp_mode,
                                      Register exclusion1 = no_reg,
                                      Register exclusion2 = no_reg,
                                      Register exclusion3 = no_reg) const;

  // PushCallerSaved and PopCallerSaved do not arrange the registers in any
  // particular order so they are not useful for calls that can cause a GC.
  // The caller can exclude up to 3 registers that do not need to be saved and
  // restored.

  // Push caller saved registers on the stack, and return the number of bytes
  // stack pointer is adjusted.
  int PushCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1 = no_reg,
                      Register exclusion2 = no_reg,
                      Register exclusion3 = no_reg);
  // Restore caller saved registers from the stack, and return the number of
  // bytes stack pointer is adjusted.
  int PopCallerSaved(SaveFPRegsMode fp_mode, Register exclusion1 = no_reg,
                     Register exclusion2 = no_reg,
                     Register exclusion3 = no_reg);

  // Compute the start of the generated instruction stream from the current PC.
  // This is an alternative to embedding the {CodeObject} handle as a reference.
  void ComputeCodeStartAddress(Register dst);

  void ResetSpeculationPoisonRegister();

  // Control-flow integrity:

  // Define a function entrypoint. This doesn't emit any code for this
  // architecture, as control-flow integrity is not supported for it.
  void CodeEntry() {}
  // Define an exception handler.
  void ExceptionHandler() {}
  // Define an exception handler and bind a label.
  void BindExceptionHandler(Label* label) { bind(label); }

  // ---------------------------------------------------------------------------
  // Pointer compression support

  // Loads a field containing a HeapObject and decompresses it if pointer
  // compression is enabled.
  void LoadTaggedPointerField(Register destination, Operand field_operand);

  // Loads a field containing a Smi and decompresses it if pointer compression
  // is enabled.
  void LoadTaggedSignedField(Register destination, Operand field_operand);

  // Loads a field containing any tagged value and decompresses it if necessary.
  void LoadAnyTaggedField(Register destination, Operand field_operand);

  // Loads a field containing a HeapObject, decompresses it if necessary and
  // pushes full pointer to the stack. When pointer compression is enabled,
  // uses |scratch| to decompress the value.
  void PushTaggedPointerField(Operand field_operand, Register scratch);

  // Loads a field containing any tagged value, decompresses it if necessary and
  // pushes the full pointer to the stack. When pointer compression is enabled,
  // uses |scratch| to decompress the value.
  void PushTaggedAnyField(Operand field_operand, Register scratch);

  // Loads a field containing smi value and untags it.
  void SmiUntagField(Register dst, Operand src);

  // Compresses tagged value if necessary and stores it to given on-heap
  // location.
  void StoreTaggedField(Operand dst_field_operand, Immediate immediate);
  void StoreTaggedField(Operand dst_field_operand, Register value);
  void StoreTaggedSignedField(Operand dst_field_operand, Smi value);

  // The following macros work even when pointer compression is not enabled.
  void DecompressTaggedSigned(Register destination, Operand field_operand);
  void DecompressTaggedPointer(Register destination, Operand field_operand);
  void DecompressTaggedPointer(Register destination, Register source);
  void DecompressAnyTagged(Register destination, Operand field_operand);

  // ---------------------------------------------------------------------------
  // V8 Heap sandbox support

  enum class IsolateRootLocation { kInScratchRegister, kInRootRegister };
  // Loads a field containing off-heap pointer and does necessary decoding
  // if V8 heap sandbox is enabled.
  void LoadExternalPointerField(Register destination, Operand field_operand,
                                ExternalPointerTag tag, Register scratch,
                                IsolateRootLocation isolateRootLocation =
                                    IsolateRootLocation::kInRootRegister);

 protected:
  static const int kSmiShift = kSmiTagSize + kSmiShiftSize;

  // Returns a register holding the smi value. The register MUST NOT be
  // modified. It may be the "smi 1 constant" register.
  Register GetSmiConstant(Smi value);
};

// MacroAssembler implements a collection of frequently used macros.
class V8_EXPORT_PRIVATE MacroAssembler : public TurboAssembler {
 public:
  using TurboAssembler::TurboAssembler;

  // Loads and stores the value of an external reference.
  // Special case code for load and store to take advantage of
  // load_rax/store_rax if possible/necessary.
  // For other operations, just use:
  //   Operand operand = ExternalReferenceAsOperand(extref);
  //   operation(operand, ..);
  void Load(Register destination, ExternalReference source);
  void Store(ExternalReference destination, Register source);

  // Pushes the address of the external reference onto the stack.
  void PushAddress(ExternalReference source);

  // Operations on roots in the root-array.
  // Load a root value where the index (or part of it) is variable.
  // The variable_offset register is added to the fixed_offset value
  // to get the index into the root-array.
  void PushRoot(RootIndex index);

  // Compare the object in a register to a value and jump if they are equal.
  void JumpIfRoot(Register with, RootIndex index, Label* if_equal,
                  Label::Distance if_equal_distance = Label::kFar) {
    CompareRoot(with, index);
    j(equal, if_equal, if_equal_distance);
  }
  void JumpIfRoot(Operand with, RootIndex index, Label* if_equal,
                  Label::Distance if_equal_distance = Label::kFar) {
    CompareRoot(with, index);
    j(equal, if_equal, if_equal_distance);
  }

  // Compare the object in a register to a value and jump if they are not equal.
  void JumpIfNotRoot(Register with, RootIndex index, Label* if_not_equal,
                     Label::Distance if_not_equal_distance = Label::kFar) {
    CompareRoot(with, index);
    j(not_equal, if_not_equal, if_not_equal_distance);
  }
  void JumpIfNotRoot(Operand with, RootIndex index, Label* if_not_equal,
                     Label::Distance if_not_equal_distance = Label::kFar) {
    CompareRoot(with, index);
    j(not_equal, if_not_equal, if_not_equal_distance);
  }

  // ---------------------------------------------------------------------------
  // GC Support

  // Notify the garbage collector that we wrote a pointer into an object.
  // |object| is the object being stored into, |value| is the object being
  // stored.  value and scratch registers are clobbered by the operation.
  // The offset is the offset from the start of the object, not the offset from
  // the tagged HeapObject pointer.  For use with FieldOperand(reg, off).
  void RecordWriteField(
      Register object, int offset, Register value, Register scratch,
      SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = RememberedSetAction::kEmit,
      SmiCheck smi_check = SmiCheck::kInline);

  // For page containing |object| mark region covering |address|
  // dirty. |object| is the object being stored into, |value| is the
  // object being stored. The address and value registers are clobbered by the
  // operation.  RecordWrite filters out smis so it does not update
  // the write barrier if the value is a smi.
  void RecordWrite(
      Register object, Register address, Register value, SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = RememberedSetAction::kEmit,
      SmiCheck smi_check = SmiCheck::kInline);

  // Enter specific kind of exit frame; either in normal or
  // debug mode. Expects the number of arguments in register rax and
  // sets up the number of arguments in register rdi and the pointer
  // to the first argument in register rsi.
  //
  // Allocates arg_stack_space * kSystemPointerSize memory (not GCed) on the
  // stack accessible via StackSpaceOperand.
  void EnterExitFrame(int arg_stack_space = 0, bool save_doubles = false,
                      StackFrame::Type frame_type = StackFrame::EXIT);

  // Enter specific kind of exit frame. Allocates
  // (arg_stack_space * kSystemPointerSize) memory (not GCed) on the stack
  // accessible via StackSpaceOperand.
  void EnterApiExitFrame(int arg_stack_space);

  // Leave the current exit frame. Expects/provides the return value in
  // register rax:rdx (untouched) and the pointer to the first
  // argument in register rsi (if pop_arguments == true).
  void LeaveExitFrame(bool save_doubles = false, bool pop_arguments = true);

  // Leave the current exit frame. Expects/provides the return value in
  // register rax (untouched).
  void LeaveApiExitFrame();

  // ---------------------------------------------------------------------------
  // JavaScript invokes

  // Invoke the JavaScript function code by either calling or jumping.
  void InvokeFunctionCode(Register function, Register new_target,
                          Register expected_parameter_count,
                          Register actual_parameter_count, InvokeType type);

  // On function call, call into the debugger.
  void CallDebugOnFunctionCall(Register fun, Register new_target,
                               Register expected_parameter_count,
                               Register actual_parameter_count);

  // Invoke the JavaScript function in the given register. Changes the
  // current context to the context in the function before invoking.
  void InvokeFunction(Register function, Register new_target,
                      Register actual_parameter_count, InvokeType type);

  void InvokeFunction(Register function, Register new_target,
                      Register expected_parameter_count,
                      Register actual_parameter_count, InvokeType type);

  // ---------------------------------------------------------------------------
  // Conversions between tagged smi values and non-tagged integer values.

  // Tag an word-size value. The result must be known to be a valid smi value.
  void SmiTag(Register reg);
  // Requires dst != src
  void SmiTag(Register dst, Register src);

  // Simple comparison of smis.  Both sides must be known smis to use these,
  // otherwise use Cmp.
  void SmiCompare(Register smi1, Register smi2);
  void SmiCompare(Register dst, Smi src);
  void SmiCompare(Register dst, Operand src);
  void SmiCompare(Operand dst, Register src);
  void SmiCompare(Operand dst, Smi src);

  // Functions performing a check on a known or potential smi. Returns
  // a condition that is satisfied if the check is successful.

  // Test-and-jump functions. Typically combines a check function
  // above with a conditional jump.

  // Jump to label if the value is not a tagged smi.
  void JumpIfNotSmi(Register src, Label* on_not_smi,
                    Label::Distance near_jump = Label::kFar);

  // Jump to label if the value is not a tagged smi.
  void JumpIfNotSmi(Operand src, Label* on_not_smi,
                    Label::Distance near_jump = Label::kFar);

  // Operations on tagged smi values.

  // Smis represent a subset of integers. The subset is always equivalent to
  // a two's complement interpretation of a fixed number of bits.

  // Add an integer constant to a tagged smi, giving a tagged smi as result.
  // No overflow testing on the result is done.
  void SmiAddConstant(Operand dst, Smi constant);

  // Specialized operations

  // Converts, if necessary, a smi to a combination of number and
  // multiplier to be used as a scaled index.
  // The src register contains a *positive* smi value. The shift is the
  // power of two to multiply the index value by (e.g. to index by
  // smi-value * kSystemPointerSize, pass the smi and kSystemPointerSizeLog2).
  // The returned index register may be either src or dst, depending
  // on what is most efficient. If src and dst are different registers,
  // src is always unchanged.
  SmiIndex SmiToIndex(Register dst, Register src, int shift);

  // ---------------------------------------------------------------------------
  // Macro instructions.

  void Cmp(Register dst, Handle<Object> source);
  void Cmp(Operand dst, Handle<Object> source);
  void Cmp(Register dst, Smi src);
  void Cmp(Operand dst, Smi src);
  void Cmp(Register dst, int32_t src);

  // Checks if value is in range [lower_limit, higher_limit] using a single
  // comparison.
  void JumpIfIsInRange(Register value, unsigned lower_limit,
                       unsigned higher_limit, Label* on_in_range,
                       Label::Distance near_jump = Label::kFar);

  // Emit code to discard a non-negative number of pointer-sized elements
  // from the stack, clobbering only the rsp register.
  void Drop(int stack_elements);
  // Emit code to discard a positive number of pointer-sized elements
  // from the stack under the return address which remains on the top,
  // clobbering the rsp register.
  void DropUnderReturnAddress(int stack_elements,
                              Register scratch = kScratchRegister);

  void PushQuad(Operand src);
  void PushImm32(int32_t imm32);
  void Pop(Register dst);
  void Pop(Operand dst);
  void PopQuad(Operand dst);

  // ---------------------------------------------------------------------------
  // SIMD macros.
  void Absps(XMMRegister dst);
  void Negps(XMMRegister dst);
  // Generates a trampoline to jump to the off-heap instruction stream.
  void JumpToInstructionStream(Address entry);

  // Compare object type for heap object.
  // Always use unsigned comparisons: above and below, not less and greater.
  // Incoming register is heap_object and outgoing register is map.
  // They may be the same register, and may be kScratchRegister.
  void CmpObjectType(Register heap_object, InstanceType type, Register map);

  // Compare instance type for map.
  // Always use unsigned comparisons: above and below, not less and greater.
  void CmpInstanceType(Register map, InstanceType type);

  // Compare instance type ranges for a map (low and high inclusive)
  // Always use unsigned comparisons: below_equal for a positive result.
  void CmpInstanceTypeRange(Register map, InstanceType low, InstanceType high);

  template <typename Field>
  void DecodeField(Register reg) {
    static const int shift = Field::kShift;
    static const int mask = Field::kMask >> Field::kShift;
    if (shift != 0) {
      shrq(reg, Immediate(shift));
    }
    andq(reg, Immediate(mask));
  }

  // Abort execution if argument is a smi, enabled via --debug-code.
  void AssertNotSmi(Register object);

  // Abort execution if argument is not a smi, enabled via --debug-code.
  void AssertSmi(Register object);
  void AssertSmi(Operand object);

  // Abort execution if argument is not a Constructor, enabled via --debug-code.
  void AssertConstructor(Register object);

  // Abort execution if argument is not a JSFunction, enabled via --debug-code.
  void AssertFunction(Register object);

  // Abort execution if argument is not a JSBoundFunction,
  // enabled via --debug-code.
  void AssertBoundFunction(Register object);

  // Abort execution if argument is not a JSGeneratorObject (or subclass),
  // enabled via --debug-code.
  void AssertGeneratorObject(Register object);

  // Abort execution if argument is not undefined or an AllocationSite, enabled
  // via --debug-code.
  void AssertUndefinedOrAllocationSite(Register object);

  // ---------------------------------------------------------------------------
  // Exception handling

  // Push a new stack handler and link it into stack handler chain.
  void PushStackHandler();

  // Unlink the stack handler on top of the stack from the stack handler chain.
  void PopStackHandler();

  // ---------------------------------------------------------------------------
  // Support functions.

  // Load the global proxy from the current context.
  void LoadGlobalProxy(Register dst) {
    LoadNativeContextSlot(dst, Context::GLOBAL_PROXY_INDEX);
  }

  // Load the native context slot with the current index.
  void LoadNativeContextSlot(Register dst, int index);

  // ---------------------------------------------------------------------------
  // Runtime calls

  // Call a runtime routine.
  void CallRuntime(const Runtime::Function* f, int num_arguments,
                   SaveFPRegsMode save_doubles = SaveFPRegsMode::kIgnore);

  // Convenience function: Same as above, but takes the fid instead.
  void CallRuntime(Runtime::FunctionId fid,
                   SaveFPRegsMode save_doubles = SaveFPRegsMode::kIgnore) {
    const Runtime::Function* function = Runtime::FunctionForId(fid);
    CallRuntime(function, function->nargs, save_doubles);
  }

  // Convenience function: Same as above, but takes the fid instead.
  void CallRuntime(Runtime::FunctionId fid, int num_arguments,
                   SaveFPRegsMode save_doubles = SaveFPRegsMode::kIgnore) {
    CallRuntime(Runtime::FunctionForId(fid), num_arguments, save_doubles);
  }

  // Convenience function: tail call a runtime routine (jump)
  void TailCallRuntime(Runtime::FunctionId fid);

  // Jump to a runtime routines
  void JumpToExternalReference(const ExternalReference& ext,
                               bool builtin_exit_frame = false);

  // ---------------------------------------------------------------------------
  // StatsCounter support
  void IncrementCounter(StatsCounter* counter, int value);
  void DecrementCounter(StatsCounter* counter, int value);

  // ---------------------------------------------------------------------------
  // Stack limit utilities
  Operand StackLimitAsOperand(StackLimitKind kind);
  void StackOverflowCheck(
      Register num_args, Register scratch, Label* stack_overflow,
      Label::Distance stack_overflow_distance = Label::kFar);

  // ---------------------------------------------------------------------------
  // In-place weak references.
  void LoadWeakValue(Register in_out, Label* target_if_cleared);

 private:
  // Helper functions for generating invokes.
  void InvokePrologue(Register expected_parameter_count,
                      Register actual_parameter_count, Label* done,
                      InvokeType type);

  void EnterExitFramePrologue(Register saved_rax_reg,
                              StackFrame::Type frame_type);

  // Allocates arg_stack_space * kSystemPointerSize memory (not GCed) on the
  // stack accessible via StackSpaceOperand.
  void EnterExitFrameEpilogue(int arg_stack_space, bool save_doubles);

  void LeaveExitFrameEpilogue();

  DISALLOW_IMPLICIT_CONSTRUCTORS(MacroAssembler);
};

// -----------------------------------------------------------------------------
// Static helper functions.

// Generate an Operand for loading a field from an object.
inline Operand FieldOperand(Register object, int offset) {
  return Operand(object, offset - kHeapObjectTag);
}

// Generate an Operand for loading an indexed field from an object.
inline Operand FieldOperand(Register object, Register index, ScaleFactor scale,
                            int offset) {
  return Operand(object, index, scale, offset - kHeapObjectTag);
}

// Provides access to exit frame stack space (not GCed).
inline Operand StackSpaceOperand(int index) {
#ifdef V8_TARGET_OS_WIN
  const int kShaddowSpace = 4;
  return Operand(rsp, (index + kShaddowSpace) * kSystemPointerSize);
#else
  return Operand(rsp, index * kSystemPointerSize);
#endif
}

inline Operand StackOperandForReturnAddress(int32_t disp) {
  return Operand(rsp, disp);
}

#define ACCESS_MASM(masm) masm->

}  // namespace internal
}  // namespace v8

#endif  // V8_CODEGEN_X64_MACRO_ASSEMBLER_X64_H_
