// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/api/api.h"
#include "src/baseline/baseline.h"
#include "src/builtins/builtins-utils-gen.h"
#include "src/builtins/builtins.h"
#include "src/codegen/code-stub-assembler.h"
#include "src/codegen/interface-descriptors-inl.h"
#include "src/codegen/macro-assembler.h"
#include "src/execution/frame-constants.h"
#include "src/heap/memory-chunk.h"
#include "src/ic/accessor-assembler.h"
#include "src/ic/keyed-store-generic.h"
#include "src/logging/counters.h"
#include "src/objects/debug-objects.h"
#include "src/objects/shared-function-info.h"
#include "src/runtime/runtime.h"

namespace v8 {
namespace internal {

// -----------------------------------------------------------------------------
// Stack checks.

void Builtins::Generate_StackCheck(MacroAssembler* masm) {
  masm->TailCallRuntime(Runtime::kStackGuard);
}

// -----------------------------------------------------------------------------
// TurboFan support builtins.

TF_BUILTIN(CopyFastSmiOrObjectElements, CodeStubAssembler) {
  auto js_object = Parameter<JSObject>(Descriptor::kObject);

  // Load the {object}s elements.
  TNode<FixedArrayBase> source =
      CAST(LoadObjectField(js_object, JSObject::kElementsOffset));
  TNode<FixedArrayBase> target =
      CloneFixedArray(source, ExtractFixedArrayFlag::kFixedArrays);
  StoreObjectField(js_object, JSObject::kElementsOffset, target);
  Return(target);
}

TF_BUILTIN(GrowFastDoubleElements, CodeStubAssembler) {
  auto object = Parameter<JSObject>(Descriptor::kObject);
  auto key = Parameter<Smi>(Descriptor::kKey);

  Label runtime(this, Label::kDeferred);
  TNode<FixedArrayBase> elements = LoadElements(object);
  elements = TryGrowElementsCapacity(object, elements, PACKED_DOUBLE_ELEMENTS,
                                     key, &runtime);
  Return(elements);

  BIND(&runtime);
  TailCallRuntime(Runtime::kGrowArrayElements, NoContextConstant(), object,
                  key);
}

TF_BUILTIN(GrowFastSmiOrObjectElements, CodeStubAssembler) {
  auto object = Parameter<JSObject>(Descriptor::kObject);
  auto key = Parameter<Smi>(Descriptor::kKey);

  Label runtime(this, Label::kDeferred);
  TNode<FixedArrayBase> elements = LoadElements(object);
  elements =
      TryGrowElementsCapacity(object, elements, PACKED_ELEMENTS, key, &runtime);
  Return(elements);

  BIND(&runtime);
  TailCallRuntime(Runtime::kGrowArrayElements, NoContextConstant(), object,
                  key);
}

TF_BUILTIN(ReturnReceiver, CodeStubAssembler) {
  auto receiver = Parameter<Object>(Descriptor::kReceiver);
  Return(receiver);
}

TF_BUILTIN(DebugBreakTrampoline, CodeStubAssembler) {
  Label tailcall_to_shared(this);
  auto context = Parameter<Context>(Descriptor::kContext);
  auto new_target = Parameter<Object>(Descriptor::kJSNewTarget);
  auto arg_count =
      UncheckedParameter<Int32T>(Descriptor::kJSActualArgumentsCount);
  auto function = Parameter<JSFunction>(Descriptor::kJSTarget);

  // Check break-at-entry flag on the debug info.
  TNode<SharedFunctionInfo> shared =
      CAST(LoadObjectField(function, JSFunction::kSharedFunctionInfoOffset));
  TNode<Object> maybe_heap_object_or_smi =
      LoadObjectField(shared, SharedFunctionInfo::kScriptOrDebugInfoOffset);
  TNode<HeapObject> maybe_debug_info =
      TaggedToHeapObject(maybe_heap_object_or_smi, &tailcall_to_shared);
  GotoIfNot(HasInstanceType(maybe_debug_info, InstanceType::DEBUG_INFO_TYPE),
            &tailcall_to_shared);

  {
    TNode<DebugInfo> debug_info = CAST(maybe_debug_info);
    TNode<Smi> flags =
        CAST(LoadObjectField(debug_info, DebugInfo::kFlagsOffset));
    GotoIfNot(SmiToInt32(SmiAnd(flags, SmiConstant(DebugInfo::kBreakAtEntry))),
              &tailcall_to_shared);

    CallRuntime(Runtime::kDebugBreakAtEntry, context, function);
    Goto(&tailcall_to_shared);
  }

  BIND(&tailcall_to_shared);
  // Tail call into code object on the SharedFunctionInfo.
  TNode<Code> code = GetSharedFunctionInfoCode(shared);
  TailCallJSCode(code, context, function, new_target, arg_count);
}

class WriteBarrierCodeStubAssembler : public CodeStubAssembler {
 public:
  explicit WriteBarrierCodeStubAssembler(compiler::CodeAssemblerState* state)
      : CodeStubAssembler(state) {}

  TNode<BoolT> IsMarking() {
    TNode<ExternalReference> is_marking_addr = ExternalConstant(
        ExternalReference::heap_is_marking_flag_address(this->isolate()));
    return Word32NotEqual(Load<Uint8T>(is_marking_addr), Int32Constant(0));
  }

  TNode<BoolT> IsPageFlagSet(TNode<IntPtrT> object, int mask) {
    TNode<IntPtrT> page = PageFromAddress(object);
    TNode<IntPtrT> flags = UncheckedCast<IntPtrT>(
        Load(MachineType::Pointer(), page,
             IntPtrConstant(BasicMemoryChunk::kFlagsOffset)));
    return WordNotEqual(WordAnd(flags, IntPtrConstant(mask)),
                        IntPtrConstant(0));
  }

  TNode<BoolT> IsWhite(TNode<IntPtrT> object) {
    DCHECK_EQ(strcmp(Marking::kWhiteBitPattern, "00"), 0);
    TNode<IntPtrT> cell;
    TNode<IntPtrT> mask;
    GetMarkBit(object, &cell, &mask);
    TNode<Int32T> mask32 = TruncateIntPtrToInt32(mask);
    // Non-white has 1 for the first bit, so we only need to check for the first
    // bit.
    return Word32Equal(Word32And(Load<Int32T>(cell), mask32), Int32Constant(0));
  }

