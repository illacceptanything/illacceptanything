// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_WASM_BASELINE_LIFTOFF_ASSEMBLER_H_
#define V8_WASM_BASELINE_LIFTOFF_ASSEMBLER_H_

#include <iosfwd>
#include <memory>

#include "src/base/bits.h"
#include "src/base/small-vector.h"
#include "src/codegen/macro-assembler.h"
#include "src/wasm/baseline/liftoff-assembler-defs.h"
#include "src/wasm/baseline/liftoff-compiler.h"
#include "src/wasm/baseline/liftoff-register.h"
#include "src/wasm/function-body-decoder.h"
#include "src/wasm/wasm-code-manager.h"
#include "src/wasm/wasm-module.h"
#include "src/wasm/wasm-opcodes.h"
#include "src/wasm/wasm-value.h"

namespace v8 {
namespace internal {

// Forward declarations.
namespace compiler {
class CallDescriptor;
}  // namespace compiler

namespace wasm {

enum LiftoffCondition {
  kEqual,
  kUnequal,
  kSignedLessThan,
  kSignedLessEqual,
  kSignedGreaterThan,
  kSignedGreaterEqual,
  kUnsignedLessThan,
  kUnsignedLessEqual,
  kUnsignedGreaterThan,
  kUnsignedGreaterEqual
};

inline constexpr LiftoffCondition Negate(LiftoffCondition liftoff_cond) {
  switch (liftoff_cond) {
    case kEqual:
      return kUnequal;
    case kUnequal:
      return kEqual;
    case kSignedLessThan:
      return kSignedGreaterEqual;
    case kSignedLessEqual:
      return kSignedGreaterThan;
    case kSignedGreaterEqual:
      return kSignedLessThan;
    case kSignedGreaterThan:
      return kSignedLessEqual;
    case kUnsignedLessThan:
      return kUnsignedGreaterEqual;
    case kUnsignedLessEqual:
      return kUnsignedGreaterThan;
    case kUnsignedGreaterEqual:
      return kUnsignedLessThan;
    case kUnsignedGreaterThan:
      return kUnsignedLessEqual;
  }
}

class LiftoffAssembler : public TurboAssembler {
 public:
  // Each slot in our stack frame currently has exactly 8 bytes.
  static constexpr int kStackSlotSize = 8;

  static constexpr ValueKind kPointerKind =
      kSystemPointerSize == kInt32Size ? kI32 : kI64;
  static constexpr ValueKind kTaggedKind =
      kTaggedSize == kInt32Size ? kI32 : kI64;
  static constexpr ValueKind kSmiKind = kTaggedKind;

  using ValueKindSig = Signature<ValueKind>;

  class VarState {
   public:
    enum Location : uint8_t { kStack, kRegister, kIntConst };

    explicit VarState(ValueKind kind, int offset)
        : loc_(kStack), kind_(kind), spill_offset_(offset) {}
    explicit VarState(ValueKind kind, LiftoffRegister r, int offset)
        : loc_(kRegister), kind_(kind), reg_(r), spill_offset_(offset) {
      DCHECK_EQ(r.reg_class(), reg_class_for(kind));
    }
    explicit VarState(ValueKind kind, int32_t i32_const, int offset)
        : loc_(kIntConst),
          kind_(kind),
          i32_const_(i32_const),
          spill_offset_(offset) {
      DCHECK(kind_ == kI32 || kind_ == kI64);
    }

    bool is_stack() const { return loc_ == kStack; }
    bool is_gp_reg() const { return loc_ == kRegister && reg_.is_gp(); }
    bool is_fp_reg() const { return loc_ == kRegister && reg_.is_fp(); }
    bool is_reg() const { return loc_ == kRegister; }
    bool is_const() const { return loc_ == kIntConst; }

    ValueKind kind() const { return kind_; }

    Location loc() const { return loc_; }

    int32_t i32_const() const {
      DCHECK_EQ(loc_, kIntConst);
      return i32_const_;
    }
    WasmValue constant() const {
      DCHECK(kind_ == kI32 || kind_ == kI64);
      DCHECK_EQ(loc_, kIntConst);
      return kind_ == kI32 ? WasmValue(i32_const_)
                           : WasmValue(int64_t{i32_const_});
    }

    int offset() const { return spill_offset_; }

    Register gp_reg() const { return reg().gp(); }
    DoubleRegister fp_reg() const { return reg().fp(); }
    LiftoffRegister reg() const {
      DCHECK_EQ(loc_, kRegister);
      return reg_;
    }
    RegClass reg_class() const { return reg().reg_class(); }

    void MakeStack() { loc_ = kStack; }

    void MakeRegister(LiftoffRegister r) {
      loc_ = kRegister;
      reg_ = r;
    }

    void MakeConstant(int32_t i32_const) {
      DCHECK(kind_ == kI32 || kind_ == kI64);
      loc_ = kIntConst;
      i32_const_ = i32_const;
    }

    // Copy src to this, except for offset, since src and this could have been
    // from different stack states.
    void Copy(VarState src) {
      loc_ = src.loc();
      kind_ = src.kind();
      if (loc_ == kRegister) {
        reg_ = src.reg();
      } else if (loc_ == kIntConst) {
        i32_const_ = src.i32_const();
      }
    }

   private:
    Location loc_;
    // TODO(wasm): This is redundant, the decoder already knows the type of each
    // stack value. Try to collapse.
    ValueKind kind_;

    union {
      LiftoffRegister reg_;  // used if loc_ == kRegister
      int32_t i32_const_;    // used if loc_ == kIntConst
    };
    int spill_offset_;
  };

  ASSERT_TRIVIALLY_COPYABLE(VarState);

  struct CacheState {
    // Allow default construction, move construction, and move assignment.
    CacheState() = default;
    CacheState(CacheState&&) V8_NOEXCEPT = default;
    CacheState& operator=(CacheState&&) V8_NOEXCEPT = default;
    // Disallow copy construction.
    CacheState(const CacheState&) = delete;

    enum class SpillLocation { kTopOfStack, kStackSlots };
    // Generates two lists of locations that contain references. {slots}
    // contains the indices of slots on the value stack that contain references.
    // {spills} contains all registers that contain references. The
    // {spill_location} defines where register values will be spilled for a
    // function call within the out-of-line code. {kStackSlots} means that the
    // values in the registers will be written back to their stack slots.
    // {kTopOfStack} means that the registers will be spilled on the stack with
    // a {push} instruction.
    void GetTaggedSlotsForOOLCode(/*out*/ ZoneVector<int>* slots,
                                  /*out*/ LiftoffRegList* spills,
                                  SpillLocation spill_location);

    void DefineSafepoint(Safepoint& safepoint);

    void DefineSafepointWithCalleeSavedRegisters(Safepoint& safepoint);

    base::SmallVector<VarState, 8> stack_state;
    LiftoffRegList used_registers;
    uint32_t register_use_count[kAfterMaxLiftoffRegCode] = {0};
    LiftoffRegList last_spilled_regs;
    Register cached_instance = no_reg;

    bool has_unused_register(RegClass rc, LiftoffRegList pinned = {}) const {
      if (kNeedI64RegPair && rc == kGpRegPair) {
        LiftoffRegList available_regs =
            kGpCacheRegList.MaskOut(used_registers).MaskOut(pinned);
        return available_regs.GetNumRegsSet() >= 2;
      } else if (kNeedS128RegPair && rc == kFpRegPair) {
        LiftoffRegList available_regs =
            kFpCacheRegList.MaskOut(used_registers).MaskOut(pinned);
        return available_regs.HasAdjacentFpRegsSet();
      }
      DCHECK(rc == kGpReg || rc == kFpReg);
      LiftoffRegList candidates = GetCacheRegList(rc);
      return has_unused_register(candidates.MaskOut(pinned));
    }

    bool has_unused_register(LiftoffRegList candidates) const {
      LiftoffRegList available_regs = candidates.MaskOut(used_registers);
      return !available_regs.is_empty();
    }

