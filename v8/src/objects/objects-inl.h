// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Review notes:
//
// - The use of macros in these inline functions may seem superfluous
// but it is absolutely needed to make sure gcc generates optimal
// code. gcc is not happy when attempting to inline too deep.
//

#ifndef V8_OBJECTS_OBJECTS_INL_H_
#define V8_OBJECTS_OBJECTS_INL_H_

#include "src/base/bits.h"
#include "src/base/memory.h"
#include "src/builtins/builtins.h"
#include "src/common/external-pointer-inl.h"
#include "src/common/globals.h"
#include "src/handles/handles-inl.h"
#include "src/heap/factory.h"
#include "src/heap/heap-write-barrier-inl.h"
#include "src/heap/read-only-heap-inl.h"
#include "src/numbers/conversions.h"
#include "src/numbers/double.h"
#include "src/objects/bigint.h"
#include "src/objects/heap-number-inl.h"
#include "src/objects/heap-object.h"
#include "src/objects/js-proxy-inl.h"  // TODO(jkummerow): Drop.
#include "src/objects/keys.h"
#include "src/objects/literal-objects.h"
#include "src/objects/lookup-inl.h"  // TODO(jkummerow): Drop.
#include "src/objects/objects.h"
#include "src/objects/oddball-inl.h"
#include "src/objects/property-details.h"
#include "src/objects/property.h"
#include "src/objects/regexp-match-info.h"
#include "src/objects/scope-info-inl.h"
#include "src/objects/shared-function-info.h"
#include "src/objects/slots-inl.h"
#include "src/objects/smi-inl.h"
#include "src/objects/tagged-field-inl.h"
#include "src/objects/tagged-impl-inl.h"
#include "src/objects/tagged-index.h"
#include "src/objects/templates.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

PropertyDetails::PropertyDetails(Smi smi) { value_ = smi.value(); }

Smi PropertyDetails::AsSmi() const {
  // Ensure the upper 2 bits have the same value by sign extending it. This is
  // necessary to be able to use the 31st bit of the property details.
  int value = value_ << 1;
  return Smi::FromInt(value >> 1);
}

int PropertyDetails::field_width_in_words() const {
  DCHECK_EQ(location(), kField);
  return 1;
}

DEF_GETTER(HeapObject, IsClassBoilerplate, bool) {
  return IsFixedArrayExact(cage_base);
}

bool Object::IsTaggedIndex() const {
  return IsSmi() && TaggedIndex::IsValid(TaggedIndex(ptr()).value());
}

#define IS_TYPE_FUNCTION_DEF(type_)                                        \
  bool Object::Is##type_() const {                                         \
    return IsHeapObject() && HeapObject::cast(*this).Is##type_();          \
  }                                                                        \
  bool Object::Is##type_(PtrComprCageBase cage_base) const {               \
    return IsHeapObject() && HeapObject::cast(*this).Is##type_(cage_base); \
  }
HEAP_OBJECT_TYPE_LIST(IS_TYPE_FUNCTION_DEF)
IS_TYPE_FUNCTION_DEF(HashTableBase)
IS_TYPE_FUNCTION_DEF(SmallOrderedHashTable)
#undef IS_TYPE_FUNCTION_DEF

#define IS_TYPE_FUNCTION_DEF(Type, Value)                        \
  bool Object::Is##Type(Isolate* isolate) const {                \
    return Is##Type(ReadOnlyRoots(isolate));                     \
  }                                                              \
  bool Object::Is##Type(LocalIsolate* isolate) const {           \
    return Is##Type(ReadOnlyRoots(isolate));                     \
  }                                                              \
  bool Object::Is##Type(ReadOnlyRoots roots) const {             \
    return *this == roots.Value();                               \
  }                                                              \
  bool Object::Is##Type() const {                                \
    return IsHeapObject() && HeapObject::cast(*this).Is##Type(); \
  }                                                              \
  bool HeapObject::Is##Type(Isolate* isolate) const {            \
    return Object::Is##Type(isolate);                            \
  }                                                              \
  bool HeapObject::Is##Type(LocalIsolate* isolate) const {       \
    return Object::Is##Type(isolate);                            \
  }                                                              \
  bool HeapObject::Is##Type(ReadOnlyRoots roots) const {         \
    return Object::Is##Type(roots);                              \
  }                                                              \
  bool HeapObject::Is##Type() const { return Is##Type(GetReadOnlyRoots()); }
ODDBALL_LIST(IS_TYPE_FUNCTION_DEF)
#undef IS_TYPE_FUNCTION_DEF

bool Object::IsNullOrUndefined(Isolate* isolate) const {
  return IsNullOrUndefined(ReadOnlyRoots(isolate));
}

bool Object::IsNullOrUndefined(ReadOnlyRoots roots) const {
  return IsNull(roots) || IsUndefined(roots);
}

bool Object::IsNullOrUndefined() const {
  return IsHeapObject() && HeapObject::cast(*this).IsNullOrUndefined();
}

bool Object::IsZero() const { return *this == Smi::zero(); }

bool Object::IsPublicSymbol() const {
  return IsSymbol() && !Symbol::cast(*this).is_private();
}
bool Object::IsPrivateSymbol() const {
  return IsSymbol() && Symbol::cast(*this).is_private();
}

bool Object::IsNoSharedNameSentinel() const {
  return *this == SharedFunctionInfo::kNoSharedNameSentinel;
}

