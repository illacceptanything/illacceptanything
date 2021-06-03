// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_MAP_INL_H_
#define V8_OBJECTS_MAP_INL_H_

#include "src/heap/heap-write-barrier-inl.h"
#include "src/objects/api-callbacks-inl.h"
#include "src/objects/cell-inl.h"
#include "src/objects/descriptor-array-inl.h"
#include "src/objects/field-type.h"
#include "src/objects/instance-type-inl.h"
#include "src/objects/js-function-inl.h"
#include "src/objects/map.h"
#include "src/objects/objects-inl.h"
#include "src/objects/property.h"
#include "src/objects/prototype-info-inl.h"
#include "src/objects/shared-function-info.h"
#include "src/objects/templates-inl.h"
#include "src/objects/transitions-inl.h"
#include "src/objects/transitions.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/wasm-objects-inl.h"
#endif  // V8_ENABLE_WEBASSEMBLY

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/map-tq-inl.inc"

OBJECT_CONSTRUCTORS_IMPL(Map, HeapObject)
CAST_ACCESSOR(Map)

ACCESSORS(Map, instance_descriptors, DescriptorArray,
          kInstanceDescriptorsOffset)
RELAXED_ACCESSORS(Map, instance_descriptors, DescriptorArray,
                  kInstanceDescriptorsOffset)
RELEASE_ACQUIRE_ACCESSORS(Map, instance_descriptors, DescriptorArray,
                          kInstanceDescriptorsOffset)

// A freshly allocated layout descriptor can be set on an existing map.
// We need to use release-store and acquire-load accessor pairs to ensure
// that the concurrent marking thread observes initializing stores of the
// layout descriptor.
WEAK_ACCESSORS(Map, raw_transitions, kTransitionsOrPrototypeInfoOffset)
RELEASE_ACQUIRE_WEAK_ACCESSORS(Map, raw_transitions,
                               kTransitionsOrPrototypeInfoOffset)

ACCESSORS_CHECKED2(Map, prototype, HeapObject, kPrototypeOffset, true,
                   value.IsNull() || value.IsJSReceiver())

ACCESSORS_CHECKED(Map, prototype_info, Object,
                  kTransitionsOrPrototypeInfoOffset, this->is_prototype_map())

// |bit_field| fields.
// Concurrent access to |has_prototype_slot| and |has_non_instance_prototype|
// is explicitly allowlisted here. The former is never modified after the map
// is setup but it's being read by concurrent marker when pointer compression
// is enabled. The latter bit can be modified on a live objects.
BIT_FIELD_ACCESSORS(Map, relaxed_bit_field, has_non_instance_prototype,
                    Map::Bits1::HasNonInstancePrototypeBit)
BIT_FIELD_ACCESSORS(Map, relaxed_bit_field, has_prototype_slot,
                    Map::Bits1::HasPrototypeSlotBit)

// These are fine to be written as non-atomic since we don't have data races.
// However, they have to be read atomically from the background since the
// |bit_field| as a whole can mutate when using the above setters.
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field, bit_field, is_callable,
                     Map::Bits1::IsCallableBit)
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field, bit_field, has_named_interceptor,
                     Map::Bits1::HasNamedInterceptorBit)
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field, bit_field, has_indexed_interceptor,
                     Map::Bits1::HasIndexedInterceptorBit)
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field, bit_field, is_undetectable,
                     Map::Bits1::IsUndetectableBit)
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field, bit_field, is_access_check_needed,
                     Map::Bits1::IsAccessCheckNeededBit)
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field, bit_field, is_constructor,
                     Map::Bits1::IsConstructorBit)

// |bit_field2| fields.
BIT_FIELD_ACCESSORS(Map, bit_field2, new_target_is_base,
                    Map::Bits2::NewTargetIsBaseBit)
BIT_FIELD_ACCESSORS(Map, bit_field2, is_immutable_proto,
                    Map::Bits2::IsImmutablePrototypeBit)

// |bit_field3| fields.
BIT_FIELD_ACCESSORS(Map, relaxed_bit_field3, owns_descriptors,
                    Map::Bits3::OwnsDescriptorsBit)
BIT_FIELD_ACCESSORS(Map, release_acquire_bit_field3, is_deprecated,
                    Map::Bits3::IsDeprecatedBit)
BIT_FIELD_ACCESSORS(Map, relaxed_bit_field3, is_in_retained_map_list,
                    Map::Bits3::IsInRetainedMapListBit)