  void GetMarkBit(TNode<IntPtrT> object, TNode<IntPtrT>* cell,
                  TNode<IntPtrT>* mask) {
    TNode<IntPtrT> page = PageFromAddress(object);
    TNode<IntPtrT> bitmap =
        IntPtrAdd(page, IntPtrConstant(MemoryChunk::kMarkingBitmapOffset));

    {
      // Temp variable to calculate cell offset in bitmap.
      TNode<WordT> r0;
      int shift = Bitmap::kBitsPerCellLog2 + kTaggedSizeLog2 -
                  Bitmap::kBytesPerCellLog2;
      r0 = WordShr(object, IntPtrConstant(shift));
      r0 = WordAnd(r0, IntPtrConstant((kPageAlignmentMask >> shift) &
                                      ~(Bitmap::kBytesPerCell - 1)));
      *cell = IntPtrAdd(bitmap, Signed(r0));
    }
    {
      // Temp variable to calculate bit offset in cell.
      TNode<WordT> r1;
      r1 = WordShr(object, IntPtrConstant(kTaggedSizeLog2));
      r1 = WordAnd(r1, IntPtrConstant((1 << Bitmap::kBitsPerCellLog2) - 1));
      // It seems that LSB(e.g. cl) is automatically used, so no manual masking
      // is needed. Uncomment the following line otherwise.
      // WordAnd(r1, IntPtrConstant((1 << kBitsPerByte) - 1)));
      *mask = WordShl(IntPtrConstant(1), r1);
    }
  }

  void InsertIntoRememberedSet(TNode<IntPtrT> object, TNode<IntPtrT> slot,
                               SaveFPRegsMode fp_mode) {
    Label slow_path(this), next(this);
    TNode<IntPtrT> page = PageFromAddress(object);

    // Load address of SlotSet
    TNode<IntPtrT> slot_set = LoadSlotSet(page, &slow_path);
    TNode<IntPtrT> slot_offset = IntPtrSub(slot, page);

    // Load bucket
    TNode<IntPtrT> bucket = LoadBucket(slot_set, slot_offset, &slow_path);

    // Update cell
    SetBitInCell(bucket, slot_offset);
    Goto(&next);

    BIND(&slow_path);
    {
      TNode<ExternalReference> function =
          ExternalConstant(ExternalReference::insert_remembered_set_function());
      CallCFunctionWithCallerSavedRegisters(
          function, MachineTypeOf<Int32T>::value, fp_mode,
          std::make_pair(MachineTypeOf<IntPtrT>::value, page),
          std::make_pair(MachineTypeOf<IntPtrT>::value, slot));
      Goto(&next);
    }

    BIND(&next);
  }

  TNode<IntPtrT> LoadSlotSet(TNode<IntPtrT> page, Label* slow_path) {
    TNode<IntPtrT> slot_set = UncheckedCast<IntPtrT>(
        Load(MachineType::Pointer(), page,
             IntPtrConstant(MemoryChunk::kOldToNewSlotSetOffset)));
    GotoIf(WordEqual(slot_set, IntPtrConstant(0)), slow_path);
    return slot_set;
  }

  TNode<IntPtrT> LoadBucket(TNode<IntPtrT> slot_set, TNode<WordT> slot_offset,
                            Label* slow_path) {
    TNode<WordT> bucket_index =
        WordShr(slot_offset, SlotSet::kBitsPerBucketLog2 + kTaggedSizeLog2);
    TNode<IntPtrT> bucket = UncheckedCast<IntPtrT>(
        Load(MachineType::Pointer(), slot_set,
             WordShl(bucket_index, kSystemPointerSizeLog2)));
    GotoIf(WordEqual(bucket, IntPtrConstant(0)), slow_path);
    return bucket;
  }

  void SetBitInCell(TNode<IntPtrT> bucket, TNode<WordT> slot_offset) {
    // Load cell value
    TNode<WordT> cell_offset = WordAnd(
        WordShr(slot_offset, SlotSet::kBitsPerCellLog2 + kTaggedSizeLog2 -
                                 SlotSet::kCellSizeBytesLog2),
        IntPtrConstant((SlotSet::kCellsPerBucket - 1)
                       << SlotSet::kCellSizeBytesLog2));
    TNode<IntPtrT> cell_address =
        UncheckedCast<IntPtrT>(IntPtrAdd(bucket, cell_offset));
    TNode<IntPtrT> old_cell_value =
        ChangeInt32ToIntPtr(Load<Int32T>(cell_address));

    // Calculate new cell value
    TNode<WordT> bit_index = WordAnd(WordShr(slot_offset, kTaggedSizeLog2),
                                     IntPtrConstant(SlotSet::kBitsPerCell - 1));
    TNode<IntPtrT> new_cell_value = UncheckedCast<IntPtrT>(
        WordOr(old_cell_value, WordShl(IntPtrConstant(1), bit_index)));

    // Update cell value
    StoreNoWriteBarrier(MachineRepresentation::kWord32, cell_address,
                        TruncateIntPtrToInt32(new_cell_value));
  }

  void GenerationalWriteBarrier(SaveFPRegsMode fp_mode) {
    Label incremental_wb(this), test_old_to_young_flags(this),
        store_buffer_exit(this), store_buffer_incremental_wb(this), next(this);

    // When incremental marking is not on, we skip cross generation pointer
    // checking here, because there are checks for
    // `kPointersFromHereAreInterestingMask` and
    // `kPointersToHereAreInterestingMask` in
    // `src/compiler/<arch>/code-generator-<arch>.cc` before calling this
    // stub, which serves as the cross generation checking.
    auto slot =
        UncheckedParameter<IntPtrT>(WriteBarrierDescriptor::kSlotAddress);
    Branch(IsMarking(), &test_old_to_young_flags, &store_buffer_exit);

    BIND(&test_old_to_young_flags);
    {
      // TODO(ishell): do a new-space range check instead.
      TNode<IntPtrT> value = BitcastTaggedToWord(Load<HeapObject>(slot));

      // TODO(albertnetymk): Try to cache the page flag for value and
      // object, instead of calling IsPageFlagSet each time.
      TNode<BoolT> value_is_young =
          IsPageFlagSet(value, MemoryChunk::kIsInYoungGenerationMask);
      GotoIfNot(value_is_young, &incremental_wb);

      TNode<IntPtrT> object = BitcastTaggedToWord(
          UncheckedParameter<Object>(WriteBarrierDescriptor::kObject));
      TNode<BoolT> object_is_young =
          IsPageFlagSet(object, MemoryChunk::kIsInYoungGenerationMask);
      Branch(object_is_young, &incremental_wb, &store_buffer_incremental_wb);
    }

    BIND(&store_buffer_exit);
    {
      TNode<IntPtrT> object = BitcastTaggedToWord(
          UncheckedParameter<Object>(WriteBarrierDescriptor::kObject));
      InsertIntoRememberedSet(object, slot, fp_mode);
      Goto(&next);
    }

    BIND(&store_buffer_incremental_wb);
    {
      TNode<IntPtrT> object = BitcastTaggedToWord(
          UncheckedParameter<Object>(WriteBarrierDescriptor::kObject));
      InsertIntoRememberedSet(object, slot, fp_mode);
      Goto(&incremental_wb);
    }

    BIND(&incremental_wb);
    {
      TNode<IntPtrT> value = BitcastTaggedToWord(Load<HeapObject>(slot));
      IncrementalWriteBarrier(slot, value, fp_mode);
      Goto(&next);
    }

    BIND(&next);
  }