bool HeapObject::IsNullOrUndefined(Isolate* isolate) const {
  return IsNullOrUndefined(ReadOnlyRoots(isolate));
}

bool HeapObject::IsNullOrUndefined(ReadOnlyRoots roots) const {
  return Object::IsNullOrUndefined(roots);
}

bool HeapObject::IsNullOrUndefined() const {
  return IsNullOrUndefined(GetReadOnlyRoots());
}

DEF_GETTER(HeapObject, IsUniqueName, bool) {
  return IsInternalizedString(cage_base) || IsSymbol(cage_base);
}

DEF_GETTER(HeapObject, IsFunction, bool) {
  return IsJSFunctionOrBoundFunction();
}

DEF_GETTER(HeapObject, IsCallable, bool) {
  return map(cage_base).is_callable();
}

DEF_GETTER(HeapObject, IsCallableJSProxy, bool) {
  return IsCallable(cage_base) && IsJSProxy(cage_base);
}

DEF_GETTER(HeapObject, IsCallableApiObject, bool) {
  InstanceType type = map(cage_base).instance_type();
  return IsCallable(cage_base) &&
         (type == JS_API_OBJECT_TYPE || type == JS_SPECIAL_API_OBJECT_TYPE);
}

DEF_GETTER(HeapObject, IsNonNullForeign, bool) {
  return IsForeign(cage_base) &&
         Foreign::cast(*this).foreign_address() != kNullAddress;
}

DEF_GETTER(HeapObject, IsConstructor, bool) {
  return map(cage_base).is_constructor();
}

DEF_GETTER(HeapObject, IsSourceTextModuleInfo, bool) {
  return map(cage_base) == GetReadOnlyRoots(cage_base).module_info_map();
}

DEF_GETTER(HeapObject, IsConsString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsCons();
}

DEF_GETTER(HeapObject, IsThinString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsThin();
}

DEF_GETTER(HeapObject, IsSlicedString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsSliced();
}

DEF_GETTER(HeapObject, IsSeqString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsSequential();
}

DEF_GETTER(HeapObject, IsSeqOneByteString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsSequential() &&
         String::cast(*this).IsOneByteRepresentation(cage_base);
}

DEF_GETTER(HeapObject, IsSeqTwoByteString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsSequential() &&
         String::cast(*this).IsTwoByteRepresentation(cage_base);
}

DEF_GETTER(HeapObject, IsExternalOneByteString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsExternal() &&
         String::cast(*this).IsOneByteRepresentation(cage_base);
}

DEF_GETTER(HeapObject, IsExternalTwoByteString, bool) {
  if (!IsString(cage_base)) return false;
  return StringShape(String::cast(*this).map(cage_base)).IsExternal() &&
         String::cast(*this).IsTwoByteRepresentation(cage_base);
}

bool Object::IsNumber() const {
  if (IsSmi()) return true;
  HeapObject this_heap_object = HeapObject::cast(*this);
  PtrComprCageBase cage_base = GetPtrComprCageBase(this_heap_object);
  return this_heap_object.IsHeapNumber(cage_base);
}

bool Object::IsNumber(PtrComprCageBase cage_base) const {
  return IsSmi() || IsHeapNumber(cage_base);
}

bool Object::IsNumeric() const {
  if (IsSmi()) return true;
  HeapObject this_heap_object = HeapObject::cast(*this);
  PtrComprCageBase cage_base = GetPtrComprCageBase(this_heap_object);
  return this_heap_object.IsHeapNumber(cage_base) ||
         this_heap_object.IsBigInt(cage_base);
}

bool Object::IsNumeric(PtrComprCageBase cage_base) const {
  return IsNumber(cage_base) || IsBigInt(cage_base);
}

DEF_GETTER(HeapObject, IsFreeSpaceOrFiller, bool) {
  InstanceType instance_type = map(cage_base).instance_type();
  return instance_type == FREE_SPACE_TYPE || instance_type == FILLER_TYPE;
}

DEF_GETTER(HeapObject, IsArrayList, bool) {
  ReadOnlyRoots roots = GetReadOnlyRoots(cage_base);
  return *this == roots.empty_fixed_array() ||
         map(cage_base) == roots.array_list_map();
}

DEF_GETTER(HeapObject, IsRegExpMatchInfo, bool) {
  return IsFixedArrayExact(cage_base);
}

DEF_GETTER(HeapObject, IsDeoptimizationData, bool) {
  // Must be a fixed array.
  if (!IsFixedArrayExact(cage_base)) return false;

  // There's no sure way to detect the difference between a fixed array and
  // a deoptimization data array.  Since this is used for asserts we can
  // check that the length is zero or else the fixed size plus a multiple of
  // the entry size.
  int length = FixedArray::cast(*this).length();
  if (length == 0) return true;

  length -= DeoptimizationData::kFirstDeoptEntryIndex;
  return length >= 0 && length % DeoptimizationData::kDeoptEntrySize == 0;
}

DEF_GETTER(HeapObject, IsHandlerTable, bool) {
  if (!IsFixedArrayExact(cage_base)) return false;
  // There's actually no way to see the difference between a fixed array and
  // a handler table array.
  return true;
}

DEF_GETTER(HeapObject, IsTemplateList, bool) {
  if (!IsFixedArrayExact(cage_base)) return false;
  // There's actually no way to see the difference between a fixed array and
  // a template list.
  if (FixedArray::cast(*this).length() < 1) return false;
  return true;
}