BIT_FIELD_ACCESSORS(Map, release_acquire_bit_field3, is_prototype_map,
                    Map::Bits3::IsPrototypeMapBit)
BIT_FIELD_ACCESSORS(Map, relaxed_bit_field3, is_migration_target,
                    Map::Bits3::IsMigrationTargetBit)
BIT_FIELD_ACCESSORS2(Map, relaxed_bit_field3, bit_field3, is_extensible,
                     Map::Bits3::IsExtensibleBit)
BIT_FIELD_ACCESSORS(Map, bit_field3, may_have_interesting_symbols,
                    Map::Bits3::MayHaveInterestingSymbolsBit)
BIT_FIELD_ACCESSORS(Map, relaxed_bit_field3, construction_counter,
                    Map::Bits3::ConstructionCounterBits)

DEF_GETTER(Map, GetNamedInterceptor, InterceptorInfo) {
  DCHECK(has_named_interceptor());
  FunctionTemplateInfo info = GetFunctionTemplateInfo(cage_base);
  return InterceptorInfo::cast(info.GetNamedPropertyHandler(cage_base));
}

DEF_GETTER(Map, GetIndexedInterceptor, InterceptorInfo) {
  DCHECK(has_indexed_interceptor());
  FunctionTemplateInfo info = GetFunctionTemplateInfo(cage_base);
  return InterceptorInfo::cast(info.GetIndexedPropertyHandler(cage_base));
}

// static
bool Map::IsMostGeneralFieldType(Representation representation,
                                 FieldType field_type) {
  return !representation.IsHeapObject() || field_type.IsAny();
}

// static
bool Map::FieldTypeIsCleared(Representation rep, FieldType type) {
  return type.IsNone() && rep.IsHeapObject();
}

// static
bool Map::CanHaveFastTransitionableElementsKind(InstanceType instance_type) {
  return instance_type == JS_ARRAY_TYPE ||
         instance_type == JS_PRIMITIVE_WRAPPER_TYPE ||
         instance_type == JS_ARGUMENTS_OBJECT_TYPE;
}

bool Map::CanHaveFastTransitionableElementsKind() const {
  return CanHaveFastTransitionableElementsKind(instance_type());
}

bool Map::IsDetached(Isolate* isolate) const {
  if (is_prototype_map()) return true;
  return instance_type() == JS_OBJECT_TYPE && NumberOfOwnDescriptors() > 0 &&
         GetBackPointer().IsUndefined(isolate);
}

// static
void Map::GeneralizeIfCanHaveTransitionableFastElementsKind(
    Isolate* isolate, InstanceType instance_type,
    Representation* representation, Handle<FieldType>* field_type) {
  if (CanHaveFastTransitionableElementsKind(instance_type)) {
    // We don't support propagation of field generalization through elements
    // kind transitions because they are inserted into the transition tree
    // before field transitions. In order to avoid complexity of handling
    // such a case we ensure that all maps with transitionable elements kinds
    // have the most general field representation and type.
    *field_type = FieldType::Any(isolate);
    *representation = Representation::Tagged();
  }
}

Handle<Map> Map::Normalize(Isolate* isolate, Handle<Map> fast_map,
                           PropertyNormalizationMode mode, const char* reason) {
  return Normalize(isolate, fast_map, fast_map->elements_kind(), mode, reason);
}

bool Map::EquivalentToForNormalization(const Map other,
                                       PropertyNormalizationMode mode) const {
  return EquivalentToForNormalization(other, elements_kind(), mode);
}

bool Map::TooManyFastProperties(StoreOrigin store_origin) const {
  if (UnusedPropertyFields() != 0) return false;
  if (is_prototype_map()) return false;
  if (store_origin == StoreOrigin::kNamed) {
    int limit = std::max({kMaxFastProperties, GetInObjectProperties()});
    FieldCounts counts = GetFieldCounts();
    // Only count mutable fields so that objects with large numbers of
    // constant functions do not go to dictionary mode. That would be bad
    // because such objects have often been used as modules.
    int external = counts.mutable_count() - GetInObjectProperties();
    return external > limit || counts.GetTotal() > kMaxNumberOfDescriptors;
  } else {
    int limit = std::max({kFastPropertiesSoftLimit, GetInObjectProperties()});
    int external = NumberOfFields() - GetInObjectProperties();
    return external > limit;
  }
}

Name Map::GetLastDescriptorName(Isolate* isolate) const {
  return instance_descriptors(isolate).GetKey(LastAdded());
}