  void IncrementalWriteBarrier(SaveFPRegsMode fp_mode) {
    auto slot =
        UncheckedParameter<IntPtrT>(WriteBarrierDescriptor::kSlotAddress);
    TNode<IntPtrT> value = BitcastTaggedToWord(Load<HeapObject>(slot));
    IncrementalWriteBarrier(slot, value, fp_mode);
  }

  void IncrementalWriteBarrier(TNode<IntPtrT> slot, TNode<IntPtrT> value,
                               SaveFPRegsMode fp_mode) {
    Label call_incremental_wb(this), next(this);

    // There are two cases we need to call incremental write barrier.
    // 1) value_is_white
    GotoIf(IsWhite(value), &call_incremental_wb);

    // 2) is_compacting && value_in_EC && obj_isnt_skip
    // is_compacting = true when is_marking = true
    GotoIfNot(IsPageFlagSet(value, MemoryChunk::kEvacuationCandidateMask),
              &next);

    TNode<IntPtrT> object = BitcastTaggedToWord(
        UncheckedParameter<Object>(WriteBarrierDescriptor::kObject));
    Branch(
        IsPageFlagSet(object, MemoryChunk::kSkipEvacuationSlotsRecordingMask),
        &next, &call_incremental_wb);

    BIND(&call_incremental_wb);
    {
      TNode<ExternalReference> function = ExternalConstant(
          ExternalReference::write_barrier_marking_from_code_function());
      TNode<IntPtrT> object = BitcastTaggedToWord(
          UncheckedParameter<Object>(WriteBarrierDescriptor::kObject));
      CallCFunctionWithCallerSavedRegisters(
          function, MachineTypeOf<Int32T>::value, fp_mode,
          std::make_pair(MachineTypeOf<IntPtrT>::value, object),
          std::make_pair(MachineTypeOf<IntPtrT>::value, slot));
      Goto(&next);
    }
    BIND(&next);
  }

  void GenerateRecordWrite(RememberedSetAction rs_mode,
                           SaveFPRegsMode fp_mode) {
    switch (rs_mode) {
      case RememberedSetAction::kEmit:
        GenerationalWriteBarrier(fp_mode);
        break;
      case RememberedSetAction::kOmit:
        IncrementalWriteBarrier(fp_mode);
        break;
    }
    IncrementCounter(isolate()->counters()->write_barriers(), 1);
    Return(TrueConstant());
  }

  void GenerateEphemeronKeyBarrier(SaveFPRegsMode fp_mode) {
    TNode<ExternalReference> function = ExternalConstant(
        ExternalReference::ephemeron_key_write_barrier_function());
    TNode<ExternalReference> isolate_constant =
        ExternalConstant(ExternalReference::isolate_address(isolate()));
    // In this method we limit the allocatable registers so we have to use
    // UncheckedParameter. Parameter does not work because the checked cast
    // needs more registers.
    auto address =
        UncheckedParameter<IntPtrT>(WriteBarrierDescriptor::kSlotAddress);
    TNode<IntPtrT> object = BitcastTaggedToWord(
        UncheckedParameter<Object>(WriteBarrierDescriptor::kObject));

    CallCFunctionWithCallerSavedRegisters(
        function, MachineTypeOf<Int32T>::value, fp_mode,
        std::make_pair(MachineTypeOf<IntPtrT>::value, object),
        std::make_pair(MachineTypeOf<IntPtrT>::value, address),
        std::make_pair(MachineTypeOf<ExternalReference>::value,
                       isolate_constant));

    IncrementCounter(isolate()->counters()->write_barriers(), 1);
    Return(TrueConstant());
  }
};

TF_BUILTIN(RecordWriteEmitRememberedSetSaveFP, WriteBarrierCodeStubAssembler) {
  GenerateRecordWrite(RememberedSetAction::kEmit, SaveFPRegsMode::kSave);
}

TF_BUILTIN(RecordWriteOmitRememberedSetSaveFP, WriteBarrierCodeStubAssembler) {
  GenerateRecordWrite(RememberedSetAction::kOmit, SaveFPRegsMode::kSave);
}

TF_BUILTIN(RecordWriteEmitRememberedSetIgnoreFP,
           WriteBarrierCodeStubAssembler) {
  GenerateRecordWrite(RememberedSetAction::kEmit, SaveFPRegsMode::kIgnore);
}

TF_BUILTIN(RecordWriteOmitRememberedSetIgnoreFP,
           WriteBarrierCodeStubAssembler) {
  GenerateRecordWrite(RememberedSetAction::kOmit, SaveFPRegsMode::kIgnore);
}

TF_BUILTIN(EphemeronKeyBarrierSaveFP, WriteBarrierCodeStubAssembler) {
  GenerateEphemeronKeyBarrier(SaveFPRegsMode::kSave);
}

TF_BUILTIN(EphemeronKeyBarrierIgnoreFP, WriteBarrierCodeStubAssembler) {
  GenerateEphemeronKeyBarrier(SaveFPRegsMode::kIgnore);
}

class DeletePropertyBaseAssembler : public AccessorAssembler {
 public:
  explicit DeletePropertyBaseAssembler(compiler::CodeAssemblerState* state)
      : AccessorAssembler(state) {}