    LiftoffRegister unused_register(RegClass rc,
                                    LiftoffRegList pinned = {}) const {
      if (kNeedI64RegPair && rc == kGpRegPair) {
        Register low = pinned.set(unused_register(kGpReg, pinned)).gp();
        Register high = unused_register(kGpReg, pinned).gp();
        return LiftoffRegister::ForPair(low, high);
      } else if (kNeedS128RegPair && rc == kFpRegPair) {
        LiftoffRegList available_regs =
            kFpCacheRegList.MaskOut(used_registers).MaskOut(pinned);
        DoubleRegister low =
            available_regs.GetAdjacentFpRegsSet().GetFirstRegSet().fp();
        DCHECK(is_free(LiftoffRegister::ForFpPair(low)));
        return LiftoffRegister::ForFpPair(low);
      }
      DCHECK(rc == kGpReg || rc == kFpReg);
      LiftoffRegList candidates = GetCacheRegList(rc);
      return unused_register(candidates, pinned);
    }

    LiftoffRegister unused_register(LiftoffRegList candidates,
                                    LiftoffRegList pinned = {}) const {
      LiftoffRegList available_regs =
          candidates.MaskOut(used_registers).MaskOut(pinned);
      return available_regs.GetFirstRegSet();
    }

    // Volatile registers are registers which are used for caching values that
    // can easily be reloaded. Those are returned first if we run out of free
    // registers.
    // Note: This interface is a bit more generic than currently needed, in
    // anticipation of more "volatile registers" being added later.
    bool has_volatile_register(LiftoffRegList candidates) {
      return cached_instance != no_reg && candidates.has(cached_instance);
    }

    LiftoffRegister take_volatile_register(LiftoffRegList candidates) {
      DCHECK(candidates.has(cached_instance));
      LiftoffRegister ret{cached_instance};
      DCHECK_EQ(1, register_use_count[ret.liftoff_code()]);
      register_use_count[ret.liftoff_code()] = 0;
      used_registers.clear(ret);
      cached_instance = no_reg;
      return ret;
    }

    void SetInstanceCacheRegister(Register reg) {
      DCHECK_EQ(no_reg, cached_instance);
      cached_instance = reg;
      int liftoff_code = LiftoffRegister{reg}.liftoff_code();
      DCHECK_EQ(0, register_use_count[liftoff_code]);
      register_use_count[liftoff_code] = 1;
      used_registers.set(reg);
    }

    Register TrySetCachedInstanceRegister(LiftoffRegList pinned) {
      DCHECK_EQ(no_reg, cached_instance);
      LiftoffRegList available_regs =
          kGpCacheRegList.MaskOut(pinned).MaskOut(used_registers);
      if (available_regs.is_empty()) return no_reg;
      // Prefer the {kWasmInstanceRegister}, because that's where the instance
      // initially is, and where it needs to be for calls.
      Register new_cache_reg = available_regs.has(kWasmInstanceRegister)
                                   ? kWasmInstanceRegister
                                   : available_regs.GetFirstRegSet().gp();
      SetInstanceCacheRegister(new_cache_reg);
      DCHECK_EQ(new_cache_reg, cached_instance);
      return new_cache_reg;
    }

    void ClearCachedInstanceRegister() {
      if (cached_instance == no_reg) return;
      int liftoff_code = LiftoffRegister{cached_instance}.liftoff_code();
      DCHECK_EQ(1, register_use_count[liftoff_code]);
      register_use_count[liftoff_code] = 0;
      used_registers.clear(cached_instance);
      cached_instance = no_reg;
    }

    void inc_used(LiftoffRegister reg) {
      if (reg.is_pair()) {
        inc_used(reg.low());
        inc_used(reg.high());
        return;
      }
      used_registers.set(reg);
      DCHECK_GT(kMaxInt, register_use_count[reg.liftoff_code()]);
      ++register_use_count[reg.liftoff_code()];
    }

    // Returns whether this was the last use.
    void dec_used(LiftoffRegister reg) {
      DCHECK(is_used(reg));
      if (reg.is_pair()) {
        dec_used(reg.low());
        dec_used(reg.high());
        return;
      }
      int code = reg.liftoff_code();
      DCHECK_LT(0, register_use_count[code]);
      if (--register_use_count[code] == 0) used_registers.clear(reg);
    }

    bool is_used(LiftoffRegister reg) const {
      if (reg.is_pair()) return is_used(reg.low()) || is_used(reg.high());
      bool used = used_registers.has(reg);
      DCHECK_EQ(used, register_use_count[reg.liftoff_code()] != 0);
      return used;
    }

    uint32_t get_use_count(LiftoffRegister reg) const {
      if (reg.is_pair()) {
        DCHECK_EQ(register_use_count[reg.low().liftoff_code()],
                  register_use_count[reg.high().liftoff_code()]);
        reg = reg.low();
      }
      DCHECK_GT(arraysize(register_use_count), reg.liftoff_code());
      return register_use_count[reg.liftoff_code()];
    }

    void clear_used(LiftoffRegister reg) {
      if (reg.is_pair()) {
        clear_used(reg.low());
        clear_used(reg.high());
        return;
      }
      register_use_count[reg.liftoff_code()] = 0;
      used_registers.clear(reg);
    }

    bool is_free(LiftoffRegister reg) const { return !is_used(reg); }

    void reset_used_registers() {
      used_registers = {};
      memset(register_use_count, 0, sizeof(register_use_count));
    }

    LiftoffRegister GetNextSpillReg(LiftoffRegList candidates) {
      DCHECK(!candidates.is_empty());
      // This method should only be called if none of the candidates is free.
      DCHECK(candidates.MaskOut(used_registers).is_empty());
      LiftoffRegList unspilled = candidates.MaskOut(last_spilled_regs);
      if (unspilled.is_empty()) {
        unspilled = candidates;
        last_spilled_regs = {};
      }
      LiftoffRegister reg = unspilled.GetFirstRegSet();
      return reg;
    }

    // TODO(clemensb): Don't copy the full parent state (this makes us N^2).
    void InitMerge(const CacheState& source, uint32_t num_locals,
                   uint32_t arity, uint32_t stack_depth);

    void Steal(const CacheState& source);

    void Split(const CacheState& source);

    uint32_t stack_height() const {
      return static_cast<uint32_t>(stack_state.size());
    }

   private:
    // Make the copy assignment operator private (to be used from {Split()}).
    CacheState& operator=(const CacheState&) V8_NOEXCEPT = default;
  };

  explicit LiftoffAssembler(std::unique_ptr<AssemblerBuffer>);
  ~LiftoffAssembler() override;

  LiftoffRegister LoadToRegister(VarState slot, LiftoffRegList pinned);

  LiftoffRegister PopToRegister(LiftoffRegList pinned = {}) {
    DCHECK(!cache_state_.stack_state.empty());
    VarState slot = cache_state_.stack_state.back();
    cache_state_.stack_state.pop_back();
    if (slot.is_reg()) {
      cache_state_.dec_used(slot.reg());
      return slot.reg();
    }
    return LoadToRegister(slot, pinned);
  }

  // Use this to pop a value into a register that has no other uses, so it
  // can be modified.
  LiftoffRegister PopToModifiableRegister(LiftoffRegList pinned = {}) {
    ValueKind kind = cache_state_.stack_state.back().kind();
    LiftoffRegister reg = PopToRegister(pinned);
    if (cache_state()->is_free(reg)) return reg;

    pinned.set(reg);
    LiftoffRegister new_reg = GetUnusedRegister(reg.reg_class(), pinned);
    Move(new_reg, reg, kind);
    return new_reg;
  }

  // Returns the register which holds the value of stack slot {index}. If the
  // value is not stored in a register yet, a register is allocated for it. The
  // register is then assigned to the stack slot. The value stack height is not
  // modified. The top of the stack is index 0, i.e. {PopToRegister()} and
  // {PeekToRegister(0)} should result in the same register.
  // When the value is finally popped, the use counter of its register has to be
  // decremented. This can be done by popping the value with {DropValues}.
  LiftoffRegister PeekToRegister(int index, LiftoffRegList pinned);

  void DropValues(int count);

  // Careful: this indexes "from the other end", i.e. depth=0 is the value
  // at the bottom of the stack!
  void DropValue(int depth);

  // Ensure that the loop inputs are either in a register or spilled to the
  // stack, so that we can merge different values on the back-edge.
  void PrepareLoopArgs(int num);

  int NextSpillOffset(ValueKind kind) {
    int offset = TopSpillOffset() + SlotSizeForType(kind);
    if (NeedsAlignment(kind)) {
      offset = RoundUp(offset, SlotSizeForType(kind));
    }
    return offset;
  }