PropertyDetails Map::GetLastDescriptorDetails(Isolate* isolate) const {
  return instance_descriptors(isolate).GetDetails(LastAdded());
}

InternalIndex Map::LastAdded() const {
  int number_of_own_descriptors = NumberOfOwnDescriptors();
  DCHECK_GT(number_of_own_descriptors, 0);
  return InternalIndex(number_of_own_descriptors - 1);
}

int Map::NumberOfOwnDescriptors() const {
  return Bits3::NumberOfOwnDescriptorsBits::decode(
      release_acquire_bit_field3());
}

void Map::SetNumberOfOwnDescriptors(int number) {
  DCHECK_LE(number, instance_descriptors().number_of_descriptors());
  CHECK_LE(static_cast<unsigned>(number),
           static_cast<unsigned>(kMaxNumberOfDescriptors));
  set_release_acquire_bit_field3(
      Bits3::NumberOfOwnDescriptorsBits::update(bit_field3(), number));
}

InternalIndex::Range Map::IterateOwnDescriptors() const {
  return InternalIndex::Range(NumberOfOwnDescriptors());
}

int Map::EnumLength() const {
  return Bits3::EnumLengthBits::decode(bit_field3());
}

void Map::SetEnumLength(int length) {
  if (length != kInvalidEnumCacheSentinel) {
    DCHECK_LE(length, NumberOfOwnDescriptors());
    CHECK_LE(static_cast<unsigned>(length),
             static_cast<unsigned>(kMaxNumberOfDescriptors));
  }
  set_relaxed_bit_field3(Bits3::EnumLengthBits::update(bit_field3(), length));
}

FixedArrayBase Map::GetInitialElements() const {
  FixedArrayBase result;
  if (has_fast_elements() || has_fast_string_wrapper_elements() ||
      has_any_nonextensible_elements()) {
    result = GetReadOnlyRoots().empty_fixed_array();
  } else if (has_typed_array_or_rab_gsab_typed_array_elements()) {
    result = GetReadOnlyRoots().empty_byte_array();
  } else if (has_dictionary_elements()) {
    result = GetReadOnlyRoots().empty_slow_element_dictionary();
  } else {
    UNREACHABLE();
  }
  DCHECK(!ObjectInYoungGeneration(result));
  return result;
}

VisitorId Map::visitor_id() const {
  return static_cast<VisitorId>(
      RELAXED_READ_BYTE_FIELD(*this, kVisitorIdOffset));
}

void Map::set_visitor_id(VisitorId id) {
  CHECK_LT(static_cast<unsigned>(id), 256);
  RELAXED_WRITE_BYTE_FIELD(*this, kVisitorIdOffset, static_cast<byte>(id));
}

int Map::instance_size_in_words() const {
  return RELAXED_READ_BYTE_FIELD(*this, kInstanceSizeInWordsOffset);
}

void Map::set_instance_size_in_words(int value) {
  RELAXED_WRITE_BYTE_FIELD(*this, kInstanceSizeInWordsOffset,
                           static_cast<byte>(value));
}

int Map::instance_size() const {
  return instance_size_in_words() << kTaggedSizeLog2;
}

void Map::set_instance_size(int value) {
  CHECK(IsAligned(value, kTaggedSize));
  value >>= kTaggedSizeLog2;
  CHECK_LT(static_cast<unsigned>(value), 256);
  set_instance_size_in_words(value);
}

int Map::inobject_properties_start_or_constructor_function_index() const {
  if (V8_CONCURRENT_MARKING_BOOL) {
    // TODO(solanes, v8:7790, v8:11353): Make this and the setter non-atomic
    // when TSAN sees the map's store synchronization.
    return RELAXED_READ_BYTE_FIELD(
        *this, kInObjectPropertiesStartOrConstructorFunctionIndexOffset);
  } else {
    return ReadField<byte>(
        kInObjectPropertiesStartOrConstructorFunctionIndexOffset);
  }
}

void Map::set_inobject_properties_start_or_constructor_function_index(
    int value) {
  CHECK_LT(static_cast<unsigned>(value), 256);
  if (V8_CONCURRENT_MARKING_BOOL) {
    RELAXED_WRITE_BYTE_FIELD(
        *this, kInObjectPropertiesStartOrConstructorFunctionIndexOffset,
        static_cast<byte>(value));
  } else {
    WriteField<byte>(kInObjectPropertiesStartOrConstructorFunctionIndexOffset,
                     static_cast<byte>(value));
  }
}

int Map::GetInObjectPropertiesStartInWords() const {
  DCHECK(IsJSObjectMap());
  return inobject_properties_start_or_constructor_function_index();
}