  void DictionarySpecificDelete(TNode<JSReceiver> receiver,
                                TNode<NameDictionary> properties,
                                TNode<IntPtrT> key_index,
                                TNode<Context> context) {
    // Overwrite the entry itself (see NameDictionary::SetEntry).
    TNode<Oddball> filler = TheHoleConstant();
    DCHECK(RootsTable::IsImmortalImmovable(RootIndex::kTheHoleValue));
    StoreFixedArrayElement(properties, key_index, filler, SKIP_WRITE_BARRIER);
    StoreValueByKeyIndex<NameDictionary>(properties, key_index, filler,
                                         SKIP_WRITE_BARRIER);
    StoreDetailsByKeyIndex<NameDictionary>(properties, key_index,
                                           SmiConstant(0));

    // Update bookkeeping information (see NameDictionary::ElementRemoved).
    TNode<Smi> nof = GetNumberOfElements<NameDictionary>(properties);
    TNode<Smi> new_nof = SmiSub(nof, SmiConstant(1));
    SetNumberOfElements<NameDictionary>(properties, new_nof);
    TNode<Smi> num_deleted =
        GetNumberOfDeletedElements<NameDictionary>(properties);
    TNode<Smi> new_deleted = SmiAdd(num_deleted, SmiConstant(1));
    SetNumberOfDeletedElements<NameDictionary>(properties, new_deleted);

    // Shrink the dictionary if necessary (see NameDictionary::Shrink).
    Label shrinking_done(this);
    TNode<Smi> capacity = GetCapacity<NameDictionary>(properties);
    GotoIf(SmiGreaterThan(new_nof, SmiShr(capacity, 2)), &shrinking_done);
    GotoIf(SmiLessThan(new_nof, SmiConstant(16)), &shrinking_done);

    TNode<NameDictionary> new_properties =
        CAST(CallRuntime(Runtime::kShrinkNameDictionary, context, properties));

    StoreJSReceiverPropertiesOrHash(receiver, new_properties);

    Goto(&shrinking_done);
    BIND(&shrinking_done);
  }

  void DictionarySpecificDelete(TNode<JSReceiver> receiver,
                                TNode<SwissNameDictionary> properties,
                                TNode<IntPtrT> key_index,
                                TNode<Context> context) {
    Label shrunk(this), done(this);
    TVARIABLE(SwissNameDictionary, shrunk_table);

    SwissNameDictionaryDelete(properties, key_index, &shrunk, &shrunk_table);
    Goto(&done);
    BIND(&shrunk);
    StoreJSReceiverPropertiesOrHash(receiver, shrunk_table.value());
    Goto(&done);

    BIND(&done);
  }

  template <typename Dictionary>
  void DeleteDictionaryProperty(TNode<JSReceiver> receiver,
                                TNode<Dictionary> properties, TNode<Name> name,
                                TNode<Context> context, Label* dont_delete,
                                Label* notfound) {
    TVARIABLE(IntPtrT, var_name_index);
    Label dictionary_found(this, &var_name_index);
    NameDictionaryLookup<Dictionary>(properties, name, &dictionary_found,
                                     &var_name_index, notfound);

    BIND(&dictionary_found);
    TNode<IntPtrT> key_index = var_name_index.value();
    TNode<Uint32T> details = LoadDetailsByKeyIndex(properties, key_index);
    GotoIf(IsSetWord32(details, PropertyDetails::kAttributesDontDeleteMask),
           dont_delete);

    DictionarySpecificDelete(receiver, properties, key_index, context);

    Return(TrueConstant());
  }
};

TF_BUILTIN(DeleteProperty, DeletePropertyBaseAssembler) {
  auto receiver = Parameter<Object>(Descriptor::kObject);
  auto key = Parameter<Object>(Descriptor::kKey);
  auto language_mode = Parameter<Smi>(Descriptor::kLanguageMode);
  auto context = Parameter<Context>(Descriptor::kContext);

  TVARIABLE(IntPtrT, var_index);
  TVARIABLE(Name, var_unique);
  Label if_index(this, &var_index), if_unique_name(this), if_notunique(this),
      if_notfound(this), slow(this), if_proxy(this);

  GotoIf(TaggedIsSmi(receiver), &slow);
  TNode<Map> receiver_map = LoadMap(CAST(receiver));
  TNode<Uint16T> instance_type = LoadMapInstanceType(receiver_map);
  GotoIf(InstanceTypeEqual(instance_type, JS_PROXY_TYPE), &if_proxy);
  GotoIf(IsCustomElementsReceiverInstanceType(instance_type), &slow);
  TryToName(key, &if_index, &var_index, &if_unique_name, &var_unique, &slow,
            &if_notunique);

  BIND(&if_index);
  {
    Comment("integer index");
    Goto(&slow);  // TODO(jkummerow): Implement more smarts here.
  }

  BIND(&if_unique_name);
  {
    Comment("key is unique name");
    CheckForAssociatedProtector(var_unique.value(), &slow);

    Label dictionary(this), dont_delete(this);
    GotoIf(IsDictionaryMap(receiver_map), &dictionary);

    // Fast properties need to clear recorded slots and mark the deleted
    // property as mutable, which can only be done in C++.
    Goto(&slow);

    BIND(&dictionary);
    {
      InvalidateValidityCellIfPrototype(receiver_map);

      TNode<PropertyDictionary> properties =
          CAST(LoadSlowProperties(CAST(receiver)));
      DeleteDictionaryProperty(CAST(receiver), properties, var_unique.value(),
                               context, &dont_delete, &if_notfound);
    }

    BIND(&dont_delete);
    {
      STATIC_ASSERT(LanguageModeSize == 2);
      GotoIf(SmiNotEqual(language_mode, SmiConstant(LanguageMode::kSloppy)),
             &slow);
      Return(FalseConstant());
    }
  }

  BIND(&if_notunique);
  {
    // If the string was not found in the string table, then no object can
    // have a property with that name.
    TryInternalizeString(CAST(key), &if_index, &var_index, &if_unique_name,
                         &var_unique, &if_notfound, &slow);
  }

  BIND(&if_notfound);
  Return(TrueConstant());

  BIND(&if_proxy);
  {
    TNode<Name> name = CAST(CallBuiltin(Builtins::kToName, context, key));
    GotoIf(IsPrivateSymbol(name), &slow);
    TailCallBuiltin(Builtins::kProxyDeleteProperty, context, receiver, name,
                    language_mode);
  }

  BIND(&slow);
  {
    TailCallRuntime(Runtime::kDeleteProperty, context, receiver, key,
                    language_mode);
  }
}

namespace {

class SetOrCopyDataPropertiesAssembler : public CodeStubAssembler {
 public:
  explicit SetOrCopyDataPropertiesAssembler(compiler::CodeAssemblerState* state)
      : CodeStubAssembler(state) {}

