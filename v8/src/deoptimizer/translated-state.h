// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_DEOPTIMIZER_TRANSLATED_STATE_H_
#define V8_DEOPTIMIZER_TRANSLATED_STATE_H_

#include <stack>
#include <vector>

#include "src/deoptimizer/translation-array.h"
#include "src/objects/feedback-vector.h"
#include "src/objects/heap-object.h"
#include "src/objects/shared-function-info.h"
#include "src/utils/boxed-float.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/value-type.h"
#endif  // V8_ENABLE_WEBASSEMBLY

namespace v8 {
namespace internal {

class RegisterValues;
class TranslatedState;

// TODO(jgruber): This duplicates decoding logic already present in
// TranslatedState/TranslatedFrame. Deduplicate into one class, e.g. by basing
// printing off TranslatedFrame.
void TranslationArrayPrintSingleFrame(std::ostream& os,
                                      TranslationArray translation_array,
                                      int translation_index,
                                      FixedArray literal_array);

// The Translated{Value,Frame,State} class hierarchy are a set of utility
// functions to work with the combination of translations (built from a
// TranslationArray) and the actual current CPU state (represented by
// RegisterValues).
//
// TranslatedState: describes the entire stack state of the current optimized
// frame, contains:
//
// TranslatedFrame: describes a single unoptimized frame, contains:
//
// TranslatedValue: the actual value of some location.

class TranslatedValue {
 public:
  // Allocation-free getter of the value.
  // Returns ReadOnlyRoots::arguments_marker() if allocation would be necessary
  // to get the value. In the case of numbers, returns a Smi if possible.
  Object GetRawValue() const;

  // Convenience wrapper around GetRawValue (checked).
  int GetSmiValue() const;

  // Returns the value, possibly materializing it first (and the whole subgraph
  // reachable from this value). In the case of numbers, returns a Smi if
  // possible.
  Handle<Object> GetValue();

  bool IsMaterializedObject() const;
  bool IsMaterializableByDebugger() const;

 private:
  friend class TranslatedState;
  friend class TranslatedFrame;
  friend class Deoptimizer;

  enum Kind : uint8_t {
    kInvalid,
    kTagged,
    kInt32,
    kInt64,
    kInt64ToBigInt,
    kUInt32,
    kBoolBit,
    kFloat,
    kDouble,
    kCapturedObject,   // Object captured by the escape analysis.
                       // The number of nested objects can be obtained
                       // with the DeferredObjectLength() method
                       // (the values of the nested objects follow
                       // this value in the depth-first order.)
    kDuplicatedObject  // Duplicated object of a deferred object.
  };

  enum MaterializationState : uint8_t {
    kUninitialized,
    kAllocated,  // Storage for the object has been allocated (or
                 // enqueued for allocation).
    kFinished,   // The object has been initialized (or enqueued for
                 // initialization).
  };

  TranslatedValue(TranslatedState* container, Kind kind)
      : kind_(kind), container_(container) {}
  Kind kind() const { return kind_; }
  MaterializationState materialization_state() const {
    return materialization_state_;
  }
  void Handlify();
  int GetChildrenCount() const;

  static TranslatedValue NewDeferredObject(TranslatedState* container,
                                           int length, int object_index);
  static TranslatedValue NewDuplicateObject(TranslatedState* container, int id);
  static TranslatedValue NewFloat(TranslatedState* container, Float32 value);
  static TranslatedValue NewDouble(TranslatedState* container, Float64 value);
  static TranslatedValue NewInt32(TranslatedState* container, int32_t value);
  static TranslatedValue NewInt64(TranslatedState* container, int64_t value);
  static TranslatedValue NewInt64ToBigInt(TranslatedState* container,
                                          int64_t value);
  static TranslatedValue NewUInt32(TranslatedState* container, uint32_t value);
  static TranslatedValue NewBool(TranslatedState* container, uint32_t value);
  static TranslatedValue NewTagged(TranslatedState* container, Object literal);
  static TranslatedValue NewInvalid(TranslatedState* container);

  Isolate* isolate() const;

  void set_storage(Handle<HeapObject> storage) { storage_ = storage; }
  void set_initialized_storage(Handle<HeapObject> storage);
  void mark_finished() { materialization_state_ = kFinished; }
  void mark_allocated() { materialization_state_ = kAllocated; }

  Handle<HeapObject> storage() {
    DCHECK_NE(materialization_state(), kUninitialized);
    return storage_;
  }

  void ReplaceElementsArrayWithCopy();