void Map::SetInObjectPropertiesStartInWords(int value) {
  CHECK(IsJSObjectMap());
  set_inobject_properties_start_or_constructor_function_index(value);
}

int Map::GetInObjectProperties() const {
  DCHECK(IsJSObjectMap());
  return instance_size_in_words() - GetInObjectPropertiesStartInWords();
}

int Map::GetConstructorFunctionIndex() const {
  DCHECK(IsPrimitiveMap());
  return inobject_properties_start_or_constructor_function_index();
}

void Map::SetConstructorFunctionIndex(int value) {
  CHECK(IsPrimitiveMap());
  set_inobject_properties_start_or_constructor_function_index(value);
}

int Map::GetInObjectPropertyOffset(int index) const {
  return (GetInObjectPropertiesStartInWords() + index) * kTaggedSize;
}

Handle<Map> Map::AddMissingTransitionsForTesting(
    Isolate* isolate, Handle<Map> split_map,
    Handle<DescriptorArray> descriptors) {
  return AddMissingTransitions(isolate, split_map, descriptors);
}

InstanceType Map::instance_type() const {
  if (V8_CONCURRENT_MARKING_BOOL) {
    // TODO(solanes, v8:7790, v8:11353): Make this and the setter non-atomic
    // when TSAN sees the map's store synchronization.
    return static_cast<InstanceType>(
        RELAXED_READ_UINT16_FIELD(*this, kInstanceTypeOffset));
  } else {
    return static_cast<InstanceType>(ReadField<uint16_t>(kInstanceTypeOffset));
  }
}

void Map::set_instance_type(InstanceType value) {
  if (V8_CONCURRENT_MARKING_BOOL) {
    RELAXED_WRITE_UINT16_FIELD(*this, kInstanceTypeOffset, value);
  } else {
    WriteField<uint16_t>(kInstanceTypeOffset, value);
  }
}

int Map::UnusedPropertyFields() const {
  int value = used_or_unused_instance_size_in_words();
  DCHECK_IMPLIES(!IsJSObjectMap(), value == 0);
  int unused;
  if (value >= JSObject::kFieldsAdded) {
    unused = instance_size_in_words() - value;
  } else {
    // For out of object properties "used_or_unused_instance_size_in_words"
    // byte encodes the slack in the property array.
    unused = value;
  }
  return unused;
}

int Map::UnusedInObjectProperties() const {
  // Like Map::UnusedPropertyFields(), but returns 0 for out of object
  // properties.
  int value = used_or_unused_instance_size_in_words();
  DCHECK_IMPLIES(!IsJSObjectMap(), value == 0);
  if (value >= JSObject::kFieldsAdded) {
    return instance_size_in_words() - value;
  }
  return 0;
}

int Map::used_or_unused_instance_size_in_words() const {
  return RELAXED_READ_BYTE_FIELD(*this, kUsedOrUnusedInstanceSizeInWordsOffset);
}

void Map::set_used_or_unused_instance_size_in_words(int value) {
  CHECK_LE(static_cast<unsigned>(value), 255);
  RELAXED_WRITE_BYTE_FIELD(*this, kUsedOrUnusedInstanceSizeInWordsOffset,
                           static_cast<byte>(value));
}

int Map::UsedInstanceSize() const {
  int words = used_or_unused_instance_size_in_words();
  if (words < JSObject::kFieldsAdded) {
    // All in-object properties are used and the words is tracking the slack
    // in the property array.
    return instance_size();
  }
  return words * kTaggedSize;
}

void Map::SetInObjectUnusedPropertyFields(int value) {
  STATIC_ASSERT(JSObject::kFieldsAdded == JSObject::kHeaderSize / kTaggedSize);
  if (!IsJSObjectMap()) {
    CHECK_EQ(0, value);
    set_used_or_unused_instance_size_in_words(0);
    DCHECK_EQ(0, UnusedPropertyFields());
    return;
  }
  CHECK_LE(0, value);
  DCHECK_LE(value, GetInObjectProperties());
  int used_inobject_properties = GetInObjectProperties() - value;
  set_used_or_unused_instance_size_in_words(
      GetInObjectPropertyOffset(used_inobject_properties) / kTaggedSize);
  DCHECK_EQ(value, UnusedPropertyFields());
}