 protected:
  TNode<Object> SetOrCopyDataProperties(TNode<Context> context,
                                        TNode<JSReceiver> target,
                                        TNode<Object> source, Label* if_runtime,
                                        bool use_set = true) {
    Label if_done(this), if_noelements(this),
        if_sourcenotjsobject(this, Label::kDeferred);

    // JSPrimitiveWrapper wrappers for numbers don't have any enumerable own
    // properties, so we can immediately skip the whole operation if {source} is
    // a Smi.
    GotoIf(TaggedIsSmi(source), &if_done);

    // Otherwise check if {source} is a proper JSObject, and if not, defer
    // to testing for non-empty strings below.
    TNode<Map> source_map = LoadMap(CAST(source));
    TNode<Uint16T> source_instance_type = LoadMapInstanceType(source_map);
    GotoIfNot(IsJSObjectInstanceType(source_instance_type),
              &if_sourcenotjsobject);

    TNode<FixedArrayBase> source_elements = LoadElements(CAST(source));
    GotoIf(IsEmptyFixedArray(source_elements), &if_noelements);
    Branch(IsEmptySlowElementDictionary(source_elements), &if_noelements,
           if_runtime);

    BIND(&if_noelements);
    {
      // If the target is deprecated, the object will be updated on first store.
      // If the source for that store equals the target, this will invalidate
      // the cached representation of the source. Handle this case in runtime.
      TNode<Map> target_map = LoadMap(target);
      GotoIf(IsDeprecatedMap(target_map), if_runtime);

      if (use_set) {
        TNode<BoolT> target_is_simple_receiver = IsSimpleObjectMap(target_map);
        ForEachEnumerableOwnProperty(
            context, source_map, CAST(source), kEnumerationOrder,
            [=](TNode<Name> key, TNode<Object> value) {
              KeyedStoreGenericGenerator::SetProperty(
                  state(), context, target, target_is_simple_receiver, key,
                  value, LanguageMode::kStrict);
            },
            if_runtime);
      } else {
        ForEachEnumerableOwnProperty(
            context, source_map, CAST(source), kEnumerationOrder,
            [=](TNode<Name> key, TNode<Object> value) {
              CallBuiltin(Builtins::kSetPropertyInLiteral, context, target, key,
                          value);
            },
            if_runtime);
      }
      Goto(&if_done);
    }

    BIND(&if_sourcenotjsobject);
    {
      // Handle other JSReceivers in the runtime.
      GotoIf(IsJSReceiverInstanceType(source_instance_type), if_runtime);

      // Non-empty strings are the only non-JSReceivers that need to be
      // handled explicitly by Object.assign() and CopyDataProperties.
      GotoIfNot(IsStringInstanceType(source_instance_type), &if_done);
      TNode<IntPtrT> source_length = LoadStringLengthAsWord(CAST(source));
      Branch(IntPtrEqual(source_length, IntPtrConstant(0)), &if_done,
             if_runtime);
    }

    BIND(&if_done);
    return UndefinedConstant();
  }
};

}  // namespace

// ES #sec-copydataproperties
TF_BUILTIN(CopyDataProperties, SetOrCopyDataPropertiesAssembler) {
  auto target = Parameter<JSObject>(Descriptor::kTarget);
  auto source = Parameter<Object>(Descriptor::kSource);
  auto context = Parameter<Context>(Descriptor::kContext);

  CSA_ASSERT(this, TaggedNotEqual(target, source));

  Label if_runtime(this, Label::kDeferred);
  Return(SetOrCopyDataProperties(context, target, source, &if_runtime, false));

  BIND(&if_runtime);
  TailCallRuntime(Runtime::kCopyDataProperties, context, target, source);
}

TF_BUILTIN(SetDataProperties, SetOrCopyDataPropertiesAssembler) {
  auto target = Parameter<JSReceiver>(Descriptor::kTarget);
  auto source = Parameter<Object>(Descriptor::kSource);
  auto context = Parameter<Context>(Descriptor::kContext);

  Label if_runtime(this, Label::kDeferred);
  GotoIfForceSlowPath(&if_runtime);
  Return(SetOrCopyDataProperties(context, target, source, &if_runtime, true));

  BIND(&if_runtime);
  TailCallRuntime(Runtime::kSetDataProperties, context, target, source);
}

TF_BUILTIN(ForInEnumerate, CodeStubAssembler) {
  auto receiver = Parameter<JSReceiver>(Descriptor::kReceiver);
  auto context = Parameter<Context>(Descriptor::kContext);

  Label if_empty(this), if_runtime(this, Label::kDeferred);
  TNode<Map> receiver_map = CheckEnumCache(receiver, &if_empty, &if_runtime);
  Return(receiver_map);

  BIND(&if_empty);
  Return(EmptyFixedArrayConstant());

  BIND(&if_runtime);
  TailCallRuntime(Runtime::kForInEnumerate, context, receiver);
}

TF_BUILTIN(ForInPrepare, CodeStubAssembler) {
  // The {enumerator} is either a Map or a FixedArray.
  auto enumerator = Parameter<HeapObject>(Descriptor::kEnumerator);
  auto index = Parameter<TaggedIndex>(Descriptor::kVectorIndex);
  auto feedback_vector = Parameter<FeedbackVector>(Descriptor::kFeedbackVector);
  TNode<UintPtrT> vector_index = Unsigned(TaggedIndexToIntPtr(index));

  TNode<FixedArray> cache_array;
  TNode<Smi> cache_length;
  ForInPrepare(enumerator, vector_index, feedback_vector, &cache_array,
               &cache_length, UpdateFeedbackMode::kGuaranteedFeedback);
  Return(cache_array, cache_length);
}

TF_BUILTIN(ForInFilter, CodeStubAssembler) {
  auto key = Parameter<String>(Descriptor::kKey);
  auto object = Parameter<HeapObject>(Descriptor::kObject);
  auto context = Parameter<Context>(Descriptor::kContext);

  Label if_true(this), if_false(this);
  TNode<Oddball> result = HasProperty(context, object, key, kForInHasProperty);
  Branch(IsTrue(result), &if_true, &if_false);

  BIND(&if_true);
  Return(key);

  BIND(&if_false);
  Return(UndefinedConstant());
}

TF_BUILTIN(SameValue, CodeStubAssembler) {
  auto lhs = Parameter<Object>(Descriptor::kLeft);
  auto rhs = Parameter<Object>(Descriptor::kRight);

  Label if_true(this), if_false(this);
  BranchIfSameValue(lhs, rhs, &if_true, &if_false);

  BIND(&if_true);
  Return(TrueConstant());

  BIND(&if_false);
  Return(FalseConstant());
}

TF_BUILTIN(SameValueNumbersOnly, CodeStubAssembler) {
  auto lhs = Parameter<Object>(Descriptor::kLeft);
  auto rhs = Parameter<Object>(Descriptor::kRight);

  Label if_true(this), if_false(this);
  BranchIfSameValue(lhs, rhs, &if_true, &if_false, SameValueMode::kNumbersOnly);

  BIND(&if_true);
  Return(TrueConstant());

  BIND(&if_false);
  Return(FalseConstant());
}

TF_BUILTIN(AdaptorWithBuiltinExitFrame, CodeStubAssembler) {
  auto target = Parameter<JSFunction>(Descriptor::kTarget);
  auto new_target = Parameter<Object>(Descriptor::kNewTarget);
  auto c_function = UncheckedParameter<WordT>(Descriptor::kCFunction);

  // The logic contained here is mirrored for TurboFan inlining in
  // JSTypedLowering::ReduceJSCall{Function,Construct}. Keep these in sync.

  // Make sure we operate in the context of the called function (for example
  // ConstructStubs implemented in C++ will be run in the context of the caller
  // instead of the callee, due to the way that [[Construct]] is defined for
  // ordinary functions).
  TNode<Context> context = LoadJSFunctionContext(target);

  auto actual_argc =
      UncheckedParameter<Int32T>(Descriptor::kActualArgumentsCount);

  TVARIABLE(Int32T, pushed_argc, actual_argc);

  TNode<SharedFunctionInfo> shared = LoadJSFunctionSharedFunctionInfo(target);

  TNode<Int32T> formal_count =
      UncheckedCast<Int32T>(LoadSharedFunctionInfoFormalParameterCount(shared));

  // The number of arguments pushed is the maximum of actual arguments count
  // and formal parameters count. Except when the formal parameters count is
  // the sentinel.
  Label check_argc(this), update_argc(this), done_argc(this);

  Branch(Word32Equal(formal_count, Int32Constant(kDontAdaptArgumentsSentinel)),
         &done_argc, &check_argc);
  BIND(&check_argc);
  Branch(Int32GreaterThan(formal_count, pushed_argc.value()), &update_argc,
         &done_argc);
  BIND(&update_argc);
  pushed_argc = formal_count;
  Goto(&done_argc);
  BIND(&done_argc);

  // Update arguments count for CEntry to contain the number of arguments
  // including the receiver and the extra arguments.
  TNode<Int32T> argc = Int32Add(
      pushed_argc.value(),
      Int32Constant(BuiltinExitFrameConstants::kNumExtraArgsWithReceiver));

  const bool builtin_exit_frame = true;
  TNode<Code> code =
      HeapConstant(CodeFactory::CEntry(isolate(), 1, SaveFPRegsMode::kIgnore,
                                       ArgvMode::kStack, builtin_exit_frame));

  // Unconditionally push argc, target and new target as extra stack arguments.
  // They will be used by stack frame iterators when constructing stack trace.
  TailCallStub(CEntry1ArgvOnStackDescriptor{},  // descriptor
               code, context,       // standard arguments for TailCallStub
               argc, c_function,    // register arguments
               TheHoleConstant(),   // additional stack argument 1 (padding)
               SmiFromInt32(argc),  // additional stack argument 2
               target,              // additional stack argument 3
               new_target);         // additional stack argument 4
}

TF_BUILTIN(AllocateInYoungGeneration, CodeStubAssembler) {
  auto requested_size = UncheckedParameter<IntPtrT>(Descriptor::kRequestedSize);
  CSA_CHECK(this, IsValidPositiveSmi(requested_size));

  TNode<Smi> allocation_flags =
      SmiConstant(Smi::FromInt(AllocateDoubleAlignFlag::encode(false) |
                               AllowLargeObjectAllocationFlag::encode(true)));
  TailCallRuntime(Runtime::kAllocateInYoungGeneration, NoContextConstant(),
                  SmiFromIntPtr(requested_size), allocation_flags);
}

TF_BUILTIN(AllocateRegularInYoungGeneration, CodeStubAssembler) {
  auto requested_size = UncheckedParameter<IntPtrT>(Descriptor::kRequestedSize);
  CSA_CHECK(this, IsValidPositiveSmi(requested_size));

  TNode<Smi> allocation_flags =
      SmiConstant(Smi::FromInt(AllocateDoubleAlignFlag::encode(false) |
                               AllowLargeObjectAllocationFlag::encode(false)));
  TailCallRuntime(Runtime::kAllocateInYoungGeneration, NoContextConstant(),
                  SmiFromIntPtr(requested_size), allocation_flags);
}

TF_BUILTIN(AllocateInOldGeneration, CodeStubAssembler) {
  auto requested_size = UncheckedParameter<IntPtrT>(Descriptor::kRequestedSize);
  CSA_CHECK(this, IsValidPositiveSmi(requested_size));

  TNode<Smi> runtime_flags =
      SmiConstant(Smi::FromInt(AllocateDoubleAlignFlag::encode(false) |
                               AllowLargeObjectAllocationFlag::encode(true)));
  TailCallRuntime(Runtime::kAllocateInOldGeneration, NoContextConstant(),
                  SmiFromIntPtr(requested_size), runtime_flags);
}

TF_BUILTIN(AllocateRegularInOldGeneration, CodeStubAssembler) {
  auto requested_size = UncheckedParameter<IntPtrT>(Descriptor::kRequestedSize);
  CSA_CHECK(this, IsValidPositiveSmi(requested_size));

  TNode<Smi> runtime_flags =
      SmiConstant(Smi::FromInt(AllocateDoubleAlignFlag::encode(false) |
                               AllowLargeObjectAllocationFlag::encode(false)));
  TailCallRuntime(Runtime::kAllocateInOldGeneration, NoContextConstant(),
                  SmiFromIntPtr(requested_size), runtime_flags);
}

TF_BUILTIN(Abort, CodeStubAssembler) {
  auto message_id = Parameter<Smi>(Descriptor::kMessageOrMessageId);
  TailCallRuntime(Runtime::kAbort, NoContextConstant(), message_id);
}

TF_BUILTIN(AbortCSAAssert, CodeStubAssembler) {
  auto message = Parameter<String>(Descriptor::kMessageOrMessageId);
  TailCallRuntime(Runtime::kAbortCSAAssert, NoContextConstant(), message);
}

void Builtins::Generate_CEntry_Return1_DontSaveFPRegs_ArgvOnStack_NoBuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 1, SaveFPRegsMode::kIgnore, ArgvMode::kStack, false);
}

