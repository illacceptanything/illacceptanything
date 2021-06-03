// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/codegen/shared-ia32-x64/macro-assembler-shared-ia32-x64.h"

#include "src/codegen/assembler.h"
#include "src/codegen/cpu-features.h"

#if V8_TARGET_ARCH_IA32
#include "src/codegen/ia32/register-ia32.h"
#elif V8_TARGET_ARCH_X64
#include "src/codegen/x64/register-x64.h"
#else
#error Unsupported target architecture.
#endif

namespace v8 {
namespace internal {

void SharedTurboAssembler::Movapd(XMMRegister dst, XMMRegister src) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vmovapd(dst, src);
  } else {
    // On SSE, movaps is 1 byte shorter than movapd, and has the same
    // behavior.
    movaps(dst, src);
  }
}

void SharedTurboAssembler::Shufps(XMMRegister dst, XMMRegister src1,
                                  XMMRegister src2, uint8_t imm8) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vshufps(dst, src1, src2, imm8);
  } else {
    if (dst != src1) {
      movaps(dst, src1);
    }
    shufps(dst, src2, imm8);
  }
}

void SharedTurboAssembler::F64x2ExtractLane(DoubleRegister dst, XMMRegister src,
                                            uint8_t lane) {
  if (lane == 0) {
    if (dst != src) {
      Movaps(dst, src);
    }
  } else {
    DCHECK_EQ(1, lane);
    if (CpuFeatures::IsSupported(AVX)) {
      CpuFeatureScope avx_scope(this, AVX);
      // Pass src as operand to avoid false-dependency on dst.
      vmovhlps(dst, src, src);
    } else {
      movhlps(dst, src);
    }
  }
}

void SharedTurboAssembler::F64x2ReplaceLane(XMMRegister dst, XMMRegister src,
                                            DoubleRegister rep, uint8_t lane) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    if (lane == 0) {
      vpblendw(dst, src, rep, 0b00001111);
    } else {
      vmovlhps(dst, src, rep);
    }
  } else {
    CpuFeatureScope scope(this, SSE4_1);
    if (dst != src) {
      DCHECK_NE(dst, rep);  // Ensure rep is not overwritten.
      movaps(dst, src);
    }
    if (lane == 0) {
      pblendw(dst, rep, 0b00001111);
    } else {
      movlhps(dst, rep);
    }
  }
}

void SharedTurboAssembler::F64x2Min(XMMRegister dst, XMMRegister lhs,
                                    XMMRegister rhs, XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    // The minpd instruction doesn't propagate NaNs and +0's in its first
    // operand. Perform minpd in both orders, merge the resuls, and adjust.
    vminpd(scratch, lhs, rhs);
    vminpd(dst, rhs, lhs);
    // propagate -0's and NaNs, which may be non-canonical.
    vorpd(scratch, scratch, dst);
    // Canonicalize NaNs by quieting and clearing the payload.
    vcmpunordpd(dst, dst, scratch);
    vorpd(scratch, scratch, dst);
    vpsrlq(dst, dst, byte{13});
    vandnpd(dst, dst, scratch);
  } else {
    // Compare lhs with rhs, and rhs with lhs, and have the results in scratch
    // and dst. If dst overlaps with lhs or rhs, we can save a move.
    if (dst == lhs || dst == rhs) {
      XMMRegister src = dst == lhs ? rhs : lhs;
      movaps(scratch, src);
      minpd(scratch, dst);
      minpd(dst, src);
    } else {
      movaps(scratch, lhs);
      movaps(dst, rhs);
      minpd(scratch, rhs);
      minpd(dst, lhs);
    }
    orpd(scratch, dst);
    cmpunordpd(dst, scratch);
    orpd(scratch, dst);
    psrlq(dst, byte{13});
    andnpd(dst, scratch);
  }
}

