// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/interpreter/bytecode-array-iterator.h"

#include "src/interpreter/bytecode-decoder.h"
#include "src/interpreter/interpreter-intrinsics.h"
#include "src/objects/code-inl.h"
#include "src/objects/feedback-vector.h"
#include "src/objects/objects-inl.h"

namespace v8 {
namespace internal {
namespace interpreter {

BytecodeArrayIterator::BytecodeArrayIterator(
    Handle<BytecodeArray> bytecode_array, int initial_offset)
    : bytecode_array_(bytecode_array),
      start_(reinterpret_cast<uint8_t*>(
          bytecode_array_->GetFirstBytecodeAddress())),
      end_(start_ + bytecode_array_->length()),
      cursor_(start_ + initial_offset),
      operand_scale_(OperandScale::kSingle),
      prefix_size_(0),
      local_heap_(LocalHeap::Current()
                      ? LocalHeap::Current()
                      : Isolate::Current()->main_thread_local_heap()) {
  local_heap_->AddGCEpilogueCallback(UpdatePointersCallback, this);
  UpdateOperandScale();
}

BytecodeArrayIterator::~BytecodeArrayIterator() {
  local_heap_->RemoveGCEpilogueCallback(UpdatePointersCallback, this);
}

void BytecodeArrayIterator::SetOffset(int offset) {
  if (offset < 0) return;
  cursor_ = reinterpret_cast<uint8_t*>(
      bytecode_array()->GetFirstBytecodeAddress() + offset);
  UpdateOperandScale();
}

void BytecodeArrayIterator::ApplyDebugBreak() {
  // Get the raw bytecode from the bytecode array. This may give us a
  // scaling prefix, which we can patch with the matching debug-break
  // variant.
  uint8_t* cursor = cursor_ - prefix_size_;
  interpreter::Bytecode bytecode = interpreter::Bytecodes::FromByte(*cursor);
  if (interpreter::Bytecodes::IsDebugBreak(bytecode)) return;
  interpreter::Bytecode debugbreak =
      interpreter::Bytecodes::GetDebugBreak(bytecode);
  *cursor = interpreter::Bytecodes::ToByte(debugbreak);
}

int BytecodeArrayIterator::current_bytecode_size() const {
  return prefix_size_ + current_bytecode_size_without_prefix();
}

int BytecodeArrayIterator::current_bytecode_size_without_prefix() const {
  return Bytecodes::Size(current_bytecode(), current_operand_scale());
}

uint32_t BytecodeArrayIterator::GetUnsignedOperand(
    int operand_index, OperandType operand_type) const {
  DCHECK_GE(operand_index, 0);
  DCHECK_LT(operand_index, Bytecodes::NumberOfOperands(current_bytecode()));
  DCHECK_EQ(operand_type,
            Bytecodes::GetOperandType(current_bytecode(), operand_index));
  DCHECK(Bytecodes::IsUnsignedOperandType(operand_type));
  Address operand_start =
      reinterpret_cast<Address>(cursor_) +
      Bytecodes::GetOperandOffset(current_bytecode(), operand_index,
                                  current_operand_scale());
  return BytecodeDecoder::DecodeUnsignedOperand(operand_start, operand_type,
                                                current_operand_scale());
}

int32_t BytecodeArrayIterator::GetSignedOperand(
    int operand_index, OperandType operand_type) const {
  DCHECK_GE(operand_index, 0);
  DCHECK_LT(operand_index, Bytecodes::NumberOfOperands(current_bytecode()));
  DCHECK_EQ(operand_type,
            Bytecodes::GetOperandType(current_bytecode(), operand_index));
  DCHECK(!Bytecodes::IsUnsignedOperandType(operand_type));
  Address operand_start =
      reinterpret_cast<Address>(cursor_) +
      Bytecodes::GetOperandOffset(current_bytecode(), operand_index,
                                  current_operand_scale());
  return BytecodeDecoder::DecodeSignedOperand(operand_start, operand_type,
                                              current_operand_scale());
}

uint32_t BytecodeArrayIterator::GetFlagOperand(int operand_index) const {
  DCHECK_EQ(Bytecodes::GetOperandType(current_bytecode(), operand_index),
            OperandType::kFlag8);
  return GetUnsignedOperand(operand_index, OperandType::kFlag8);
}

uint32_t BytecodeArrayIterator::GetUnsignedImmediateOperand(
    int operand_index) const {
  DCHECK_EQ(Bytecodes::GetOperandType(current_bytecode(), operand_index),
            OperandType::kUImm);
  return GetUnsignedOperand(operand_index, OperandType::kUImm);
}

int32_t BytecodeArrayIterator::GetImmediateOperand(int operand_index) const {
  DCHECK_EQ(Bytecodes::GetOperandType(current_bytecode(), operand_index),
            OperandType::kImm);
  return GetSignedOperand(operand_index, OperandType::kImm);
}

uint32_t BytecodeArrayIterator::GetRegisterCountOperand(
    int operand_index) const {
  DCHECK_EQ(Bytecodes::GetOperandType(current_bytecode(), operand_index),
            OperandType::kRegCount);
  return GetUnsignedOperand(operand_index, OperandType::kRegCount);
}

uint32_t BytecodeArrayIterator::GetIndexOperand(int operand_index) const {
  OperandType operand_type =
      Bytecodes::GetOperandType(current_bytecode(), operand_index);
  DCHECK_EQ(operand_type, OperandType::kIdx);
  return GetUnsignedOperand(operand_index, operand_type);
}

FeedbackSlot BytecodeArrayIterator::GetSlotOperand(int operand_index) const {
  int index = GetIndexOperand(operand_index);
  return FeedbackVector::ToSlot(index);
}

Register BytecodeArrayIterator::GetReceiver() const {
  return Register::FromParameterIndex(0, bytecode_array()->parameter_count());
}

Register BytecodeArrayIterator::GetParameter(int parameter_index) const {
  DCHECK_GE(parameter_index, 0);
  // The parameter indices are shifted by 1 (receiver is the
  // first entry).
  return Register::FromParameterIndex(parameter_index + 1,
                                      bytecode_array()->parameter_count());
}

Register BytecodeArrayIterator::GetRegisterOperand(int operand_index) const {
  OperandType operand_type =
      Bytecodes::GetOperandType(current_bytecode(), operand_index);
  Address operand_start =
      reinterpret_cast<Address>(cursor_) +
      Bytecodes::GetOperandOffset(current_bytecode(), operand_index,
                                  current_operand_scale());
  return BytecodeDecoder::DecodeRegisterOperand(operand_start, operand_type,
                                                current_operand_scale());
}

std::pair<Register, Register> BytecodeArrayIterator::GetRegisterPairOperand(
    int operand_index) const {
  Register first = GetRegisterOperand(operand_index);
  Register second(first.index() + 1);
  return std::make_pair(first, second);
}

RegisterList BytecodeArrayIterator::GetRegisterListOperand(
    int operand_index) const {
  Register first = GetRegisterOperand(operand_index);
  uint32_t count = GetRegisterCountOperand(operand_index + 1);
  return RegisterList(first.index(), count);
}

int BytecodeArrayIterator::GetRegisterOperandRange(int operand_index) const {
  DCHECK_LE(operand_index, Bytecodes::NumberOfOperands(current_bytecode()));
  const OperandType* operand_types =
      Bytecodes::GetOperandTypes(current_bytecode());
  OperandType operand_type = operand_types[operand_index];
  DCHECK(Bytecodes::IsRegisterOperandType(operand_type));
  if (operand_type == OperandType::kRegList ||
      operand_type == OperandType::kRegOutList) {
    return GetRegisterCountOperand(operand_index + 1);
  } else {
    return Bytecodes::GetNumberOfRegistersRepresentedBy(operand_type);
  }
}

Runtime::FunctionId BytecodeArrayIterator::GetRuntimeIdOperand(
    int operand_index) const {
  OperandType operand_type =
      Bytecodes::GetOperandType(current_bytecode(), operand_index);
  DCHECK_EQ(operand_type, OperandType::kRuntimeId);
  uint32_t raw_id = GetUnsignedOperand(operand_index, operand_type);
  return static_cast<Runtime::FunctionId>(raw_id);
}

uint32_t BytecodeArrayIterator::GetNativeContextIndexOperand(
    int operand_index) const {
  OperandType operand_type =
      Bytecodes::GetOperandType(current_bytecode(), operand_index);
  DCHECK_EQ(operand_type, OperandType::kNativeContextIndex);
  return GetUnsignedOperand(operand_index, operand_type);
}

Runtime::FunctionId BytecodeArrayIterator::GetIntrinsicIdOperand(
    int operand_index) const {
  OperandType operand_type =
      Bytecodes::GetOperandType(current_bytecode(), operand_index);
  DCHECK_EQ(operand_type, OperandType::kIntrinsicId);
  uint32_t raw_id = GetUnsignedOperand(operand_index, operand_type);
  return IntrinsicsHelper::ToRuntimeId(
      static_cast<IntrinsicsHelper::IntrinsicId>(raw_id));
}

template <typename IsolateT>
Handle<Object> BytecodeArrayIterator::GetConstantAtIndex(
    int index, IsolateT* isolate) const {
  return handle(bytecode_array()->constant_pool().get(index), isolate);
}

bool BytecodeArrayIterator::IsConstantAtIndexSmi(int index) const {
  return bytecode_array()->constant_pool().get(index).IsSmi();
}

Smi BytecodeArrayIterator::GetConstantAtIndexAsSmi(int index) const {
  return Smi::cast(bytecode_array()->constant_pool().get(index));
}

template <typename IsolateT>
Handle<Object> BytecodeArrayIterator::GetConstantForIndexOperand(
    int operand_index, IsolateT* isolate) const {
  return GetConstantAtIndex(GetIndexOperand(operand_index), isolate);
}

template EXPORT_TEMPLATE_DEFINE(V8_EXPORT_PRIVATE)
    Handle<Object> BytecodeArrayIterator::GetConstantForIndexOperand(
        int operand_index, Isolate* isolate) const;
template Handle<Object> BytecodeArrayIterator::GetConstantForIndexOperand(
    int operand_index, LocalIsolate* isolate) const;

int BytecodeArrayIterator::GetRelativeJumpTargetOffset() const {
  Bytecode bytecode = current_bytecode();
  if (interpreter::Bytecodes::IsJumpImmediate(bytecode)) {
    int relative_offset = GetUnsignedImmediateOperand(0);
    if (bytecode == Bytecode::kJumpLoop) {
      relative_offset = -relative_offset;
    }
    return relative_offset;
  } else if (interpreter::Bytecodes::IsJumpConstant(bytecode)) {
    Smi smi = GetConstantAtIndexAsSmi(GetIndexOperand(0));
    return smi.value();
  } else {
    UNREACHABLE();
  }
}

int BytecodeArrayIterator::GetJumpTargetOffset() const {
  return GetAbsoluteOffset(GetRelativeJumpTargetOffset());
}

JumpTableTargetOffsets BytecodeArrayIterator::GetJumpTableTargetOffsets()
    const {
  uint32_t table_start, table_size;
  int32_t case_value_base;
  if (current_bytecode() == Bytecode::kSwitchOnGeneratorState) {
    table_start = GetIndexOperand(1);
    table_size = GetUnsignedImmediateOperand(2);
    case_value_base = 0;
  } else {
    DCHECK_EQ(current_bytecode(), Bytecode::kSwitchOnSmiNoFeedback);
    table_start = GetIndexOperand(0);
    table_size = GetUnsignedImmediateOperand(1);
    case_value_base = GetImmediateOperand(2);
  }
  return JumpTableTargetOffsets(this, table_start, table_size, case_value_base);
}

int BytecodeArrayIterator::GetAbsoluteOffset(int relative_offset) const {
  return current_offset() + relative_offset + prefix_size_;
}

std::ostream& BytecodeArrayIterator::PrintTo(std::ostream& os) const {
  return BytecodeDecoder::Decode(os, cursor_ - prefix_size_,
                                 bytecode_array()->parameter_count());
}

void BytecodeArrayIterator::UpdatePointers() {
  DisallowGarbageCollection no_gc;
  uint8_t* start =
      reinterpret_cast<uint8_t*>(bytecode_array_->GetFirstBytecodeAddress());
  if (start != start_) {
    start_ = start;
    uint8_t* end = start + bytecode_array_->length();
    size_t distance_to_end = end_ - cursor_;
    cursor_ = end - distance_to_end;
    end_ = end;
  }
}

JumpTableTargetOffsets::JumpTableTargetOffsets(
    const BytecodeArrayIterator* iterator, int table_start, int table_size,
    int case_value_base)
    : iterator_(iterator),
      table_start_(table_start),
      table_size_(table_size),
      case_value_base_(case_value_base) {}

JumpTableTargetOffsets::iterator JumpTableTargetOffsets::begin() const {
  return iterator(case_value_base_, table_start_, table_start_ + table_size_,
                  iterator_);
}
JumpTableTargetOffsets::iterator JumpTableTargetOffsets::end() const {
  return iterator(case_value_base_ + table_size_, table_start_ + table_size_,
                  table_start_ + table_size_, iterator_);
}
int JumpTableTargetOffsets::size() const {
  int ret = 0;
  // TODO(leszeks): Is there a more efficient way of doing this than iterating?
  for (const auto& entry : *this) {
    USE(entry);
    ret++;
  }
  return ret;
}

JumpTableTargetOffsets::iterator::iterator(
    int case_value, int table_offset, int table_end,
    const BytecodeArrayIterator* iterator)
    : iterator_(iterator),
      current_(Smi::zero()),
      index_(case_value),
      table_offset_(table_offset),
      table_end_(table_end) {
  UpdateAndAdvanceToValid();
}

JumpTableTargetOffset JumpTableTargetOffsets::iterator::operator*() {
  DCHECK_LT(table_offset_, table_end_);
  return {index_, iterator_->GetAbsoluteOffset(Smi::ToInt(current_))};
}

JumpTableTargetOffsets::iterator&
JumpTableTargetOffsets::iterator::operator++() {
  DCHECK_LT(table_offset_, table_end_);
  ++table_offset_;
  ++index_;
  UpdateAndAdvanceToValid();
  return *this;
}

bool JumpTableTargetOffsets::iterator::operator!=(
    const JumpTableTargetOffsets::iterator& other) {
  DCHECK_EQ(iterator_, other.iterator_);
  DCHECK_EQ(table_end_, other.table_end_);
  DCHECK_EQ(index_ - other.index_, table_offset_ - other.table_offset_);
  return index_ != other.index_;
}

void JumpTableTargetOffsets::iterator::UpdateAndAdvanceToValid() {
  while (table_offset_ < table_end_ &&
         !iterator_->IsConstantAtIndexSmi(table_offset_)) {
    ++table_offset_;
    ++index_;
  }

  // Make sure we haven't reached the end of the table with a hole in current.
  if (table_offset_ < table_end_) {
    DCHECK(iterator_->IsConstantAtIndexSmi(table_offset_));
    current_ = iterator_->GetConstantAtIndexAsSmi(table_offset_);
  }
}

}  // namespace interpreter
}  // namespace internal
}  // namespace v8
