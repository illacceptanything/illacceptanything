// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_DEOPTIMIZER_FRAME_DESCRIPTION_H_
#define V8_DEOPTIMIZER_FRAME_DESCRIPTION_H_

#include "src/codegen/register-arch.h"
#include "src/execution/frame-constants.h"
#include "src/utils/boxed-float.h"

namespace v8 {
namespace internal {

// Classes in this file describe the physical stack frame state.
//
// RegisterValues: stores gp and fp register values. Can be filled in either by
// the DeoptimizationEntry builtin (which fills in the input state of the
// optimized frame); or by the FrameWriter (fills in the output state of the
// interpreted frame).
//
// - FrameDescription: contains RegisterValues and other things.

class RegisterValues {
 public:
  intptr_t GetRegister(unsigned n) const {
#if DEBUG
    // This convoluted DCHECK is needed to work around a gcc problem that
    // improperly detects an array bounds overflow in optimized debug builds
    // when using a plain DCHECK.
    if (n >= arraysize(registers_)) {
      DCHECK(false);
      return 0;
    }
#endif
    return registers_[n];
  }

  Float32 GetFloatRegister(unsigned n) const;

  Float64 GetDoubleRegister(unsigned n) const {
    DCHECK(n < arraysize(double_registers_));
    return double_registers_[n];
  }

  void SetRegister(unsigned n, intptr_t value) {
    DCHECK(n < arraysize(registers_));
    registers_[n] = value;
  }

  intptr_t registers_[Register::kNumRegisters];
  // Generated code writes directly into the following array, make sure the
  // element size matches what the machine instructions expect.
  static_assert(sizeof(Float64) == kDoubleSize, "size mismatch");
  Float64 double_registers_[DoubleRegister::kNumRegisters];
};

class FrameDescription {
 public:
  FrameDescription(uint32_t frame_size, int parameter_count)
      : frame_size_(frame_size),
        parameter_count_(parameter_count),
        top_(kZapUint32),
        pc_(kZapUint32),
        fp_(kZapUint32),
        context_(kZapUint32),
        constant_pool_(kZapUint32) {
    // Zap all the registers.
    for (int r = 0; r < Register::kNumRegisters; r++) {
      // TODO(jbramley): It isn't safe to use kZapUint32 here. If the register
      // isn't used before the next safepoint, the GC will try to scan it as a
      // tagged value. kZapUint32 looks like a valid tagged pointer, but it
      // isn't.
#if defined(V8_OS_WIN) && defined(V8_TARGET_ARCH_ARM64)
      // x18 is reserved as platform register on Windows arm64 platform
      const int kPlatformRegister = 18;
      if (r != kPlatformRegister) {
        SetRegister(r, kZapUint32);
      }
#else
      SetRegister(r, kZapUint32);
#endif
    }

    // Zap all the slots.
    for (unsigned o = 0; o < frame_size; o += kSystemPointerSize) {
      SetFrameSlot(o, kZapUint32);
    }
  }

  void* operator new(size_t size, uint32_t frame_size) {
    // Subtracts kSystemPointerSize, as the member frame_content_ already
    // supplies the first element of the area to store the frame.
    return base::Malloc(size + frame_size - kSystemPointerSize);
  }

  void operator delete(void* pointer, uint32_t frame_size) {
    base::Free(pointer);
  }

  void operator delete(void* description) { base::Free(description); }

  uint32_t GetFrameSize() const {
    USE(frame_content_);
    DCHECK(static_cast<uint32_t>(frame_size_) == frame_size_);
    return static_cast<uint32_t>(frame_size_);
  }

  intptr_t GetFrameSlot(unsigned offset) {
    return *GetFrameSlotPointer(offset);
  }

  unsigned GetLastArgumentSlotOffset(bool pad_arguments = true) {
    int parameter_slots = parameter_count();
    if (pad_arguments) {
      parameter_slots = AddArgumentPaddingSlots(parameter_slots);
    }
    return GetFrameSize() - parameter_slots * kSystemPointerSize;
  }

  Address GetFramePointerAddress() {
    // We should not pad arguments in the bottom frame, since this
    // already contain a padding if necessary and it might contain
    // extra arguments (actual argument count > parameter count).
    const bool pad_arguments_bottom_frame = false;
    int fp_offset = GetLastArgumentSlotOffset(pad_arguments_bottom_frame) -
                    StandardFrameConstants::kCallerSPOffset;
    return reinterpret_cast<Address>(GetFrameSlotPointer(fp_offset));
  }

  RegisterValues* GetRegisterValues() { return &register_values_; }

  void SetFrameSlot(unsigned offset, intptr_t value) {
    *GetFrameSlotPointer(offset) = value;
  }

  void SetCallerPc(unsigned offset, intptr_t value);

  void SetCallerFp(unsigned offset, intptr_t value);

  void SetCallerConstantPool(unsigned offset, intptr_t value);

  intptr_t GetRegister(unsigned n) const {
    return register_values_.GetRegister(n);
  }

  Float64 GetDoubleRegister(unsigned n) const {
    return register_values_.GetDoubleRegister(n);
  }

  void SetRegister(unsigned n, intptr_t value) {
    register_values_.SetRegister(n, value);
  }

  intptr_t GetTop() const { return top_; }
  void SetTop(intptr_t top) { top_ = top; }

  intptr_t GetPc() const { return pc_; }
  void SetPc(intptr_t pc);

  intptr_t GetFp() const { return fp_; }
  void SetFp(intptr_t fp) { fp_ = fp; }

  intptr_t GetContext() const { return context_; }
  void SetContext(intptr_t context) { context_ = context; }

  intptr_t GetConstantPool() const { return constant_pool_; }
  void SetConstantPool(intptr_t constant_pool) {
    constant_pool_ = constant_pool;
  }

  void SetContinuation(intptr_t pc) { continuation_ = pc; }

  // Argument count, including receiver.
  int parameter_count() { return parameter_count_; }

  static int registers_offset() {
    return offsetof(FrameDescription, register_values_.registers_);
  }

  static constexpr int double_registers_offset() {
    return offsetof(FrameDescription, register_values_.double_registers_);
  }

  static int frame_size_offset() {
    return offsetof(FrameDescription, frame_size_);
  }

  static int pc_offset() { return offsetof(FrameDescription, pc_); }

  static int continuation_offset() {
    return offsetof(FrameDescription, continuation_);
  }

  static int frame_content_offset() {
    return offsetof(FrameDescription, frame_content_);
  }

 private:
  static const uint32_t kZapUint32 = 0xbeeddead;

  // Frame_size_ must hold a uint32_t value.  It is only a uintptr_t to
  // keep the variable-size array frame_content_ of type intptr_t at
  // the end of the structure aligned.
  uintptr_t frame_size_;  // Number of bytes.
  int parameter_count_;
  RegisterValues register_values_;
  intptr_t top_;
  intptr_t pc_;
  intptr_t fp_;
  intptr_t context_;
  intptr_t constant_pool_;

  // Continuation is the PC where the execution continues after
  // deoptimizing.
  intptr_t continuation_;

  // This must be at the end of the object as the object is allocated larger
  // than its definition indicates to extend this array.
  intptr_t frame_content_[1];

  intptr_t* GetFrameSlotPointer(unsigned offset) {
    DCHECK(offset < frame_size_);
    return reinterpret_cast<intptr_t*>(reinterpret_cast<Address>(this) +
                                       frame_content_offset() + offset);
  }
};

}  // namespace internal
}  // namespace v8

#endif  // V8_DEOPTIMIZER_FRAME_DESCRIPTION_H_
