// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_PROPERTY_DETAILS_H_
#define V8_OBJECTS_PROPERTY_DETAILS_H_

#include "include/v8.h"
#include "src/base/bit-field.h"
#include "src/common/globals.h"
#include "src/flags/flags.h"
#include "src/utils/allocation.h"

namespace v8 {
namespace internal {

// ES6 6.1.7.1
enum PropertyAttributes {
  NONE = ::v8::None,
  READ_ONLY = ::v8::ReadOnly,
  DONT_ENUM = ::v8::DontEnum,
  DONT_DELETE = ::v8::DontDelete,

  ALL_ATTRIBUTES_MASK = READ_ONLY | DONT_ENUM | DONT_DELETE,

  SEALED = DONT_DELETE,
  FROZEN = SEALED | READ_ONLY,

  ABSENT = 64,  // Used in runtime to indicate a property is absent.
  // ABSENT can never be stored in or returned from a descriptor's attributes
  // bitfield.  It is only used as a return value meaning the attributes of
  // a non-existent property.
};

// Number of distinct bits in PropertyAttributes.
static const int kPropertyAttributesBitsCount = 3;

static const int kPropertyAttributesCombinationsCount =
    1 << kPropertyAttributesBitsCount;

enum PropertyFilter {
  ALL_PROPERTIES = 0,
  ONLY_WRITABLE = 1,
  ONLY_ENUMERABLE = 2,
  ONLY_CONFIGURABLE = 4,
  SKIP_STRINGS = 8,
  SKIP_SYMBOLS = 16,
  ONLY_ALL_CAN_READ = 32,
  PRIVATE_NAMES_ONLY = 64,
  ENUMERABLE_STRINGS = ONLY_ENUMERABLE | SKIP_SYMBOLS,
};
// Enable fast comparisons of PropertyAttributes against PropertyFilters.
STATIC_ASSERT(ALL_PROPERTIES == static_cast<PropertyFilter>(NONE));
STATIC_ASSERT(ONLY_WRITABLE == static_cast<PropertyFilter>(READ_ONLY));
STATIC_ASSERT(ONLY_ENUMERABLE == static_cast<PropertyFilter>(DONT_ENUM));
STATIC_ASSERT(ONLY_CONFIGURABLE == static_cast<PropertyFilter>(DONT_DELETE));
STATIC_ASSERT(((SKIP_STRINGS | SKIP_SYMBOLS | ONLY_ALL_CAN_READ) &
               ALL_ATTRIBUTES_MASK) == 0);
STATIC_ASSERT(ALL_PROPERTIES ==
              static_cast<PropertyFilter>(v8::PropertyFilter::ALL_PROPERTIES));
STATIC_ASSERT(ONLY_WRITABLE ==
              static_cast<PropertyFilter>(v8::PropertyFilter::ONLY_WRITABLE));
STATIC_ASSERT(ONLY_ENUMERABLE ==
              static_cast<PropertyFilter>(v8::PropertyFilter::ONLY_ENUMERABLE));
STATIC_ASSERT(ONLY_CONFIGURABLE == static_cast<PropertyFilter>(
                                       v8::PropertyFilter::ONLY_CONFIGURABLE));
STATIC_ASSERT(SKIP_STRINGS ==
              static_cast<PropertyFilter>(v8::PropertyFilter::SKIP_STRINGS));
STATIC_ASSERT(SKIP_SYMBOLS ==
              static_cast<PropertyFilter>(v8::PropertyFilter::SKIP_SYMBOLS));

// Assert that kPropertyAttributesBitsCount value matches the definition of
// ALL_ATTRIBUTES_MASK.
STATIC_ASSERT((ALL_ATTRIBUTES_MASK == (READ_ONLY | DONT_ENUM | DONT_DELETE)) ==
              (kPropertyAttributesBitsCount == 3));

class Smi;
class TypeInfo;

// Order of kinds is significant.
// Must fit in the BitField PropertyDetails::KindField.
enum PropertyKind { kData = 0, kAccessor = 1 };

// Order of modes is significant.
// Must fit in the BitField PropertyDetails::LocationField.
enum PropertyLocation { kField = 0, kDescriptor = 1 };

// Order of modes is significant.
// Must fit in the BitField PropertyDetails::ConstnessField.
enum class PropertyConstness { kMutable = 0, kConst = 1 };

class Representation {
 public:
  enum Kind { kNone, kSmi, kDouble, kHeapObject, kTagged, kNumRepresentations };