DEF_GETTER(HeapObject, IsDependentCode, bool) {
  if (!IsWeakFixedArray(cage_base)) return false;
  // There's actually no way to see the difference between a weak fixed array
  // and a dependent codes array.
  return true;
}

DEF_GETTER(HeapObject, IsOSROptimizedCodeCache, bool) {
  if (!IsWeakFixedArray(cage_base)) return false;
  // There's actually no way to see the difference between a weak fixed array
  // and a osr optimized code cache.
  return true;
}

DEF_GETTER(HeapObject, IsAbstractCode, bool) {
  return IsBytecodeArray(cage_base) || IsCode(cage_base);
}

DEF_GETTER(HeapObject, IsStringWrapper, bool) {
  return IsJSPrimitiveWrapper(cage_base) &&
         JSPrimitiveWrapper::cast(*this).value().IsString(cage_base);
}

DEF_GETTER(HeapObject, IsBooleanWrapper, bool) {
  return IsJSPrimitiveWrapper(cage_base) &&
         JSPrimitiveWrapper::cast(*this).value().IsBoolean(cage_base);
}

DEF_GETTER(HeapObject, IsScriptWrapper, bool) {
  return IsJSPrimitiveWrapper(cage_base) &&
         JSPrimitiveWrapper::cast(*this).value().IsScript(cage_base);
}

DEF_GETTER(HeapObject, IsNumberWrapper, bool) {
  return IsJSPrimitiveWrapper(cage_base) &&
         JSPrimitiveWrapper::cast(*this).value().IsNumber(cage_base);
}

DEF_GETTER(HeapObject, IsBigIntWrapper, bool) {
  return IsJSPrimitiveWrapper(cage_base) &&
         JSPrimitiveWrapper::cast(*this).value().IsBigInt(cage_base);
}

DEF_GETTER(HeapObject, IsSymbolWrapper, bool) {
  return IsJSPrimitiveWrapper(cage_base) &&
         JSPrimitiveWrapper::cast(*this).value().IsSymbol(cage_base);
}

DEF_GETTER(HeapObject, IsStringSet, bool) { return IsHashTable(cage_base); }

DEF_GETTER(HeapObject, IsObjectHashSet, bool) { return IsHashTable(cage_base); }

DEF_GETTER(HeapObject, IsCompilationCacheTable, bool) {
  return IsHashTable(cage_base);
}

DEF_GETTER(HeapObject, IsMapCache, bool) { return IsHashTable(cage_base); }

DEF_GETTER(HeapObject, IsObjectHashTable, bool) {
  return IsHashTable(cage_base);
}

DEF_GETTER(HeapObject, IsHashTableBase, bool) { return IsHashTable(cage_base); }

#if V8_ENABLE_WEBASSEMBLY
DEF_GETTER(HeapObject, IsWasmExceptionPackage, bool) {
  // It is not possible to check for the existence of certain properties on the
  // underlying {JSReceiver} here because that requires calling handlified code.
  return IsJSReceiver(cage_base);
}
#endif  // V8_ENABLE_WEBASSEMBLY

bool Object::IsPrimitive() const {
  if (IsSmi()) return true;
  HeapObject this_heap_object = HeapObject::cast(*this);
  PtrComprCageBase cage_base = GetPtrComprCageBase(this_heap_object);
  return this_heap_object.map(cage_base).IsPrimitiveMap();
}

bool Object::IsPrimitive(PtrComprCageBase cage_base) const {
  return IsSmi() || HeapObject::cast(*this).map(cage_base).IsPrimitiveMap();
}

// static
Maybe<bool> Object::IsArray(Handle<Object> object) {
  if (object->IsSmi()) return Just(false);
  Handle<HeapObject> heap_object = Handle<HeapObject>::cast(object);
  if (heap_object->IsJSArray()) return Just(true);
  if (!heap_object->IsJSProxy()) return Just(false);
  return JSProxy::IsArray(Handle<JSProxy>::cast(object));
}

DEF_GETTER(HeapObject, IsUndetectable, bool) {
  return map(cage_base).is_undetectable();
}

DEF_GETTER(HeapObject, IsAccessCheckNeeded, bool) {
  if (IsJSGlobalProxy(cage_base)) {
    const JSGlobalProxy proxy = JSGlobalProxy::cast(*this);
    JSGlobalObject global = proxy.GetIsolate()->context().global_object();
    return proxy.IsDetachedFrom(global);
  }
  return map(cage_base).is_access_check_needed();
}

#define MAKE_STRUCT_PREDICATE(NAME, Name, name)                           \
  bool Object::Is##Name() const {                                         \
    return IsHeapObject() && HeapObject::cast(*this).Is##Name();          \
  }                                                                       \
  bool Object::Is##Name(PtrComprCageBase cage_base) const {               \
    return IsHeapObject() && HeapObject::cast(*this).Is##Name(cage_base); \
  }
STRUCT_LIST(MAKE_STRUCT_PREDICATE)
#undef MAKE_STRUCT_PREDICATE

double Object::Number() const {
  DCHECK(IsNumber());
  return IsSmi() ? static_cast<double>(Smi(this->ptr()).value())
                 : HeapNumber::unchecked_cast(*this).value();
}

// static
bool Object::SameNumberValue(double value1, double value2) {
  // SameNumberValue(NaN, NaN) is true.
  if (value1 != value2) {
    return std::isnan(value1) && std::isnan(value2);
  }
  // SameNumberValue(0.0, -0.0) is false.
  return (std::signbit(value1) == std::signbit(value2));
}