void Map::SetOutOfObjectUnusedPropertyFields(int value) {
  STATIC_ASSERT(JSObject::kFieldsAdded == JSObject::kHeaderSize / kTaggedSize);
  CHECK_LT(static_cast<unsigned>(value), JSObject::kFieldsAdded);
  // For out of object properties "used_instance_size_in_words" byte encodes
  // the slack in the property array.
  set_used_or_unused_instance_size_in_words(value);
  DCHECK_EQ(value, UnusedPropertyFields());
}

void Map::CopyUnusedPropertyFields(Map map) {
  set_used_or_unused_instance_size_in_words(
      map.used_or_unused_instance_size_in_words());
  DCHECK_EQ(UnusedPropertyFields(), map.UnusedPropertyFields());
}

void Map::CopyUnusedPropertyFieldsAdjustedForInstanceSize(Map map) {
  int value = map.used_or_unused_instance_size_in_words();
  if (value >= JSPrimitiveWrapper::kFieldsAdded) {
    // Unused in-object fields. Adjust the offset from the object’s start
    // so it matches the distance to the object’s end.
    value += instance_size_in_words() - map.instance_size_in_words();
  }
  set_used_or_unused_instance_size_in_words(value);
  DCHECK_EQ(UnusedPropertyFields(), map.UnusedPropertyFields());
}

void Map::AccountAddedPropertyField() {
  // Update used instance size and unused property fields number.
  STATIC_ASSERT(JSObject::kFieldsAdded == JSObject::kHeaderSize / kTaggedSize);
#ifdef DEBUG
  int new_unused = UnusedPropertyFields() - 1;
  if (new_unused < 0) new_unused += JSObject::kFieldsAdded;
#endif
  int value = used_or_unused_instance_size_in_words();
  if (value >= JSObject::kFieldsAdded) {
    if (value == instance_size_in_words()) {
      AccountAddedOutOfObjectPropertyField(0);
    } else {
      // The property is added in-object, so simply increment the counter.
      set_used_or_unused_instance_size_in_words(value + 1);
    }
  } else {
    AccountAddedOutOfObjectPropertyField(value);
  }
  DCHECK_EQ(new_unused, UnusedPropertyFields());
}

void Map::AccountAddedOutOfObjectPropertyField(int unused_in_property_array) {
  unused_in_property_array--;
  if (unused_in_property_array < 0) {
    unused_in_property_array += JSObject::kFieldsAdded;
  }
  CHECK_LT(static_cast<unsigned>(unused_in_property_array),
           JSObject::kFieldsAdded);
  set_used_or_unused_instance_size_in_words(unused_in_property_array);
  DCHECK_EQ(unused_in_property_array, UnusedPropertyFields());
}

byte Map::bit_field() const { return ReadField<byte>(kBitFieldOffset); }

void Map::set_bit_field(byte value) {
  if (V8_CONCURRENT_MARKING_BOOL) {
    // TODO(solanes, v8:7790, v8:11353): Make this non-atomic when TSAN sees the
    // map's store synchronization.
    set_relaxed_bit_field(value);
  } else {
    WriteField<byte>(kBitFieldOffset, value);
  }
}

byte Map::relaxed_bit_field() const {
  return RELAXED_READ_BYTE_FIELD(*this, kBitFieldOffset);
}

void Map::set_relaxed_bit_field(byte value) {
  RELAXED_WRITE_BYTE_FIELD(*this, kBitFieldOffset, value);
}

byte Map::bit_field2() const { return ReadField<byte>(kBitField2Offset); }

void Map::set_bit_field2(byte value) {
  WriteField<byte>(kBitField2Offset, value);
}

uint32_t Map::bit_field3() const {
  if (V8_CONCURRENT_MARKING_BOOL) {
    // TODO(solanes, v8:7790, v8:11353): Make this and the setter non-atomic
    // when TSAN sees the map's store synchronization.
    return relaxed_bit_field3();
  } else {
    return ReadField<uint32_t>(kBitField3Offset);
  }
}

void Map::set_bit_field3(uint32_t value) {
  if (V8_CONCURRENT_MARKING_BOOL) {
    set_relaxed_bit_field3(value);
  } else {
    WriteField<uint32_t>(kBitField3Offset, value);
  }
}

uint32_t Map::relaxed_bit_field3() const {
  return RELAXED_READ_UINT32_FIELD(*this, kBitField3Offset);
}

void Map::set_relaxed_bit_field3(uint32_t value) {
  RELAXED_WRITE_UINT32_FIELD(*this, kBitField3Offset, value);
}

uint32_t Map::release_acquire_bit_field3() const {
  return ACQUIRE_READ_UINT32_FIELD(*this, kBitField3Offset);
}