  Representation() : kind_(kNone) {}

  static Representation None() { return Representation(kNone); }
  static Representation Tagged() { return Representation(kTagged); }
  static Representation Smi() { return Representation(kSmi); }
  static Representation Double() { return Representation(kDouble); }
  static Representation HeapObject() { return Representation(kHeapObject); }

  static Representation FromKind(Kind kind) { return Representation(kind); }

  bool Equals(const Representation& other) const {
    return kind_ == other.kind_;
  }

  bool IsCompatibleForLoad(const Representation& other) const {
    return IsDouble() == other.IsDouble();
  }

  bool IsCompatibleForStore(const Representation& other) const {
    return Equals(other);
  }

  // Returns true if a change from this representation to a more general one
  // might cause a map deprecation.
  bool MightCauseMapDeprecation() const {
    // HeapObject to tagged representation change can be done in-place.
    // Boxed double to tagged transition is always done in-place.
    if (IsTagged() || IsHeapObject() || IsDouble()) return false;
    // None to double and smi to double representation changes require
    // deprecation, because doubles might require box allocation, see
    // CanBeInPlaceChangedTo().
    DCHECK(IsNone() || IsSmi());
    return true;
  }

  bool CanBeInPlaceChangedTo(const Representation& other) const {
    if (Equals(other)) return true;
    // If it's just a representation generalization case (i.e. property kind and
    // attributes stays unchanged) it's fine to transition from None to anything
    // but double without any modification to the object, because the default
    // uninitialized value for representation None can be overwritten by both
    // smi and tagged values. Doubles, however, would require a box allocation.
    if (IsNone()) return !other.IsDouble();
    if (!other.IsTagged()) return false;
    DCHECK(IsSmi() || IsDouble() || IsHeapObject());
    return true;
  }

  // Return the most generic representation that this representation can be
  // changed to in-place. If an in-place representation change is not allowed,
  // then this will return the current representation.
  Representation MostGenericInPlaceChange() const {
    return Representation::Tagged();
  }

  bool is_more_general_than(const Representation& other) const {
    if (IsHeapObject()) return other.IsNone();
    return kind_ > other.kind_;
  }

  bool fits_into(const Representation& other) const {
    return other.is_more_general_than(*this) || other.Equals(*this);
  }

  Representation generalize(Representation other) {
    if (other.fits_into(*this)) return *this;
    if (other.is_more_general_than(*this)) return other;
    return Representation::Tagged();
  }

  int size() const {
    DCHECK(!IsNone());
    if (IsDouble()) return kDoubleSize;
    DCHECK(IsTagged() || IsSmi() || IsHeapObject());
    return kTaggedSize;
  }

  Kind kind() const { return static_cast<Kind>(kind_); }
  bool IsNone() const { return kind_ == kNone; }
  bool IsTagged() const { return kind_ == kTagged; }
  bool IsSmi() const { return kind_ == kSmi; }
  bool IsSmiOrTagged() const { return IsSmi() || IsTagged(); }
  bool IsDouble() const { return kind_ == kDouble; }
  bool IsHeapObject() const { return kind_ == kHeapObject; }

  const char* Mnemonic() const {
    switch (kind_) {
      case kNone:
        return "v";
      case kTagged:
        return "t";
      case kSmi:
        return "s";
      case kDouble:
        return "d";
      case kHeapObject:
        return "h";
    }
    UNREACHABLE();
  }

 private:
  explicit Representation(Kind k) : kind_(k) {}

  // Make sure kind fits in int8.
  STATIC_ASSERT(kNumRepresentations <= (1 << kBitsPerByte));