bool Object::IsNaN() const {
  return this->IsHeapNumber() && std::isnan(HeapNumber::cast(*this).value());
}

bool Object::IsMinusZero() const {
  return this->IsHeapNumber() &&
         i::IsMinusZero(HeapNumber::cast(*this).value());
}

OBJECT_CONSTRUCTORS_IMPL(RegExpMatchInfo, FixedArray)
OBJECT_CONSTRUCTORS_IMPL(BigIntBase, PrimitiveHeapObject)
OBJECT_CONSTRUCTORS_IMPL(BigInt, BigIntBase)
OBJECT_CONSTRUCTORS_IMPL(FreshlyAllocatedBigInt, BigIntBase)

// ------------------------------------
// Cast operations

CAST_ACCESSOR(BigIntBase)
CAST_ACCESSOR(BigInt)
CAST_ACCESSOR(RegExpMatchInfo)

bool Object::HasValidElements() {
  // Dictionary is covered under FixedArray. ByteArray is used
  // for the JSTypedArray backing stores.
  return IsFixedArray() || IsFixedDoubleArray() || IsByteArray();
}

bool Object::FilterKey(PropertyFilter filter) {
  DCHECK(!IsPropertyCell());
  if (filter == PRIVATE_NAMES_ONLY) {
    if (!IsSymbol()) return true;
    return !Symbol::cast(*this).is_private_name();
  } else if (IsSymbol()) {
    if (filter & SKIP_SYMBOLS) return true;

    if (Symbol::cast(*this).is_private()) return true;
  } else {
    if (filter & SKIP_STRINGS) return true;
  }
  return false;
}

Representation Object::OptimalRepresentation(PtrComprCageBase cage_base) const {
  if (!FLAG_track_fields) return Representation::Tagged();
  if (IsSmi()) {
    return Representation::Smi();
  }
  HeapObject heap_object = HeapObject::cast(*this);
  if (FLAG_track_double_fields && heap_object.IsHeapNumber(cage_base)) {
    return Representation::Double();
  } else if (FLAG_track_computed_fields &&
             heap_object.IsUninitialized(
                 heap_object.GetReadOnlyRoots(cage_base))) {
    return Representation::None();
  } else if (FLAG_track_heap_object_fields) {
    return Representation::HeapObject();
  } else {
    return Representation::Tagged();
  }
}

ElementsKind Object::OptimalElementsKind(PtrComprCageBase cage_base) const {
  if (IsSmi()) return PACKED_SMI_ELEMENTS;
  if (IsNumber(cage_base)) return PACKED_DOUBLE_ELEMENTS;
  return PACKED_ELEMENTS;
}

bool Object::FitsRepresentation(Representation representation) {
  if (FLAG_track_fields && representation.IsSmi()) {
    return IsSmi();
  } else if (FLAG_track_double_fields && representation.IsDouble()) {
    return IsNumber();
  } else if (FLAG_track_heap_object_fields && representation.IsHeapObject()) {
    return IsHeapObject();
  } else if (FLAG_track_fields && representation.IsNone()) {
    return false;
  }
  return true;
}

bool Object::ToUint32(uint32_t* value) const {
  if (IsSmi()) {
    int num = Smi::ToInt(*this);
    if (num < 0) return false;
    *value = static_cast<uint32_t>(num);
    return true;
  }
  if (IsHeapNumber()) {
    double num = HeapNumber::cast(*this).value();
    return DoubleToUint32IfEqualToSelf(num, value);
  }
  return false;
}

// static
MaybeHandle<JSReceiver> Object::ToObject(Isolate* isolate,
                                         Handle<Object> object,
                                         const char* method_name) {
  if (object->IsJSReceiver()) return Handle<JSReceiver>::cast(object);
  return ToObjectImpl(isolate, object, method_name);
}

// static
MaybeHandle<Name> Object::ToName(Isolate* isolate, Handle<Object> input) {
  if (input->IsName()) return Handle<Name>::cast(input);
  return ConvertToName(isolate, input);
}

// static
MaybeHandle<Object> Object::ToPropertyKey(Isolate* isolate,
                                          Handle<Object> value) {
  if (value->IsSmi() || HeapObject::cast(*value).IsName()) return value;
  return ConvertToPropertyKey(isolate, value);
}

// static
MaybeHandle<Object> Object::ToPrimitive(Handle<Object> input,
                                        ToPrimitiveHint hint) {
  if (input->IsPrimitive()) return input;
  return JSReceiver::ToPrimitive(Handle<JSReceiver>::cast(input), hint);
}

// static
MaybeHandle<Object> Object::ToNumber(Isolate* isolate, Handle<Object> input) {
  if (input->IsNumber()) return input;  // Shortcut.
  return ConvertToNumberOrNumeric(isolate, input, Conversion::kToNumber);
}

// static
MaybeHandle<Object> Object::ToNumeric(Isolate* isolate, Handle<Object> input) {
  if (input->IsNumber() || input->IsBigInt()) return input;  // Shortcut.
  return ConvertToNumberOrNumeric(isolate, input, Conversion::kToNumeric);
}

// static
MaybeHandle<Object> Object::ToInteger(Isolate* isolate, Handle<Object> input) {
  if (input->IsSmi()) return input;
  return ConvertToInteger(isolate, input);
}

// static
MaybeHandle<Object> Object::ToInt32(Isolate* isolate, Handle<Object> input) {
  if (input->IsSmi()) return input;
  return ConvertToInt32(isolate, input);
}