  int TopSpillOffset() const {
    return cache_state_.stack_state.empty()
               ? StaticStackFrameSize()
               : cache_state_.stack_state.back().offset();
  }

  void PushRegister(ValueKind kind, LiftoffRegister reg) {
    DCHECK_EQ(reg_class_for(kind), reg.reg_class());
    cache_state_.inc_used(reg);
    cache_state_.stack_state.emplace_back(kind, reg, NextSpillOffset(kind));
  }

  // Assumes that the exception is in {kReturnRegister0}. This is where the
  // exception is stored by the unwinder after a throwing call.
  void PushException() {
    LiftoffRegister reg{kReturnRegister0};
    // This is used after a call, so {kReturnRegister0} is not used yet.
    DCHECK(cache_state_.is_free(reg));
    cache_state_.inc_used(reg);
    cache_state_.stack_state.emplace_back(kRef, reg, NextSpillOffset(kRef));
  }

  void PushConstant(ValueKind kind, int32_t i32_const) {
    DCHECK(kind == kI32 || kind == kI64);
    cache_state_.stack_state.emplace_back(kind, i32_const,
                                          NextSpillOffset(kind));
  }

  void PushStack(ValueKind kind) {
    cache_state_.stack_state.emplace_back(kind, NextSpillOffset(kind));
  }

  void SpillRegister(LiftoffRegister);

  uint32_t GetNumUses(LiftoffRegister reg) const {
    return cache_state_.get_use_count(reg);
  }

  // Get an unused register for class {rc}, reusing one of {try_first} if
  // possible.
  LiftoffRegister GetUnusedRegister(
      RegClass rc, std::initializer_list<LiftoffRegister> try_first,
      LiftoffRegList pinned) {
    for (LiftoffRegister reg : try_first) {
      DCHECK_EQ(reg.reg_class(), rc);
      if (cache_state_.is_free(reg)) return reg;
    }
    return GetUnusedRegister(rc, pinned);
  }

  // Get an unused register for class {rc}, potentially spilling to free one.
  LiftoffRegister GetUnusedRegister(RegClass rc, LiftoffRegList pinned) {
    if (kNeedI64RegPair && rc == kGpRegPair) {
      LiftoffRegList candidates = kGpCacheRegList.MaskOut(pinned);
      Register low = candidates.clear(GetUnusedRegister(candidates)).gp();
      Register high = GetUnusedRegister(candidates).gp();
      return LiftoffRegister::ForPair(low, high);
    } else if (kNeedS128RegPair && rc == kFpRegPair) {
      // kFpRegPair specific logic here because we need adjacent registers, not
      // just any two registers (like kGpRegPair).
      if (cache_state_.has_unused_register(rc, pinned)) {
        return cache_state_.unused_register(rc, pinned);
      }
      DoubleRegister low_fp = SpillAdjacentFpRegisters(pinned).fp();
      return LiftoffRegister::ForFpPair(low_fp);
    }
    DCHECK(rc == kGpReg || rc == kFpReg);
    LiftoffRegList candidates = GetCacheRegList(rc).MaskOut(pinned);
    return GetUnusedRegister(candidates);
  }

  // Get an unused register of {candidates}, potentially spilling to free one.
  LiftoffRegister GetUnusedRegister(LiftoffRegList candidates) {
    DCHECK(!candidates.is_empty());
    if (cache_state_.has_unused_register(candidates)) {
      return cache_state_.unused_register(candidates);
    }
    if (cache_state_.has_volatile_register(candidates)) {
      return cache_state_.take_volatile_register(candidates);
    }
    return SpillOneRegister(candidates);
  }

  void MaterializeMergedConstants(uint32_t arity);

  enum JumpDirection { kForwardJump, kBackwardJump };
  void MergeFullStackWith(CacheState& target, const CacheState& source);
  void MergeStackWith(CacheState& target, uint32_t arity, JumpDirection);

  void Spill(VarState* slot);
  void SpillLocals();
  void SpillAllRegisters();

  // Clear any uses of {reg} in both the cache and in {possible_uses}.
  // Any use in the stack is spilled. If any register in {possible_uses} matches
  // {reg}, then the content of {reg} is moved to a new temporary register, and
  // all matches in {possible_uses} are rewritten to that temporary register.
  void ClearRegister(Register reg,
                     std::initializer_list<Register*> possible_uses,
                     LiftoffRegList pinned);

  // Spills all passed registers.
  template <typename... Regs>
  void SpillRegisters(Regs... regs) {
    for (LiftoffRegister r : {LiftoffRegister(regs)...}) {
      if (cache_state_.is_free(r)) continue;
      if (r.is_gp() && cache_state_.cached_instance == r.gp()) {
        cache_state_.ClearCachedInstanceRegister();
      } else {
        SpillRegister(r);
      }
    }
  }

  // Call this method whenever spilling something, such that the number of used
  // spill slot can be tracked and the stack frame will be allocated big enough.
  void RecordUsedSpillOffset(int offset) {
    if (offset >= max_used_spill_offset_) max_used_spill_offset_ = offset;
  }

  void RecordOolSpillSpaceSize(int size) {
    if (size > ool_spill_space_size_) ool_spill_space_size_ = size;
  }

  // Load parameters into the right registers / stack slots for the call.
  void PrepareBuiltinCall(const ValueKindSig* sig,
                          compiler::CallDescriptor* call_descriptor,
                          std::initializer_list<VarState> params);

  // Load parameters into the right registers / stack slots for the call.
  // Move {*target} into another register if needed and update {*target} to that
  // register, or {no_reg} if target was spilled to the stack.
  void PrepareCall(const ValueKindSig*, compiler::CallDescriptor*,
                   Register* target = nullptr,
                   Register* target_instance = nullptr);
  // Process return values of the call.
  void FinishCall(const ValueKindSig*, compiler::CallDescriptor*);

  // Move {src} into {dst}. {src} and {dst} must be different.
  void Move(LiftoffRegister dst, LiftoffRegister src, ValueKind);

  // Parallel register move: For a list of tuples <dst, src, kind>, move the
  // {src} register of kind {kind} into {dst}. If {src} equals {dst}, ignore
  // that tuple.
  struct ParallelRegisterMoveTuple {
    LiftoffRegister dst;
    LiftoffRegister src;
    ValueKind kind;
    template <typename Dst, typename Src>
    ParallelRegisterMoveTuple(Dst dst, Src src, ValueKind kind)
        : dst(dst), src(src), kind(kind) {}
  };

  void ParallelRegisterMove(Vector<const ParallelRegisterMoveTuple>);

  void ParallelRegisterMove(
      std::initializer_list<ParallelRegisterMoveTuple> moves) {
    ParallelRegisterMove(VectorOf(moves));
  }

  void MoveToReturnLocations(const FunctionSig*,
                             compiler::CallDescriptor* descriptor);

#ifdef ENABLE_SLOW_DCHECKS
  // Validate that the register use counts reflect the state of the cache.
  bool ValidateCacheState() const;
#endif

  ////////////////////////////////////
  // Platform-specific part.        //
  ////////////////////////////////////

  // This function emits machine code to prepare the stack frame, before the
  // size of the stack frame is known. It returns an offset in the machine code
  // which can later be patched (via {PatchPrepareStackFrame)} when the size of
  // the frame is known.
  inline int PrepareStackFrame();
  inline void PrepareTailCall(int num_callee_stack_params,
                              int stack_param_delta);
  inline void AlignFrameSize();
  inline void PatchPrepareStackFrame(int offset);
  inline void FinishCode();
  inline void AbortCompilation();
  inline static constexpr int StaticStackFrameSize();
  inline static int SlotSizeForType(ValueKind kind);
  inline static bool NeedsAlignment(ValueKind kind);