  int8_t kind_;
};

static const int kDescriptorIndexBitCount = 10;
static const int kFirstInobjectPropertyOffsetBitCount = 7;
// The maximum number of descriptors we want in a descriptor array.  It should
// fit in a page and also the following should hold:
// kMaxNumberOfDescriptors + kFieldsAdded <= PropertyArray::kMaxLength.
static const int kMaxNumberOfDescriptors = (1 << kDescriptorIndexBitCount) - 4;
static const int kInvalidEnumCacheSentinel =
    (1 << kDescriptorIndexBitCount) - 1;

// A PropertyCell's property details contains a cell type that is meaningful if
// the cell is still valid (does not hold the hole).
enum class PropertyCellType {
  kMutable,       // Cell will no longer be tracked as constant.
  kUndefined,     // The PREMONOMORPHIC of property cells.
  kConstant,      // Cell has been assigned only once.
  kConstantType,  // Cell has been assigned only one type.
  // Value for dictionaries not holding cells, must be 0:
  kNoCell = kMutable,
};

// PropertyDetails captures type and attributes for a property.
// They are used both in property dictionaries and instance descriptors.
class PropertyDetails {
 public:
  // Property details for global dictionary properties.
  PropertyDetails(PropertyKind kind, PropertyAttributes attributes,
                  PropertyCellType cell_type, int dictionary_index = 0) {
    value_ = KindField::encode(kind) | LocationField::encode(kField) |
             AttributesField::encode(attributes) |
             // We track PropertyCell constness via PropertyCellTypeField,
             // so we set ConstnessField to kMutable to simplify DCHECKs related
             // to non-global property constness tracking.
             ConstnessField::encode(PropertyConstness::kMutable) |
             DictionaryStorageField::encode(dictionary_index) |
             PropertyCellTypeField::encode(cell_type);
  }

  // Property details for dictionary mode properties/elements.
  PropertyDetails(PropertyKind kind, PropertyAttributes attributes,
                  PropertyConstness constness, int dictionary_index = 0) {
    value_ = KindField::encode(kind) | LocationField::encode(kField) |
             AttributesField::encode(attributes) |
             ConstnessField::encode(constness) |
             DictionaryStorageField::encode(dictionary_index) |
             PropertyCellTypeField::encode(PropertyCellType::kNoCell);
  }

  // Property details for fast mode properties.
  PropertyDetails(PropertyKind kind, PropertyAttributes attributes,
                  PropertyLocation location, PropertyConstness constness,
                  Representation representation, int field_index = 0) {
    value_ = KindField::encode(kind) | AttributesField::encode(attributes) |
             LocationField::encode(location) |
             ConstnessField::encode(constness) |
             RepresentationField::encode(EncodeRepresentation(representation)) |
             FieldIndexField::encode(field_index);
  }

  static PropertyDetails Empty(
      PropertyCellType cell_type = PropertyCellType::kNoCell) {
    return PropertyDetails(kData, NONE, cell_type);
  }

  bool operator==(PropertyDetails const& other) {
    return value_ == other.value_;
  }

  bool operator!=(PropertyDetails const& other) {
    return value_ != other.value_;
  }

  int pointer() const { return DescriptorPointer::decode(value_); }

  PropertyDetails set_pointer(int i) const {
    return PropertyDetails(value_, i);
  }

  PropertyDetails set_cell_type(PropertyCellType type) const {
    PropertyDetails details = *this;
    details.value_ = PropertyCellTypeField::update(details.value_, type);
    return details;
  }

  PropertyDetails set_index(int index) const {
    PropertyDetails details = *this;
    details.value_ = DictionaryStorageField::update(details.value_, index);
    return details;
  }

  PropertyDetails CopyWithRepresentation(Representation representation) const {
    return PropertyDetails(value_, representation);
  }
  PropertyDetails CopyWithConstness(PropertyConstness constness) const {
    return PropertyDetails(value_, constness);
  }
  PropertyDetails CopyAddAttributes(PropertyAttributes new_attributes) const {
    new_attributes =
        static_cast<PropertyAttributes>(attributes() | new_attributes);
    return PropertyDetails(value_, new_attributes);
  }