void Builtins::Generate_CEntry_Return1_DontSaveFPRegs_ArgvOnStack_BuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 1, SaveFPRegsMode::kIgnore, ArgvMode::kStack, true);
}

void Builtins::
    Generate_CEntry_Return1_DontSaveFPRegs_ArgvInRegister_NoBuiltinExit(
        MacroAssembler* masm) {
  Generate_CEntry(masm, 1, SaveFPRegsMode::kIgnore, ArgvMode::kRegister, false);
}

void Builtins::Generate_CEntry_Return1_SaveFPRegs_ArgvOnStack_NoBuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 1, SaveFPRegsMode::kSave, ArgvMode::kStack, false);
}

void Builtins::Generate_CEntry_Return1_SaveFPRegs_ArgvOnStack_BuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 1, SaveFPRegsMode::kSave, ArgvMode::kStack, true);
}

void Builtins::Generate_CEntry_Return2_DontSaveFPRegs_ArgvOnStack_NoBuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 2, SaveFPRegsMode::kIgnore, ArgvMode::kStack, false);
}

void Builtins::Generate_CEntry_Return2_DontSaveFPRegs_ArgvOnStack_BuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 2, SaveFPRegsMode::kIgnore, ArgvMode::kStack, true);
}

void Builtins::
    Generate_CEntry_Return2_DontSaveFPRegs_ArgvInRegister_NoBuiltinExit(
        MacroAssembler* masm) {
  Generate_CEntry(masm, 2, SaveFPRegsMode::kIgnore, ArgvMode::kRegister, false);
}