void SharedTurboAssembler::F64x2Max(XMMRegister dst, XMMRegister lhs,
                                    XMMRegister rhs, XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    // The maxpd instruction doesn't propagate NaNs and +0's in its first
    // operand. Perform maxpd in both orders, merge the resuls, and adjust.
    vmaxpd(scratch, lhs, rhs);
    vmaxpd(dst, rhs, lhs);
    // Find discrepancies.
    vxorpd(dst, dst, scratch);
    // Propagate NaNs, which may be non-canonical.
    vorpd(scratch, scratch, dst);
    // Propagate sign discrepancy and (subtle) quiet NaNs.
    vsubpd(scratch, scratch, dst);
    // Canonicalize NaNs by clearing the payload. Sign is non-deterministic.
    vcmpunordpd(dst, dst, scratch);
    vpsrlq(dst, dst, byte{13});
    vandnpd(dst, dst, scratch);
  } else {
    if (dst == lhs || dst == rhs) {
      XMMRegister src = dst == lhs ? rhs : lhs;
      movaps(scratch, src);
      maxpd(scratch, dst);
      maxpd(dst, src);
    } else {
      movaps(scratch, lhs);
      movaps(dst, rhs);
      maxpd(scratch, rhs);
      maxpd(dst, lhs);
    }
    xorpd(dst, scratch);
    orpd(scratch, dst);
    subpd(scratch, dst);
    cmpunordpd(dst, scratch);
    psrlq(dst, byte{13});
    andnpd(dst, scratch);
  }
}

void SharedTurboAssembler::F32x4Splat(XMMRegister dst, DoubleRegister src) {
  if (CpuFeatures::IsSupported(AVX2)) {
    CpuFeatureScope avx2_scope(this, AVX2);
    vbroadcastss(dst, src);
  } else if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vshufps(dst, src, src, 0);
  } else {
    if (dst == src) {
      // 1 byte shorter than pshufd.
      shufps(dst, src, 0);
    } else {
      pshufd(dst, src, 0);
    }
  }
}

void SharedTurboAssembler::F32x4ExtractLane(FloatRegister dst, XMMRegister src,
                                            uint8_t lane) {
  DCHECK_LT(lane, 4);
  // These instructions are shorter than insertps, but will leave junk in
  // the top lanes of dst.
  if (lane == 0) {
    if (dst != src) {
      Movaps(dst, src);
    }
  } else if (lane == 1) {
    Movshdup(dst, src);
  } else if (lane == 2 && dst == src) {
    // Check dst == src to avoid false dependency on dst.
    Movhlps(dst, src);
  } else if (dst == src) {
    Shufps(dst, src, src, lane);
  } else {
    Pshufd(dst, src, lane);
  }
}

void SharedTurboAssembler::S128Store32Lane(Operand dst, XMMRegister src,
                                           uint8_t laneidx) {
  if (laneidx == 0) {
    Movss(dst, src);
  } else {
    DCHECK_GE(3, laneidx);
    Extractps(dst, src, laneidx);
  }
}

void SharedTurboAssembler::I16x8ExtMulLow(XMMRegister dst, XMMRegister src1,
                                          XMMRegister src2, XMMRegister scratch,
                                          bool is_signed) {
  is_signed ? Pmovsxbw(scratch, src1) : Pmovzxbw(scratch, src1);
  is_signed ? Pmovsxbw(dst, src2) : Pmovzxbw(dst, src2);
  Pmullw(dst, scratch);
}

void SharedTurboAssembler::I16x8ExtMulHighS(XMMRegister dst, XMMRegister src1,
                                            XMMRegister src2,
                                            XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpunpckhbw(scratch, src1, src1);
    vpsraw(scratch, scratch, 8);
    vpunpckhbw(dst, src2, src2);
    vpsraw(dst, dst, 8);
    vpmullw(dst, dst, scratch);
  } else {
    if (dst != src1) {
      movaps(dst, src1);
    }
    movaps(scratch, src2);
    punpckhbw(dst, dst);
    psraw(dst, 8);
    punpckhbw(scratch, scratch);
    psraw(scratch, 8);
    pmullw(dst, scratch);
  }
}