  // Conversion for storing details as Object.
  explicit inline PropertyDetails(Smi smi);
  inline Smi AsSmi() const;

  static uint8_t EncodeRepresentation(Representation representation) {
    return representation.kind();
  }

  static Representation DecodeRepresentation(uint32_t bits) {
    return Representation::FromKind(static_cast<Representation::Kind>(bits));
  }

  PropertyKind kind() const { return KindField::decode(value_); }
  PropertyLocation location() const { return LocationField::decode(value_); }
  PropertyConstness constness() const { return ConstnessField::decode(value_); }

  PropertyAttributes attributes() const {
    return AttributesField::decode(value_);
  }

  bool HasKindAndAttributes(PropertyKind kind, PropertyAttributes attributes) {
    return (value_ & (KindField::kMask | AttributesField::kMask)) ==
           (KindField::encode(kind) | AttributesField::encode(attributes));
  }

  int dictionary_index() const {
    return DictionaryStorageField::decode(value_);
  }

  Representation representation() const {
    return DecodeRepresentation(RepresentationField::decode(value_));
  }

  int field_index() const { return FieldIndexField::decode(value_); }

  inline int field_width_in_words() const;

  static bool IsValidIndex(int index) {
    return DictionaryStorageField::is_valid(index);
  }

  bool IsReadOnly() const { return (attributes() & READ_ONLY) != 0; }
  bool IsConfigurable() const { return (attributes() & DONT_DELETE) == 0; }
  bool IsDontEnum() const { return (attributes() & DONT_ENUM) != 0; }
  bool IsEnumerable() const { return !IsDontEnum(); }
  PropertyCellType cell_type() const {
    return PropertyCellTypeField::decode(value_);
  }

  bool operator==(const PropertyDetails& b) const { return value_ == b.value_; }

  // Bit fields in value_ (type, shift, size). Must be public so the
  // constants can be embedded in generated code.
  using KindField = base::BitField<PropertyKind, 0, 1>;
  using LocationField = KindField::Next<PropertyLocation, 1>;
  using ConstnessField = LocationField::Next<PropertyConstness, 1>;
  using AttributesField = ConstnessField::Next<PropertyAttributes, 3>;
  static const int kAttributesReadOnlyMask =
      (READ_ONLY << AttributesField::kShift);
  static const int kAttributesDontDeleteMask =
      (DONT_DELETE << AttributesField::kShift);
  static const int kAttributesDontEnumMask =
      (DONT_ENUM << AttributesField::kShift);

  // Bit fields for normalized/dictionary mode objects.
  using PropertyCellTypeField = AttributesField::Next<PropertyCellType, 2>;
  using DictionaryStorageField = PropertyCellTypeField::Next<uint32_t, 23>;

  // Bit fields for fast objects.
  using RepresentationField = AttributesField::Next<uint32_t, 3>;
  using DescriptorPointer =
      RepresentationField::Next<uint32_t, kDescriptorIndexBitCount>;
  using FieldIndexField =
      DescriptorPointer::Next<uint32_t, kDescriptorIndexBitCount>;

  // All bits for both fast and slow objects must fit in a smi.
  STATIC_ASSERT(DictionaryStorageField::kLastUsedBit < 31);
  STATIC_ASSERT(FieldIndexField::kLastUsedBit < 31);

  // DictionaryStorageField must be the last field, so that overflowing it
  // doesn't overwrite other fields.
  STATIC_ASSERT(DictionaryStorageField::kLastUsedBit == 30);

  // All bits for non-global dictionary mode objects except enumeration index
  // must fit in a byte.
  STATIC_ASSERT(KindField::kLastUsedBit < 8);
  STATIC_ASSERT(ConstnessField::kLastUsedBit < 8);
  STATIC_ASSERT(AttributesField::kLastUsedBit < 8);
  STATIC_ASSERT(LocationField::kLastUsedBit < 8);

  static const int kInitialIndex = 1;