void Builtins::Generate_CEntry_Return2_SaveFPRegs_ArgvOnStack_NoBuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 2, SaveFPRegsMode::kSave, ArgvMode::kStack, false);
}

void Builtins::Generate_CEntry_Return2_SaveFPRegs_ArgvOnStack_BuiltinExit(
    MacroAssembler* masm) {
  Generate_CEntry(masm, 2, SaveFPRegsMode::kSave, ArgvMode::kStack, true);
}

#if !defined(V8_TARGET_ARCH_ARM) && !defined(V8_TARGET_ARCH_MIPS)
void Builtins::Generate_MemCopyUint8Uint8(MacroAssembler* masm) {
  masm->Call(BUILTIN_CODE(masm->isolate(), Illegal), RelocInfo::CODE_TARGET);
}
#endif  // !defined(V8_TARGET_ARCH_ARM) && !defined(V8_TARGET_ARCH_MIPS)

#ifndef V8_TARGET_ARCH_IA32
void Builtins::Generate_MemMove(MacroAssembler* masm) {
  masm->Call(BUILTIN_CODE(masm->isolate(), Illegal), RelocInfo::CODE_TARGET);
}
#endif  // V8_TARGET_ARCH_IA32

// TODO(v8:11421): Remove #if once baseline compiler is ported to other
// architectures.
#if V8_TARGET_ARCH_IA32 || V8_TARGET_ARCH_X64 || V8_TARGET_ARCH_ARM64 || \
    V8_TARGET_ARCH_ARM || V8_TARGET_ARCH_RISCV64
void Builtins::Generate_BaselineLeaveFrame(MacroAssembler* masm) {
  EmitReturnBaseline(masm);
}
#else
// Stub out implementations of arch-specific baseline builtins.
void Builtins::Generate_BaselineOutOfLinePrologue(MacroAssembler* masm) {
  masm->Trap();
}
void Builtins::Generate_BaselineLeaveFrame(MacroAssembler* masm) {
  masm->Trap();
}
void Builtins::Generate_BaselineOnStackReplacement(MacroAssembler* masm) {
  masm->Trap();
}
void Builtins::Generate_TailCallOptimizedCodeSlot(MacroAssembler* masm) {
  masm->Trap();
}
#endif

// ES6 [[Get]] operation.
TF_BUILTIN(GetProperty, CodeStubAssembler) {
  auto object = Parameter<Object>(Descriptor::kObject);
  auto key = Parameter<Object>(Descriptor::kKey);
  auto context = Parameter<Context>(Descriptor::kContext);
  // TODO(duongn): consider tailcalling to GetPropertyWithReceiver(object,
  // object, key, OnNonExistent::kReturnUndefined).
  Label if_notfound(this), if_proxy(this, Label::kDeferred),
      if_slow(this, Label::kDeferred);

  CodeStubAssembler::LookupPropertyInHolder lookup_property_in_holder =
      [=](TNode<HeapObject> receiver, TNode<HeapObject> holder,
          TNode<Map> holder_map, TNode<Int32T> holder_instance_type,
          TNode<Name> unique_name, Label* next_holder, Label* if_bailout) {
        TVARIABLE(Object, var_value);
        Label if_found(this);
        TryGetOwnProperty(context, receiver, CAST(holder), holder_map,
                          holder_instance_type, unique_name, &if_found,
                          &var_value, next_holder, if_bailout);
        BIND(&if_found);
        Return(var_value.value());
      };

  CodeStubAssembler::LookupElementInHolder lookup_element_in_holder =
      [=](TNode<HeapObject> receiver, TNode<HeapObject> holder,
          TNode<Map> holder_map, TNode<Int32T> holder_instance_type,
          TNode<IntPtrT> index, Label* next_holder, Label* if_bailout) {
        // Not supported yet.
        Use(next_holder);
        Goto(if_bailout);
      };

  TryPrototypeChainLookup(object, object, key, lookup_property_in_holder,
                          lookup_element_in_holder, &if_notfound, &if_slow,
                          &if_proxy);

  BIND(&if_notfound);
  Return(UndefinedConstant());

  BIND(&if_slow);
  TailCallRuntime(Runtime::kGetProperty, context, object, key);

  BIND(&if_proxy);
  {
    // Convert the {key} to a Name first.
    TNode<Object> name = CallBuiltin(Builtins::kToName, context, key);

    // The {object} is a JSProxy instance, look up the {name} on it, passing
    // {object} both as receiver and holder. If {name} is absent we can safely
    // return undefined from here.
    TailCallBuiltin(Builtins::kProxyGetProperty, context, object, name, object,
                    SmiConstant(OnNonExistent::kReturnUndefined));
  }
}