void SharedTurboAssembler::I16x8ExtMulHighU(XMMRegister dst, XMMRegister src1,
                                            XMMRegister src2,
                                            XMMRegister scratch) {
  // The logic here is slightly complicated to handle all the cases of register
  // aliasing. This allows flexibility for callers in TurboFan and Liftoff.
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    if (src1 == src2) {
      vpxor(scratch, scratch, scratch);
      vpunpckhbw(dst, src1, scratch);
      vpmullw(dst, dst, dst);
    } else {
      if (dst == src2) {
        // We overwrite dst, then use src2, so swap src1 and src2.
        std::swap(src1, src2);
      }
      vpxor(scratch, scratch, scratch);
      vpunpckhbw(dst, src1, scratch);
      vpunpckhbw(scratch, src2, scratch);
      vpmullw(dst, dst, scratch);
    }
  } else {
    if (src1 == src2) {
      xorps(scratch, scratch);
      if (dst != src1) {
        movaps(dst, src1);
      }
      punpckhbw(dst, scratch);
      pmullw(dst, scratch);
    } else {
      // When dst == src1, nothing special needs to be done.
      // When dst == src2, swap src1 and src2, since we overwrite dst.
      // When dst is unique, copy src1 to dst first.
      if (dst == src2) {
        std::swap(src1, src2);
        // Now, dst == src1.
      } else if (dst != src1) {
        // dst != src1 && dst != src2.
        movaps(dst, src1);
      }
      xorps(scratch, scratch);
      punpckhbw(dst, scratch);
      punpckhbw(scratch, src2);
      psrlw(scratch, 8);
      pmullw(dst, scratch);
    }
  }
}

void SharedTurboAssembler::I16x8SConvertI8x16High(XMMRegister dst,
                                                  XMMRegister src) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    // src = |a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p| (high)
    // dst = |i|i|j|j|k|k|l|l|m|m|n|n|o|o|p|p|
    vpunpckhbw(dst, src, src);
    vpsraw(dst, dst, 8);
  } else {
    CpuFeatureScope sse_scope(this, SSE4_1);
    if (dst == src) {
      // 2 bytes shorter than pshufd, but has depdency on dst.
      movhlps(dst, src);
      pmovsxbw(dst, dst);
    } else {
      // No dependency on dst.
      pshufd(dst, src, 0xEE);
      pmovsxbw(dst, dst);
    }
  }
}

void SharedTurboAssembler::I16x8UConvertI8x16High(XMMRegister dst,
                                                  XMMRegister src,
                                                  XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    // tmp = |0|0|0|0|0|0|0|0 | 0|0|0|0|0|0|0|0|
    // src = |a|b|c|d|e|f|g|h | i|j|k|l|m|n|o|p|
    // dst = |0|a|0|b|0|c|0|d | 0|e|0|f|0|g|0|h|
    XMMRegister tmp = dst == src ? scratch : dst;
    vpxor(tmp, tmp, tmp);
    vpunpckhbw(dst, src, tmp);
  } else {
    CpuFeatureScope sse_scope(this, SSE4_1);
    if (dst == src) {
      // xorps can be executed on more ports than pshufd.
      xorps(scratch, scratch);
      punpckhbw(dst, scratch);
    } else {
      // No dependency on dst.
      pshufd(dst, src, 0xEE);
      pmovzxbw(dst, dst);
    }
  }
}

// 1. Multiply low word into scratch.
// 2. Multiply high word (can be signed or unsigned) into dst.
// 3. Unpack and interleave scratch and dst into dst.
void SharedTurboAssembler::I32x4ExtMul(XMMRegister dst, XMMRegister src1,
                                       XMMRegister src2, XMMRegister scratch,
                                       bool low, bool is_signed) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpmullw(scratch, src1, src2);
    is_signed ? vpmulhw(dst, src1, src2) : vpmulhuw(dst, src1, src2);
    low ? vpunpcklwd(dst, scratch, dst) : vpunpckhwd(dst, scratch, dst);
  } else {
    DCHECK_EQ(dst, src1);
    movaps(scratch, src1);
    pmullw(dst, src2);
    is_signed ? pmulhw(scratch, src2) : pmulhuw(scratch, src2);
    low ? punpcklwd(dst, scratch) : punpckhwd(dst, scratch);
  }
}