  static constexpr PropertyConstness kConstIfDictConstnessTracking =
      V8_DICT_PROPERTY_CONST_TRACKING_BOOL ? PropertyConstness::kConst
                                           : PropertyConstness::kMutable;

#ifdef OBJECT_PRINT
  // For our gdb macros, we should perhaps change these in the future.
  void Print(bool dictionary_mode);
#endif

  enum PrintMode {
    kPrintAttributes = 1 << 0,
    kPrintFieldIndex = 1 << 1,
    kPrintRepresentation = 1 << 2,
    kPrintPointer = 1 << 3,

    kForProperties = kPrintFieldIndex,
    kForTransitions = kPrintAttributes,
    kPrintFull = -1,
  };
  void PrintAsSlowTo(std::ostream& out, bool print_dict_index);
  void PrintAsFastTo(std::ostream& out, PrintMode mode = kPrintFull);

  // Encodes those property details for non-global dictionary properties
  // with an enumeration index of 0 as a single byte.
  uint8_t ToByte() {
    // We only care about the value of KindField, ConstnessField, and
    // AttributesField. LocationField is also stored, but it will always be
    // kField. We've statically asserted earlier that all those fields fit into
    // a byte together.

    // PropertyCellTypeField comes next, its value must be kNoCell == 0 for
    // dictionary mode PropertyDetails anyway.
    DCHECK_EQ(PropertyCellType::kNoCell, cell_type());
    STATIC_ASSERT(static_cast<int>(PropertyCellType::kNoCell) == 0);

    // Only to be used when the enum index isn't actually maintained
    // by the PropertyDetails:
    DCHECK_EQ(0, dictionary_index());

    return value_;
  }

  // Only to be used for bytes obtained by ToByte. In particular, only used for
  // non-global dictionary properties.
  static PropertyDetails FromByte(uint8_t encoded_details) {
    // The 0-extension to 32bit sets PropertyCellType to kNoCell and
    // enumeration index to 0, as intended. Everything else is obtained from
    // |encoded_details|.

    PropertyDetails details(encoded_details);

    DCHECK_EQ(0, details.dictionary_index());
    DCHECK_EQ(PropertyLocation::kField, details.location());
    DCHECK_EQ(PropertyCellType::kNoCell, details.cell_type());

    return details;
  }

 private:
  PropertyDetails(int value, int pointer) {
    value_ = DescriptorPointer::update(value, pointer);
  }
  PropertyDetails(int value, Representation representation) {
    value_ = RepresentationField::update(value,
                                         EncodeRepresentation(representation));
  }
  PropertyDetails(int value, PropertyConstness constness) {
    value_ = ConstnessField::update(value, constness);
  }
  PropertyDetails(int value, PropertyAttributes attributes) {
    value_ = AttributesField::update(value, attributes);
  }

  explicit PropertyDetails(uint32_t value) : value_{value} {}

  uint32_t value_;
};

// kField location is more general than kDescriptor, kDescriptor generalizes
// only to itself.
inline bool IsGeneralizableTo(PropertyLocation a, PropertyLocation b) {
  return b == kField || a == kDescriptor;
}

// PropertyConstness::kMutable constness is more general than
// VariableMode::kConst, VariableMode::kConst generalizes only to itself.
inline bool IsGeneralizableTo(PropertyConstness a, PropertyConstness b) {
  return b == PropertyConstness::kMutable || a == PropertyConstness::kConst;
}

inline PropertyConstness GeneralizeConstness(PropertyConstness a,
                                             PropertyConstness b) {
  return a == PropertyConstness::kMutable ? PropertyConstness::kMutable : b;
}

V8_EXPORT_PRIVATE std::ostream& operator<<(
    std::ostream& os, const PropertyAttributes& attributes);
V8_EXPORT_PRIVATE std::ostream& operator<<(std::ostream& os,
                                           PropertyConstness constness);
V8_EXPORT_PRIVATE std::ostream& operator<<(std::ostream& os,
                                           PropertyCellType type);
}  // namespace internal
}  // namespace v8

#endif  // V8_OBJECTS_PROPERTY_DETAILS_H_
