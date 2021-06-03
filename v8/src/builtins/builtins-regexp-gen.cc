// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-regexp-gen.h"

#include "src/builtins/builtins-constructor-gen.h"
#include "src/builtins/builtins-utils-gen.h"
#include "src/builtins/builtins.h"
#include "src/builtins/growable-fixed-array-gen.h"
#include "src/codegen/code-factory.h"
#include "src/codegen/code-stub-assembler.h"
#include "src/codegen/macro-assembler.h"
#include "src/common/globals.h"
#include "src/execution/protectors.h"
#include "src/heap/factory-inl.h"
#include "src/logging/counters.h"
#include "src/objects/js-regexp-string-iterator.h"
#include "src/objects/js-regexp.h"
#include "src/objects/regexp-match-info.h"

namespace v8 {
namespace internal {

// Tail calls the regular expression interpreter.
// static
void Builtins::Generate_RegExpInterpreterTrampoline(MacroAssembler* masm) {
  ExternalReference interpreter_code_entry =
      ExternalReference::re_match_for_call_from_js();
  masm->Jump(interpreter_code_entry);
}

// Tail calls the experimental regular expression engine.
// static
void Builtins::Generate_RegExpExperimentalTrampoline(MacroAssembler* masm) {
  ExternalReference interpreter_code_entry =
      ExternalReference::re_experimental_match_for_call_from_js();
  masm->Jump(interpreter_code_entry);
}

TNode<Smi> RegExpBuiltinsAssembler::SmiZero() { return SmiConstant(0); }

TNode<IntPtrT> RegExpBuiltinsAssembler::IntPtrZero() {
  return IntPtrConstant(0);
}

// If code is a builtin, return the address to the (possibly embedded) builtin
// code entry, otherwise return the entry of the code object itself.
TNode<RawPtrT> RegExpBuiltinsAssembler::LoadCodeObjectEntry(TNode<Code> code) {
  TVARIABLE(RawPtrT, var_result);

  Label if_code_is_off_heap(this), out(this);
  TNode<Int32T> builtin_index =
      LoadObjectField<Int32T>(code, Code::kBuiltinIndexOffset);
  {
    GotoIfNot(Word32Equal(builtin_index, Int32Constant(Builtins::kNoBuiltinId)),
              &if_code_is_off_heap);
    var_result = ReinterpretCast<RawPtrT>(
        IntPtrAdd(BitcastTaggedToWord(code),
                  IntPtrConstant(Code::kHeaderSize - kHeapObjectTag)));
    Goto(&out);
  }

  BIND(&if_code_is_off_heap);
  {
    TNode<IntPtrT> builtin_entry_offset_from_isolate_root =
        IntPtrAdd(IntPtrConstant(IsolateData::builtin_entry_table_offset()),
                  ChangeInt32ToIntPtr(Word32Shl(
                      builtin_index, Int32Constant(kSystemPointerSizeLog2))));

    var_result = ReinterpretCast<RawPtrT>(
        Load(MachineType::Pointer(),
             ExternalConstant(ExternalReference::isolate_root(isolate())),
             builtin_entry_offset_from_isolate_root));
    Goto(&out);
  }

  BIND(&out);
  return var_result.value();
}

// -----------------------------------------------------------------------------
// ES6 section 21.2 RegExp Objects

TNode<JSRegExpResult> RegExpBuiltinsAssembler::AllocateRegExpResult(
    TNode<Context> context, TNode<Smi> length, TNode<Smi> index,
    TNode<String> input, TNode<JSRegExp> regexp, TNode<Number> last_index,
    TNode<BoolT> has_indices, TNode<FixedArray>* elements_out) {
  CSA_ASSERT(this, SmiLessThanOrEqual(
                       length, SmiConstant(JSArray::kMaxFastArrayLength)));
  CSA_ASSERT(this, SmiGreaterThan(length, SmiConstant(0)));

  // Allocate.

  Label result_has_indices(this), allocated(this);
  const ElementsKind elements_kind = PACKED_ELEMENTS;
  base::Optional<TNode<AllocationSite>> no_gc_site = base::nullopt;
  TNode<IntPtrT> length_intptr = SmiUntag(length);
  // Note: The returned `var_elements` may be in young large object space, but
  // `var_array` is guaranteed to be in new space so we could skip write
  // barriers below.
  TVARIABLE(JSArray, var_array);
  TVARIABLE(FixedArrayBase, var_elements);

  GotoIf(has_indices, &result_has_indices);
  {
    TNode<Map> map = CAST(LoadContextElement(LoadNativeContext(context),
                                             Context::REGEXP_RESULT_MAP_INDEX));
    std::tie(var_array, var_elements) =
        AllocateUninitializedJSArrayWithElements(
            elements_kind, map, length, no_gc_site, length_intptr,
            kAllowLargeObjectAllocation, JSRegExpResult::kSize);
    Goto(&allocated);
  }

  BIND(&result_has_indices);
  {
    TNode<Map> map =
        CAST(LoadContextElement(LoadNativeContext(context),
                                Context::REGEXP_RESULT_WITH_INDICES_MAP_INDEX));
    std::tie(var_array, var_elements) =
        AllocateUninitializedJSArrayWithElements(
            elements_kind, map, length, no_gc_site, length_intptr,
            kAllowLargeObjectAllocation, JSRegExpResultWithIndices::kSize);
    Goto(&allocated);
  }

  BIND(&allocated);

  // Finish result initialization.

  TNode<JSRegExpResult> result =
      UncheckedCast<JSRegExpResult>(var_array.value());

  // Load undefined value once here to avoid multiple LoadRoots.
  TNode<Oddball> undefined_value = UncheckedCast<Oddball>(
      CodeAssembler::LoadRoot(RootIndex::kUndefinedValue));

  StoreObjectFieldNoWriteBarrier(result, JSRegExpResult::kIndexOffset, index);
  // TODO(jgruber,turbofan): Could skip barrier but the MemoryOptimizer
  // complains.
  StoreObjectField(result, JSRegExpResult::kInputOffset, input);
  StoreObjectFieldNoWriteBarrier(result, JSRegExpResult::kGroupsOffset,
                                 undefined_value);
  StoreObjectFieldNoWriteBarrier(result, JSRegExpResult::kNamesOffset,
                                 undefined_value);

  StoreObjectField(result, JSRegExpResult::kRegexpInputOffset, input);

  // If non-smi last_index then store an SmiZero instead.
  {
    TNode<Smi> last_index_smi = Select<Smi>(
        TaggedIsSmi(last_index), [=] { return CAST(last_index); },
        [=] { return SmiZero(); });
    StoreObjectField(result, JSRegExpResult::kRegexpLastIndexOffset,
                     last_index_smi);
  }

  Label finish_initialization(this);
  GotoIfNot(has_indices, &finish_initialization);
  {
    static_assert(
        std::is_base_of<JSRegExpResult, JSRegExpResultWithIndices>::value,
        "JSRegExpResultWithIndices is a subclass of JSRegExpResult");
    StoreObjectFieldNoWriteBarrier(
        result, JSRegExpResultWithIndices::kIndicesOffset, undefined_value);
    Goto(&finish_initialization);
  }

  BIND(&finish_initialization);

  // Finish elements initialization.

  FillFixedArrayWithValue(elements_kind, var_elements.value(), IntPtrZero(),
                          length_intptr, RootIndex::kUndefinedValue);

  if (elements_out) *elements_out = CAST(var_elements.value());
  return result;
}

TNode<Object> RegExpBuiltinsAssembler::FastLoadLastIndexBeforeSmiCheck(
    TNode<JSRegExp> regexp) {
  // Load the in-object field.
  static const int field_offset =
      JSRegExp::kHeaderSize + JSRegExp::kLastIndexFieldIndex * kTaggedSize;
  return LoadObjectField(regexp, field_offset);
}

TNode<Object> RegExpBuiltinsAssembler::SlowLoadLastIndex(TNode<Context> context,
                                                         TNode<Object> regexp) {
  return GetProperty(context, regexp, isolate()->factory()->lastIndex_string());
}

// The fast-path of StoreLastIndex when regexp is guaranteed to be an unmodified
// JSRegExp instance.
void RegExpBuiltinsAssembler::FastStoreLastIndex(TNode<JSRegExp> regexp,
                                                 TNode<Smi> value) {
  // Store the in-object field.
  static const int field_offset =
      JSRegExp::kHeaderSize + JSRegExp::kLastIndexFieldIndex * kTaggedSize;
  StoreObjectField(regexp, field_offset, value);
}

void RegExpBuiltinsAssembler::SlowStoreLastIndex(TNode<Context> context,
                                                 TNode<Object> regexp,
                                                 TNode<Object> value) {
  TNode<String> name = HeapConstant(isolate()->factory()->lastIndex_string());
  SetPropertyStrict(context, regexp, name, value);
}

TNode<JSRegExpResult> RegExpBuiltinsAssembler::ConstructNewResultFromMatchInfo(
    TNode<Context> context, TNode<JSRegExp> regexp,
    TNode<RegExpMatchInfo> match_info, TNode<String> string,
    TNode<Number> last_index) {
  Label named_captures(this), maybe_build_indices(this), out(this);

  TNode<IntPtrT> num_indices = SmiUntag(CAST(UnsafeLoadFixedArrayElement(
      match_info, RegExpMatchInfo::kNumberOfCapturesIndex)));
  TNode<Smi> num_results = SmiTag(WordShr(num_indices, 1));
  TNode<Smi> start = CAST(UnsafeLoadFixedArrayElement(
      match_info, RegExpMatchInfo::kFirstCaptureIndex));
  TNode<Smi> end = CAST(UnsafeLoadFixedArrayElement(
      match_info, RegExpMatchInfo::kFirstCaptureIndex + 1));

  // Calculate the substring of the first match before creating the result array
  // to avoid an unnecessary write barrier storing the first result.

  TNode<String> first =
      CAST(CallBuiltin(Builtins::kSubString, context, string, start, end));

  // Load flags and check if the result object needs to have indices.
  const TNode<Smi> flags =
      CAST(LoadObjectField(regexp, JSRegExp::kFlagsOffset));
  const TNode<BoolT> has_indices = IsSetSmi(flags, JSRegExp::kHasIndices);
  TNode<FixedArray> result_elements;
  TNode<JSRegExpResult> result =
      AllocateRegExpResult(context, num_results, start, string, regexp,
                           last_index, has_indices, &result_elements);

  UnsafeStoreFixedArrayElement(result_elements, 0, first);

  // If no captures exist we can skip named capture handling as well.
  GotoIf(SmiEqual(num_results, SmiConstant(1)), &maybe_build_indices);

  // Store all remaining captures.
  TNode<IntPtrT> limit = IntPtrAdd(
      IntPtrConstant(RegExpMatchInfo::kFirstCaptureIndex), num_indices);

  TVARIABLE(IntPtrT, var_from_cursor,
            IntPtrConstant(RegExpMatchInfo::kFirstCaptureIndex + 2));
  TVARIABLE(IntPtrT, var_to_cursor, IntPtrConstant(1));

  Label loop(this, {&var_from_cursor, &var_to_cursor});

  Goto(&loop);
  BIND(&loop);
  {
    TNode<IntPtrT> from_cursor = var_from_cursor.value();
    TNode<IntPtrT> to_cursor = var_to_cursor.value();
    TNode<Smi> start =
        CAST(UnsafeLoadFixedArrayElement(match_info, from_cursor));

    Label next_iter(this);
    GotoIf(SmiEqual(start, SmiConstant(-1)), &next_iter);

    TNode<IntPtrT> from_cursor_plus1 =
        IntPtrAdd(from_cursor, IntPtrConstant(1));
    TNode<Smi> end =
        CAST(UnsafeLoadFixedArrayElement(match_info, from_cursor_plus1));

    TNode<String> capture =
        CAST(CallBuiltin(Builtins::kSubString, context, string, start, end));
    UnsafeStoreFixedArrayElement(result_elements, to_cursor, capture);
    Goto(&next_iter);

    BIND(&next_iter);
    var_from_cursor = IntPtrAdd(from_cursor, IntPtrConstant(2));
    var_to_cursor = IntPtrAdd(to_cursor, IntPtrConstant(1));
    Branch(UintPtrLessThan(var_from_cursor.value(), limit), &loop,
           &named_captures);
  }

  BIND(&named_captures);
  {
    CSA_ASSERT(this, SmiGreaterThan(num_results, SmiConstant(1)));

    // Preparations for named capture properties. Exit early if the result does
    // not have any named captures to minimize performance impact.

    TNode<FixedArray> data =
        CAST(LoadObjectField(regexp, JSRegExp::kDataOffset));

    // We reach this point only if captures exist, implying that the assigned
    // regexp engine must be able to handle captures.
    CSA_ASSERT(
        this,
        Word32Or(
            SmiEqual(CAST(LoadFixedArrayElement(data, JSRegExp::kTagIndex)),
                     SmiConstant(JSRegExp::IRREGEXP)),
            SmiEqual(CAST(LoadFixedArrayElement(data, JSRegExp::kTagIndex)),
                     SmiConstant(JSRegExp::EXPERIMENTAL))));

    // The names fixed array associates names at even indices with a capture
    // index at odd indices.
    TNode<Object> maybe_names =
        LoadFixedArrayElement(data, JSRegExp::kIrregexpCaptureNameMapIndex);
    GotoIf(TaggedEqual(maybe_names, SmiZero()), &maybe_build_indices);

    // One or more named captures exist, add a property for each one.

    TNode<FixedArray> names = CAST(maybe_names);
    TNode<IntPtrT> names_length = LoadAndUntagFixedArrayBaseLength(names);
    CSA_ASSERT(this, IntPtrGreaterThan(names_length, IntPtrZero()));

    // Stash names in case we need them to build the indices array later.
    StoreObjectField(result, JSRegExpResult::kNamesOffset, names);

    // Allocate a new object to store the named capture properties.
    // TODO(jgruber): Could be optimized by adding the object map to the heap
    // root list.

    TNode<IntPtrT> num_properties = WordSar(names_length, 1);
    TNode<NativeContext> native_context = LoadNativeContext(context);
    TNode<Map> map = LoadSlowObjectWithNullPrototypeMap(native_context);
    TNode<HeapObject> properties;
    if (V8_ENABLE_SWISS_NAME_DICTIONARY_BOOL) {
      properties = AllocateSwissNameDictionary(num_properties);
    } else {
      properties =
          AllocateNameDictionary(num_properties, kAllowLargeObjectAllocation);
    }

    TNode<JSObject> group_object = AllocateJSObjectFromMap(map, properties);
    StoreObjectField(result, JSRegExpResult::kGroupsOffset, group_object);

    TVARIABLE(IntPtrT, var_i, IntPtrZero());

    Label loop(this, &var_i);

    Goto(&loop);
    BIND(&loop);
    {
      TNode<IntPtrT> i = var_i.value();
      TNode<IntPtrT> i_plus_1 = IntPtrAdd(i, IntPtrConstant(1));
      TNode<IntPtrT> i_plus_2 = IntPtrAdd(i_plus_1, IntPtrConstant(1));

      TNode<String> name = CAST(LoadFixedArrayElement(names, i));
      TNode<Smi> index = CAST(LoadFixedArrayElement(names, i_plus_1));
      TNode<HeapObject> capture =
          CAST(LoadFixedArrayElement(result_elements, SmiUntag(index)));

      // TODO(v8:8213): For maintainability, we should call a CSA/Torque
      // implementation of CreateDataProperty instead.

      // At this point the spec says to call CreateDataProperty. However, we can
      // skip most of the steps and go straight to adding a dictionary entry
      // because we know a bunch of useful facts:
      // - All keys are non-numeric internalized strings
      // - No keys repeat
      // - Receiver has no prototype
      // - Receiver isn't used as a prototype
      // - Receiver isn't any special object like a Promise intrinsic object
      // - Receiver is extensible
      // - Receiver has no interceptors
      Label add_dictionary_property_slow(this, Label::kDeferred);
      Add<PropertyDictionary>(CAST(properties), name, capture,
                              &add_dictionary_property_slow);

      var_i = i_plus_2;
      Branch(IntPtrGreaterThanOrEqual(var_i.value(), names_length),
             &maybe_build_indices, &loop);

      BIND(&add_dictionary_property_slow);
      // If the dictionary needs resizing, the above Add call will jump here
      // before making any changes. This shouldn't happen because we allocated
      // the dictionary with enough space above.
      Unreachable();
    }
  }

  // Build indices if needed (i.e. if the /d flag is present) after named
  // capture groups are processed.
  BIND(&maybe_build_indices);
  GotoIfNot(has_indices, &out);
  {
    const TNode<Object> maybe_names =
        LoadObjectField(result, JSRegExpResultWithIndices::kNamesOffset);
    const TNode<JSRegExpResultIndices> indices =
        UncheckedCast<JSRegExpResultIndices>(
            CallRuntime(Runtime::kRegExpBuildIndices, context, regexp,
                        match_info, maybe_names));
    StoreObjectField(result, JSRegExpResultWithIndices::kIndicesOffset,
                     indices);
    Goto(&out);
  }

  BIND(&out);
  return result;
}

void RegExpBuiltinsAssembler::GetStringPointers(
    TNode<RawPtrT> string_data, TNode<IntPtrT> offset,
    TNode<IntPtrT> last_index, TNode<IntPtrT> string_length,
    String::Encoding encoding, TVariable<RawPtrT>* var_string_start,
    TVariable<RawPtrT>* var_string_end) {
  DCHECK_EQ(var_string_start->rep(), MachineType::PointerRepresentation());
  DCHECK_EQ(var_string_end->rep(), MachineType::PointerRepresentation());

  const ElementsKind kind = (encoding == String::ONE_BYTE_ENCODING)
                                ? UINT8_ELEMENTS
                                : UINT16_ELEMENTS;

  TNode<IntPtrT> from_offset =
      ElementOffsetFromIndex(IntPtrAdd(offset, last_index), kind);
  *var_string_start =
      ReinterpretCast<RawPtrT>(IntPtrAdd(string_data, from_offset));

  TNode<IntPtrT> to_offset =
      ElementOffsetFromIndex(IntPtrAdd(offset, string_length), kind);
  *var_string_end = ReinterpretCast<RawPtrT>(IntPtrAdd(string_data, to_offset));
}

TNode<HeapObject> RegExpBuiltinsAssembler::RegExpExecInternal(
    TNode<Context> context, TNode<JSRegExp> regexp, TNode<String> string,
    TNode<Number> last_index, TNode<RegExpMatchInfo> match_info,
    RegExp::ExecQuirks exec_quirks) {
  ToDirectStringAssembler to_direct(state(), string);

  TVARIABLE(HeapObject, var_result);
  Label out(this), atom(this), runtime(this, Label::kDeferred),
      retry_experimental(this, Label::kDeferred);

  // External constants.
  TNode<ExternalReference> isolate_address =
      ExternalConstant(ExternalReference::isolate_address(isolate()));
  TNode<ExternalReference> regexp_stack_memory_top_address = ExternalConstant(
      ExternalReference::address_of_regexp_stack_memory_top_address(isolate()));
  TNode<ExternalReference> static_offsets_vector_address = ExternalConstant(
      ExternalReference::address_of_static_offsets_vector(isolate()));

  // At this point, last_index is definitely a canonicalized non-negative
  // number, which implies that any non-Smi last_index is greater than
  // the maximal string length. If lastIndex > string.length then the matcher
  // must fail.

  Label if_failure(this);

  CSA_ASSERT(this, IsNumberNormalized(last_index));
  CSA_ASSERT(this, IsNumberPositive(last_index));
  GotoIf(TaggedIsNotSmi(last_index), &if_failure);

  TNode<IntPtrT> int_string_length = LoadStringLengthAsWord(string);
  TNode<IntPtrT> int_last_index = SmiUntag(CAST(last_index));

  GotoIf(UintPtrGreaterThan(int_last_index, int_string_length), &if_failure);

  // Since the RegExp has been compiled, data contains a fixed array.
  TNode<FixedArray> data = CAST(LoadObjectField(regexp, JSRegExp::kDataOffset));
  {
    // Dispatch on the type of the RegExp.
    {
      Label next(this), unreachable(this, Label::kDeferred);
      TNode<Int32T> tag = LoadAndUntagToWord32FixedArrayElement(
          data, IntPtrConstant(JSRegExp::kTagIndex));

      int32_t values[] = {
          JSRegExp::IRREGEXP,
          JSRegExp::ATOM,
          JSRegExp::EXPERIMENTAL,
      };
      Label* labels[] = {&next, &atom, &next};

      STATIC_ASSERT(arraysize(values) == arraysize(labels));
      Switch(tag, &unreachable, values, labels, arraysize(values));

      BIND(&unreachable);
      Unreachable();

      BIND(&next);
    }

    // Check (number_of_captures + 1) * 2 <= offsets vector size
    // Or              number_of_captures <= offsets vector size / 2 - 1
    TNode<Smi> capture_count = CAST(UnsafeLoadFixedArrayElement(
        data, JSRegExp::kIrregexpCaptureCountIndex));

    const int kOffsetsSize = Isolate::kJSRegexpStaticOffsetsVectorSize;
    STATIC_ASSERT(kOffsetsSize >= 2);
    GotoIf(SmiAbove(capture_count, SmiConstant(kOffsetsSize / 2 - 1)),
           &runtime);
  }

  // Unpack the string if possible.

  to_direct.TryToDirect(&runtime);

  // Load the irregexp code or bytecode object and offsets into the subject
  // string. Both depend on whether the string is one- or two-byte.

  TVARIABLE(RawPtrT, var_string_start);
  TVARIABLE(RawPtrT, var_string_end);
  TVARIABLE(Object, var_code);
  TVARIABLE(Object, var_bytecode);

  {
    TNode<RawPtrT> direct_string_data = to_direct.PointerToData(&runtime);

    Label next(this), if_isonebyte(this), if_istwobyte(this, Label::kDeferred);
    Branch(IsOneByteStringInstanceType(to_direct.instance_type()),
           &if_isonebyte, &if_istwobyte);

    BIND(&if_isonebyte);
    {
      GetStringPointers(direct_string_data, to_direct.offset(), int_last_index,
                        int_string_length, String::ONE_BYTE_ENCODING,
                        &var_string_start, &var_string_end);
      var_code =
          UnsafeLoadFixedArrayElement(data, JSRegExp::kIrregexpLatin1CodeIndex);
      var_bytecode = UnsafeLoadFixedArrayElement(
          data, JSRegExp::kIrregexpLatin1BytecodeIndex);
      Goto(&next);
    }

    BIND(&if_istwobyte);
    {
      GetStringPointers(direct_string_data, to_direct.offset(), int_last_index,
                        int_string_length, String::TWO_BYTE_ENCODING,
                        &var_string_start, &var_string_end);
      var_code =
          UnsafeLoadFixedArrayElement(data, JSRegExp::kIrregexpUC16CodeIndex);
      var_bytecode = UnsafeLoadFixedArrayElement(
          data, JSRegExp::kIrregexpUC16BytecodeIndex);
      Goto(&next);
    }

    BIND(&next);
  }

  // Check that the irregexp code has been generated for the actual string
  // encoding. If it has, the field contains a code object; and otherwise it
  // contains the uninitialized sentinel as a smi.
#ifdef DEBUG
  {
    Label next(this);
    GotoIfNot(TaggedIsSmi(var_code.value()), &next);
    CSA_ASSERT(this, SmiEqual(CAST(var_code.value()),
                              SmiConstant(JSRegExp::kUninitializedValue)));
    Goto(&next);
    BIND(&next);
  }
#endif

  GotoIf(TaggedIsSmi(var_code.value()), &runtime);
  TNode<Code> code = CAST(var_code.value());

  Label if_success(this), if_exception(this, Label::kDeferred);
  {
    IncrementCounter(isolate()->counters()->regexp_entry_native(), 1);

    // Set up args for the final call into generated Irregexp code.

    MachineType type_int32 = MachineType::Int32();
    MachineType type_tagged = MachineType::AnyTagged();
    MachineType type_ptr = MachineType::Pointer();

    // Result: A NativeRegExpMacroAssembler::Result return code.
    MachineType retval_type = type_int32;

    // Argument 0: Original subject string.
    MachineType arg0_type = type_tagged;
    TNode<String> arg0 = string;

    // Argument 1: Previous index.
    MachineType arg1_type = type_int32;
    TNode<Int32T> arg1 = TruncateIntPtrToInt32(int_last_index);

    // Argument 2: Start of string data. This argument is ignored in the
    // interpreter.
    MachineType arg2_type = type_ptr;
    TNode<RawPtrT> arg2 = var_string_start.value();

    // Argument 3: End of string data. This argument is ignored in the
    // interpreter.
    MachineType arg3_type = type_ptr;
    TNode<RawPtrT> arg3 = var_string_end.value();

    // Argument 4: static offsets vector buffer.
    MachineType arg4_type = type_ptr;
    TNode<ExternalReference> arg4 = static_offsets_vector_address;

    // Argument 5: Number of capture registers.
    // Setting this to the number of registers required to store all captures
    // forces global regexps to behave as non-global.
    TNode<Smi> capture_count = CAST(UnsafeLoadFixedArrayElement(
        data, JSRegExp::kIrregexpCaptureCountIndex));
    // capture_count is the number of captures without the match itself.
    // Required registers = (capture_count + 1) * 2.
    STATIC_ASSERT(Internals::IsValidSmi((JSRegExp::kMaxCaptures + 1) * 2));
    TNode<Smi> register_count =
        SmiShl(SmiAdd(capture_count, SmiConstant(1)), 1);

    MachineType arg5_type = type_int32;
    TNode<Int32T> arg5 = SmiToInt32(register_count);

    // Argument 6: Start (high end) of backtracking stack memory area. This
    // argument is ignored in the interpreter.
    TNode<RawPtrT> stack_top = UncheckedCast<RawPtrT>(
        Load(MachineType::Pointer(), regexp_stack_memory_top_address));

    MachineType arg6_type = type_ptr;
    TNode<RawPtrT> arg6 = stack_top;

    // Argument 7: Indicate that this is a direct call from JavaScript.
    MachineType arg7_type = type_int32;
    TNode<Int32T> arg7 = Int32Constant(RegExp::CallOrigin::kFromJs);

    // Argument 8: Pass current isolate address.
    MachineType arg8_type = type_ptr;
    TNode<ExternalReference> arg8 = isolate_address;

    // Argument 9: Regular expression object. This argument is ignored in native
    // irregexp code.
    MachineType arg9_type = type_tagged;
    TNode<JSRegExp> arg9 = regexp;

    TNode<RawPtrT> code_entry = LoadCodeObjectEntry(code);

    // AIX uses function descriptors on CFunction calls. code_entry in this case
    // may also point to a Regex interpreter entry trampoline which does not
    // have a function descriptor. This method is ineffective on other platforms
    // and is equivalent to CallCFunction.
    TNode<Int32T> result =
        UncheckedCast<Int32T>(CallCFunctionWithoutFunctionDescriptor(
            code_entry, retval_type, std::make_pair(arg0_type, arg0),
            std::make_pair(arg1_type, arg1), std::make_pair(arg2_type, arg2),
            std::make_pair(arg3_type, arg3), std::make_pair(arg4_type, arg4),
            std::make_pair(arg5_type, arg5), std::make_pair(arg6_type, arg6),
            std::make_pair(arg7_type, arg7), std::make_pair(arg8_type, arg8),
            std::make_pair(arg9_type, arg9)));

    // Check the result.
    // We expect exactly one result since we force the called regexp to behave
    // as non-global.
    TNode<IntPtrT> int_result = ChangeInt32ToIntPtr(result);
    GotoIf(
        IntPtrEqual(int_result, IntPtrConstant(RegExp::kInternalRegExpSuccess)),
        &if_success);
    GotoIf(
        IntPtrEqual(int_result, IntPtrConstant(RegExp::kInternalRegExpFailure)),
        &if_failure);
    GotoIf(IntPtrEqual(int_result,
                       IntPtrConstant(RegExp::kInternalRegExpException)),
           &if_exception);
    GotoIf(IntPtrEqual(
               int_result,
               IntPtrConstant(RegExp::kInternalRegExpFallbackToExperimental)),
           &retry_experimental);

    CSA_ASSERT(this, IntPtrEqual(int_result,
                                 IntPtrConstant(RegExp::kInternalRegExpRetry)));
    Goto(&runtime);
  }

  BIND(&if_success);
  {
    if (exec_quirks == RegExp::ExecQuirks::kTreatMatchAtEndAsFailure) {
      static constexpr int kMatchStartOffset = 0;
      TNode<IntPtrT> value = ChangeInt32ToIntPtr(UncheckedCast<Int32T>(
          Load(MachineType::Int32(), static_offsets_vector_address,
               IntPtrConstant(kMatchStartOffset))));
      GotoIf(UintPtrGreaterThanOrEqual(value, int_string_length), &if_failure);
    }

    // Check that the last match info has space for the capture registers and
    // the additional information. Ensure no overflow in add.
    STATIC_ASSERT(FixedArray::kMaxLength < kMaxInt - FixedArray::kLengthOffset);
    TNode<Smi> available_slots =
        SmiSub(LoadFixedArrayBaseLength(match_info),
               SmiConstant(RegExpMatchInfo::kLastMatchOverhead));
    TNode<Smi> capture_count = CAST(UnsafeLoadFixedArrayElement(
        data, JSRegExp::kIrregexpCaptureCountIndex));
    // Calculate number of register_count = (capture_count + 1) * 2.
    TNode<Smi> register_count =
        SmiShl(SmiAdd(capture_count, SmiConstant(1)), 1);
    GotoIf(SmiGreaterThan(register_count, available_slots), &runtime);

    // Fill match_info.
    UnsafeStoreFixedArrayElement(
        match_info, RegExpMatchInfo::kNumberOfCapturesIndex, register_count);
    UnsafeStoreFixedArrayElement(match_info, RegExpMatchInfo::kLastSubjectIndex,
                                 string);
    UnsafeStoreFixedArrayElement(match_info, RegExpMatchInfo::kLastInputIndex,
                                 string);

    // Fill match and capture offsets in match_info.
    {
      TNode<IntPtrT> limit_offset =
          ElementOffsetFromIndex(register_count, INT32_ELEMENTS, 0);

      TNode<IntPtrT> to_offset = ElementOffsetFromIndex(
          IntPtrConstant(RegExpMatchInfo::kFirstCaptureIndex), PACKED_ELEMENTS,
          RegExpMatchInfo::kHeaderSize - kHeapObjectTag);
      TVARIABLE(IntPtrT, var_to_offset, to_offset);

      VariableList vars({&var_to_offset}, zone());
      BuildFastLoop<IntPtrT>(
          vars, IntPtrZero(), limit_offset,
          [&](TNode<IntPtrT> offset) {
            TNode<Int32T> value = UncheckedCast<Int32T>(Load(
                MachineType::Int32(), static_offsets_vector_address, offset));
            TNode<Smi> smi_value = SmiFromInt32(value);
            StoreNoWriteBarrier(MachineRepresentation::kTagged, match_info,
                                var_to_offset.value(), smi_value);
            Increment(&var_to_offset, kTaggedSize);
          },
          kInt32Size, IndexAdvanceMode::kPost);
    }

    var_result = match_info;
    Goto(&out);
  }

  BIND(&if_failure);
  {
    var_result = NullConstant();
    Goto(&out);
  }

  BIND(&if_exception);
  {
// A stack overflow was detected in RegExp code.
#ifdef DEBUG
    TNode<ExternalReference> pending_exception_address =
        ExternalConstant(ExternalReference::Create(
            IsolateAddressId::kPendingExceptionAddress, isolate()));
    CSA_ASSERT(this, IsTheHole(Load<Object>(pending_exception_address)));
#endif  // DEBUG
    CallRuntime(Runtime::kThrowStackOverflow, context);
    Unreachable();
  }

  BIND(&retry_experimental);
  {
    auto target_fn =
        exec_quirks == RegExp::ExecQuirks::kTreatMatchAtEndAsFailure
            ? Runtime::kRegExpExperimentalOneshotExecTreatMatchAtEndAsFailure
            : Runtime::kRegExpExperimentalOneshotExec;
    var_result = CAST(CallRuntime(target_fn, context, regexp, string,
                                  last_index, match_info));
    Goto(&out);
  }

  BIND(&runtime);
  {
    auto target_fn =
        exec_quirks == RegExp::ExecQuirks::kTreatMatchAtEndAsFailure
            ? Runtime::kRegExpExecTreatMatchAtEndAsFailure
            : Runtime::kRegExpExec;
    var_result = CAST(CallRuntime(target_fn, context, regexp, string,
                                  last_index, match_info));
    Goto(&out);
  }

  BIND(&atom);
  {
    // TODO(jgruber): A call with 4 args stresses register allocation, this
    // should probably just be inlined.
    var_result = CAST(CallBuiltin(Builtins::kRegExpExecAtom, context, regexp,
                                  string, last_index, match_info));
    Goto(&out);
  }

  BIND(&out);
  return var_result.value();
}

TNode<BoolT> RegExpBuiltinsAssembler::IsFastRegExpNoPrototype(
    TNode<Context> context, TNode<Object> object, TNode<Map> map) {
  Label out(this);
  TVARIABLE(BoolT, var_result);

#ifdef V8_ENABLE_FORCE_SLOW_PATH
  var_result = Int32FalseConstant();
  GotoIfForceSlowPath(&out);
#endif

  const TNode<NativeContext> native_context = LoadNativeContext(context);
  const TNode<HeapObject> regexp_fun =
      CAST(LoadContextElement(native_context, Context::REGEXP_FUNCTION_INDEX));
  const TNode<Object> initial_map =
      LoadObjectField(regexp_fun, JSFunction::kPrototypeOrInitialMapOffset);
  const TNode<BoolT> has_initialmap = TaggedEqual(map, initial_map);

  var_result = has_initialmap;
  GotoIfNot(has_initialmap, &out);

  // The smi check is required to omit ToLength(lastIndex) calls with possible
  // user-code execution on the fast path.
  TNode<Object> last_index = FastLoadLastIndexBeforeSmiCheck(CAST(object));
  var_result = TaggedIsPositiveSmi(last_index);
  Goto(&out);

  BIND(&out);
  return var_result.value();
}

TNode<BoolT> RegExpBuiltinsAssembler::IsFastRegExpNoPrototype(
    TNode<Context> context, TNode<Object> object) {
  CSA_ASSERT(this, TaggedIsNotSmi(object));
  return IsFastRegExpNoPrototype(context, object, LoadMap(CAST(object)));
}

void RegExpBuiltinsAssembler::BranchIfFastRegExp(
    TNode<Context> context, TNode<HeapObject> object, TNode<Map> map,
    PrototypeCheckAssembler::Flags prototype_check_flags,
    base::Optional<DescriptorIndexNameValue> additional_property_to_check,
    Label* if_isunmodified, Label* if_ismodified) {
  CSA_ASSERT(this, TaggedEqual(LoadMap(object), map));

  GotoIfForceSlowPath(if_ismodified);

  // This should only be needed for String.p.(split||matchAll), but we are
  // conservative here.
  GotoIf(IsRegExpSpeciesProtectorCellInvalid(), if_ismodified);

  TNode<NativeContext> native_context = LoadNativeContext(context);
  TNode<JSFunction> regexp_fun =
      CAST(LoadContextElement(native_context, Context::REGEXP_FUNCTION_INDEX));
  TNode<Map> initial_map = CAST(
      LoadObjectField(regexp_fun, JSFunction::kPrototypeOrInitialMapOffset));
  TNode<BoolT> has_initialmap = TaggedEqual(map, initial_map);

  GotoIfNot(has_initialmap, if_ismodified);

  // The smi check is required to omit ToLength(lastIndex) calls with possible
  // user-code execution on the fast path.
  TNode<Object> last_index = FastLoadLastIndexBeforeSmiCheck(CAST(object));
  GotoIfNot(TaggedIsPositiveSmi(last_index), if_ismodified);

  // Verify the prototype.

  TNode<Map> initial_proto_initial_map = CAST(
      LoadContextElement(native_context, Context::REGEXP_PROTOTYPE_MAP_INDEX));

  DescriptorIndexNameValue properties_to_check[2];
  int property_count = 0;
  properties_to_check[property_count++] = DescriptorIndexNameValue{
      JSRegExp::kExecFunctionDescriptorIndex, RootIndex::kexec_string,
      Context::REGEXP_EXEC_FUNCTION_INDEX};
  if (additional_property_to_check) {
    properties_to_check[property_count++] = *additional_property_to_check;
  }

  PrototypeCheckAssembler prototype_check_assembler(
      state(), prototype_check_flags, native_context, initial_proto_initial_map,
      Vector<DescriptorIndexNameValue>(properties_to_check, property_count));

  TNode<HeapObject> prototype = LoadMapPrototype(map);
  prototype_check_assembler.CheckAndBranch(prototype, if_isunmodified,
                                           if_ismodified);
}
void RegExpBuiltinsAssembler::BranchIfFastRegExpForSearch(
    TNode<Context> context, TNode<HeapObject> object, Label* if_isunmodified,
    Label* if_ismodified) {
  BranchIfFastRegExp(
      context, object, LoadMap(object),
      PrototypeCheckAssembler::kCheckPrototypePropertyConstness,
      DescriptorIndexNameValue{JSRegExp::kSymbolSearchFunctionDescriptorIndex,
                               RootIndex::ksearch_symbol,
                               Context::REGEXP_SEARCH_FUNCTION_INDEX},
      if_isunmodified, if_ismodified);
}

void RegExpBuiltinsAssembler::BranchIfFastRegExpForMatch(
    TNode<Context> context, TNode<HeapObject> object, Label* if_isunmodified,
    Label* if_ismodified) {
  BranchIfFastRegExp(
      context, object, LoadMap(object),
      PrototypeCheckAssembler::kCheckPrototypePropertyConstness,
      DescriptorIndexNameValue{JSRegExp::kSymbolMatchFunctionDescriptorIndex,
                               RootIndex::kmatch_symbol,
                               Context::REGEXP_MATCH_FUNCTION_INDEX},
      if_isunmodified, if_ismodified);
}

void RegExpBuiltinsAssembler::BranchIfFastRegExp_Strict(
    TNode<Context> context, TNode<HeapObject> object, Label* if_isunmodified,
    Label* if_ismodified) {
  BranchIfFastRegExp(context, object, LoadMap(object),
                     PrototypeCheckAssembler::kCheckPrototypePropertyConstness,
                     base::nullopt, if_isunmodified, if_ismodified);
}

void RegExpBuiltinsAssembler::BranchIfFastRegExp_Permissive(
    TNode<Context> context, TNode<HeapObject> object, Label* if_isunmodified,
    Label* if_ismodified) {
  BranchIfFastRegExp(context, object, LoadMap(object),
                     PrototypeCheckAssembler::kCheckFull, base::nullopt,
                     if_isunmodified, if_ismodified);
}

void RegExpBuiltinsAssembler::BranchIfRegExpResult(const TNode<Context> context,
                                                   const TNode<Object> object,
                                                   Label* if_isunmodified,
                                                   Label* if_ismodified) {
  // Could be a Smi.
  const TNode<Map> map = LoadReceiverMap(object);

  const TNode<NativeContext> native_context = LoadNativeContext(context);
  const TNode<Object> initial_regexp_result_map =
      LoadContextElement(native_context, Context::REGEXP_RESULT_MAP_INDEX);

  Label maybe_result_with_indices(this);
  Branch(TaggedEqual(map, initial_regexp_result_map), if_isunmodified,
         &maybe_result_with_indices);
  BIND(&maybe_result_with_indices);
  {
    static_assert(
        std::is_base_of<JSRegExpResult, JSRegExpResultWithIndices>::value,
        "JSRegExpResultWithIndices is a subclass of JSRegExpResult");
    const TNode<Object> initial_regexp_result_with_indices_map =
        LoadContextElement(native_context,
                           Context::REGEXP_RESULT_WITH_INDICES_MAP_INDEX);
    Branch(TaggedEqual(map, initial_regexp_result_with_indices_map),
           if_isunmodified, if_ismodified);
  }
}

// Fast path stub for ATOM regexps. String matching is done by StringIndexOf,
// and {match_info} is updated on success.
// The slow path is implemented in RegExp::AtomExec.
TF_BUILTIN(RegExpExecAtom, RegExpBuiltinsAssembler) {
  auto regexp = Parameter<JSRegExp>(Descriptor::kRegExp);
  auto subject_string = Parameter<String>(Descriptor::kString);
  auto last_index = Parameter<Smi>(Descriptor::kLastIndex);
  auto match_info = Parameter<FixedArray>(Descriptor::kMatchInfo);
  auto context = Parameter<Context>(Descriptor::kContext);

  CSA_ASSERT(this, TaggedIsPositiveSmi(last_index));

  TNode<FixedArray> data = CAST(LoadObjectField(regexp, JSRegExp::kDataOffset));
  CSA_ASSERT(
      this,
      SmiEqual(CAST(UnsafeLoadFixedArrayElement(data, JSRegExp::kTagIndex)),
               SmiConstant(JSRegExp::ATOM)));

  // Callers ensure that last_index is in-bounds.
  CSA_ASSERT(this,
             UintPtrLessThanOrEqual(SmiUntag(last_index),
                                    LoadStringLengthAsWord(subject_string)));

  const TNode<String> needle_string =
      CAST(UnsafeLoadFixedArrayElement(data, JSRegExp::kAtomPatternIndex));

  // ATOM patterns are guaranteed to not be the empty string (these are
  // intercepted and replaced in JSRegExp::Initialize.
  //
  // This is especially relevant for crbug.com/1075514: atom patterns are
  // non-empty and thus guaranteed not to match at the end of the string.
  CSA_ASSERT(this, IntPtrGreaterThan(LoadStringLengthAsWord(needle_string),
                                     IntPtrConstant(0)));

  const TNode<Smi> match_from =
      CAST(CallBuiltin(Builtins::kStringIndexOf, context, subject_string,
                       needle_string, last_index));

  Label if_failure(this), if_success(this);
  Branch(SmiEqual(match_from, SmiConstant(-1)), &if_failure, &if_success);

  BIND(&if_success);
  {
    CSA_ASSERT(this, TaggedIsPositiveSmi(match_from));
    CSA_ASSERT(this, UintPtrLessThan(SmiUntag(match_from),
                                     LoadStringLengthAsWord(subject_string)));

    const int kNumRegisters = 2;
    STATIC_ASSERT(RegExpMatchInfo::kInitialCaptureIndices >= kNumRegisters);

    const TNode<Smi> match_to =
        SmiAdd(match_from, LoadStringLengthAsSmi(needle_string));

    UnsafeStoreFixedArrayElement(match_info,
                                 RegExpMatchInfo::kNumberOfCapturesIndex,
                                 SmiConstant(kNumRegisters));
    UnsafeStoreFixedArrayElement(match_info, RegExpMatchInfo::kLastSubjectIndex,
                                 subject_string);
    UnsafeStoreFixedArrayElement(match_info, RegExpMatchInfo::kLastInputIndex,
                                 subject_string);
    UnsafeStoreFixedArrayElement(
        match_info, RegExpMatchInfo::kFirstCaptureIndex, match_from);
    UnsafeStoreFixedArrayElement(
        match_info, RegExpMatchInfo::kFirstCaptureIndex + 1, match_to);

    Return(match_info);
  }

  BIND(&if_failure);
  Return(NullConstant());
}

TF_BUILTIN(RegExpExecInternal, RegExpBuiltinsAssembler) {
  auto regexp = Parameter<JSRegExp>(Descriptor::kRegExp);
  auto string = Parameter<String>(Descriptor::kString);
  auto last_index = Parameter<Number>(Descriptor::kLastIndex);
  auto match_info = Parameter<RegExpMatchInfo>(Descriptor::kMatchInfo);
  auto context = Parameter<Context>(Descriptor::kContext);

  CSA_ASSERT(this, IsNumberNormalized(last_index));
  CSA_ASSERT(this, IsNumberPositive(last_index));

  Return(RegExpExecInternal(context, regexp, string, last_index, match_info));
}

TNode<String> RegExpBuiltinsAssembler::FlagsGetter(TNode<Context> context,
                                                   TNode<Object> regexp,
                                                   bool is_fastpath) {
  TVARIABLE(String, result);
  Label runtime(this, Label::kDeferred), done(this, &result);
  if (is_fastpath) {
    GotoIfForceSlowPath(&runtime);
  }

  Isolate* isolate = this->isolate();

  const TNode<IntPtrT> int_one = IntPtrConstant(1);
  TVARIABLE(Uint32T, var_length, Uint32Constant(0));
  TVARIABLE(IntPtrT, var_flags);

  // First, count the number of characters we will need and check which flags
  // are set.

  if (is_fastpath) {
    // Refer to JSRegExp's flag property on the fast-path.
    CSA_ASSERT(this, IsJSRegExp(CAST(regexp)));
    const TNode<Smi> flags_smi =
        CAST(LoadObjectField(CAST(regexp), JSRegExp::kFlagsOffset));
    var_flags = SmiUntag(flags_smi);

#define CASE_FOR_FLAG(FLAG)                                        \
  do {                                                             \
    Label next(this);                                              \
    GotoIfNot(IsSetWord(var_flags.value(), FLAG), &next);          \
    var_length = Uint32Add(var_length.value(), Uint32Constant(1)); \
    Goto(&next);                                                   \
    BIND(&next);                                                   \
  } while (false)

    CASE_FOR_FLAG(JSRegExp::kHasIndices);
    CASE_FOR_FLAG(JSRegExp::kGlobal);
    CASE_FOR_FLAG(JSRegExp::kIgnoreCase);
    CASE_FOR_FLAG(JSRegExp::kLinear);
    CASE_FOR_FLAG(JSRegExp::kMultiline);
    CASE_FOR_FLAG(JSRegExp::kDotAll);
    CASE_FOR_FLAG(JSRegExp::kUnicode);
    CASE_FOR_FLAG(JSRegExp::kSticky);
#undef CASE_FOR_FLAG
  } else {
    DCHECK(!is_fastpath);

    // Fall back to GetProperty stub on the slow-path.
    var_flags = IntPtrZero();

#define CASE_FOR_FLAG(NAME, FLAG)                                          \
  do {                                                                     \
    Label next(this);                                                      \
    const TNode<Object> flag = GetProperty(                                \
        context, regexp, isolate->factory()->InternalizeUtf8String(NAME)); \
    Label if_isflagset(this);                                              \
    BranchIfToBooleanIsTrue(flag, &if_isflagset, &next);                   \
    BIND(&if_isflagset);                                                   \
    var_length = Uint32Add(var_length.value(), Uint32Constant(1));         \
    var_flags = Signed(WordOr(var_flags.value(), IntPtrConstant(FLAG)));   \
    Goto(&next);                                                           \
    BIND(&next);                                                           \
  } while (false)

    CASE_FOR_FLAG("global", JSRegExp::kGlobal);
    CASE_FOR_FLAG("ignoreCase", JSRegExp::kIgnoreCase);
    CASE_FOR_FLAG("multiline", JSRegExp::kMultiline);
    CASE_FOR_FLAG("dotAll", JSRegExp::kDotAll);
    CASE_FOR_FLAG("unicode", JSRegExp::kUnicode);
    CASE_FOR_FLAG("sticky", JSRegExp::kSticky);
#undef CASE_FOR_FLAG

#define CASE_FOR_FLAG(NAME, V8_FLAG_EXTERN_REF, FLAG)                      \
  do {                                                                     \
    Label next(this);                                                      \
    TNode<Word32T> flag_value = UncheckedCast<Word32T>(                    \
        Load(MachineType::Uint8(), ExternalConstant(V8_FLAG_EXTERN_REF))); \
    GotoIf(Word32Equal(Word32And(flag_value, Int32Constant(0xFF)),         \
                       Int32Constant(0)),                                  \
           &next);                                                         \
    const TNode<Object> flag = GetProperty(                                \
        context, regexp, isolate->factory()->InternalizeUtf8String(NAME)); \
    Label if_isflagset(this);                                              \
    BranchIfToBooleanIsTrue(flag, &if_isflagset, &next);                   \
    BIND(&if_isflagset);                                                   \
    var_length = Uint32Add(var_length.value(), Uint32Constant(1));         \
    var_flags = Signed(WordOr(var_flags.value(), IntPtrConstant(FLAG)));   \
    Goto(&next);                                                           \
    BIND(&next);                                                           \
  } while (false)

    CASE_FOR_FLAG(
        "hasIndices",
        ExternalReference::address_of_harmony_regexp_match_indices_flag(),
        JSRegExp::kHasIndices);
    CASE_FOR_FLAG(
        "linear",
        ExternalReference::address_of_enable_experimental_regexp_engine(),
        JSRegExp::kLinear);
#undef CASE_FOR_FLAG
  }

  // Allocate a string of the required length and fill it with the
  // corresponding char for each set flag.

  {
    const TNode<String> string = AllocateSeqOneByteString(var_length.value());

    TVARIABLE(IntPtrT, var_offset,
              IntPtrConstant(SeqOneByteString::kHeaderSize - kHeapObjectTag));

#define CASE_FOR_FLAG(FLAG, CHAR)                              \
  do {                                                         \
    Label next(this);                                          \
    GotoIfNot(IsSetWord(var_flags.value(), FLAG), &next);      \
    const TNode<Int32T> value = Int32Constant(CHAR);           \
    StoreNoWriteBarrier(MachineRepresentation::kWord8, string, \
                        var_offset.value(), value);            \
    var_offset = IntPtrAdd(var_offset.value(), int_one);       \
    Goto(&next);                                               \
    BIND(&next);                                               \
  } while (false)

    CASE_FOR_FLAG(JSRegExp::kHasIndices, 'd');
    CASE_FOR_FLAG(JSRegExp::kGlobal, 'g');
    CASE_FOR_FLAG(JSRegExp::kIgnoreCase, 'i');
    CASE_FOR_FLAG(JSRegExp::kLinear, 'l');
    CASE_FOR_FLAG(JSRegExp::kMultiline, 'm');
    CASE_FOR_FLAG(JSRegExp::kDotAll, 's');
    CASE_FOR_FLAG(JSRegExp::kUnicode, 'u');
    CASE_FOR_FLAG(JSRegExp::kSticky, 'y');
#undef CASE_FOR_FLAG

    if (is_fastpath) {
#ifdef V8_ENABLE_FORCE_SLOW_PATH
      result = string;
      Goto(&done);

      BIND(&runtime);
      {
        result =
            CAST(CallRuntime(Runtime::kRegExpStringFromFlags, context, regexp));
        Goto(&done);
      }

      BIND(&done);
      return result.value();
#else
      return string;
#endif
    } else {
      return string;
    }
  }
}

// ES#sec-regexpinitialize
// Runtime Semantics: RegExpInitialize ( obj, pattern, flags )
TNode<Object> RegExpBuiltinsAssembler::RegExpInitialize(
    const TNode<Context> context, const TNode<JSRegExp> regexp,
    const TNode<Object> maybe_pattern, const TNode<Object> maybe_flags) {
  // Normalize pattern.
  const TNode<Object> pattern = Select<Object>(
      IsUndefined(maybe_pattern), [=] { return EmptyStringConstant(); },
      [=] { return ToString_Inline(context, maybe_pattern); });

  // Normalize flags.
  const TNode<Object> flags = Select<Object>(
      IsUndefined(maybe_flags), [=] { return EmptyStringConstant(); },
      [=] { return ToString_Inline(context, maybe_flags); });

  // Initialize.

  return CallRuntime(Runtime::kRegExpInitializeAndCompile, context, regexp,
                     pattern, flags);
}

// ES#sec-regexp-pattern-flags
// RegExp ( pattern, flags )
TF_BUILTIN(RegExpConstructor, RegExpBuiltinsAssembler) {
  auto pattern = Parameter<Object>(Descriptor::kPattern);
  auto flags = Parameter<Object>(Descriptor::kFlags);
  auto new_target = Parameter<Object>(Descriptor::kJSNewTarget);
  auto context = Parameter<Context>(Descriptor::kContext);

  Isolate* isolate = this->isolate();

  TVARIABLE(Object, var_flags, flags);
  TVARIABLE(Object, var_pattern, pattern);
  TVARIABLE(Object, var_new_target, new_target);

  TNode<NativeContext> native_context = LoadNativeContext(context);
  TNode<JSFunction> regexp_function =
      CAST(LoadContextElement(native_context, Context::REGEXP_FUNCTION_INDEX));

  TNode<BoolT> pattern_is_regexp = IsRegExp(context, pattern);

  {
    Label next(this);

    GotoIfNot(IsUndefined(new_target), &next);
    var_new_target = regexp_function;

    GotoIfNot(pattern_is_regexp, &next);
    GotoIfNot(IsUndefined(flags), &next);

    TNode<Object> value =
        GetProperty(context, pattern, isolate->factory()->constructor_string());

    GotoIfNot(TaggedEqual(value, regexp_function), &next);
    Return(pattern);

    BIND(&next);
  }

  {
    Label next(this), if_patternisfastregexp(this),
        if_patternisslowregexp(this);
    GotoIf(TaggedIsSmi(pattern), &next);

    GotoIf(IsJSRegExp(CAST(pattern)), &if_patternisfastregexp);

    Branch(pattern_is_regexp, &if_patternisslowregexp, &next);

    BIND(&if_patternisfastregexp);
    {
      TNode<Object> source =
          LoadObjectField(CAST(pattern), JSRegExp::kSourceOffset);
      var_pattern = source;

      {
        Label inner_next(this);
        GotoIfNot(IsUndefined(flags), &inner_next);

        var_flags = FlagsGetter(context, pattern, true);
        Goto(&inner_next);

        BIND(&inner_next);
      }

      Goto(&next);
    }

    BIND(&if_patternisslowregexp);
    {
      var_pattern =
          GetProperty(context, pattern, isolate->factory()->source_string());

      {
        Label inner_next(this);
        GotoIfNot(IsUndefined(flags), &inner_next);

        var_flags =
            GetProperty(context, pattern, isolate->factory()->flags_string());
        Goto(&inner_next);

        BIND(&inner_next);
      }

      Goto(&next);
    }

    BIND(&next);
  }

  // Allocate.

  TVARIABLE(JSRegExp, var_regexp);
  {
    Label allocate_jsregexp(this), allocate_generic(this, Label::kDeferred),
        next(this);
    Branch(TaggedEqual(var_new_target.value(), regexp_function),
           &allocate_jsregexp, &allocate_generic);

    BIND(&allocate_jsregexp);
    {
      const TNode<Map> initial_map = CAST(LoadObjectField(
          regexp_function, JSFunction::kPrototypeOrInitialMapOffset));
      var_regexp = CAST(AllocateJSObjectFromMap(initial_map));
      Goto(&next);
    }

    BIND(&allocate_generic);
    {
      ConstructorBuiltinsAssembler constructor_assembler(this->state());
      var_regexp = CAST(constructor_assembler.FastNewObject(
          context, regexp_function, CAST(var_new_target.value())));
      Goto(&next);
    }

    BIND(&next);
  }

  const TNode<Object> result = RegExpInitialize(
      context, var_regexp.value(), var_pattern.value(), var_flags.value());
  Return(result);
}

// ES#sec-regexp.prototype.compile
// RegExp.prototype.compile ( pattern, flags )
TF_BUILTIN(RegExpPrototypeCompile, RegExpBuiltinsAssembler) {
  auto maybe_receiver = Parameter<Object>(Descriptor::kReceiver);
  auto maybe_pattern = Parameter<Object>(Descriptor::kPattern);
  auto maybe_flags = Parameter<Object>(Descriptor::kFlags);
  auto context = Parameter<Context>(Descriptor::kContext);

  ThrowIfNotInstanceType(context, maybe_receiver, JS_REG_EXP_TYPE,
                         "RegExp.prototype.compile");
  const TNode<JSRegExp> receiver = CAST(maybe_receiver);

  TVARIABLE(Object, var_flags, maybe_flags);
  TVARIABLE(Object, var_pattern, maybe_pattern);

  // Handle a JSRegExp pattern.
  {
    Label next(this);

    GotoIf(TaggedIsSmi(maybe_pattern), &next);
    GotoIfNot(IsJSRegExp(CAST(maybe_pattern)), &next);

    // {maybe_flags} must be undefined in this case, otherwise throw.
    {
      Label next(this);
      GotoIf(IsUndefined(maybe_flags), &next);

      ThrowTypeError(context, MessageTemplate::kRegExpFlags);

      BIND(&next);
    }

    const TNode<JSRegExp> pattern = CAST(maybe_pattern);
    const TNode<String> new_flags = FlagsGetter(context, pattern, true);
    const TNode<Object> new_pattern =
        LoadObjectField(pattern, JSRegExp::kSourceOffset);

    var_flags = new_flags;
    var_pattern = new_pattern;

    Goto(&next);
    BIND(&next);
  }

  const TNode<Object> result = RegExpInitialize(
      context, receiver, var_pattern.value(), var_flags.value());
  Return(result);
}

// Fast-path implementation for flag checks on an unmodified JSRegExp instance.
TNode<BoolT> RegExpBuiltinsAssembler::FastFlagGetter(TNode<JSRegExp> regexp,
                                                     JSRegExp::Flag flag) {
  TNode<Smi> flags = CAST(LoadObjectField(regexp, JSRegExp::kFlagsOffset));
  TNode<Smi> mask = SmiConstant(flag);
  return ReinterpretCast<BoolT>(SmiToInt32(
      SmiShr(SmiAnd(flags, mask),
             base::bits::CountTrailingZeros(static_cast<int>(flag)))));
}

// Load through the GetProperty stub.
TNode<BoolT> RegExpBuiltinsAssembler::SlowFlagGetter(TNode<Context> context,
                                                     TNode<Object> regexp,
                                                     JSRegExp::Flag flag) {
  Label out(this), if_true(this), if_false(this);
  TVARIABLE(BoolT, var_result);

  // Only enabled based on a runtime flag.
  if (flag == JSRegExp::kLinear) {
    TNode<Word32T> flag_value = UncheckedCast<Word32T>(Load(
        MachineType::Uint8(),
        ExternalConstant(ExternalReference::
                             address_of_enable_experimental_regexp_engine())));
    GotoIf(Word32Equal(Word32And(flag_value, Int32Constant(0xFF)),
                       Int32Constant(0)),
           &if_false);
  }

  Handle<String> name;
  switch (flag) {
    case JSRegExp::kNone:
      UNREACHABLE();
    case JSRegExp::kGlobal:
      name = isolate()->factory()->global_string();
      break;
    case JSRegExp::kIgnoreCase:
      name = isolate()->factory()->ignoreCase_string();
      break;
    case JSRegExp::kMultiline:
      name = isolate()->factory()->multiline_string();
      break;
    case JSRegExp::kDotAll:
      UNREACHABLE();  // Never called for dotAll.
      break;
    case JSRegExp::kSticky:
      name = isolate()->factory()->sticky_string();
      break;
    case JSRegExp::kUnicode:
      name = isolate()->factory()->unicode_string();
      break;
    case JSRegExp::kHasIndices:
      name = isolate()->factory()->has_indices_string();
      break;
    case JSRegExp::kLinear:
      name = isolate()->factory()->linear_string();
      break;
  }

  TNode<Object> value = GetProperty(context, regexp, name);
  BranchIfToBooleanIsTrue(value, &if_true, &if_false);

  BIND(&if_true);
  var_result = BoolConstant(true);
  Goto(&out);

  BIND(&if_false);
  var_result = BoolConstant(false);
  Goto(&out);

  BIND(&out);
  return var_result.value();
}

TNode<BoolT> RegExpBuiltinsAssembler::FlagGetter(TNode<Context> context,
                                                 TNode<Object> regexp,
                                                 JSRegExp::Flag flag,
                                                 bool is_fastpath) {
  return is_fastpath ? FastFlagGetter(CAST(regexp), flag)
                     : SlowFlagGetter(context, regexp, flag);
}

TNode<Number> RegExpBuiltinsAssembler::AdvanceStringIndex(
    TNode<String> string, TNode<Number> index, TNode<BoolT> is_unicode,
    bool is_fastpath) {
  CSA_ASSERT(this, IsNumberNormalized(index));
  if (is_fastpath) CSA_ASSERT(this, TaggedIsPositiveSmi(index));

  // Default to last_index + 1.
  // TODO(pwong): Consider using TrySmiAdd for the fast path to reduce generated
  // code.
  TNode<Number> index_plus_one = NumberInc(index);
  TVARIABLE(Number, var_result, index_plus_one);

  // TODO(v8:9880): Given that we have to convert index from Number to UintPtrT
  // anyway, consider using UintPtrT index to simplify the code below.

  // Advancing the index has some subtle issues involving the distinction
  // between Smis and HeapNumbers. There's three cases:
  // * {index} is a Smi, {index_plus_one} is a Smi. The standard case.
  // * {index} is a Smi, {index_plus_one} overflows into a HeapNumber.
  //   In this case we can return the result early, because
  //   {index_plus_one} > {string}.length.
  // * {index} is a HeapNumber, {index_plus_one} is a HeapNumber. This can only
  //   occur when {index} is outside the Smi range since we normalize
  //   explicitly. Again we can return early.
  if (is_fastpath) {
    // Must be in Smi range on the fast path. We control the value of {index}
    // on all call-sites and can never exceed the length of the string.
    STATIC_ASSERT(String::kMaxLength + 2 < Smi::kMaxValue);
    CSA_ASSERT(this, TaggedIsPositiveSmi(index_plus_one));
  }

  Label if_isunicode(this), out(this);
  GotoIfNot(is_unicode, &out);

  // Keep this unconditional (even on the fast path) just to be safe.
  Branch(TaggedIsPositiveSmi(index_plus_one), &if_isunicode, &out);

  BIND(&if_isunicode);
  {
    TNode<UintPtrT> string_length = Unsigned(LoadStringLengthAsWord(string));
    TNode<UintPtrT> untagged_plus_one =
        Unsigned(SmiUntag(CAST(index_plus_one)));
    GotoIfNot(UintPtrLessThan(untagged_plus_one, string_length), &out);

    TNode<Int32T> lead =
        StringCharCodeAt(string, Unsigned(SmiUntag(CAST(index))));
    GotoIfNot(Word32Equal(Word32And(lead, Int32Constant(0xFC00)),
                          Int32Constant(0xD800)),
              &out);

    TNode<Int32T> trail = StringCharCodeAt(string, untagged_plus_one);
    GotoIfNot(Word32Equal(Word32And(trail, Int32Constant(0xFC00)),
                          Int32Constant(0xDC00)),
              &out);

    // At a surrogate pair, return index + 2.
    TNode<Number> index_plus_two = NumberInc(index_plus_one);
    var_result = index_plus_two;

    Goto(&out);
  }

  BIND(&out);
  return var_result.value();
}

// ES#sec-createregexpstringiterator
// CreateRegExpStringIterator ( R, S, global, fullUnicode )
TNode<Object> RegExpMatchAllAssembler::CreateRegExpStringIterator(
    TNode<NativeContext> native_context, TNode<Object> regexp,
    TNode<String> string, TNode<BoolT> global, TNode<BoolT> full_unicode) {
  TNode<Map> map = CAST(LoadContextElement(
      native_context,
      Context::INITIAL_REGEXP_STRING_ITERATOR_PROTOTYPE_MAP_INDEX));

  // 4. Let iterator be ObjectCreate(%RegExpStringIteratorPrototype%, «
  // [[IteratingRegExp]], [[IteratedString]], [[Global]], [[Unicode]],
  // [[Done]] »).
  TNode<HeapObject> iterator = Allocate(JSRegExpStringIterator::kHeaderSize);
  StoreMapNoWriteBarrier(iterator, map);
  StoreObjectFieldRoot(iterator,
                       JSRegExpStringIterator::kPropertiesOrHashOffset,
                       RootIndex::kEmptyFixedArray);
  StoreObjectFieldRoot(iterator, JSRegExpStringIterator::kElementsOffset,
                       RootIndex::kEmptyFixedArray);

  // 5. Set iterator.[[IteratingRegExp]] to R.
  StoreObjectFieldNoWriteBarrier(
      iterator, JSRegExpStringIterator::kIteratingRegExpOffset, regexp);

  // 6. Set iterator.[[IteratedString]] to S.
  StoreObjectFieldNoWriteBarrier(
      iterator, JSRegExpStringIterator::kIteratedStringOffset, string);

  // 7. Set iterator.[[Global]] to global.
  // 8. Set iterator.[[Unicode]] to fullUnicode.
  // 9. Set iterator.[[Done]] to false.
  TNode<Int32T> global_flag =
      Word32Shl(ReinterpretCast<Int32T>(global),
                Int32Constant(JSRegExpStringIterator::GlobalBit::kShift));
  TNode<Int32T> unicode_flag =
      Word32Shl(ReinterpretCast<Int32T>(full_unicode),
                Int32Constant(JSRegExpStringIterator::UnicodeBit::kShift));
  TNode<Int32T> iterator_flags = Word32Or(global_flag, unicode_flag);
  StoreObjectFieldNoWriteBarrier(iterator, JSRegExpStringIterator::kFlagsOffset,
                                 SmiFromInt32(iterator_flags));

  return iterator;
}

// Generates the fast path for @@split. {regexp} is an unmodified, non-sticky
// JSRegExp, {string} is a String, and {limit} is a Smi.
TNode<JSArray> RegExpBuiltinsAssembler::RegExpPrototypeSplitBody(
    TNode<Context> context, TNode<JSRegExp> regexp, TNode<String> string,
    const TNode<Smi> limit) {
  CSA_ASSERT(this, IsFastRegExpPermissive(context, regexp));
  CSA_ASSERT(this, Word32BinaryNot(FastFlagGetter(regexp, JSRegExp::kSticky)));

  const TNode<IntPtrT> int_limit = SmiUntag(limit);

  const ElementsKind kind = PACKED_ELEMENTS;

  const TNode<NativeContext> native_context = LoadNativeContext(context);
  TNode<Map> array_map = LoadJSArrayElementsMap(kind, native_context);

  Label return_empty_array(this, Label::kDeferred);
  TVARIABLE(JSArray, var_result);
  Label done(this);

  // If limit is zero, return an empty array.
  {
    Label next(this), if_limitiszero(this, Label::kDeferred);
    Branch(SmiEqual(limit, SmiZero()), &return_empty_array, &next);
    BIND(&next);
  }

  const TNode<Smi> string_length = LoadStringLengthAsSmi(string);

  // If passed the empty {string}, return either an empty array or a singleton
  // array depending on whether the {regexp} matches.
  {
    Label next(this), if_stringisempty(this, Label::kDeferred);
    Branch(SmiEqual(string_length, SmiZero()), &if_stringisempty, &next);

    BIND(&if_stringisempty);
    {
      const TNode<Object> last_match_info = LoadContextElement(
          native_context, Context::REGEXP_LAST_MATCH_INFO_INDEX);

      const TNode<Object> match_indices =
          CallBuiltin(Builtins::kRegExpExecInternal, context, regexp, string,
                      SmiZero(), last_match_info);

      Label return_singleton_array(this);
      Branch(IsNull(match_indices), &return_singleton_array,
             &return_empty_array);

      BIND(&return_singleton_array);
      {
        TNode<Smi> length = SmiConstant(1);
        TNode<IntPtrT> capacity = IntPtrConstant(1);
        base::Optional<TNode<AllocationSite>> allocation_site = base::nullopt;
        var_result =
            AllocateJSArray(kind, array_map, capacity, length, allocation_site);

        TNode<FixedArray> fixed_array = CAST(LoadElements(var_result.value()));
        UnsafeStoreFixedArrayElement(fixed_array, 0, string);

        Goto(&done);
      }
    }

    BIND(&next);
  }

  // Loop preparations.

  GrowableFixedArray array(state());

  TVARIABLE(Smi, var_last_matched_until, SmiZero());
  TVARIABLE(Smi, var_next_search_from, SmiZero());

  Label loop(this, {array.var_array(), array.var_length(), array.var_capacity(),
                    &var_last_matched_until, &var_next_search_from}),
      push_suffix_and_out(this), out(this);
  Goto(&loop);

  BIND(&loop);
  {
    const TNode<Smi> next_search_from = var_next_search_from.value();
    const TNode<Smi> last_matched_until = var_last_matched_until.value();

    // We're done if we've reached the end of the string.
    {
      Label next(this);
      Branch(SmiEqual(next_search_from, string_length), &push_suffix_and_out,
             &next);
      BIND(&next);
    }

    // Search for the given {regexp}.

    const TNode<Object> last_match_info = LoadContextElement(
        native_context, Context::REGEXP_LAST_MATCH_INFO_INDEX);

    const TNode<HeapObject> match_indices_ho = RegExpExecInternal(
        context, regexp, string, next_search_from, CAST(last_match_info),
        RegExp::ExecQuirks::kTreatMatchAtEndAsFailure);

    // We're done if no match was found.
    {
      Label next(this);
      Branch(IsNull(match_indices_ho), &push_suffix_and_out, &next);
      BIND(&next);
    }

    TNode<FixedArray> match_indices = CAST(match_indices_ho);
    const TNode<Smi> match_from = CAST(UnsafeLoadFixedArrayElement(
        match_indices, RegExpMatchInfo::kFirstCaptureIndex));
    const TNode<Smi> match_to = CAST(UnsafeLoadFixedArrayElement(
        match_indices, RegExpMatchInfo::kFirstCaptureIndex + 1));
    CSA_ASSERT(this, SmiNotEqual(match_from, string_length));

    // Advance index and continue if the match is empty.
    {
      Label next(this);

      GotoIfNot(SmiEqual(match_to, next_search_from), &next);
      GotoIfNot(SmiEqual(match_to, last_matched_until), &next);

      const TNode<BoolT> is_unicode =
          FastFlagGetter(regexp, JSRegExp::kUnicode);
      const TNode<Number> new_next_search_from =
          AdvanceStringIndex(string, next_search_from, is_unicode, true);
      var_next_search_from = CAST(new_next_search_from);
      Goto(&loop);

      BIND(&next);
    }

    // A valid match was found, add the new substring to the array.
    {
      const TNode<Smi> from = last_matched_until;
      const TNode<Smi> to = match_from;
      array.Push(CallBuiltin(Builtins::kSubString, context, string, from, to));
      GotoIf(WordEqual(array.length(), int_limit), &out);
    }

    // Add all captures to the array.
    {
      const TNode<Smi> num_registers = CAST(LoadFixedArrayElement(
          match_indices, RegExpMatchInfo::kNumberOfCapturesIndex));
      const TNode<IntPtrT> int_num_registers = SmiUntag(num_registers);

      TVARIABLE(IntPtrT, var_reg, IntPtrConstant(2));

      Label nested_loop(this, {array.var_array(), array.var_length(),
                               array.var_capacity(), &var_reg}),
          nested_loop_out(this);
      Branch(IntPtrLessThan(var_reg.value(), int_num_registers), &nested_loop,
             &nested_loop_out);

      BIND(&nested_loop);
      {
        const TNode<IntPtrT> reg = var_reg.value();
        const TNode<Object> from = LoadFixedArrayElement(
            match_indices, reg,
            RegExpMatchInfo::kFirstCaptureIndex * kTaggedSize);
        const TNode<Smi> to = CAST(LoadFixedArrayElement(
            match_indices, reg,
            (RegExpMatchInfo::kFirstCaptureIndex + 1) * kTaggedSize));

        Label select_capture(this), select_undefined(this), store_value(this);
        TVARIABLE(Object, var_value);
        Branch(SmiEqual(to, SmiConstant(-1)), &select_undefined,
               &select_capture);

        BIND(&select_capture);
        {
          var_value =
              CallBuiltin(Builtins::kSubString, context, string, from, to);
          Goto(&store_value);
        }

        BIND(&select_undefined);
        {
          var_value = UndefinedConstant();
          Goto(&store_value);
        }

        BIND(&store_value);
        {
          array.Push(var_value.value());
          GotoIf(WordEqual(array.length(), int_limit), &out);

          const TNode<IntPtrT> new_reg = IntPtrAdd(reg, IntPtrConstant(2));
          var_reg = new_reg;

          Branch(IntPtrLessThan(new_reg, int_num_registers), &nested_loop,
                 &nested_loop_out);
        }
      }

      BIND(&nested_loop_out);
    }

    var_last_matched_until = match_to;
    var_next_search_from = match_to;
    Goto(&loop);
  }

  BIND(&push_suffix_and_out);
  {
    const TNode<Smi> from = var_last_matched_until.value();
    const TNode<Smi> to = string_length;
    array.Push(CallBuiltin(Builtins::kSubString, context, string, from, to));
    Goto(&out);
  }

  BIND(&out);
  {
    var_result = array.ToJSArray(context);
    Goto(&done);
  }

  BIND(&return_empty_array);
  {
    TNode<Smi> length = SmiZero();
    TNode<IntPtrT> capacity = IntPtrZero();
    base::Optional<TNode<AllocationSite>> allocation_site = base::nullopt;
    var_result =
        AllocateJSArray(kind, array_map, capacity, length, allocation_site);
    Goto(&done);
  }

  BIND(&done);
  return var_result.value();
}

}  // namespace internal
}  // namespace v8