// static
MaybeHandle<Object> Object::ToUint32(Isolate* isolate, Handle<Object> input) {
  if (input->IsSmi()) return handle(Smi::cast(*input).ToUint32Smi(), isolate);
  return ConvertToUint32(isolate, input);
}

// static
MaybeHandle<String> Object::ToString(Isolate* isolate, Handle<Object> input) {
  if (input->IsString()) return Handle<String>::cast(input);
  return ConvertToString(isolate, input);
}

// static
MaybeHandle<Object> Object::ToLength(Isolate* isolate, Handle<Object> input) {
  if (input->IsSmi()) {
    int value = std::max(Smi::ToInt(*input), 0);
    return handle(Smi::FromInt(value), isolate);
  }
  return ConvertToLength(isolate, input);
}

// static
MaybeHandle<Object> Object::ToIndex(Isolate* isolate, Handle<Object> input,
                                    MessageTemplate error_index) {
  if (input->IsSmi() && Smi::ToInt(*input) >= 0) return input;
  return ConvertToIndex(isolate, input, error_index);
}

MaybeHandle<Object> Object::GetProperty(Isolate* isolate, Handle<Object> object,
                                        Handle<Name> name) {
  LookupIterator it(isolate, object, name);
  if (!it.IsFound()) return it.factory()->undefined_value();
  return GetProperty(&it);
}

MaybeHandle<Object> Object::GetElement(Isolate* isolate, Handle<Object> object,
                                       uint32_t index) {
  LookupIterator it(isolate, object, index);
  if (!it.IsFound()) return it.factory()->undefined_value();
  return GetProperty(&it);
}

MaybeHandle<Object> Object::SetElement(Isolate* isolate, Handle<Object> object,
                                       uint32_t index, Handle<Object> value,
                                       ShouldThrow should_throw) {
  LookupIterator it(isolate, object, index);
  MAYBE_RETURN_NULL(
      SetProperty(&it, value, StoreOrigin::kMaybeKeyed, Just(should_throw)));
  return value;
}

void Object::InitExternalPointerField(size_t offset, Isolate* isolate) {
  i::InitExternalPointerField(field_address(offset), isolate);
}

void Object::InitExternalPointerField(size_t offset, Isolate* isolate,
                                      Address value, ExternalPointerTag tag) {
  i::InitExternalPointerField(field_address(offset), isolate, value, tag);
}

Address Object::ReadExternalPointerField(size_t offset, Isolate* isolate,
                                         ExternalPointerTag tag) const {
  return i::ReadExternalPointerField(field_address(offset), isolate, tag);
}

void Object::WriteExternalPointerField(size_t offset, Isolate* isolate,
                                       Address value, ExternalPointerTag tag) {
  i::WriteExternalPointerField(field_address(offset), isolate, value, tag);
}

ObjectSlot HeapObject::RawField(int byte_offset) const {
  return ObjectSlot(field_address(byte_offset));
}

MaybeObjectSlot HeapObject::RawMaybeWeakField(int byte_offset) const {
  return MaybeObjectSlot(field_address(byte_offset));
}

MapWord MapWord::FromMap(const Map map) {
  DCHECK(map.is_null() || !MapWord::IsPacked(map.ptr()));
#ifdef V8_MAP_PACKING
  return MapWord(Pack(map.ptr()));
#else
  return MapWord(map.ptr());
#endif
}

Map MapWord::ToMap() const {
#ifdef V8_MAP_PACKING
  return Map::unchecked_cast(Object(Unpack(value_)));
#else
  return Map::unchecked_cast(Object(value_));
#endif
}

bool MapWord::IsForwardingAddress() const {
  return (value_ & kForwardingTagMask) == kForwardingTag;
}

MapWord MapWord::FromForwardingAddress(HeapObject object) {
  return MapWord(object.ptr() - kHeapObjectTag);
}

HeapObject MapWord::ToForwardingAddress() {
  DCHECK(IsForwardingAddress());
  return HeapObject::FromAddress(value_);
}

#ifdef VERIFY_HEAP
void HeapObject::VerifyObjectField(Isolate* isolate, int offset) {
  VerifyPointer(isolate, TaggedField<Object>::load(isolate, *this, offset));
  STATIC_ASSERT(!COMPRESS_POINTERS_BOOL || kTaggedSize == kInt32Size);
}

void HeapObject::VerifyMaybeObjectField(Isolate* isolate, int offset) {
  MaybeObject::VerifyMaybeObjectPointer(
      isolate, TaggedField<MaybeObject>::load(isolate, *this, offset));
  STATIC_ASSERT(!COMPRESS_POINTERS_BOOL || kTaggedSize == kInt32Size);
}

void HeapObject::VerifySmiField(int offset) {
  CHECK(TaggedField<Object>::load(*this, offset).IsSmi());
  STATIC_ASSERT(!COMPRESS_POINTERS_BOOL || kTaggedSize == kInt32Size);
}

#endif

ReadOnlyRoots HeapObject::GetReadOnlyRoots() const {
  return ReadOnlyHeap::GetReadOnlyRoots(*this);
}

ReadOnlyRoots HeapObject::GetReadOnlyRoots(PtrComprCageBase cage_base) const {
#ifdef V8_COMPRESS_POINTERS_IN_ISOLATE_CAGE
  DCHECK_NE(cage_base.address(), 0);
  return ReadOnlyRoots(Isolate::FromRootAddress(cage_base.address()));
#else
  return GetReadOnlyRoots();
#endif
}