  Kind kind_;
  MaterializationState materialization_state_ = kUninitialized;
  TranslatedState* container_;  // This is only needed for materialization of
                                // objects and constructing handles (to get
                                // to the isolate).

  Handle<HeapObject> storage_;  // Contains the materialized value or the
                                // byte-array that will be later morphed into
                                // the materialized object.

  struct MaterializedObjectInfo {
    int id_;
    int length_;  // Applies only to kCapturedObject kinds.
  };

  union {
    // kind kTagged. After handlification it is always nullptr.
    Object raw_literal_;
    // kind is kUInt32 or kBoolBit.
    uint32_t uint32_value_;
    // kind is kInt32.
    int32_t int32_value_;
    // kind is kInt64.
    int64_t int64_value_;
    // kind is kFloat
    Float32 float_value_;
    // kind is kDouble
    Float64 double_value_;
    // kind is kDuplicatedObject or kCapturedObject.
    MaterializedObjectInfo materialization_info_;
  };

  // Checked accessors for the union members.
  Object raw_literal() const;
  int32_t int32_value() const;
  int64_t int64_value() const;
  uint32_t uint32_value() const;
  Float32 float_value() const;
  Float64 double_value() const;
  int object_length() const;
  int object_index() const;
};

class TranslatedFrame {
 public:
  enum Kind {
    kUnoptimizedFunction,
    kArgumentsAdaptor,
    kConstructStub,
    kBuiltinContinuation,
#if V8_ENABLE_WEBASSEMBLY
    kJSToWasmBuiltinContinuation,
#endif  // V8_ENABLE_WEBASSEMBLY
    kJavaScriptBuiltinContinuation,
    kJavaScriptBuiltinContinuationWithCatch,
    kInvalid
  };

  int GetValueCount();

  Kind kind() const { return kind_; }
  BytecodeOffset bytecode_offset() const { return bytecode_offset_; }
  Handle<SharedFunctionInfo> shared_info() const { return shared_info_; }

  // TODO(jgruber): Simplify/clarify the semantics of this field. The name
  // `height` is slightly misleading. Yes, this value is related to stack frame
  // height, but must undergo additional mutations to arrive at the real stack
  // frame height (e.g.: addition/subtraction of context, accumulator, fixed
  // frame sizes, padding).
  int height() const { return height_; }

  int return_value_offset() const { return return_value_offset_; }
  int return_value_count() const { return return_value_count_; }

  SharedFunctionInfo raw_shared_info() const {
    CHECK(!raw_shared_info_.is_null());
    return raw_shared_info_;
  }

  class iterator {
   public:
    iterator& operator++() {
      ++input_index_;
      AdvanceIterator(&position_);
      return *this;
    }

    iterator operator++(int) {
      iterator original(position_, input_index_);
      ++input_index_;
      AdvanceIterator(&position_);
      return original;
    }

    bool operator==(const iterator& other) const {
      // Ignore {input_index_} for equality.
      return position_ == other.position_;
    }
    bool operator!=(const iterator& other) const { return !(*this == other); }

    TranslatedValue& operator*() { return (*position_); }
    TranslatedValue* operator->() { return &(*position_); }
    const TranslatedValue& operator*() const { return (*position_); }
    const TranslatedValue* operator->() const { return &(*position_); }

    int input_index() const { return input_index_; }

   private:
    friend TranslatedFrame;

    explicit iterator(std::deque<TranslatedValue>::iterator position,
                      int input_index = 0)
        : position_(position), input_index_(input_index) {}

    std::deque<TranslatedValue>::iterator position_;
    int input_index_;
  };

  using reference = TranslatedValue&;
  using const_reference = TranslatedValue const&;

  iterator begin() { return iterator(values_.begin()); }
  iterator end() { return iterator(values_.end()); }

  reference front() { return values_.front(); }
  const_reference front() const { return values_.front(); }

#if V8_ENABLE_WEBASSEMBLY
  // Only for Kind == kJSToWasmBuiltinContinuation
  base::Optional<wasm::ValueKind> wasm_call_return_kind() const {
    DCHECK_EQ(kind(), kJSToWasmBuiltinContinuation);
    return return_kind_;
  }
#endif  // V8_ENABLE_WEBASSEMBLY

 private:
  friend class TranslatedState;
  friend class Deoptimizer;