  inline void LoadConstant(LiftoffRegister, WasmValue,
                           RelocInfo::Mode rmode = RelocInfo::NONE);
  inline void LoadInstanceFromFrame(Register dst);
  inline void LoadFromInstance(Register dst, Register instance, int offset,
                               int size);
  inline void LoadTaggedPointerFromInstance(Register dst, Register instance,
                                            int offset);
  inline void SpillInstance(Register instance);
  inline void ResetOSRTarget();
  inline void FillInstanceInto(Register dst);
  inline void LoadTaggedPointer(Register dst, Register src_addr,
                                Register offset_reg, int32_t offset_imm,
                                LiftoffRegList pinned);
  inline void LoadFullPointer(Register dst, Register src_addr,
                              int32_t offset_imm);
  enum SkipWriteBarrier : bool {
    kSkipWriteBarrier = true,
    kNoSkipWriteBarrier = false
  };
  inline void StoreTaggedPointer(Register dst_addr, Register offset_reg,
                                 int32_t offset_imm, LiftoffRegister src,
                                 LiftoffRegList pinned,
                                 SkipWriteBarrier = kNoSkipWriteBarrier);
  inline void LoadFixedArrayLengthAsInt32(LiftoffRegister dst, Register array,
                                          LiftoffRegList pinned) {
    int offset = FixedArray::kLengthOffset - kHeapObjectTag;
    LoadSmiAsInt32(dst, array, offset, pinned);
  }
  inline void LoadSmiAsInt32(LiftoffRegister dst, Register src_addr,
                             int32_t offset, LiftoffRegList pinned) {
    if (SmiValuesAre32Bits()) {
#if V8_TARGET_LITTLE_ENDIAN
      DCHECK_EQ(kSmiShiftSize + kSmiTagSize, 4 * kBitsPerByte);
      offset += 4;
#endif
      Load(dst, src_addr, no_reg, offset, LoadType::kI32Load, pinned);
    } else {
      DCHECK(SmiValuesAre31Bits());
      Load(dst, src_addr, no_reg, offset, LoadType::kI32Load, pinned);
      emit_i32_sari(dst.gp(), dst.gp(), kSmiTagSize);
    }
  }
  inline void Load(LiftoffRegister dst, Register src_addr, Register offset_reg,
                   uintptr_t offset_imm, LoadType type, LiftoffRegList pinned,
                   uint32_t* protected_load_pc = nullptr,
                   bool is_load_mem = false);
  inline void Store(Register dst_addr, Register offset_reg,
                    uintptr_t offset_imm, LiftoffRegister src, StoreType type,
                    LiftoffRegList pinned,
                    uint32_t* protected_store_pc = nullptr,
                    bool is_store_mem = false);
  inline void AtomicLoad(LiftoffRegister dst, Register src_addr,
                         Register offset_reg, uintptr_t offset_imm,
                         LoadType type, LiftoffRegList pinned);
  inline void AtomicStore(Register dst_addr, Register offset_reg,
                          uintptr_t offset_imm, LiftoffRegister src,
                          StoreType type, LiftoffRegList pinned);

  inline void AtomicAdd(Register dst_addr, Register offset_reg,
                        uintptr_t offset_imm, LiftoffRegister value,
                        LiftoffRegister result, StoreType type);

  inline void AtomicSub(Register dst_addr, Register offset_reg,
                        uintptr_t offset_imm, LiftoffRegister value,
                        LiftoffRegister result, StoreType type);

  inline void AtomicAnd(Register dst_addr, Register offset_reg,
                        uintptr_t offset_imm, LiftoffRegister value,
                        LiftoffRegister result, StoreType type);

  inline void AtomicOr(Register dst_addr, Register offset_reg,
                       uintptr_t offset_imm, LiftoffRegister value,
                       LiftoffRegister result, StoreType type);

  inline void AtomicXor(Register dst_addr, Register offset_reg,
                        uintptr_t offset_imm, LiftoffRegister value,
                        LiftoffRegister result, StoreType type);

  inline void AtomicExchange(Register dst_addr, Register offset_reg,
                             uintptr_t offset_imm, LiftoffRegister value,
                             LiftoffRegister result, StoreType type);

  inline void AtomicCompareExchange(Register dst_addr, Register offset_reg,
                                    uintptr_t offset_imm,
                                    LiftoffRegister expected,
                                    LiftoffRegister new_value,
                                    LiftoffRegister value, StoreType type);

  inline void AtomicFence();

  inline void LoadCallerFrameSlot(LiftoffRegister, uint32_t caller_slot_idx,
                                  ValueKind);
  inline void StoreCallerFrameSlot(LiftoffRegister, uint32_t caller_slot_idx,
                                   ValueKind);
  inline void LoadReturnStackSlot(LiftoffRegister, int offset, ValueKind);
  inline void MoveStackValue(uint32_t dst_offset, uint32_t src_offset,
                             ValueKind);

  inline void Move(Register dst, Register src, ValueKind);
  inline void Move(DoubleRegister dst, DoubleRegister src, ValueKind);

  inline void Spill(int offset, LiftoffRegister, ValueKind);
  inline void Spill(int offset, WasmValue);
  inline void Fill(LiftoffRegister, int offset, ValueKind);
  // Only used on 32-bit systems: Fill a register from a "half stack slot", i.e.
  // 4 bytes on the stack holding half of a 64-bit value.
  inline void FillI64Half(Register, int offset, RegPairHalf);
  inline void FillStackSlotsWithZero(int start, int size);

  // i32 binops.
  inline void emit_i32_add(Register dst, Register lhs, Register rhs);
  inline void emit_i32_addi(Register dst, Register lhs, int32_t imm);
  inline void emit_i32_sub(Register dst, Register lhs, Register rhs);
  inline void emit_i32_subi(Register dst, Register lhs, int32_t imm);
  inline void emit_i32_mul(Register dst, Register lhs, Register rhs);
  inline void emit_i32_divs(Register dst, Register lhs, Register rhs,
                            Label* trap_div_by_zero,
                            Label* trap_div_unrepresentable);
  inline void emit_i32_divu(Register dst, Register lhs, Register rhs,
                            Label* trap_div_by_zero);
  inline void emit_i32_rems(Register dst, Register lhs, Register rhs,
                            Label* trap_rem_by_zero);
  inline void emit_i32_remu(Register dst, Register lhs, Register rhs,
                            Label* trap_rem_by_zero);
  inline void emit_i32_and(Register dst, Register lhs, Register rhs);
  inline void emit_i32_andi(Register dst, Register lhs, int32_t imm);
  inline void emit_i32_or(Register dst, Register lhs, Register rhs);
  inline void emit_i32_ori(Register dst, Register lhs, int32_t imm);
  inline void emit_i32_xor(Register dst, Register lhs, Register rhs);
  inline void emit_i32_xori(Register dst, Register lhs, int32_t imm);
  inline void emit_i32_shl(Register dst, Register src, Register amount);
  inline void emit_i32_shli(Register dst, Register src, int32_t amount);
  inline void emit_i32_sar(Register dst, Register src, Register amount);
  inline void emit_i32_sari(Register dst, Register src, int32_t amount);
  inline void emit_i32_shr(Register dst, Register src, Register amount);
  inline void emit_i32_shri(Register dst, Register src, int32_t amount);

  // i32 unops.
  inline void emit_i32_clz(Register dst, Register src);
  inline void emit_i32_ctz(Register dst, Register src);
  inline bool emit_i32_popcnt(Register dst, Register src);

  // i64 binops.
  inline void emit_i64_add(LiftoffRegister dst, LiftoffRegister lhs,
                           LiftoffRegister rhs);
  inline void emit_i64_addi(LiftoffRegister dst, LiftoffRegister lhs,
                            int64_t imm);
  inline void emit_i64_sub(LiftoffRegister dst, LiftoffRegister lhs,
                           LiftoffRegister rhs);
  inline void emit_i64_mul(LiftoffRegister dst, LiftoffRegister lhs,
                           LiftoffRegister rhs);
  inline bool emit_i64_divs(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs, Label* trap_div_by_zero,
                            Label* trap_div_unrepresentable);
  inline bool emit_i64_divu(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs, Label* trap_div_by_zero);
  inline bool emit_i64_rems(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs, Label* trap_rem_by_zero);
  inline bool emit_i64_remu(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs, Label* trap_rem_by_zero);
  inline void emit_i64_and(LiftoffRegister dst, LiftoffRegister lhs,
                           LiftoffRegister rhs);
  inline void emit_i64_andi(LiftoffRegister dst, LiftoffRegister lhs,
                            int32_t imm);
  inline void emit_i64_or(LiftoffRegister dst, LiftoffRegister lhs,
                          LiftoffRegister rhs);
  inline void emit_i64_ori(LiftoffRegister dst, LiftoffRegister lhs,
                           int32_t imm);
  inline void emit_i64_xor(LiftoffRegister dst, LiftoffRegister lhs,
                           LiftoffRegister rhs);
  inline void emit_i64_xori(LiftoffRegister dst, LiftoffRegister lhs,
                            int32_t imm);
  inline void emit_i64_shl(LiftoffRegister dst, LiftoffRegister src,
                           Register amount);
  inline void emit_i64_shli(LiftoffRegister dst, LiftoffRegister src,
                            int32_t amount);
  inline void emit_i64_sar(LiftoffRegister dst, LiftoffRegister src,
                           Register amount);
  inline void emit_i64_sari(LiftoffRegister dst, LiftoffRegister src,
                            int32_t amount);
  inline void emit_i64_shr(LiftoffRegister dst, LiftoffRegister src,
                           Register amount);
  inline void emit_i64_shri(LiftoffRegister dst, LiftoffRegister src,
                            int32_t amount);

