// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/codegen/assembler-inl.h"
#include "src/codegen/macro-assembler.h"
#include "src/compiler/globals.h"
#include "src/compiler/linkage.h"
#include "src/zone/zone.h"

namespace v8 {
namespace internal {
namespace compiler {

namespace {

// Platform-specific configuration for C calling convention.
#if V8_TARGET_ARCH_IA32
// ===========================================================================
// == ia32 ===================================================================
// ===========================================================================
#define CALLEE_SAVE_REGISTERS esi.bit() | edi.bit() | ebx.bit()

#elif V8_TARGET_ARCH_X64
// ===========================================================================
// == x64 ====================================================================
// ===========================================================================

#ifdef V8_TARGET_OS_WIN
// == x64 windows ============================================================
#define STACK_SHADOW_WORDS 4
#define PARAM_REGISTERS rcx, rdx, r8, r9
#define FP_PARAM_REGISTERS xmm0, xmm1, xmm2, xmm3
#define FP_RETURN_REGISTER xmm0
#define CALLEE_SAVE_REGISTERS                                             \
  rbx.bit() | rdi.bit() | rsi.bit() | r12.bit() | r13.bit() | r14.bit() | \
      r15.bit()
#define CALLEE_SAVE_FP_REGISTERS                                        \
  (1 << xmm6.code()) | (1 << xmm7.code()) | (1 << xmm8.code()) |        \
      (1 << xmm9.code()) | (1 << xmm10.code()) | (1 << xmm11.code()) |  \
      (1 << xmm12.code()) | (1 << xmm13.code()) | (1 << xmm14.code()) | \
      (1 << xmm15.code())
#else  // V8_TARGET_OS_WIN
// == x64 other ==============================================================
#define PARAM_REGISTERS rdi, rsi, rdx, rcx, r8, r9
#define FP_PARAM_REGISTERS xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
#define FP_RETURN_REGISTER xmm0
#define CALLEE_SAVE_REGISTERS \
  rbx.bit() | r12.bit() | r13.bit() | r14.bit() | r15.bit()
#endif  // V8_TARGET_OS_WIN

#elif V8_TARGET_ARCH_ARM
// ===========================================================================
// == arm ====================================================================
// ===========================================================================
#define PARAM_REGISTERS r0, r1, r2, r3
#define CALLEE_SAVE_REGISTERS \
  r4.bit() | r5.bit() | r6.bit() | r7.bit() | r8.bit() | r9.bit() | r10.bit()
#define CALLEE_SAVE_FP_REGISTERS                                  \
  (1 << d8.code()) | (1 << d9.code()) | (1 << d10.code()) |       \
      (1 << d11.code()) | (1 << d12.code()) | (1 << d13.code()) | \
      (1 << d14.code()) | (1 << d15.code())

#elif V8_TARGET_ARCH_ARM64
// ===========================================================================
// == arm64 ====================================================================
// ===========================================================================
#define PARAM_REGISTERS x0, x1, x2, x3, x4, x5, x6, x7
#define CALLEE_SAVE_REGISTERS                                     \
  (1 << x19.code()) | (1 << x20.code()) | (1 << x21.code()) |     \
      (1 << x22.code()) | (1 << x23.code()) | (1 << x24.code()) | \
      (1 << x25.code()) | (1 << x26.code()) | (1 << x27.code()) | \
      (1 << x28.code())

#define CALLEE_SAVE_FP_REGISTERS                                  \
  (1 << d8.code()) | (1 << d9.code()) | (1 << d10.code()) |       \
      (1 << d11.code()) | (1 << d12.code()) | (1 << d13.code()) | \
      (1 << d14.code()) | (1 << d15.code())

#elif V8_TARGET_ARCH_MIPS
// ===========================================================================
// == mips ===================================================================
// ===========================================================================
#define STACK_SHADOW_WORDS 4
#define PARAM_REGISTERS a0, a1, a2, a3
#define CALLEE_SAVE_REGISTERS                                                  \
  s0.bit() | s1.bit() | s2.bit() | s3.bit() | s4.bit() | s5.bit() | s6.bit() | \
      s7.bit()
#define CALLEE_SAVE_FP_REGISTERS \
  f20.bit() | f22.bit() | f24.bit() | f26.bit() | f28.bit() | f30.bit()

#elif V8_TARGET_ARCH_MIPS64
// ===========================================================================
// == mips64 =================================================================
// ===========================================================================
#define PARAM_REGISTERS a0, a1, a2, a3, a4, a5, a6, a7
#define CALLEE_SAVE_REGISTERS                                                  \
  s0.bit() | s1.bit() | s2.bit() | s3.bit() | s4.bit() | s5.bit() | s6.bit() | \
      s7.bit()
#define CALLEE_SAVE_FP_REGISTERS \
  f20.bit() | f22.bit() | f24.bit() | f26.bit() | f28.bit() | f30.bit()

#elif V8_TARGET_ARCH_PPC64
// ===========================================================================
// == ppc & ppc64 ============================================================
// ===========================================================================
#ifdef V8_TARGET_LITTLE_ENDIAN  // ppc64le linux
#define STACK_SHADOW_WORDS 12
#else  // AIX
#define STACK_SHADOW_WORDS 14
#endif
#define PARAM_REGISTERS r3, r4, r5, r6, r7, r8, r9, r10
#define CALLEE_SAVE_REGISTERS                                                 \
  r14.bit() | r15.bit() | r16.bit() | r17.bit() | r18.bit() | r19.bit() |     \
      r20.bit() | r21.bit() | r22.bit() | r23.bit() | r24.bit() | r25.bit() | \
      r26.bit() | r27.bit() | r28.bit() | r29.bit() | r30.bit()
#define CALLEE_SAVE_FP_REGISTERS                                              \
  d14.bit() | d15.bit() | d16.bit() | d17.bit() | d18.bit() | d19.bit() |     \
      d20.bit() | d21.bit() | d22.bit() | d23.bit() | d24.bit() | d25.bit() | \
      d26.bit() | d27.bit() | d28.bit() | d29.bit() | d30.bit() | d31.bit()

#elif V8_TARGET_ARCH_S390X
// ===========================================================================
// == s390x ==================================================================
// ===========================================================================
#define STACK_SHADOW_WORDS 20
#define PARAM_REGISTERS r2, r3, r4, r5, r6
#define CALLEE_SAVE_REGISTERS \
  r6.bit() | r7.bit() | r8.bit() | r9.bit() | r10.bit() | ip.bit() | r13.bit()
#define CALLEE_SAVE_FP_REGISTERS                                        \
  d8.bit() | d9.bit() | d10.bit() | d11.bit() | d12.bit() | d13.bit() | \
      d14.bit() | d15.bit()

#elif V8_TARGET_ARCH_RISCV64
// ===========================================================================
// == riscv64 =================================================================
// ===========================================================================
#define PARAM_REGISTERS a0, a1, a2, a3, a4, a5, a6, a7
// fp is not part of CALLEE_SAVE_REGISTERS (similar to how MIPS64 or PPC defines
// it)
#define CALLEE_SAVE_REGISTERS                                                  \
  s1.bit() | s2.bit() | s3.bit() | s4.bit() | s5.bit() | s6.bit() | s7.bit() | \
      s8.bit() | s9.bit() | s10.bit() | s11.bit()
#define CALLEE_SAVE_FP_REGISTERS                                          \
  fs0.bit() | fs1.bit() | fs2.bit() | fs3.bit() | fs4.bit() | fs5.bit() | \
      fs6.bit() | fs7.bit() | fs8.bit() | fs9.bit() | fs10.bit() | fs11.bit()
#else
// ===========================================================================
// == unknown ================================================================
// ===========================================================================
#define UNSUPPORTED_C_LINKAGE 1
#endif
}  // namespace

#if defined(V8_TARGET_OS_WIN) && defined(V8_TARGET_ARCH_X64)
// As defined in
// https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=vs-2019#parameter-passing,
// Windows calling convention doesn't differentiate between GP and FP params
// when counting how many of them should be placed in registers. That's why
// we use the same counter {i} for both types here.
void BuildParameterLocations(const MachineSignature* msig,
                             size_t kFPParamRegisterCount,
                             size_t kParamRegisterCount,
                             const DoubleRegister* kFPParamRegisters,
                             const v8::internal::Register* kParamRegisters,
                             LocationSignature::Builder* out_locations) {
#ifdef STACK_SHADOW_WORDS
  int stack_offset = STACK_SHADOW_WORDS;
#else
  int stack_offset = 0;
#endif
  CHECK_EQ(kFPParamRegisterCount, kParamRegisterCount);

  for (size_t i = 0; i < msig->parameter_count(); i++) {
    MachineType type = msig->GetParam(i);
    bool spill = (i >= kParamRegisterCount);
    if (spill) {
      out_locations->AddParam(
          LinkageLocation::ForCallerFrameSlot(-1 - stack_offset, type));
      stack_offset++;
    } else {
      if (IsFloatingPoint(type.representation())) {
        out_locations->AddParam(
            LinkageLocation::ForRegister(kFPParamRegisters[i].code(), type));
      } else {
        out_locations->AddParam(
            LinkageLocation::ForRegister(kParamRegisters[i].code(), type));
      }
    }
  }
}
#else  // defined(V8_TARGET_OS_WIN) && defined(V8_TARGET_ARCH_X64)
// As defined in https://www.agner.org/optimize/calling_conventions.pdf,
// Section 7, Linux and Mac place parameters in consecutive registers,
// differentiating between GP and FP params. That's why we maintain two
// separate counters here. This also applies to Arm systems following
// the AAPCS and Windows on Arm.
void BuildParameterLocations(const MachineSignature* msig,
                             size_t kFPParamRegisterCount,
                             size_t kParamRegisterCount,
                             const DoubleRegister* kFPParamRegisters,
                             const v8::internal::Register* kParamRegisters,
                             LocationSignature::Builder* out_locations) {
#ifdef STACK_SHADOW_WORDS
  int stack_offset = STACK_SHADOW_WORDS;
#else
  int stack_offset = 0;
#endif
  size_t num_params = 0;
  size_t num_fp_params = 0;
  for (size_t i = 0; i < msig->parameter_count(); i++) {
    MachineType type = msig->GetParam(i);
    bool spill = IsFloatingPoint(type.representation())
                     ? (num_fp_params >= kFPParamRegisterCount)
                     : (num_params >= kParamRegisterCount);
    if (spill) {
      out_locations->AddParam(
          LinkageLocation::ForCallerFrameSlot(-1 - stack_offset, type));
      stack_offset++;
    } else {
      if (IsFloatingPoint(type.representation())) {
        out_locations->AddParam(LinkageLocation::ForRegister(
            kFPParamRegisters[num_fp_params].code(), type));
        ++num_fp_params;
      } else {
        out_locations->AddParam(LinkageLocation::ForRegister(
            kParamRegisters[num_params].code(), type));
        ++num_params;
      }
    }
  }
}
#endif  // defined(V8_TARGET_OS_WIN) && defined(V8_TARGET_ARCH_X64)

// General code uses the above configuration data.
CallDescriptor* Linkage::GetSimplifiedCDescriptor(Zone* zone,
                                                  const MachineSignature* msig,
                                                  CallDescriptor::Flags flags) {
#ifdef UNSUPPORTED_C_LINKAGE
  // This method should not be called on unknown architectures.
  FATAL("requested C call descriptor on unsupported architecture");
  return nullptr;
#endif

  DCHECK_LE(msig->parameter_count(), static_cast<size_t>(kMaxCParameters));

  LocationSignature::Builder locations(zone, msig->return_count(),
                                       msig->parameter_count());

#ifndef V8_ENABLE_FP_PARAMS_IN_C_LINKAGE
  // Check the types of the signature.
  for (size_t i = 0; i < msig->parameter_count(); i++) {
    MachineType type = msig->GetParam(i);
    CHECK(!IsFloatingPoint(type.representation()));
  }

  // Check the return types.
  for (size_t i = 0; i < locations.return_count_; i++) {
    MachineType type = msig->GetReturn(i);
    CHECK(!IsFloatingPoint(type.representation()));
  }
#endif

  CHECK_GE(2, locations.return_count_);
  if (locations.return_count_ > 0) {
#ifdef FP_RETURN_REGISTER
    const v8::internal::DoubleRegister kFPReturnRegister = FP_RETURN_REGISTER;
    auto reg = IsFloatingPoint(msig->GetReturn(0).representation())
                   ? kFPReturnRegister.code()
                   : kReturnRegister0.code();
#else
    auto reg = kReturnRegister0.code();
#endif
    // TODO(chromium:1052746): Use the correctly sized register here (e.g. "al"
    // if the return type is kBit), so we don't have to use a hacky bitwise AND
    // elsewhere.
    locations.AddReturn(LinkageLocation::ForRegister(reg, msig->GetReturn(0)));
  }

  if (locations.return_count_ > 1) {
    DCHECK(!IsFloatingPoint(msig->GetReturn(0).representation()));

    locations.AddReturn(LinkageLocation::ForRegister(kReturnRegister1.code(),
                                                     msig->GetReturn(1)));
  }

#ifdef PARAM_REGISTERS
  const v8::internal::Register kParamRegisters[] = {PARAM_REGISTERS};
  const int kParamRegisterCount = static_cast<int>(arraysize(kParamRegisters));
#else
  const v8::internal::Register* kParamRegisters = nullptr;
  const int kParamRegisterCount = 0;
#endif

#ifdef FP_PARAM_REGISTERS
  const DoubleRegister kFPParamRegisters[] = {FP_PARAM_REGISTERS};
  const size_t kFPParamRegisterCount = arraysize(kFPParamRegisters);
#else
  const DoubleRegister* kFPParamRegisters = nullptr;
  const size_t kFPParamRegisterCount = 0;
#endif

  // Add register and/or stack parameter(s).
  BuildParameterLocations(msig, kFPParamRegisterCount, kParamRegisterCount,
                          kFPParamRegisters, kParamRegisters, &locations);

#ifdef CALLEE_SAVE_REGISTERS
  const RegList kCalleeSaveRegisters = CALLEE_SAVE_REGISTERS;
#else
  const RegList kCalleeSaveRegisters = 0;
#endif

#ifdef CALLEE_SAVE_FP_REGISTERS
  const RegList kCalleeSaveFPRegisters = CALLEE_SAVE_FP_REGISTERS;
#else
  const RegList kCalleeSaveFPRegisters = 0;
#endif

  // The target for C calls is always an address (i.e. machine pointer).
  MachineType target_type = MachineType::Pointer();
  LinkageLocation target_loc = LinkageLocation::ForAnyRegister(target_type);
  flags |= CallDescriptor::kNoAllocate;

  return zone->New<CallDescriptor>(  // --
      CallDescriptor::kCallAddress,  // kind
      target_type,                   // target MachineType
      target_loc,                    // target location
      locations.Build(),             // location_sig
      0,                             // stack_parameter_count
      Operator::kNoThrow,            // properties
      kCalleeSaveRegisters,          // callee-saved registers
      kCalleeSaveFPRegisters,        // callee-saved fp regs
      flags, "c-call");
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