  // Constructor static methods.
  static TranslatedFrame UnoptimizedFrame(BytecodeOffset bytecode_offset,
                                          SharedFunctionInfo shared_info,
                                          int height, int return_value_offset,
                                          int return_value_count);
  static TranslatedFrame AccessorFrame(Kind kind,
                                       SharedFunctionInfo shared_info);
  static TranslatedFrame ArgumentsAdaptorFrame(SharedFunctionInfo shared_info,
                                               int height);
  static TranslatedFrame ConstructStubFrame(BytecodeOffset bailout_id,
                                            SharedFunctionInfo shared_info,
                                            int height);
  static TranslatedFrame BuiltinContinuationFrame(
      BytecodeOffset bailout_id, SharedFunctionInfo shared_info, int height);
#if V8_ENABLE_WEBASSEMBLY
  static TranslatedFrame JSToWasmBuiltinContinuationFrame(
      BytecodeOffset bailout_id, SharedFunctionInfo shared_info, int height,
      base::Optional<wasm::ValueKind> return_type);
#endif  // V8_ENABLE_WEBASSEMBLY
  static TranslatedFrame JavaScriptBuiltinContinuationFrame(
      BytecodeOffset bailout_id, SharedFunctionInfo shared_info, int height);
  static TranslatedFrame JavaScriptBuiltinContinuationWithCatchFrame(
      BytecodeOffset bailout_id, SharedFunctionInfo shared_info, int height);
  static TranslatedFrame InvalidFrame() {
    return TranslatedFrame(kInvalid, SharedFunctionInfo());
  }

  static void AdvanceIterator(std::deque<TranslatedValue>::iterator* iter);

  TranslatedFrame(Kind kind,
                  SharedFunctionInfo shared_info = SharedFunctionInfo(),
                  int height = 0, int return_value_offset = 0,
                  int return_value_count = 0)
      : kind_(kind),
        bytecode_offset_(BytecodeOffset::None()),
        raw_shared_info_(shared_info),
        height_(height),
        return_value_offset_(return_value_offset),
        return_value_count_(return_value_count) {}

  void Add(const TranslatedValue& value) { values_.push_back(value); }
  TranslatedValue* ValueAt(int index) { return &(values_[index]); }
  void Handlify();

  Kind kind_;
  BytecodeOffset bytecode_offset_;
  SharedFunctionInfo raw_shared_info_;
  Handle<SharedFunctionInfo> shared_info_;
  int height_;
  int return_value_offset_;
  int return_value_count_;

  using ValuesContainer = std::deque<TranslatedValue>;

  ValuesContainer values_;

#if V8_ENABLE_WEBASSEMBLY
  // Only for Kind == kJSToWasmBuiltinContinuation
  base::Optional<wasm::ValueKind> return_kind_;
#endif  // V8_ENABLE_WEBASSEMBLY
};

// Auxiliary class for translating deoptimization values.
// Typical usage sequence:
//
// 1. Construct the instance. This will involve reading out the translations
//    and resolving them to values using the supplied frame pointer and
//    machine state (registers). This phase is guaranteed not to allocate
//    and not to use any HandleScope. Any object pointers will be stored raw.
//
// 2. Handlify pointers. This will convert all the raw pointers to handles.
//
// 3. Reading out the frame values.
//
// Note: After the instance is constructed, it is possible to iterate over
// the values eagerly.

class TranslatedState {
 public:
  // There are two constructors, each for a different purpose:

  // The default constructor is for the purpose of deoptimizing an optimized
  // frame (replacing it with one or several unoptimized frames). It is used by
  // the Deoptimizer.
  TranslatedState() : purpose_(kDeoptimization) {}

  // This constructor is for the purpose of merely inspecting an optimized
  // frame. It is used by stack trace generation and various debugging features.
  explicit TranslatedState(const JavaScriptFrame* frame);

  void Prepare(Address stack_frame_pointer);

  // Store newly materialized values into the isolate.
  void StoreMaterializedValuesAndDeopt(JavaScriptFrame* frame);

  using iterator = std::vector<TranslatedFrame>::iterator;
  iterator begin() { return frames_.begin(); }
  iterator end() { return frames_.end(); }

  using const_iterator = std::vector<TranslatedFrame>::const_iterator;
  const_iterator begin() const { return frames_.begin(); }
  const_iterator end() const { return frames_.end(); }

  std::vector<TranslatedFrame>& frames() { return frames_; }

  TranslatedFrame* GetFrameFromJSFrameIndex(int jsframe_index);
  TranslatedFrame* GetArgumentsInfoFromJSFrameIndex(int jsframe_index,
                                                    int* arguments_count);

  Isolate* isolate() { return isolate_; }