// ES6 [[Get]] operation with Receiver.
TF_BUILTIN(GetPropertyWithReceiver, CodeStubAssembler) {
  auto object = Parameter<Object>(Descriptor::kObject);
  auto key = Parameter<Object>(Descriptor::kKey);
  auto context = Parameter<Context>(Descriptor::kContext);
  auto receiver = Parameter<Object>(Descriptor::kReceiver);
  auto on_non_existent = Parameter<Object>(Descriptor::kOnNonExistent);
  Label if_notfound(this), if_proxy(this, Label::kDeferred),
      if_slow(this, Label::kDeferred);

  CodeStubAssembler::LookupPropertyInHolder lookup_property_in_holder =
      [=](TNode<HeapObject> receiver, TNode<HeapObject> holder,
          TNode<Map> holder_map, TNode<Int32T> holder_instance_type,
          TNode<Name> unique_name, Label* next_holder, Label* if_bailout) {
        TVARIABLE(Object, var_value);
        Label if_found(this);
        TryGetOwnProperty(context, receiver, CAST(holder), holder_map,
                          holder_instance_type, unique_name, &if_found,
                          &var_value, next_holder, if_bailout);
        BIND(&if_found);
        Return(var_value.value());
      };

  CodeStubAssembler::LookupElementInHolder lookup_element_in_holder =
      [=](TNode<HeapObject> receiver, TNode<HeapObject> holder,
          TNode<Map> holder_map, TNode<Int32T> holder_instance_type,
          TNode<IntPtrT> index, Label* next_holder, Label* if_bailout) {
        // Not supported yet.
        Use(next_holder);
        Goto(if_bailout);
      };

  TryPrototypeChainLookup(receiver, object, key, lookup_property_in_holder,
                          lookup_element_in_holder, &if_notfound, &if_slow,
                          &if_proxy);

  BIND(&if_notfound);
  Label throw_reference_error(this);
  GotoIf(TaggedEqual(on_non_existent,
                     SmiConstant(OnNonExistent::kThrowReferenceError)),
         &throw_reference_error);
  CSA_ASSERT(this, TaggedEqual(on_non_existent,
                               SmiConstant(OnNonExistent::kReturnUndefined)));
  Return(UndefinedConstant());

  BIND(&throw_reference_error);
  Return(CallRuntime(Runtime::kThrowReferenceError, context, key));

  BIND(&if_slow);
  TailCallRuntime(Runtime::kGetPropertyWithReceiver, context, object, key,
                  receiver, on_non_existent);

  BIND(&if_proxy);
  {
    // Convert the {key} to a Name first.
    TNode<Name> name = CAST(CallBuiltin(Builtins::kToName, context, key));

    // Proxy cannot handle private symbol so bailout.
    GotoIf(IsPrivateSymbol(name), &if_slow);

    // The {object} is a JSProxy instance, look up the {name} on it, passing
    // {object} both as receiver and holder. If {name} is absent we can safely
    // return undefined from here.
    TailCallBuiltin(Builtins::kProxyGetProperty, context, object, name,
                    receiver, on_non_existent);
  }
}

// ES6 [[Set]] operation.
TF_BUILTIN(SetProperty, CodeStubAssembler) {
  auto context = Parameter<Context>(Descriptor::kContext);
  auto receiver = Parameter<Object>(Descriptor::kReceiver);
  auto key = Parameter<Object>(Descriptor::kKey);
  auto value = Parameter<Object>(Descriptor::kValue);

  KeyedStoreGenericGenerator::SetProperty(state(), context, receiver, key,
                                          value, LanguageMode::kStrict);
}

// ES6 CreateDataProperty(), specialized for the case where objects are still
// being initialized, and have not yet been made accessible to the user. Thus,
// any operation here should be unobservable until after the object has been
// returned.
TF_BUILTIN(SetPropertyInLiteral, CodeStubAssembler) {
  auto context = Parameter<Context>(Descriptor::kContext);
  auto receiver = Parameter<JSObject>(Descriptor::kReceiver);
  auto key = Parameter<Object>(Descriptor::kKey);
  auto value = Parameter<Object>(Descriptor::kValue);

  KeyedStoreGenericGenerator::SetPropertyInLiteral(state(), context, receiver,
                                                   key, value);
}

TF_BUILTIN(InstantiateAsmJs, CodeStubAssembler) {
  Label tailcall_to_function(this);
  auto context = Parameter<Context>(Descriptor::kContext);
  auto new_target = Parameter<Object>(Descriptor::kNewTarget);
  auto arg_count =
      UncheckedParameter<Int32T>(Descriptor::kActualArgumentsCount);
  auto function = Parameter<JSFunction>(Descriptor::kTarget);

  // Retrieve arguments from caller (stdlib, foreign, heap).
  CodeStubArguments args(this, arg_count);
  TNode<Object> stdlib = args.GetOptionalArgumentValue(0);
  TNode<Object> foreign = args.GetOptionalArgumentValue(1);
  TNode<Object> heap = args.GetOptionalArgumentValue(2);

  // Call runtime, on success just pass the result to the caller and pop all
  // arguments. A smi 0 is returned on failure, an object on success.
  TNode<Object> maybe_result_or_smi_zero = CallRuntime(
      Runtime::kInstantiateAsmJs, context, function, stdlib, foreign, heap);
  GotoIf(TaggedIsSmi(maybe_result_or_smi_zero), &tailcall_to_function);

  TNode<SharedFunctionInfo> shared = LoadJSFunctionSharedFunctionInfo(function);
  TNode<Int32T> parameter_count =
      UncheckedCast<Int32T>(LoadSharedFunctionInfoFormalParameterCount(shared));
  // This builtin intercepts a call to {function}, where the number of arguments
  // pushed is the maximum of actual arguments count and formal parameters
  // count.
  Label argc_lt_param_count(this), argc_ge_param_count(this);
  Branch(IntPtrLessThan(args.GetLength(), ChangeInt32ToIntPtr(parameter_count)),
         &argc_lt_param_count, &argc_ge_param_count);
  BIND(&argc_lt_param_count);
  PopAndReturn(Int32Add(parameter_count, Int32Constant(1)),
               maybe_result_or_smi_zero);
  BIND(&argc_ge_param_count);
  args.PopAndReturn(maybe_result_or_smi_zero);

  BIND(&tailcall_to_function);
  // On failure, tail call back to regular JavaScript by re-calling the given
  // function which has been reset to the compile lazy builtin.
  TNode<Code> code = CAST(LoadObjectField(function, JSFunction::kCodeOffset));
  TailCallJSCode(code, context, function, new_target, arg_count);
}

}  // namespace internal
}  // namespace v8