void Map::set_release_acquire_bit_field3(uint32_t value) {
  RELEASE_WRITE_UINT32_FIELD(*this, kBitField3Offset, value);
}

bool Map::is_abandoned_prototype_map() const {
  return is_prototype_map() && !owns_descriptors();
}

bool Map::should_be_fast_prototype_map() const {
  if (!prototype_info().IsPrototypeInfo()) return false;
  return PrototypeInfo::cast(prototype_info()).should_be_fast_map();
}

void Map::set_elements_kind(ElementsKind elements_kind) {
  CHECK_LT(static_cast<int>(elements_kind), kElementsKindCount);
  set_bit_field2(
      Map::Bits2::ElementsKindBits::update(bit_field2(), elements_kind));
}

ElementsKind Map::elements_kind() const {
  return Map::Bits2::ElementsKindBits::decode(bit_field2());
}

bool Map::has_fast_smi_elements() const {
  return IsSmiElementsKind(elements_kind());
}

bool Map::has_fast_object_elements() const {
  return IsObjectElementsKind(elements_kind());
}

bool Map::has_fast_smi_or_object_elements() const {
  return IsSmiOrObjectElementsKind(elements_kind());
}

bool Map::has_fast_double_elements() const {
  return IsDoubleElementsKind(elements_kind());
}

bool Map::has_fast_elements() const {
  return IsFastElementsKind(elements_kind());
}

bool Map::has_sloppy_arguments_elements() const {
  return IsSloppyArgumentsElementsKind(elements_kind());
}

bool Map::has_fast_sloppy_arguments_elements() const {
  return elements_kind() == FAST_SLOPPY_ARGUMENTS_ELEMENTS;
}

bool Map::has_fast_string_wrapper_elements() const {
  return elements_kind() == FAST_STRING_WRAPPER_ELEMENTS;
}

bool Map::has_typed_array_elements() const {
  return IsTypedArrayElementsKind(elements_kind());
}

bool Map::has_rab_gsab_typed_array_elements() const {
  return IsRabGsabTypedArrayElementsKind(elements_kind());
}

bool Map::has_typed_array_or_rab_gsab_typed_array_elements() const {
  return IsTypedArrayOrRabGsabTypedArrayElementsKind(elements_kind());
}

bool Map::has_dictionary_elements() const {
  return IsDictionaryElementsKind(elements_kind());
}

bool Map::has_any_nonextensible_elements() const {
  return IsAnyNonextensibleElementsKind(elements_kind());
}

bool Map::has_nonextensible_elements() const {
  return IsNonextensibleElementsKind(elements_kind());
}

bool Map::has_sealed_elements() const {
  return IsSealedElementsKind(elements_kind());
}

bool Map::has_frozen_elements() const {
  return IsFrozenElementsKind(elements_kind());
}

void Map::set_is_dictionary_map(bool value) {
  uint32_t new_bit_field3 =
      Bits3::IsDictionaryMapBit::update(bit_field3(), value);
  new_bit_field3 = Bits3::IsUnstableBit::update(new_bit_field3, value);
  set_bit_field3(new_bit_field3);
}

bool Map::is_dictionary_map() const {
  return Bits3::IsDictionaryMapBit::decode(relaxed_bit_field3());
}

void Map::mark_unstable() {
  set_release_acquire_bit_field3(
      Bits3::IsUnstableBit::update(bit_field3(), true));
}

bool Map::is_stable() const {
  return !Bits3::IsUnstableBit::decode(release_acquire_bit_field3());
}

bool Map::CanBeDeprecated() const {
  for (InternalIndex i : IterateOwnDescriptors()) {
    PropertyDetails details = instance_descriptors(kRelaxedLoad).GetDetails(i);
    if (details.representation().MightCauseMapDeprecation()) return true;
    if (details.kind() == kData && details.location() == kDescriptor) {
      return true;
    }
  }
  return false;
}

void Map::NotifyLeafMapLayoutChange(Isolate* isolate) {
  if (is_stable()) {
    mark_unstable();
    dependent_code().DeoptimizeDependentCodeGroup(
        DependentCode::kPrototypeCheckGroup);
  }
}

bool Map::CanTransition() const {
  // Only JSObject and subtypes have map transitions and back pointers.
  return InstanceTypeChecker::IsJSObject(instance_type());
}

#define DEF_TESTER(Type, ...)                              \
  bool Map::Is##Type##Map() const {                        \
    return InstanceTypeChecker::Is##Type(instance_type()); \
  }