  void Init(Isolate* isolate, Address input_frame_pointer,
            Address stack_frame_pointer, TranslationArrayIterator* iterator,
            FixedArray literal_array, RegisterValues* registers,
            FILE* trace_file, int parameter_count, int actual_argument_count);

  void VerifyMaterializedObjects();
  bool DoUpdateFeedback();

 private:
  friend TranslatedValue;

  // See the description of the constructors for an explanation of the two
  // purposes. The only actual difference is that in the kFrameInspection case
  // extra work is needed to not violate assumptions made by left-trimming.  For
  // details, see the code around ReplaceElementsArrayWithCopy.
  enum Purpose { kDeoptimization, kFrameInspection };

  TranslatedFrame CreateNextTranslatedFrame(TranslationArrayIterator* iterator,
                                            FixedArray literal_array,
                                            Address fp, FILE* trace_file);
  int CreateNextTranslatedValue(int frame_index,
                                TranslationArrayIterator* iterator,
                                FixedArray literal_array, Address fp,
                                RegisterValues* registers, FILE* trace_file);
  Address DecompressIfNeeded(intptr_t value);
  void CreateArgumentsElementsTranslatedValues(int frame_index,
                                               Address input_frame_pointer,
                                               CreateArgumentsType type,
                                               FILE* trace_file);

  void UpdateFromPreviouslyMaterializedObjects();
  void MaterializeFixedDoubleArray(TranslatedFrame* frame, int* value_index,
                                   TranslatedValue* slot, Handle<Map> map);
  void MaterializeHeapNumber(TranslatedFrame* frame, int* value_index,
                             TranslatedValue* slot);

  void EnsureObjectAllocatedAt(TranslatedValue* slot);

  void SkipSlots(int slots_to_skip, TranslatedFrame* frame, int* value_index);

  Handle<ByteArray> AllocateStorageFor(TranslatedValue* slot);
  void EnsureJSObjectAllocated(TranslatedValue* slot, Handle<Map> map);
  void EnsurePropertiesAllocatedAndMarked(TranslatedValue* properties_slot,
                                          Handle<Map> map);
  void EnsureChildrenAllocated(int count, TranslatedFrame* frame,
                               int* value_index, std::stack<int>* worklist);
  void EnsureCapturedObjectAllocatedAt(int object_index,
                                       std::stack<int>* worklist);
  Handle<HeapObject> InitializeObjectAt(TranslatedValue* slot);
  void InitializeCapturedObjectAt(int object_index, std::stack<int>* worklist,
                                  const DisallowGarbageCollection& no_gc);
  void InitializeJSObjectAt(TranslatedFrame* frame, int* value_index,
                            TranslatedValue* slot, Handle<Map> map,
                            const DisallowGarbageCollection& no_gc);
  void InitializeObjectWithTaggedFieldsAt(
      TranslatedFrame* frame, int* value_index, TranslatedValue* slot,
      Handle<Map> map, const DisallowGarbageCollection& no_gc);

  void ReadUpdateFeedback(TranslationArrayIterator* iterator,
                          FixedArray literal_array, FILE* trace_file);

  TranslatedValue* ResolveCapturedObject(TranslatedValue* slot);
  TranslatedValue* GetValueByObjectIndex(int object_index);
  Handle<Object> GetValueAndAdvance(TranslatedFrame* frame, int* value_index);
  TranslatedValue* GetResolvedSlot(TranslatedFrame* frame, int value_index);
  TranslatedValue* GetResolvedSlotAndAdvance(TranslatedFrame* frame,
                                             int* value_index);

  static uint32_t GetUInt32Slot(Address fp, int slot_index);
  static uint64_t GetUInt64Slot(Address fp, int slot_index);
  static Float32 GetFloatSlot(Address fp, int slot_index);
  static Float64 GetDoubleSlot(Address fp, int slot_index);

  Purpose const purpose_;
  std::vector<TranslatedFrame> frames_;
  Isolate* isolate_ = nullptr;
  Address stack_frame_pointer_ = kNullAddress;
  int formal_parameter_count_;
  int actual_argument_count_;

  struct ObjectPosition {
    int frame_index_;
    int value_index_;
  };
  std::deque<ObjectPosition> object_positions_;
  Handle<FeedbackVector> feedback_vector_handle_;
  FeedbackVector feedback_vector_;
  FeedbackSlot feedback_slot_;
};

// Return kind encoding for a Wasm function returning void.
const int kNoWasmReturnKind = -1;

}  // namespace internal
}  // namespace v8

#endif  // V8_DEOPTIMIZER_TRANSLATED_STATE_H_