void SharedTurboAssembler::I32x4SConvertI16x8High(XMMRegister dst,
                                                  XMMRegister src) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    // src = |a|b|c|d|e|f|g|h| (high)
    // dst = |e|e|f|f|g|g|h|h|
    vpunpckhwd(dst, src, src);
    vpsrad(dst, dst, 16);
  } else {
    CpuFeatureScope sse_scope(this, SSE4_1);
    if (dst == src) {
      // 2 bytes shorter than pshufd, but has depdency on dst.
      movhlps(dst, src);
      pmovsxwd(dst, dst);
    } else {
      // No dependency on dst.
      pshufd(dst, src, 0xEE);
      pmovsxwd(dst, dst);
    }
  }
}

void SharedTurboAssembler::I32x4UConvertI16x8High(XMMRegister dst,
                                                  XMMRegister src,
                                                  XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    // scratch = |0|0|0|0|0|0|0|0|
    // src     = |a|b|c|d|e|f|g|h|
    // dst     = |0|a|0|b|0|c|0|d|
    XMMRegister tmp = dst == src ? scratch : dst;
    vpxor(tmp, tmp, tmp);
    vpunpckhwd(dst, src, tmp);
  } else {
    if (dst == src) {
      // xorps can be executed on more ports than pshufd.
      xorps(scratch, scratch);
      punpckhwd(dst, scratch);
    } else {
      CpuFeatureScope sse_scope(this, SSE4_1);
      // No dependency on dst.
      pshufd(dst, src, 0xEE);
      pmovzxwd(dst, dst);
    }
  }
}

void SharedTurboAssembler::I64x2Neg(XMMRegister dst, XMMRegister src,
                                    XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope scope(this, AVX);
    vpxor(scratch, scratch, scratch);
    vpsubq(dst, scratch, src);
  } else {
    if (dst == src) {
      movaps(scratch, src);
      std::swap(src, scratch);
    }
    pxor(dst, dst);
    psubq(dst, src);
  }
}

void SharedTurboAssembler::I64x2Abs(XMMRegister dst, XMMRegister src,
                                    XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    XMMRegister tmp = dst == src ? scratch : dst;
    vpxor(tmp, tmp, tmp);
    vpsubq(tmp, tmp, src);
    vblendvpd(dst, src, tmp, src);
  } else {
    CpuFeatureScope sse_scope(this, SSE3);
    movshdup(scratch, src);
    if (dst != src) {
      movaps(dst, src);
    }
    psrad(scratch, 31);
    xorps(dst, scratch);
    psubq(dst, scratch);
  }
}

void SharedTurboAssembler::I64x2GtS(XMMRegister dst, XMMRegister src0,
                                    XMMRegister src1, XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpcmpgtq(dst, src0, src1);
  } else if (CpuFeatures::IsSupported(SSE4_2)) {
    CpuFeatureScope sse_scope(this, SSE4_2);
    DCHECK_EQ(dst, src0);
    pcmpgtq(dst, src1);
  } else {
    CpuFeatureScope sse_scope(this, SSE3);
    DCHECK_NE(dst, src0);
    DCHECK_NE(dst, src1);
    movaps(dst, src1);
    movaps(scratch, src0);
    psubq(dst, src0);
    pcmpeqd(scratch, src1);
    andps(dst, scratch);
    movaps(scratch, src0);
    pcmpgtd(scratch, src1);
    orps(dst, scratch);
    movshdup(dst, dst);
  }
}