DEF_GETTER(HeapObject, map, Map) {
  return map_word(cage_base, kRelaxedLoad).ToMap();
}

void HeapObject::set_map(Map value) {
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap && !value.is_null()) {
    GetHeapFromWritableObject(*this)->VerifyObjectLayoutChange(*this, value);
  }
#endif
  set_map_word(MapWord::FromMap(value), kRelaxedStore);
#ifndef V8_DISABLE_WRITE_BARRIERS
  if (!value.is_null()) {
    // TODO(1600) We are passing kNullAddress as a slot because maps can never
    // be on an evacuation candidate.
    WriteBarrier::Marking(*this, ObjectSlot(kNullAddress), value);
  }
#endif
}

Map HeapObject::map(AcquireLoadTag tag) const {
  PtrComprCageBase cage_base = GetPtrComprCageBase(*this);
  return HeapObject::map(cage_base, tag);
}
Map HeapObject::map(PtrComprCageBase cage_base, AcquireLoadTag tag) const {
  return map_word(cage_base, tag).ToMap();
}

void HeapObject::set_map(Map value, ReleaseStoreTag tag) {
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap && !value.is_null()) {
    GetHeapFromWritableObject(*this)->VerifyObjectLayoutChange(*this, value);
  }
#endif
  set_map_word(MapWord::FromMap(value), tag);
#ifndef V8_DISABLE_WRITE_BARRIERS
  if (!value.is_null()) {
    // TODO(1600) We are passing kNullAddress as a slot because maps can never
    // be on an evacuation candidate.
    WriteBarrier::Marking(*this, ObjectSlot(kNullAddress), value);
  }
#endif
}

// Unsafe accessor omitting write barrier.
void HeapObject::set_map_no_write_barrier(Map value) {
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap && !value.is_null()) {
    GetHeapFromWritableObject(*this)->VerifyObjectLayoutChange(*this, value);
  }
#endif
  set_map_word(MapWord::FromMap(value), kRelaxedStore);
}

void HeapObject::set_map_after_allocation(Map value, WriteBarrierMode mode) {
  MapWord mapword = MapWord::FromMap(value);
  set_map_word(mapword, kRelaxedStore);
#ifndef V8_DISABLE_WRITE_BARRIERS
  if (mode != SKIP_WRITE_BARRIER) {
    DCHECK(!value.is_null());
    // TODO(1600) We are passing kNullAddress as a slot because maps can never
    // be on an evacuation candidate.
    WriteBarrier::Marking(*this, ObjectSlot(kNullAddress), value);
  }
#endif
}

ObjectSlot HeapObject::map_slot() const {
  return ObjectSlot(MapField::address(*this));
}

MapWord HeapObject::map_word(RelaxedLoadTag tag) const {
  PtrComprCageBase cage_base = GetPtrComprCageBase(*this);
  return HeapObject::map_word(cage_base, tag);
}
MapWord HeapObject::map_word(PtrComprCageBase cage_base, RelaxedLoadTag) const {
  return MapField::Relaxed_Load_Map_Word(cage_base, *this);
}

void HeapObject::set_map_word(MapWord map_word, RelaxedStoreTag) {
  MapField::Relaxed_Store_Map_Word(*this, map_word);
}

MapWord HeapObject::map_word(AcquireLoadTag tag) const {
  PtrComprCageBase cage_base = GetPtrComprCageBase(*this);
  return HeapObject::map_word(cage_base, tag);
}
MapWord HeapObject::map_word(PtrComprCageBase cage_base, AcquireLoadTag) const {
  return MapField::Acquire_Load_No_Unpack(cage_base, *this);
}

void HeapObject::set_map_word(MapWord map_word, ReleaseStoreTag) {
  MapField::Release_Store_Map_Word(*this, map_word);
}

bool HeapObject::release_compare_and_swap_map_word(MapWord old_map_word,
                                                   MapWord new_map_word) {
  Tagged_t result =
      MapField::Release_CompareAndSwap(*this, old_map_word, new_map_word);
  return result == static_cast<Tagged_t>(old_map_word.ptr());
}

int HeapObject::Size() const { return SizeFromMap(map()); }

inline bool IsSpecialReceiverInstanceType(InstanceType instance_type) {
  return instance_type <= LAST_SPECIAL_RECEIVER_TYPE;
}

// This should be in objects/map-inl.h, but can't, because of a cyclic
// dependency.
bool Map::IsSpecialReceiverMap() const {
  bool result = IsSpecialReceiverInstanceType(instance_type());
  DCHECK_IMPLIES(!result,
                 !has_named_interceptor() && !is_access_check_needed());
  return result;
}

inline bool IsCustomElementsReceiverInstanceType(InstanceType instance_type) {
  return instance_type <= LAST_CUSTOM_ELEMENTS_RECEIVER;
}

// This should be in objects/map-inl.h, but can't, because of a cyclic
// dependency.
bool Map::IsCustomElementsReceiverMap() const {
  return IsCustomElementsReceiverInstanceType(instance_type());
}

bool Object::ToArrayLength(uint32_t* index) const {
  return Object::ToUint32(index);
}

bool Object::ToArrayIndex(uint32_t* index) const {
  return Object::ToUint32(index) && *index != kMaxUInt32;
}