  // i64 unops.
  inline void emit_i64_clz(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64_ctz(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_i64_popcnt(LiftoffRegister dst, LiftoffRegister src);

  inline void emit_u32_to_intptr(Register dst, Register src);

  inline void emit_ptrsize_add(Register dst, Register lhs, Register rhs) {
    if (kSystemPointerSize == 8) {
      emit_i64_add(LiftoffRegister(dst), LiftoffRegister(lhs),
                   LiftoffRegister(rhs));
    } else {
      emit_i32_add(dst, lhs, rhs);
    }
  }
  inline void emit_ptrsize_sub(Register dst, Register lhs, Register rhs) {
    if (kSystemPointerSize == 8) {
      emit_i64_sub(LiftoffRegister(dst), LiftoffRegister(lhs),
                   LiftoffRegister(rhs));
    } else {
      emit_i32_sub(dst, lhs, rhs);
    }
  }
  inline void emit_ptrsize_and(Register dst, Register lhs, Register rhs) {
    if (kSystemPointerSize == 8) {
      emit_i64_and(LiftoffRegister(dst), LiftoffRegister(lhs),
                   LiftoffRegister(rhs));
    } else {
      emit_i32_and(dst, lhs, rhs);
    }
  }
  inline void emit_ptrsize_shri(Register dst, Register src, int amount) {
    if (kSystemPointerSize == 8) {
      emit_i64_shri(LiftoffRegister(dst), LiftoffRegister(src), amount);
    } else {
      emit_i32_shri(dst, src, amount);
    }
  }

  inline void emit_ptrsize_addi(Register dst, Register lhs, intptr_t imm) {
    if (kSystemPointerSize == 8) {
      emit_i64_addi(LiftoffRegister(dst), LiftoffRegister(lhs), imm);
    } else {
      emit_i32_addi(dst, lhs, static_cast<int32_t>(imm));
    }
  }

  inline void emit_ptrsize_set_cond(LiftoffCondition condition, Register dst,
                                    LiftoffRegister lhs, LiftoffRegister rhs) {
    if (kSystemPointerSize == 8) {
      emit_i64_set_cond(condition, dst, lhs, rhs);
    } else {
      emit_i32_set_cond(condition, dst, lhs.gp(), rhs.gp());
    }
  }

  inline void emit_ptrsize_zeroextend_i32(Register dst, Register src) {
    if (kSystemPointerSize == 8) {
      emit_type_conversion(kExprI64UConvertI32, LiftoffRegister(dst),
                           LiftoffRegister(src));
    } else if (dst != src) {
      Move(dst, src, kI32);
    }
  }

  // f32 binops.
  inline void emit_f32_add(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f32_sub(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f32_mul(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f32_div(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f32_min(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f32_max(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f32_copysign(DoubleRegister dst, DoubleRegister lhs,
                                DoubleRegister rhs);

  // f32 unops.
  inline void emit_f32_abs(DoubleRegister dst, DoubleRegister src);
  inline void emit_f32_neg(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f32_ceil(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f32_floor(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f32_trunc(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f32_nearest_int(DoubleRegister dst, DoubleRegister src);
  inline void emit_f32_sqrt(DoubleRegister dst, DoubleRegister src);

  // f64 binops.
  inline void emit_f64_add(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f64_sub(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f64_mul(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f64_div(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f64_min(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f64_max(DoubleRegister dst, DoubleRegister lhs,
                           DoubleRegister rhs);
  inline void emit_f64_copysign(DoubleRegister dst, DoubleRegister lhs,
                                DoubleRegister rhs);

  // f64 unops.
  inline void emit_f64_abs(DoubleRegister dst, DoubleRegister src);
  inline void emit_f64_neg(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f64_ceil(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f64_floor(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f64_trunc(DoubleRegister dst, DoubleRegister src);
  inline bool emit_f64_nearest_int(DoubleRegister dst, DoubleRegister src);
  inline void emit_f64_sqrt(DoubleRegister dst, DoubleRegister src);

  inline bool emit_type_conversion(WasmOpcode opcode, LiftoffRegister dst,
                                   LiftoffRegister src, Label* trap = nullptr);

  inline void emit_i32_signextend_i8(Register dst, Register src);
  inline void emit_i32_signextend_i16(Register dst, Register src);
  inline void emit_i64_signextend_i8(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64_signextend_i16(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64_signextend_i32(LiftoffRegister dst, LiftoffRegister src);

  inline void emit_jump(Label*);
  inline void emit_jump(Register);

  inline void emit_cond_jump(LiftoffCondition, Label*, ValueKind value,
                             Register lhs, Register rhs = no_reg);
  inline void emit_i32_cond_jumpi(LiftoffCondition liftoff_cond, Label* label,
                                  Register lhs, int imm);
  // Set {dst} to 1 if condition holds, 0 otherwise.
  inline void emit_i32_eqz(Register dst, Register src);
  inline void emit_i32_set_cond(LiftoffCondition, Register dst, Register lhs,
                                Register rhs);
  inline void emit_i64_eqz(Register dst, LiftoffRegister src);
  inline void emit_i64_set_cond(LiftoffCondition condition, Register dst,
                                LiftoffRegister lhs, LiftoffRegister rhs);
  inline void emit_f32_set_cond(LiftoffCondition condition, Register dst,
                                DoubleRegister lhs, DoubleRegister rhs);
  inline void emit_f64_set_cond(LiftoffCondition condition, Register dst,
                                DoubleRegister lhs, DoubleRegister rhs);

  // Optional select support: Returns false if generic code (via branches)
  // should be emitted instead.
  inline bool emit_select(LiftoffRegister dst, Register condition,
                          LiftoffRegister true_value,
                          LiftoffRegister false_value, ValueKind kind);

  enum SmiCheckMode { kJumpOnSmi, kJumpOnNotSmi };
  inline void emit_smi_check(Register obj, Label* target, SmiCheckMode mode);

  inline void LoadTransform(LiftoffRegister dst, Register src_addr,
                            Register offset_reg, uintptr_t offset_imm,
                            LoadType type, LoadTransformationKind transform,
                            uint32_t* protected_load_pc);
  inline void LoadLane(LiftoffRegister dst, LiftoffRegister src, Register addr,
                       Register offset_reg, uintptr_t offset_imm, LoadType type,
                       uint8_t lane, uint32_t* protected_load_pc);
  inline void StoreLane(Register dst, Register offset, uintptr_t offset_imm,
                        LiftoffRegister src, StoreType type, uint8_t lane,
                        uint32_t* protected_store_pc);
  inline void emit_i8x16_shuffle(LiftoffRegister dst, LiftoffRegister lhs,
                                 LiftoffRegister rhs, const uint8_t shuffle[16],
                                 bool is_swizzle);
  inline void emit_i8x16_swizzle(LiftoffRegister dst, LiftoffRegister lhs,
                                 LiftoffRegister rhs);
  inline void emit_i8x16_popcnt(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i8x16_splat(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i16x8_splat(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i32x4_splat(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64x2_splat(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f32x4_splat(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f64x2_splat(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i8x16_eq(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i8x16_ne(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i8x16_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i8x16_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i8x16_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i8x16_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i16x8_eq(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i16x8_ne(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i16x8_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i16x8_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i16x8_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i16x8_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i32x4_eq(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i32x4_ne(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i32x4_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i32x4_gt_u(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i32x4_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i32x4_ge_u(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i64x2_eq(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i64x2_ne(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_i64x2_gt_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_i64x2_ge_s(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_f32x4_eq(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f32x4_ne(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f32x4_lt(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f32x4_le(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f64x2_eq(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f64x2_ne(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f64x2_lt(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_f64x2_le(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_s128_const(LiftoffRegister dst, const uint8_t imms[16]);
  inline void emit_s128_not(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_s128_and(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_s128_or(LiftoffRegister dst, LiftoffRegister lhs,
                           LiftoffRegister rhs);
  inline void emit_s128_xor(LiftoffRegister dst, LiftoffRegister lhs,
                            LiftoffRegister rhs);
  inline void emit_s128_select(LiftoffRegister dst, LiftoffRegister src1,
                               LiftoffRegister src2, LiftoffRegister mask);
  inline void emit_i8x16_neg(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_v128_anytrue(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i8x16_alltrue(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i8x16_bitmask(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i8x16_shl(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i8x16_shli(LiftoffRegister dst, LiftoffRegister lhs,
                              int32_t rhs);
  inline void emit_i8x16_shr_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i8x16_shri_s(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i8x16_shr_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i8x16_shri_u(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i8x16_add(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i8x16_add_sat_s(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i8x16_add_sat_u(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i8x16_sub(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i8x16_sub_sat_s(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i8x16_sub_sat_u(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i8x16_min_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i8x16_min_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i8x16_max_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i8x16_max_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_neg(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i16x8_alltrue(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i16x8_bitmask(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i16x8_shl(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i16x8_shli(LiftoffRegister dst, LiftoffRegister lhs,
                              int32_t rhs);
  inline void emit_i16x8_shr_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_shri_s(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i16x8_shr_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_shri_u(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i16x8_add(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i16x8_add_sat_s(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i16x8_add_sat_u(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i16x8_sub(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i16x8_sub_sat_s(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i16x8_sub_sat_u(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs);
  inline void emit_i16x8_mul(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i16x8_min_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_min_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_max_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_max_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i16x8_extadd_pairwise_i8x16_s(LiftoffRegister dst,
                                                 LiftoffRegister src);
  inline void emit_i16x8_extadd_pairwise_i8x16_u(LiftoffRegister dst,
                                                 LiftoffRegister src);
  inline void emit_i16x8_extmul_low_i8x16_s(LiftoffRegister dst,
                                            LiftoffRegister src1,
                                            LiftoffRegister src2);
  inline void emit_i16x8_extmul_low_i8x16_u(LiftoffRegister dst,
                                            LiftoffRegister src1,
                                            LiftoffRegister src2);
  inline void emit_i16x8_extmul_high_i8x16_s(LiftoffRegister dst,
                                             LiftoffRegister src1,
                                             LiftoffRegister src2);
  inline void emit_i16x8_extmul_high_i8x16_u(LiftoffRegister dst,
                                             LiftoffRegister src1,
                                             LiftoffRegister src2);
  inline void emit_i16x8_q15mulr_sat_s(LiftoffRegister dst,
                                       LiftoffRegister src1,
                                       LiftoffRegister src2);
  inline void emit_i32x4_neg(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i32x4_alltrue(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i32x4_bitmask(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i32x4_shl(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i32x4_shli(LiftoffRegister dst, LiftoffRegister lhs,
                              int32_t rhs);
  inline void emit_i32x4_shr_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i32x4_shri_s(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i32x4_shr_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i32x4_shri_u(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i32x4_add(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i32x4_sub(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i32x4_mul(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i32x4_min_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i32x4_min_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i32x4_max_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i32x4_max_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i32x4_dot_i16x8_s(LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs);
  inline void emit_i32x4_extadd_pairwise_i16x8_s(LiftoffRegister dst,
                                                 LiftoffRegister src);
  inline void emit_i32x4_extadd_pairwise_i16x8_u(LiftoffRegister dst,
                                                 LiftoffRegister src);
  inline void emit_i32x4_extmul_low_i16x8_s(LiftoffRegister dst,
                                            LiftoffRegister src1,
                                            LiftoffRegister src2);
  inline void emit_i32x4_extmul_low_i16x8_u(LiftoffRegister dst,
                                            LiftoffRegister src1,
                                            LiftoffRegister src2);
  inline void emit_i32x4_extmul_high_i16x8_s(LiftoffRegister dst,
                                             LiftoffRegister src1,
                                             LiftoffRegister src2);
  inline void emit_i32x4_extmul_high_i16x8_u(LiftoffRegister dst,
                                             LiftoffRegister src1,
                                             LiftoffRegister src2);
  inline void emit_i64x2_neg(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64x2_alltrue(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64x2_shl(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i64x2_shli(LiftoffRegister dst, LiftoffRegister lhs,
                              int32_t rhs);
  inline void emit_i64x2_shr_s(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i64x2_shri_s(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i64x2_shr_u(LiftoffRegister dst, LiftoffRegister lhs,
                               LiftoffRegister rhs);
  inline void emit_i64x2_shri_u(LiftoffRegister dst, LiftoffRegister lhs,
                                int32_t rhs);
  inline void emit_i64x2_add(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i64x2_sub(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i64x2_mul(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_i64x2_extmul_low_i32x4_s(LiftoffRegister dst,
                                            LiftoffRegister src1,
                                            LiftoffRegister src2);
  inline void emit_i64x2_extmul_low_i32x4_u(LiftoffRegister dst,
                                            LiftoffRegister src1,
                                            LiftoffRegister src2);
  inline void emit_i64x2_extmul_high_i32x4_s(LiftoffRegister dst,
                                             LiftoffRegister src1,
                                             LiftoffRegister src2);
  inline void emit_i64x2_extmul_high_i32x4_u(LiftoffRegister dst,
                                             LiftoffRegister src1,
                                             LiftoffRegister src2);
  inline void emit_i64x2_bitmask(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64x2_sconvert_i32x4_low(LiftoffRegister dst,
                                            LiftoffRegister src);
  inline void emit_i64x2_sconvert_i32x4_high(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_i64x2_uconvert_i32x4_low(LiftoffRegister dst,
                                            LiftoffRegister src);
  inline void emit_i64x2_uconvert_i32x4_high(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_f32x4_abs(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f32x4_neg(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f32x4_sqrt(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f32x4_ceil(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f32x4_floor(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f32x4_trunc(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f32x4_nearest_int(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f32x4_add(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f32x4_sub(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f32x4_mul(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f32x4_div(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f32x4_min(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f32x4_max(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f32x4_pmin(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_f32x4_pmax(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_f64x2_abs(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f64x2_neg(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f64x2_sqrt(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f64x2_ceil(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f64x2_floor(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f64x2_trunc(LiftoffRegister dst, LiftoffRegister src);
  inline bool emit_f64x2_nearest_int(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_f64x2_add(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f64x2_sub(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f64x2_mul(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f64x2_div(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f64x2_min(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f64x2_max(LiftoffRegister dst, LiftoffRegister lhs,
                             LiftoffRegister rhs);
  inline void emit_f64x2_pmin(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_f64x2_pmax(LiftoffRegister dst, LiftoffRegister lhs,
                              LiftoffRegister rhs);
  inline void emit_f64x2_convert_low_i32x4_s(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_f64x2_convert_low_i32x4_u(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_i32x4_trunc_sat_f64x2_s_zero(LiftoffRegister dst,
                                                LiftoffRegister src);
  inline void emit_i32x4_trunc_sat_f64x2_u_zero(LiftoffRegister dst,
                                                LiftoffRegister src);
  inline void emit_f32x4_demote_f64x2_zero(LiftoffRegister dst,
                                           LiftoffRegister src);
  inline void emit_f64x2_promote_low_f32x4(LiftoffRegister dst,
                                           LiftoffRegister src);
  inline void emit_i32x4_sconvert_f32x4(LiftoffRegister dst,
                                        LiftoffRegister src);
  inline void emit_i32x4_uconvert_f32x4(LiftoffRegister dst,
                                        LiftoffRegister src);
  inline void emit_f32x4_sconvert_i32x4(LiftoffRegister dst,
                                        LiftoffRegister src);
  inline void emit_f32x4_uconvert_i32x4(LiftoffRegister dst,
                                        LiftoffRegister src);
  inline void emit_i8x16_sconvert_i16x8(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs);
  inline void emit_i8x16_uconvert_i16x8(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs);
  inline void emit_i16x8_sconvert_i32x4(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs);
  inline void emit_i16x8_uconvert_i32x4(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        LiftoffRegister rhs);
  inline void emit_i16x8_sconvert_i8x16_low(LiftoffRegister dst,
                                            LiftoffRegister src);
  inline void emit_i16x8_sconvert_i8x16_high(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_i16x8_uconvert_i8x16_low(LiftoffRegister dst,
                                            LiftoffRegister src);
  inline void emit_i16x8_uconvert_i8x16_high(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_i32x4_sconvert_i16x8_low(LiftoffRegister dst,
                                            LiftoffRegister src);
  inline void emit_i32x4_sconvert_i16x8_high(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_i32x4_uconvert_i16x8_low(LiftoffRegister dst,
                                            LiftoffRegister src);
  inline void emit_i32x4_uconvert_i16x8_high(LiftoffRegister dst,
                                             LiftoffRegister src);
  inline void emit_s128_and_not(LiftoffRegister dst, LiftoffRegister lhs,
                                LiftoffRegister rhs);
  inline void emit_i8x16_rounding_average_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs);
  inline void emit_i16x8_rounding_average_u(LiftoffRegister dst,
                                            LiftoffRegister lhs,
                                            LiftoffRegister rhs);
  inline void emit_i8x16_abs(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i16x8_abs(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i32x4_abs(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i64x2_abs(LiftoffRegister dst, LiftoffRegister src);
  inline void emit_i8x16_extract_lane_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        uint8_t imm_lane_idx);
  inline void emit_i8x16_extract_lane_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        uint8_t imm_lane_idx);
  inline void emit_i16x8_extract_lane_s(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        uint8_t imm_lane_idx);
  inline void emit_i16x8_extract_lane_u(LiftoffRegister dst,
                                        LiftoffRegister lhs,
                                        uint8_t imm_lane_idx);
  inline void emit_i32x4_extract_lane(LiftoffRegister dst, LiftoffRegister lhs,
                                      uint8_t imm_lane_idx);
  inline void emit_i64x2_extract_lane(LiftoffRegister dst, LiftoffRegister lhs,
                                      uint8_t imm_lane_idx);
  inline void emit_f32x4_extract_lane(LiftoffRegister dst, LiftoffRegister lhs,
                                      uint8_t imm_lane_idx);
  inline void emit_f64x2_extract_lane(LiftoffRegister dst, LiftoffRegister lhs,
                                      uint8_t imm_lane_idx);
  inline void emit_i8x16_replace_lane(LiftoffRegister dst, LiftoffRegister src1,
                                      LiftoffRegister src2,
                                      uint8_t imm_lane_idx);
  inline void emit_i16x8_replace_lane(LiftoffRegister dst, LiftoffRegister src1,
                                      LiftoffRegister src2,
                                      uint8_t imm_lane_idx);
  inline void emit_i32x4_replace_lane(LiftoffRegister dst, LiftoffRegister src1,
                                      LiftoffRegister src2,
                                      uint8_t imm_lane_idx);
  inline void emit_i64x2_replace_lane(LiftoffRegister dst, LiftoffRegister src1,
                                      LiftoffRegister src2,
                                      uint8_t imm_lane_idx);
  inline void emit_f32x4_replace_lane(LiftoffRegister dst, LiftoffRegister src1,
                                      LiftoffRegister src2,
                                      uint8_t imm_lane_idx);
  inline void emit_f64x2_replace_lane(LiftoffRegister dst, LiftoffRegister src1,
                                      LiftoffRegister src2,
                                      uint8_t imm_lane_idx);

  inline void StackCheck(Label* ool_code, Register limit_address);

  inline void CallTrapCallbackForTesting();

  inline void AssertUnreachable(AbortReason reason);

  inline void PushRegisters(LiftoffRegList);
  inline void PopRegisters(LiftoffRegList);

  inline void RecordSpillsInSafepoint(Safepoint& safepoint,
                                      LiftoffRegList all_spills,
                                      LiftoffRegList ref_spills,
                                      int spill_offset);

  inline void DropStackSlotsAndRet(uint32_t num_stack_slots);

  // Execute a C call. Arguments are pushed to the stack and a pointer to this
  // region is passed to the C function. If {out_argument_kind != kVoid},
  // this is the return value of the C function, stored in {rets[0]}. Further
  // outputs (specified in {sig->returns()}) are read from the buffer and stored
  // in the remaining {rets} registers.
  inline void CallC(const ValueKindSig* sig, const LiftoffRegister* args,
                    const LiftoffRegister* rets, ValueKind out_argument_kind,
                    int stack_bytes, ExternalReference ext_ref);

  inline void CallNativeWasmCode(Address addr);
  inline void TailCallNativeWasmCode(Address addr);
  // Indirect call: If {target == no_reg}, then pop the target from the stack.
  inline void CallIndirect(const ValueKindSig* sig,
                           compiler::CallDescriptor* call_descriptor,
                           Register target);
  inline void TailCallIndirect(Register target);
  inline void CallRuntimeStub(WasmCode::RuntimeStubId sid);

  // Reserve space in the current frame, store address to space in {addr}.
  inline void AllocateStackSlot(Register addr, uint32_t size);
  inline void DeallocateStackSlot(uint32_t size);

  // Instrumentation for shadow-stack-compatible OSR on x64.
  inline void MaybeOSR();

  ////////////////////////////////////
  // End of platform-specific part. //
  ////////////////////////////////////

  uint32_t num_locals() const { return num_locals_; }
  void set_num_locals(uint32_t num_locals);

  int GetTotalFrameSlotCountForGC() const;

  int GetTotalFrameSize() const { return max_used_spill_offset_; }

  ValueKind local_kind(uint32_t index) {
    DCHECK_GT(num_locals_, index);
    ValueKind* locals =
        num_locals_ <= kInlineLocalKinds ? local_kinds_ : more_local_kinds_;
    return locals[index];
  }

  void set_local_kind(uint32_t index, ValueKind kind) {
    ValueKind* locals =
        num_locals_ <= kInlineLocalKinds ? local_kinds_ : more_local_kinds_;
    locals[index] = kind;
  }

  CacheState* cache_state() { return &cache_state_; }
  const CacheState* cache_state() const { return &cache_state_; }

  bool did_bailout() { return bailout_reason_ != kSuccess; }
  LiftoffBailoutReason bailout_reason() const { return bailout_reason_; }
  const char* bailout_detail() const { return bailout_detail_; }

  void bailout(LiftoffBailoutReason reason, const char* detail) {
    DCHECK_NE(kSuccess, reason);
    if (bailout_reason_ != kSuccess) return;
    AbortCompilation();
    bailout_reason_ = reason;
    bailout_detail_ = detail;
  }

 private:
  LiftoffRegister LoadI64HalfIntoRegister(VarState slot, RegPairHalf half);

  uint32_t num_locals_ = 0;
  static constexpr uint32_t kInlineLocalKinds = 16;
  union {
    ValueKind local_kinds_[kInlineLocalKinds];
    ValueKind* more_local_kinds_;
  };
  static_assert(sizeof(ValueKind) == 1,
                "Reconsider this inlining if ValueKind gets bigger");
  CacheState cache_state_;
  // The maximum spill offset for slots in the value stack.
  int max_used_spill_offset_ = StaticStackFrameSize();
  // The amount of memory needed for register spills in OOL code.
  int ool_spill_space_size_ = 0;
  LiftoffBailoutReason bailout_reason_ = kSuccess;
  const char* bailout_detail_ = nullptr;

  V8_NOINLINE LiftoffRegister SpillOneRegister(LiftoffRegList candidates);
  // Spill one or two fp registers to get a pair of adjacent fp registers.
  LiftoffRegister SpillAdjacentFpRegisters(LiftoffRegList pinned);
};

std::ostream& operator<<(std::ostream& os, LiftoffAssembler::VarState);

// =======================================================================
// Partially platform-independent implementations of the platform-dependent
// part.

#ifdef V8_TARGET_ARCH_32_BIT

namespace liftoff {
template <void (LiftoffAssembler::*op)(Register, Register, Register)>
void EmitI64IndependentHalfOperation(LiftoffAssembler* assm,
                                     LiftoffRegister dst, LiftoffRegister lhs,
                                     LiftoffRegister rhs) {
  // If {dst.low_gp()} does not overlap with {lhs.high_gp()} or {rhs.high_gp()},
  // just first compute the lower half, then the upper half.
  if (dst.low() != lhs.high() && dst.low() != rhs.high()) {
    (assm->*op)(dst.low_gp(), lhs.low_gp(), rhs.low_gp());
    (assm->*op)(dst.high_gp(), lhs.high_gp(), rhs.high_gp());
    return;
  }
  // If {dst.high_gp()} does not overlap with {lhs.low_gp()} or {rhs.low_gp()},
  // we can compute this the other way around.
  if (dst.high() != lhs.low() && dst.high() != rhs.low()) {
    (assm->*op)(dst.high_gp(), lhs.high_gp(), rhs.high_gp());
    (assm->*op)(dst.low_gp(), lhs.low_gp(), rhs.low_gp());
    return;
  }
  // Otherwise, we need a temporary register.
  Register tmp =
      assm->GetUnusedRegister(kGpReg, LiftoffRegList::ForRegs(lhs, rhs)).gp();
  (assm->*op)(tmp, lhs.low_gp(), rhs.low_gp());
  (assm->*op)(dst.high_gp(), lhs.high_gp(), rhs.high_gp());
  assm->Move(dst.low_gp(), tmp, kI32);
}

template <void (LiftoffAssembler::*op)(Register, Register, int32_t)>
void EmitI64IndependentHalfOperationImm(LiftoffAssembler* assm,
                                        LiftoffRegister dst,
                                        LiftoffRegister lhs, int64_t imm) {
  int32_t low_word = static_cast<int32_t>(imm);
  int32_t high_word = static_cast<int32_t>(imm >> 32);
  // If {dst.low_gp()} does not overlap with {lhs.high_gp()},
  // just first compute the lower half, then the upper half.
  if (dst.low() != lhs.high()) {
    (assm->*op)(dst.low_gp(), lhs.low_gp(), low_word);
    (assm->*op)(dst.high_gp(), lhs.high_gp(), high_word);
    return;
  }
  // If {dst.high_gp()} does not overlap with {lhs.low_gp()},
  // we can compute this the other way around.
  if (dst.high() != lhs.low()) {
    (assm->*op)(dst.high_gp(), lhs.high_gp(), high_word);
    (assm->*op)(dst.low_gp(), lhs.low_gp(), low_word);
    return;
  }
  // Otherwise, we need a temporary register.
  Register tmp =
      assm->GetUnusedRegister(kGpReg, LiftoffRegList::ForRegs(lhs)).gp();
  (assm->*op)(tmp, lhs.low_gp(), low_word);
  (assm->*op)(dst.high_gp(), lhs.high_gp(), high_word);
  assm->Move(dst.low_gp(), tmp, kI32);
}
}  // namespace liftoff

void LiftoffAssembler::emit_i64_and(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::EmitI64IndependentHalfOperation<&LiftoffAssembler::emit_i32_and>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_andi(LiftoffRegister dst, LiftoffRegister lhs,
                                     int32_t imm) {
  liftoff::EmitI64IndependentHalfOperationImm<&LiftoffAssembler::emit_i32_andi>(
      this, dst, lhs, imm);
}

void LiftoffAssembler::emit_i64_or(LiftoffRegister dst, LiftoffRegister lhs,
                                   LiftoffRegister rhs) {
  liftoff::EmitI64IndependentHalfOperation<&LiftoffAssembler::emit_i32_or>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_ori(LiftoffRegister dst, LiftoffRegister lhs,
                                    int32_t imm) {
  liftoff::EmitI64IndependentHalfOperationImm<&LiftoffAssembler::emit_i32_ori>(
      this, dst, lhs, imm);
}

void LiftoffAssembler::emit_i64_xor(LiftoffRegister dst, LiftoffRegister lhs,
                                    LiftoffRegister rhs) {
  liftoff::EmitI64IndependentHalfOperation<&LiftoffAssembler::emit_i32_xor>(
      this, dst, lhs, rhs);
}

void LiftoffAssembler::emit_i64_xori(LiftoffRegister dst, LiftoffRegister lhs,
                                     int32_t imm) {
  liftoff::EmitI64IndependentHalfOperationImm<&LiftoffAssembler::emit_i32_xori>(
      this, dst, lhs, imm);
}

#endif  // V8_TARGET_ARCH_32_BIT

// End of the partially platform-independent implementations of the
// platform-dependent part.
// =======================================================================

class LiftoffStackSlots {
 public:
  explicit LiftoffStackSlots(LiftoffAssembler* wasm_asm) : asm_(wasm_asm) {}
  LiftoffStackSlots(const LiftoffStackSlots&) = delete;
  LiftoffStackSlots& operator=(const LiftoffStackSlots&) = delete;

  void Add(const LiftoffAssembler::VarState& src, uint32_t src_offset,
           RegPairHalf half, int dst_slot) {
    DCHECK_LE(0, dst_slot);
    slots_.emplace_back(src, src_offset, half, dst_slot);
  }

  void Add(const LiftoffAssembler::VarState& src, int dst_slot) {
    DCHECK_LE(0, dst_slot);
    slots_.emplace_back(src, dst_slot);
  }

  void SortInPushOrder() {
    std::sort(slots_.begin(), slots_.end(), [](const Slot& a, const Slot& b) {
      return a.dst_slot_ > b.dst_slot_;
    });
  }

  inline void Construct(int param_slots);

 private:
  // A logical slot, which may occupy multiple stack slots.
  struct Slot {
    Slot(const LiftoffAssembler::VarState& src, uint32_t src_offset,
         RegPairHalf half, int dst_slot)
        : src_(src),
          src_offset_(src_offset),
          half_(half),
          dst_slot_(dst_slot) {}
    Slot(const LiftoffAssembler::VarState& src, int dst_slot)
        : src_(src), half_(kLowWord), dst_slot_(dst_slot) {}

    LiftoffAssembler::VarState src_;
    uint32_t src_offset_ = 0;
    RegPairHalf half_;
    int dst_slot_ = 0;
  };

  // Returns the size in bytes of the given logical slot.
  static int SlotSizeInBytes(const Slot& slot) {
    const ValueKind kind = slot.src_.kind();
    if (kind == kS128) return kSimd128Size;
    if (kind == kF64) return kDoubleSize;
    return kSystemPointerSize;
  }

  base::SmallVector<Slot, 8> slots_;
  LiftoffAssembler* const asm_;
};

#if DEBUG
bool CheckCompatibleStackSlotTypes(ValueKind a, ValueKind b);
#endif

}  // namespace wasm
}  // namespace internal
}  // namespace v8

// Include platform specific implementation.
#if V8_TARGET_ARCH_IA32
#include "src/wasm/baseline/ia32/liftoff-assembler-ia32.h"
#elif V8_TARGET_ARCH_X64
#include "src/wasm/baseline/x64/liftoff-assembler-x64.h"
#elif V8_TARGET_ARCH_ARM64
#include "src/wasm/baseline/arm64/liftoff-assembler-arm64.h"
#elif V8_TARGET_ARCH_ARM
#include "src/wasm/baseline/arm/liftoff-assembler-arm.h"
#elif V8_TARGET_ARCH_PPC || V8_TARGET_ARCH_PPC64
#include "src/wasm/baseline/ppc/liftoff-assembler-ppc.h"
#elif V8_TARGET_ARCH_MIPS
#include "src/wasm/baseline/mips/liftoff-assembler-mips.h"
#elif V8_TARGET_ARCH_MIPS64
#include "src/wasm/baseline/mips64/liftoff-assembler-mips64.h"
#elif V8_TARGET_ARCH_S390
#include "src/wasm/baseline/s390/liftoff-assembler-s390.h"
#elif V8_TARGET_ARCH_RISCV64
#include "src/wasm/baseline/riscv64/liftoff-assembler-riscv64.h"
#else
#error Unsupported architecture.
#endif

#endif  // V8_WASM_BASELINE_LIFTOFF_ASSEMBLER_H_