void SharedTurboAssembler::I64x2GeS(XMMRegister dst, XMMRegister src0,
                                    XMMRegister src1, XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpcmpgtq(dst, src1, src0);
    vpcmpeqd(scratch, scratch, scratch);
    vpxor(dst, dst, scratch);
  } else if (CpuFeatures::IsSupported(SSE4_2)) {
    CpuFeatureScope sse_scope(this, SSE4_2);
    DCHECK_NE(dst, src0);
    if (dst != src1) {
      movaps(dst, src1);
    }
    pcmpgtq(dst, src0);
    pcmpeqd(scratch, scratch);
    xorps(dst, scratch);
  } else {
    CpuFeatureScope sse_scope(this, SSE3);
    DCHECK_NE(dst, src0);
    DCHECK_NE(dst, src1);
    movaps(dst, src0);
    movaps(scratch, src1);
    psubq(dst, src1);
    pcmpeqd(scratch, src0);
    andps(dst, scratch);
    movaps(scratch, src1);
    pcmpgtd(scratch, src0);
    orps(dst, scratch);
    movshdup(dst, dst);
    pcmpeqd(scratch, scratch);
    xorps(dst, scratch);
  }
}

// 1. Unpack src0, src1 into even-number elements of scratch.
// 2. Unpack src1, src0 into even-number elements of dst.
// 3. Multiply 1. with 2.
// For non-AVX, use non-destructive pshufd instead of punpckldq/punpckhdq.
void SharedTurboAssembler::I64x2ExtMul(XMMRegister dst, XMMRegister src1,
                                       XMMRegister src2, XMMRegister scratch,
                                       bool low, bool is_signed) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    if (low) {
      vpunpckldq(scratch, src1, src1);
      vpunpckldq(dst, src2, src2);
    } else {
      vpunpckhdq(scratch, src1, src1);
      vpunpckhdq(dst, src2, src2);
    }
    if (is_signed) {
      vpmuldq(dst, scratch, dst);
    } else {
      vpmuludq(dst, scratch, dst);
    }
  } else {
    uint8_t mask = low ? 0x50 : 0xFA;
    pshufd(scratch, src1, mask);
    pshufd(dst, src2, mask);
    if (is_signed) {
      CpuFeatureScope sse4_scope(this, SSE4_1);
      pmuldq(dst, scratch);
    } else {
      pmuludq(dst, scratch);
    }
  }
}

void SharedTurboAssembler::I64x2SConvertI32x4High(XMMRegister dst,
                                                  XMMRegister src) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpunpckhqdq(dst, src, src);
    vpmovsxdq(dst, dst);
  } else {
    CpuFeatureScope sse_scope(this, SSE4_1);
    if (dst == src) {
      movhlps(dst, src);
    } else {
      pshufd(dst, src, 0xEE);
    }
    pmovsxdq(dst, dst);
  }
}

void SharedTurboAssembler::I64x2UConvertI32x4High(XMMRegister dst,
                                                  XMMRegister src,
                                                  XMMRegister scratch) {
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpxor(scratch, scratch, scratch);
    vpunpckhdq(dst, src, scratch);
  } else {
    if (dst != src) {
      movaps(dst, src);
    }
    xorps(scratch, scratch);
    punpckhdq(dst, scratch);
  }
}

void SharedTurboAssembler::S128Not(XMMRegister dst, XMMRegister src,
                                   XMMRegister scratch) {
  if (dst == src) {
    Pcmpeqd(scratch, scratch);
    Pxor(dst, scratch);
  } else {
    Pcmpeqd(dst, dst);
    Pxor(dst, src);
  }
}

void SharedTurboAssembler::S128Select(XMMRegister dst, XMMRegister mask,
                                      XMMRegister src1, XMMRegister src2,
                                      XMMRegister scratch) {
  // v128.select = v128.or(v128.and(v1, c), v128.andnot(v2, c)).
  // pandn(x, y) = !x & y, so we have to flip the mask and input.
  if (CpuFeatures::IsSupported(AVX)) {
    CpuFeatureScope avx_scope(this, AVX);
    vpandn(scratch, mask, src2);
    vpand(dst, src1, mask);
    vpor(dst, dst, scratch);
  } else {
    DCHECK_EQ(dst, mask);
    // Use float ops as they are 1 byte shorter than int ops.
    movaps(scratch, mask);
    andnps(scratch, src2);
    andps(dst, src1);
    orps(dst, scratch);
  }
}

}  // namespace internal
}  // namespace v8