bool Object::ToIntegerIndex(size_t* index) const {
  if (IsSmi()) {
    int num = Smi::ToInt(*this);
    if (num < 0) return false;
    *index = static_cast<size_t>(num);
    return true;
  }
  if (IsHeapNumber()) {
    double num = HeapNumber::cast(*this).value();
    if (!(num >= 0)) return false;  // Negation to catch NaNs.
    constexpr double max =
        std::min(kMaxSafeInteger,
                 // The maximum size_t is reserved as "invalid" sentinel.
                 static_cast<double>(std::numeric_limits<size_t>::max() - 1));
    if (num > max) return false;
    size_t result = static_cast<size_t>(num);
    if (num != result) return false;  // Conversion lost fractional precision.
    *index = result;
    return true;
  }
  return false;
}

int RegExpMatchInfo::NumberOfCaptureRegisters() {
  DCHECK_GE(length(), kLastMatchOverhead);
  Object obj = get(kNumberOfCapturesIndex);
  return Smi::ToInt(obj);
}

void RegExpMatchInfo::SetNumberOfCaptureRegisters(int value) {
  DCHECK_GE(length(), kLastMatchOverhead);
  set(kNumberOfCapturesIndex, Smi::FromInt(value));
}

String RegExpMatchInfo::LastSubject() {
  DCHECK_GE(length(), kLastMatchOverhead);
  return String::cast(get(kLastSubjectIndex));
}

void RegExpMatchInfo::SetLastSubject(String value, WriteBarrierMode mode) {
  DCHECK_GE(length(), kLastMatchOverhead);
  set(kLastSubjectIndex, value, mode);
}

Object RegExpMatchInfo::LastInput() {
  DCHECK_GE(length(), kLastMatchOverhead);
  return get(kLastInputIndex);
}

void RegExpMatchInfo::SetLastInput(Object value, WriteBarrierMode mode) {
  DCHECK_GE(length(), kLastMatchOverhead);
  set(kLastInputIndex, value, mode);
}

int RegExpMatchInfo::Capture(int i) {
  DCHECK_LT(i, NumberOfCaptureRegisters());
  Object obj = get(kFirstCaptureIndex + i);
  return Smi::ToInt(obj);
}

void RegExpMatchInfo::SetCapture(int i, int value) {
  DCHECK_LT(i, NumberOfCaptureRegisters());
  set(kFirstCaptureIndex + i, Smi::FromInt(value));
}

WriteBarrierMode HeapObject::GetWriteBarrierMode(
    const DisallowGarbageCollection& promise) {
  return GetWriteBarrierModeForObject(*this, &promise);
}

// static
AllocationAlignment HeapObject::RequiredAlignment(Map map) {
  // TODO(bmeurer, v8:4153): We should think about requiring double alignment
  // in general for ByteArray, since they are used as backing store for typed
  // arrays now.
#ifdef V8_COMPRESS_POINTERS
  // TODO(ishell, v8:8875): Consider using aligned allocations once the
  // allocation alignment inconsistency is fixed. For now we keep using
  // unaligned access since both x64 and arm64 architectures (where pointer
  // compression is supported) allow unaligned access to doubles and full words.
#endif  // V8_COMPRESS_POINTERS
#ifdef V8_HOST_ARCH_32_BIT
  int instance_type = map.instance_type();
  if (instance_type == FIXED_DOUBLE_ARRAY_TYPE) return kDoubleAligned;
  if (instance_type == HEAP_NUMBER_TYPE) return kDoubleUnaligned;
#endif  // V8_HOST_ARCH_32_BIT
  return kWordAligned;
}

Address HeapObject::GetFieldAddress(int field_offset) const {
  return field_address(field_offset);
}

// static
Maybe<bool> Object::GreaterThan(Isolate* isolate, Handle<Object> x,
                                Handle<Object> y) {
  Maybe<ComparisonResult> result = Compare(isolate, x, y);
  if (result.IsJust()) {
    switch (result.FromJust()) {
      case ComparisonResult::kGreaterThan:
        return Just(true);
      case ComparisonResult::kLessThan:
      case ComparisonResult::kEqual:
      case ComparisonResult::kUndefined:
        return Just(false);
    }
  }
  return Nothing<bool>();
}

// static
Maybe<bool> Object::GreaterThanOrEqual(Isolate* isolate, Handle<Object> x,
                                       Handle<Object> y) {
  Maybe<ComparisonResult> result = Compare(isolate, x, y);
  if (result.IsJust()) {
    switch (result.FromJust()) {
      case ComparisonResult::kEqual:
      case ComparisonResult::kGreaterThan:
        return Just(true);
      case ComparisonResult::kLessThan:
      case ComparisonResult::kUndefined:
        return Just(false);
    }
  }
  return Nothing<bool>();
}

// static
Maybe<bool> Object::LessThan(Isolate* isolate, Handle<Object> x,
                             Handle<Object> y) {
  Maybe<ComparisonResult> result = Compare(isolate, x, y);
  if (result.IsJust()) {
    switch (result.FromJust()) {
      case ComparisonResult::kLessThan:
        return Just(true);
      case ComparisonResult::kEqual:
      case ComparisonResult::kGreaterThan:
      case ComparisonResult::kUndefined:
        return Just(false);
    }
  }
  return Nothing<bool>();
}

// static
Maybe<bool> Object::LessThanOrEqual(Isolate* isolate, Handle<Object> x,
                                    Handle<Object> y) {
  Maybe<ComparisonResult> result = Compare(isolate, x, y);
  if (result.IsJust()) {
    switch (result.FromJust()) {
      case ComparisonResult::kEqual:
      case ComparisonResult::kLessThan:
        return Just(true);
      case ComparisonResult::kGreaterThan:
      case ComparisonResult::kUndefined:
        return Just(false);
    }
  }
  return Nothing<bool>();
}