INSTANCE_TYPE_CHECKERS(DEF_TESTER)
#undef DEF_TESTER

bool Map::IsBooleanMap() const {
  return *this == GetReadOnlyRoots().boolean_map();
}

bool Map::IsNullOrUndefinedMap() const {
  return *this == GetReadOnlyRoots().null_map() ||
         *this == GetReadOnlyRoots().undefined_map();
}

bool Map::IsPrimitiveMap() const {
  return instance_type() <= LAST_PRIMITIVE_HEAP_OBJECT_TYPE;
}

void Map::UpdateDescriptors(Isolate* isolate, DescriptorArray descriptors,
                            int number_of_own_descriptors) {
  SetInstanceDescriptors(isolate, descriptors, number_of_own_descriptors);
}

void Map::InitializeDescriptors(Isolate* isolate, DescriptorArray descriptors) {
  SetInstanceDescriptors(isolate, descriptors,
                         descriptors.number_of_descriptors());
}

void Map::clear_padding() {
  if (FIELD_SIZE(kOptionalPaddingOffset) == 0) return;
  DCHECK_EQ(4, FIELD_SIZE(kOptionalPaddingOffset));
  memset(reinterpret_cast<void*>(address() + kOptionalPaddingOffset), 0,
         FIELD_SIZE(kOptionalPaddingOffset));
}

void Map::AppendDescriptor(Isolate* isolate, Descriptor* desc) {
  DescriptorArray descriptors = instance_descriptors(isolate);
  int number_of_own_descriptors = NumberOfOwnDescriptors();
  DCHECK(descriptors.number_of_descriptors() == number_of_own_descriptors);
  {
    // The following two operations need to happen before the marking write
    // barrier.
    descriptors.Append(desc);
    SetNumberOfOwnDescriptors(number_of_own_descriptors + 1);
#ifndef V8_DISABLE_WRITE_BARRIERS
    WriteBarrier::Marking(descriptors, number_of_own_descriptors + 1);
#endif
  }
  // Properly mark the map if the {desc} is an "interesting symbol".
  if (desc->GetKey()->IsInterestingSymbol()) {
    set_may_have_interesting_symbols(true);
  }
  PropertyDetails details = desc->GetDetails();
  if (details.location() == kField) {
    DCHECK_GT(UnusedPropertyFields(), 0);
    AccountAddedPropertyField();
  }

// This function does not support appending double field descriptors and
// it should never try to (otherwise, layout descriptor must be updated too).
#ifdef DEBUG
  DCHECK(details.location() != kField || !details.representation().IsDouble());
#endif
}

bool Map::ConcurrentIsMap(PtrComprCageBase cage_base,
                          const Object& object) const {
  return object.IsHeapObject() && HeapObject::cast(object).map(cage_base) ==
                                      GetReadOnlyRoots(cage_base).meta_map();
}

DEF_GETTER(Map, GetBackPointer, HeapObject) {
  Object object = constructor_or_back_pointer(cage_base);
  if (ConcurrentIsMap(cage_base, object)) {
    return Map::cast(object);
  }
  return GetReadOnlyRoots(cage_base).undefined_value();
}

void Map::SetBackPointer(HeapObject value, WriteBarrierMode mode) {
  CHECK_GE(instance_type(), FIRST_JS_RECEIVER_TYPE);
  CHECK(value.IsMap());
  CHECK(GetBackPointer().IsUndefined());
  CHECK_EQ(Map::cast(value).GetConstructor(), constructor_or_back_pointer());
  set_constructor_or_back_pointer(value, mode);
}

// static
Map Map::ElementsTransitionMap(Isolate* isolate) {
  DisallowGarbageCollection no_gc;
  return TransitionsAccessor(isolate, *this, &no_gc)
      .SearchSpecial(ReadOnlyRoots(isolate).elements_transition_symbol());
}

ACCESSORS(Map, dependent_code, DependentCode, kDependentCodeOffset)
ACCESSORS(Map, prototype_validity_cell, Object, kPrototypeValidityCellOffset)
ACCESSORS_CHECKED2(Map, constructor_or_back_pointer, Object,
                   kConstructorOrBackPointerOrNativeContextOffset,
                   !IsContextMap(), value.IsNull() || !IsContextMap())
ACCESSORS_CHECKED(Map, native_context, NativeContext,
                  kConstructorOrBackPointerOrNativeContextOffset,
                  IsContextMap())