MaybeHandle<Object> Object::GetPropertyOrElement(Isolate* isolate,
                                                 Handle<Object> object,
                                                 Handle<Name> name) {
  LookupIterator::Key key(isolate, name);
  LookupIterator it(isolate, object, key);
  return GetProperty(&it);
}

MaybeHandle<Object> Object::SetPropertyOrElement(
    Isolate* isolate, Handle<Object> object, Handle<Name> name,
    Handle<Object> value, Maybe<ShouldThrow> should_throw,
    StoreOrigin store_origin) {
  LookupIterator::Key key(isolate, name);
  LookupIterator it(isolate, object, key);
  MAYBE_RETURN_NULL(SetProperty(&it, value, store_origin, should_throw));
  return value;
}

MaybeHandle<Object> Object::GetPropertyOrElement(Handle<Object> receiver,
                                                 Handle<Name> name,
                                                 Handle<JSReceiver> holder) {
  Isolate* isolate = holder->GetIsolate();
  LookupIterator::Key key(isolate, name);
  LookupIterator it(isolate, receiver, key, holder);
  return GetProperty(&it);
}

// static
Object Object::GetSimpleHash(Object object) {
  DisallowGarbageCollection no_gc;
  if (object.IsSmi()) {
    uint32_t hash = ComputeUnseededHash(Smi::ToInt(object));
    return Smi::FromInt(hash & Smi::kMaxValue);
  }
  if (object.IsHeapNumber()) {
    double num = HeapNumber::cast(object).value();
    if (std::isnan(num)) return Smi::FromInt(Smi::kMaxValue);
    // Use ComputeUnseededHash for all values in Signed32 range, including -0,
    // which is considered equal to 0 because collections use SameValueZero.
    uint32_t hash;
    // Check range before conversion to avoid undefined behavior.
    if (num >= kMinInt && num <= kMaxInt && FastI2D(FastD2I(num)) == num) {
      hash = ComputeUnseededHash(FastD2I(num));
    } else {
      hash = ComputeLongHash(double_to_uint64(num));
    }
    return Smi::FromInt(hash & Smi::kMaxValue);
  }
  if (object.IsName()) {
    uint32_t hash = Name::cast(object).EnsureHash();
    return Smi::FromInt(hash);
  }
  if (object.IsOddball()) {
    uint32_t hash = Oddball::cast(object).to_string().EnsureHash();
    return Smi::FromInt(hash);
  }
  if (object.IsBigInt()) {
    uint32_t hash = BigInt::cast(object).Hash();
    return Smi::FromInt(hash & Smi::kMaxValue);
  }
  if (object.IsSharedFunctionInfo()) {
    uint32_t hash = SharedFunctionInfo::cast(object).Hash();
    return Smi::FromInt(hash & Smi::kMaxValue);
  }
  DCHECK(object.IsJSReceiver());
  return object;
}

Object Object::GetHash() {
  DisallowGarbageCollection no_gc;
  Object hash = GetSimpleHash(*this);
  if (hash.IsSmi()) return hash;

  DCHECK(IsJSReceiver());
  JSReceiver receiver = JSReceiver::cast(*this);
  return receiver.GetIdentityHash();
}

Handle<Object> ObjectHashTableShape::AsHandle(Handle<Object> key) {
  return key;
}

Relocatable::Relocatable(Isolate* isolate) {
  isolate_ = isolate;
  prev_ = isolate->relocatable_top();
  isolate->set_relocatable_top(this);
}

Relocatable::~Relocatable() {
  DCHECK_EQ(isolate_->relocatable_top(), this);
  isolate_->set_relocatable_top(prev_);
}

// Predictably converts HeapObject or Address to uint32 by calculating
// offset of the address in respective MemoryChunk.
static inline uint32_t ObjectAddressForHashing(Address object) {
  uint32_t value = static_cast<uint32_t>(object);
  return value & kPageAlignmentMask;
}

static inline Handle<Object> MakeEntryPair(Isolate* isolate, size_t index,
                                           Handle<Object> value) {
  Handle<Object> key = isolate->factory()->SizeToString(index);
  Handle<FixedArray> entry_storage = isolate->factory()->NewFixedArray(2);
  {
    entry_storage->set(0, *key, SKIP_WRITE_BARRIER);
    entry_storage->set(1, *value, SKIP_WRITE_BARRIER);
  }
  return isolate->factory()->NewJSArrayWithElements(entry_storage,
                                                    PACKED_ELEMENTS, 2);
}

static inline Handle<Object> MakeEntryPair(Isolate* isolate, Handle<Object> key,
                                           Handle<Object> value) {
  Handle<FixedArray> entry_storage = isolate->factory()->NewFixedArray(2);
  {
    entry_storage->set(0, *key, SKIP_WRITE_BARRIER);
    entry_storage->set(1, *value, SKIP_WRITE_BARRIER);
  }
  return isolate->factory()->NewJSArrayWithElements(entry_storage,
                                                    PACKED_ELEMENTS, 2);
}

FreshlyAllocatedBigInt FreshlyAllocatedBigInt::cast(Object object) {
  SLOW_DCHECK(object.IsBigInt());
  return FreshlyAllocatedBigInt(object.ptr());
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_OBJECTS_INL_H_