ACCESSORS_CHECKED(Map, native_context_or_null, Object,
                  kConstructorOrBackPointerOrNativeContextOffset,
                  (value.IsNull() || value.IsNativeContext()) && IsContextMap())
#if V8_ENABLE_WEBASSEMBLY
ACCESSORS_CHECKED(Map, wasm_type_info, WasmTypeInfo,
                  kConstructorOrBackPointerOrNativeContextOffset,
                  IsWasmStructMap() || IsWasmArrayMap())
#endif  // V8_ENABLE_WEBASSEMBLY

bool Map::IsPrototypeValidityCellValid() const {
  Object validity_cell = prototype_validity_cell();
  Object value = validity_cell.IsSmi() ? Smi::cast(validity_cell)
                                       : Cell::cast(validity_cell).value();
  return value == Smi::FromInt(Map::kPrototypeChainValid);
}

DEF_GETTER(Map, GetConstructor, Object) {
  Object maybe_constructor = constructor_or_back_pointer(cage_base);
  // Follow any back pointers.
  while (ConcurrentIsMap(cage_base, maybe_constructor)) {
    maybe_constructor =
        Map::cast(maybe_constructor).constructor_or_back_pointer(cage_base);
  }
  return maybe_constructor;
}

Object Map::TryGetConstructor(Isolate* isolate, int max_steps) {
  Object maybe_constructor = constructor_or_back_pointer(isolate);
  // Follow any back pointers.
  while (maybe_constructor.IsMap(isolate)) {
    if (max_steps-- == 0) return Smi::FromInt(0);
    maybe_constructor =
        Map::cast(maybe_constructor).constructor_or_back_pointer(isolate);
  }
  return maybe_constructor;
}

DEF_GETTER(Map, GetFunctionTemplateInfo, FunctionTemplateInfo) {
  Object constructor = GetConstructor(cage_base);
  if (constructor.IsJSFunction(cage_base)) {
    // TODO(ishell): IsApiFunction(isolate) and get_api_func_data(isolate)
    DCHECK(JSFunction::cast(constructor).shared(cage_base).IsApiFunction());
    return JSFunction::cast(constructor).shared(cage_base).get_api_func_data();
  }
  DCHECK(constructor.IsFunctionTemplateInfo(cage_base));
  return FunctionTemplateInfo::cast(constructor);
}

void Map::SetConstructor(Object constructor, WriteBarrierMode mode) {
  // Never overwrite a back pointer with a constructor.
  CHECK(!constructor_or_back_pointer().IsMap());
  set_constructor_or_back_pointer(constructor, mode);
}

Handle<Map> Map::CopyInitialMap(Isolate* isolate, Handle<Map> map) {
  return CopyInitialMap(isolate, map, map->instance_size(),
                        map->GetInObjectProperties(),
                        map->UnusedPropertyFields());
}

bool Map::IsInobjectSlackTrackingInProgress() const {
  return construction_counter() != Map::kNoSlackTracking;
}

void Map::InobjectSlackTrackingStep(Isolate* isolate) {
  DisallowGarbageCollection no_gc;
  // Slack tracking should only be performed on an initial map.
  DCHECK(GetBackPointer().IsUndefined());
  if (!IsInobjectSlackTrackingInProgress()) return;
  int counter = construction_counter();
  set_construction_counter(counter - 1);
  if (counter == kSlackTrackingCounterEnd) {
    CompleteInobjectSlackTracking(isolate);
  }
}

int Map::SlackForArraySize(int old_size, int size_limit) {
  const int max_slack = size_limit - old_size;
  CHECK_LE(0, max_slack);
  if (old_size < 4) {
    DCHECK_LE(1, max_slack);
    return 1;
  }
  return std::min(max_slack, old_size / 4);
}

int Map::InstanceSizeFromSlack(int slack) const {
  return instance_size() - slack * kTaggedSize;
}

OBJECT_CONSTRUCTORS_IMPL(NormalizedMapCache, WeakFixedArray)
CAST_ACCESSOR(NormalizedMapCache)
NEVER_READ_ONLY_SPACE_IMPL(NormalizedMapCache)

int NormalizedMapCache::GetIndex(Handle<Map> map) {
  return map->Hash() % NormalizedMapCache::kEntries;
}

DEF_GETTER(HeapObject, IsNormalizedMapCache, bool) {
  if (!IsWeakFixedArray(cage_base)) return false;
  if (WeakFixedArray::cast(*this).length() != NormalizedMapCache::kEntries) {
    return false;
  }
  return true;
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_MAP_INL_H_
