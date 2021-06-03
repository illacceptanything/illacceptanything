// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_VALUE_TYPE_H_
#define V8_WASM_VALUE_TYPE_H_

#include "src/base/bit-field.h"
#include "src/base/optional.h"
#include "src/codegen/machine-type.h"
#include "src/wasm/wasm-constants.h"
#include "src/wasm/wasm-limits.h"

namespace v8 {
namespace internal {

template <typename T>
class Signature;

namespace wasm {

// Type for holding simd values, defined in wasm-value.h.
class Simd128;

// Format: kind, log2Size, code, machineType, shortName, typeName
//
// Some of these types are from proposals that are not standardized yet:
// - "ref"/"optref" (a.k.a. "ref null") per
//   https://github.com/WebAssembly/function-references
// - "rtt" per https://github.com/WebAssembly/gc
#define FOREACH_NUMERIC_VALUE_TYPE(V)    \
  V(I32, 2, I32, Int32, 'i', "i32")      \
  V(I64, 3, I64, Int64, 'l', "i64")      \
  V(F32, 2, F32, Float32, 'f', "f32")    \
  V(F64, 3, F64, Float64, 'd', "f64")    \
  V(S128, 4, S128, Simd128, 's', "s128") \
  V(I8, 0, I8, Int8, 'b', "i8")          \
  V(I16, 1, I16, Int16, 'h', "i16")

#define FOREACH_VALUE_TYPE(V)                                               \
  V(Void, -1, Void, None, 'v', "<void>")                                    \
  FOREACH_NUMERIC_VALUE_TYPE(V)                                             \
  V(Rtt, kTaggedSizeLog2, Rtt, TaggedPointer, 't', "rtt")                   \
  V(RttWithDepth, kTaggedSizeLog2, RttWithDepth, TaggedPointer, 'k', "rtt") \
  V(Ref, kTaggedSizeLog2, Ref, AnyTagged, 'r', "ref")                       \
  V(OptRef, kTaggedSizeLog2, OptRef, AnyTagged, 'n', "ref null")            \
  V(Bottom, -1, Void, None, '*', "<bot>")

// Represents a WebAssembly heap type, as per the typed-funcref and gc
// proposals.
// The underlying Representation enumeration encodes heap types as follows:
// a number t < kV8MaxWasmTypes represents the type defined in the module at
// index t. Numbers directly beyond that represent the generic heap types. The
// next number represents the bottom heap type (internal use).
class HeapType {
 public:
  enum Representation : uint32_t {
    kFunc = kV8MaxWasmTypes,  // shorthand: c
    kExtern,                  // shorthand: e
    kEq,                      // shorthand: q
    kI31,                     // shorthand: j
    kData,                    // shorthand: o
    kAny,                     // shorthand: a
    // This value is used to represent failures in the parsing of heap types and
    // does not correspond to a wasm heap type.
    kBottom
  };
  // Internal use only; defined in the public section to make it easy to
  // check that they are defined correctly:
  static constexpr Representation kFirstSentinel = kFunc;
  static constexpr Representation kLastSentinel = kAny;

  static constexpr HeapType from_code(uint8_t code) {
    switch (code) {
      case ValueTypeCode::kFuncRefCode:
        return HeapType(kFunc);
      case ValueTypeCode::kExternRefCode:
        return HeapType(kExtern);
      case ValueTypeCode::kEqRefCode:
        return HeapType(kEq);
      case ValueTypeCode::kI31RefCode:
        return HeapType(kI31);
      case ValueTypeCode::kAnyRefCode:
        return HeapType(kAny);
      case ValueTypeCode::kDataRefCode:
        return HeapType(kData);
      default:
        return HeapType(kBottom);
    }
  }

  explicit constexpr HeapType(Representation repr) : representation_(repr) {
    DCHECK(is_bottom() || is_valid());
  }
  explicit constexpr HeapType(uint32_t repr)
      : HeapType(static_cast<Representation>(repr)) {}

  constexpr bool operator==(HeapType other) const {
    return representation_ == other.representation_;
  }
  constexpr bool operator!=(HeapType other) const {
    return representation_ != other.representation_;
  }

  constexpr bool operator==(Representation other) const {
    return representation_ == other;
  }

  constexpr bool operator!=(Representation other) const {
    return representation_ != other;
  }

  constexpr Representation representation() const { return representation_; }
  constexpr uint32_t ref_index() const {
    DCHECK(is_index());
    return representation_;
  }

  constexpr bool is_generic() const {
    return !is_bottom() && representation_ >= kFirstSentinel;
  }

  constexpr bool is_index() const { return representation_ < kFirstSentinel; }

  constexpr bool is_bottom() const { return representation_ == kBottom; }

  std::string name() const {
    switch (representation_) {
      case kFunc:
        return std::string("func");
      case kExtern:
        return std::string("extern");
      case kEq:
        return std::string("eq");
      case kI31:
        return std::string("i31");
      case kData:
        return std::string("data");
      case kAny:
        return std::string("any");
      default:
        return std::to_string(representation_);
    }
  }

  // Returns the code that represents this heap type in the wasm binary format.
  constexpr int32_t code() const {
    // Type codes represent the first byte of the LEB128 encoding. To get the
    // int32 represented by a code, we need to sign-extend it from 7 to 32 bits.
    int32_t mask = 0xFFFFFF80;
    switch (representation_) {
      case kFunc:
        return mask | kFuncRefCode;
      case kExtern:
        return mask | kExternRefCode;
      case kEq:
        return mask | kEqRefCode;
      case kI31:
        return mask | kI31RefCode;
      case kData:
        return mask | kDataRefCode;
      case kAny:
        return mask | kAnyRefCode;
      default:
        return static_cast<int32_t>(representation_);
    }
  }

 private:
  friend class ValueType;
  Representation representation_;
  constexpr bool is_valid() const { return representation_ <= kLastSentinel; }
};

enum Nullability : bool { kNonNullable, kNullable };

enum ValueKind : uint8_t {
#define DEF_ENUM(kind, ...) k##kind,
  FOREACH_VALUE_TYPE(DEF_ENUM)
#undef DEF_ENUM
};

constexpr bool is_reference(ValueKind kind) {
  return kind == kRef || kind == kOptRef || kind == kRtt ||
         kind == kRttWithDepth;
}

constexpr bool is_object_reference(ValueKind kind) {
  return kind == kRef || kind == kOptRef;
}

constexpr int element_size_log2(ValueKind kind) {
  constexpr int8_t kElementSizeLog2[] = {
#define ELEM_SIZE_LOG2(kind, log2Size, ...) log2Size,
      FOREACH_VALUE_TYPE(ELEM_SIZE_LOG2)
#undef ELEM_SIZE_LOG2
  };

  int size_log_2 = kElementSizeLog2[kind];
  DCHECK_LE(0, size_log_2);
  return size_log_2;
}

constexpr int element_size_bytes(ValueKind kind) {
  constexpr int8_t kElementSize[] = {
#define ELEM_SIZE_LOG2(kind, log2Size, ...) \
  log2Size == -1 ? -1 : (1 << std::max(0, log2Size)),
      FOREACH_VALUE_TYPE(ELEM_SIZE_LOG2)
#undef ELEM_SIZE_LOG2
  };

  int size = kElementSize[kind];
  DCHECK_LT(0, size);
  return size;
}

constexpr char short_name(ValueKind kind) {
  constexpr char kShortName[] = {
#define SHORT_NAME(kind, log2Size, code, machineType, shortName, ...) shortName,
      FOREACH_VALUE_TYPE(SHORT_NAME)
#undef SHORT_NAME
  };

  return kShortName[kind];
}

constexpr const char* name(ValueKind kind) {
  constexpr const char* kKindName[] = {
#define KIND_NAME(kind, log2Size, code, machineType, shortName, kindName, ...) \
  kindName,
      FOREACH_VALUE_TYPE(KIND_NAME)
#undef TYPE_NAME
  };

  return kKindName[kind];
}

constexpr MachineType machine_type(ValueKind kind) {
  DCHECK_NE(kBottom, kind);

  constexpr MachineType kMachineType[] = {
#define MACH_TYPE(kind, log2Size, code, machineType, ...) \
  MachineType::machineType(),
      FOREACH_VALUE_TYPE(MACH_TYPE)
#undef MACH_TYPE
  };

  return kMachineType[kind];
}

constexpr bool is_packed(ValueKind kind) { return kind == kI8 || kind == kI16; }
constexpr ValueKind unpacked(ValueKind kind) {
  return is_packed(kind) ? kI32 : kind;
}

constexpr bool is_rtt(ValueKind kind) {
  return kind == kRtt || kind == kRttWithDepth;
}

constexpr bool is_defaultable(ValueKind kind) {
  DCHECK(kind != kBottom && kind != kVoid);
  return kind != kRef && !is_rtt(kind);
}

// A ValueType is encoded by three components: A ValueKind, a heap
// representation (for reference types), and an inheritance depth (for rtts
// only). Those are encoded into 32 bits using base::BitField. The underlying
// ValueKind enumeration includes four elements which do not strictly correspond
// to value types: the two packed types i8 and i16, the type of void blocks
// (stmt), and a bottom value (for internal use).
class ValueType {
 public:
  /******************************* Constructors *******************************/
  constexpr ValueType() : bit_field_(KindField::encode(kVoid)) {}
  static constexpr ValueType Primitive(ValueKind kind) {
    DCHECK(kind == kBottom || kind <= kI16);
    return ValueType(KindField::encode(kind));
  }
  static constexpr ValueType Ref(uint32_t heap_type, Nullability nullability) {
    DCHECK(HeapType(heap_type).is_valid());
    return ValueType(
        KindField::encode(nullability == kNullable ? kOptRef : kRef) |
        HeapTypeField::encode(heap_type));
  }
  static constexpr ValueType Ref(HeapType heap_type, Nullability nullability) {
    return Ref(heap_type.representation(), nullability);
  }

  static constexpr ValueType Rtt(uint32_t type_index) {
    DCHECK(HeapType(type_index).is_index());
    return ValueType(KindField::encode(kRtt) |
                     HeapTypeField::encode(type_index));
  }

  static constexpr ValueType Rtt(uint32_t type_index,
                                 uint8_t inheritance_depth) {
    DCHECK(HeapType(type_index).is_index());
    return ValueType(KindField::encode(kRttWithDepth) |
                     HeapTypeField::encode(type_index) |
                     DepthField::encode(inheritance_depth));
  }

  // Useful when deserializing a type stored in a runtime object.
  static constexpr ValueType FromRawBitField(uint32_t bit_field) {
    return ValueType(bit_field);
  }

  /******************************** Type checks *******************************/
  constexpr bool is_reference() const { return wasm::is_reference(kind()); }

  constexpr bool is_object_reference() const {
    return wasm::is_object_reference(kind());
  }

  constexpr bool is_nullable() const { return kind() == kOptRef; }

  constexpr bool is_reference_to(uint32_t htype) const {
    return (kind() == kRef || kind() == kOptRef) &&
           heap_representation() == htype;
  }

  constexpr bool is_rtt() const { return wasm::is_rtt(kind()); }
  constexpr bool has_depth() const { return kind() == kRttWithDepth; }

  constexpr bool has_index() const {
    return is_rtt() || (is_object_reference() && heap_type().is_index());
  }

  constexpr bool is_defaultable() const { return wasm::is_defaultable(kind()); }

  constexpr bool is_bottom() const { return kind() == kBottom; }

  constexpr bool is_packed() const { return wasm::is_packed(kind()); }

  constexpr ValueType Unpacked() const {
    return is_packed() ? Primitive(kI32) : *this;
  }

  // Returns the version of this type that does not allow null values. Handles
  // bottom.
  constexpr ValueType AsNonNull() const {
    DCHECK(is_object_reference() || is_bottom());
    return is_nullable() ? Ref(heap_type(), kNonNullable) : *this;
  }

  /***************************** Field Accessors ******************************/
  constexpr ValueKind kind() const { return KindField::decode(bit_field_); }
  constexpr HeapType::Representation heap_representation() const {
    DCHECK(is_object_reference());
    return static_cast<HeapType::Representation>(
        HeapTypeField::decode(bit_field_));
  }
  constexpr HeapType heap_type() const {
    DCHECK(is_object_reference());
    return HeapType(heap_representation());
  }
  constexpr uint8_t depth() const {
    DCHECK(has_depth());
    return DepthField::decode(bit_field_);
  }
  constexpr uint32_t ref_index() const {
    DCHECK(has_index());
    return HeapTypeField::decode(bit_field_);
  }
  constexpr Nullability nullability() const {
    DCHECK(is_object_reference());
    return kind() == kOptRef ? kNullable : kNonNullable;
  }

  // Useful when serializing this type to store it into a runtime object.
  constexpr uint32_t raw_bit_field() const { return bit_field_; }

  /*************************** Other utility methods **************************/
  constexpr bool operator==(ValueType other) const {
    return bit_field_ == other.bit_field_;
  }
  constexpr bool operator!=(ValueType other) const {
    return bit_field_ != other.bit_field_;
  }

  static constexpr size_t bit_field_offset() {
    return offsetof(ValueType, bit_field_);
  }

  constexpr int element_size_log2() const {
    return wasm::element_size_log2(kind());
  }

  constexpr int element_size_bytes() const {
    return wasm::element_size_bytes(kind());
  }

  /*************************** Machine-type related ***************************/
  constexpr MachineType machine_type() const {
    return wasm::machine_type(kind());
  }

  constexpr MachineRepresentation machine_representation() const {
    return machine_type().representation();
  }

  static ValueType For(MachineType type) {
    switch (type.representation()) {
      case MachineRepresentation::kWord8:
      case MachineRepresentation::kWord16:
      case MachineRepresentation::kWord32:
        return Primitive(kI32);
      case MachineRepresentation::kWord64:
        return Primitive(kI64);
      case MachineRepresentation::kFloat32:
        return Primitive(kF32);
      case MachineRepresentation::kFloat64:
        return Primitive(kF64);
      case MachineRepresentation::kTaggedPointer:
        return Ref(HeapType::kExtern, kNullable);
      case MachineRepresentation::kSimd128:
        return Primitive(kS128);
      default:
        UNREACHABLE();
    }
  }

  /********************************* Encoding *********************************/

  // Returns the first byte of this type's representation in the wasm binary
  // format.
  // For compatibility with the reftypes and exception-handling proposals, this
  // function prioritizes shorthand encodings
  // (e.g., Ref(HeapType::kFunc, kNullable).value_type_code will return
  // kFuncrefCode and not kOptRefCode).
  constexpr ValueTypeCode value_type_code() const {
    DCHECK_NE(kBottom, kind());
    switch (kind()) {
      case kOptRef:
        switch (heap_representation()) {
          case HeapType::kFunc:
            return kFuncRefCode;
          case HeapType::kExtern:
            return kExternRefCode;
          case HeapType::kEq:
            return kEqRefCode;
          case HeapType::kAny:
            return kAnyRefCode;
          default:
            return kOptRefCode;
        }
      case kRef:
        switch (heap_representation()) {
          case HeapType::kI31:
            return kI31RefCode;
          case HeapType::kData:
            return kDataRefCode;
          default:
            return kRefCode;
        }
      case kVoid:
        return kVoidCode;
      case kRtt:
        return kRttCode;
      case kRttWithDepth:
        return kRttWithDepthCode;
#define NUMERIC_TYPE_CASE(kind, ...) \
  case k##kind:                      \
    return k##kind##Code;
        FOREACH_NUMERIC_VALUE_TYPE(NUMERIC_TYPE_CASE)
#undef NUMERIC_TYPE_CASE
      case kBottom:
        // Unreachable code
        return kVoidCode;
    }
  }

  // Returns true iff the heap type is needed to encode this type in the wasm
  // binary format, taking into account available type shorthands.
  constexpr bool encoding_needs_heap_type() const {
    return (kind() == kRef && heap_representation() != HeapType::kI31 &&
            heap_representation() != HeapType::kData) ||
           (kind() == kOptRef && (heap_type().is_index() ||
                                  heap_representation() == HeapType::kI31 ||
                                  heap_representation() == HeapType::kData));
  }

  static constexpr int kLastUsedBit = 30;

  /****************************** Pretty-printing *****************************/
  constexpr char short_name() const { return wasm::short_name(kind()); }

  std::string name() const {
    std::ostringstream buf;
    switch (kind()) {
      case kRef:
      case kOptRef:
        if (encoding_needs_heap_type()) {
          buf << "(ref " << (kind() == kOptRef ? "null " : "")
              << heap_type().name() << ")";
        } else {
          buf << heap_type().name() << "ref";
        }
        break;
      case kRttWithDepth:
        buf << "(rtt " << static_cast<uint32_t>(depth()) << " " << ref_index()
            << ")";
        break;
      case kRtt:
        buf << "(rtt " << ref_index() << ")";
        break;
      default:
        buf << kind_name();
    }
    return buf.str();
  }

 private:
  // We only use 31 bits so ValueType fits in a Smi. This can be changed if
  // needed.
  static constexpr int kKindBits = 5;
  static constexpr int kHeapTypeBits = 20;
  static constexpr int kDepthBits = 6;
  STATIC_ASSERT(kV8MaxWasmTypes < (1u << kHeapTypeBits));
  // Note: we currently conservatively allow only 5 bits, but have room to
  // store 6, so we can raise the limit if needed.
  STATIC_ASSERT(kV8MaxRttSubtypingDepth < (1u << kDepthBits));
  using KindField = base::BitField<ValueKind, 0, kKindBits>;
  using HeapTypeField = KindField::Next<uint32_t, kHeapTypeBits>;
  using DepthField = HeapTypeField::Next<uint8_t, kDepthBits>;

  // This is implemented defensively against field order changes.
  STATIC_ASSERT(kLastUsedBit == std::max(KindField::kLastUsedBit,
                                         std::max(HeapTypeField::kLastUsedBit,
                                                  DepthField::kLastUsedBit)));

  constexpr explicit ValueType(uint32_t bit_field) : bit_field_(bit_field) {}

  constexpr const char* kind_name() const { return wasm::name(kind()); }

  uint32_t bit_field_;
};

static_assert(sizeof(ValueType) <= kUInt32Size,
              "ValueType is small and can be passed by value");
static_assert(ValueType::kLastUsedBit < 8 * sizeof(ValueType) - kSmiTagSize,
              "ValueType has space to be encoded in a Smi");

inline size_t hash_value(ValueType type) {
  return static_cast<size_t>(type.kind());
}

// Output operator, useful for DCHECKS and others.
inline std::ostream& operator<<(std::ostream& oss, ValueType type) {
  return oss << type.name();
}

// Precomputed primitive types.
constexpr ValueType kWasmI32 = ValueType::Primitive(kI32);
constexpr ValueType kWasmI64 = ValueType::Primitive(kI64);
constexpr ValueType kWasmF32 = ValueType::Primitive(kF32);
constexpr ValueType kWasmF64 = ValueType::Primitive(kF64);
constexpr ValueType kWasmS128 = ValueType::Primitive(kS128);
constexpr ValueType kWasmI8 = ValueType::Primitive(kI8);
constexpr ValueType kWasmI16 = ValueType::Primitive(kI16);
constexpr ValueType kWasmVoid = ValueType::Primitive(kVoid);
constexpr ValueType kWasmBottom = ValueType::Primitive(kBottom);
// Established reference-type proposal shorthands.
constexpr ValueType kWasmFuncRef = ValueType::Ref(HeapType::kFunc, kNullable);
constexpr ValueType kWasmExternRef =
    ValueType::Ref(HeapType::kExtern, kNullable);
constexpr ValueType kWasmEqRef = ValueType::Ref(HeapType::kEq, kNullable);
constexpr ValueType kWasmI31Ref = ValueType::Ref(HeapType::kI31, kNonNullable);
constexpr ValueType kWasmDataRef =
    ValueType::Ref(HeapType::kData, kNonNullable);
constexpr ValueType kWasmAnyRef = ValueType::Ref(HeapType::kAny, kNullable);

// This is used in wasm.tq.
constexpr ValueType kWasmExternNonNullableRef =
    ValueType::Ref(HeapType::kExtern, kNonNullable);

#define FOREACH_WASMVALUE_CTYPES(V) \
  V(kI32, int32_t)                  \
  V(kI64, int64_t)                  \
  V(kF32, float)                    \
  V(kF64, double)                   \
  V(kS128, Simd128)

using FunctionSig = Signature<ValueType>;

#define FOREACH_LOAD_TYPE(V) \
  V(I32, , Int32)            \
  V(I32, 8S, Int8)           \
  V(I32, 8U, Uint8)          \
  V(I32, 16S, Int16)         \
  V(I32, 16U, Uint16)        \
  V(I64, , Int64)            \
  V(I64, 8S, Int8)           \
  V(I64, 8U, Uint8)          \
  V(I64, 16S, Int16)         \
  V(I64, 16U, Uint16)        \
  V(I64, 32S, Int32)         \
  V(I64, 32U, Uint32)        \
  V(F32, , Float32)          \
  V(F64, , Float64)          \
  V(S128, , Simd128)

class LoadType {
 public:
  enum LoadTypeValue : uint8_t {
#define DEF_ENUM(type, suffix, ...) k##type##Load##suffix,
    FOREACH_LOAD_TYPE(DEF_ENUM)
#undef DEF_ENUM
  };

  // Allow implicit conversion of the enum value to this wrapper.
  constexpr LoadType(LoadTypeValue val)  // NOLINT(runtime/explicit)
      : val_(val) {}

  constexpr LoadTypeValue value() const { return val_; }
  constexpr unsigned size_log_2() const { return kLoadSizeLog2[val_]; }
  constexpr unsigned size() const { return 1 << size_log_2(); }
  constexpr ValueType value_type() const { return kValueType[val_]; }
  constexpr MachineType mem_type() const { return kMemType[val_]; }

  static LoadType ForValueKind(ValueKind kind, bool is_signed = false) {
    switch (kind) {
      case kI32:
        return kI32Load;
      case kI64:
        return kI64Load;
      case kF32:
        return kF32Load;
      case kF64:
        return kF64Load;
      case kS128:
        return kS128Load;
      case kI8:
        return is_signed ? kI32Load8S : kI32Load8U;
      case kI16:
        return is_signed ? kI32Load16S : kI32Load16U;
      default:
        UNREACHABLE();
    }
  }

 private:
  const LoadTypeValue val_;

  static constexpr uint8_t kLoadSizeLog2[] = {
  // MSVC wants a static_cast here.
#define LOAD_SIZE(_, __, memtype) \
  static_cast<uint8_t>(           \
      ElementSizeLog2Of(MachineType::memtype().representation())),
      FOREACH_LOAD_TYPE(LOAD_SIZE)
#undef LOAD_SIZE
  };

  static constexpr ValueType kValueType[] = {
#define VALUE_TYPE(type, ...) ValueType::Primitive(k##type),
      FOREACH_LOAD_TYPE(VALUE_TYPE)
#undef VALUE_TYPE
  };

  static constexpr MachineType kMemType[] = {
#define MEMTYPE(_, __, memtype) MachineType::memtype(),
      FOREACH_LOAD_TYPE(MEMTYPE)
#undef MEMTYPE
  };
};

#define FOREACH_STORE_TYPE(V) \
  V(I32, , Word32)            \
  V(I32, 8, Word8)            \
  V(I32, 16, Word16)          \
  V(I64, , Word64)            \
  V(I64, 8, Word8)            \
  V(I64, 16, Word16)          \
  V(I64, 32, Word32)          \
  V(F32, , Float32)           \
  V(F64, , Float64)           \
  V(S128, , Simd128)

class StoreType {
 public:
  enum StoreTypeValue : uint8_t {
#define DEF_ENUM(type, suffix, ...) k##type##Store##suffix,
    FOREACH_STORE_TYPE(DEF_ENUM)
#undef DEF_ENUM
  };

  // Allow implicit convertion of the enum value to this wrapper.
  constexpr StoreType(StoreTypeValue val)  // NOLINT(runtime/explicit)
      : val_(val) {}

  constexpr StoreTypeValue value() const { return val_; }
  constexpr unsigned size_log_2() const { return kStoreSizeLog2[val_]; }
  constexpr unsigned size() const { return 1 << size_log_2(); }
  constexpr ValueType value_type() const { return kValueType[val_]; }
  constexpr MachineRepresentation mem_rep() const { return kMemRep[val_]; }

  static StoreType ForValueKind(ValueKind kind) {
    switch (kind) {
      case kI32:
        return kI32Store;
      case kI64:
        return kI64Store;
      case kF32:
        return kF32Store;
      case kF64:
        return kF64Store;
      case kS128:
        return kS128Store;
      case kI8:
        return kI32Store8;
      case kI16:
        return kI32Store16;
      default:
        UNREACHABLE();
    }
  }

 private:
  const StoreTypeValue val_;

  static constexpr uint8_t kStoreSizeLog2[] = {
  // MSVC wants a static_cast here.
#define STORE_SIZE(_, __, memrep) \
  static_cast<uint8_t>(ElementSizeLog2Of(MachineRepresentation::k##memrep)),
      FOREACH_STORE_TYPE(STORE_SIZE)
#undef STORE_SIZE
  };

  static constexpr ValueType kValueType[] = {
#define VALUE_TYPE(type, ...) ValueType::Primitive(k##type),
      FOREACH_STORE_TYPE(VALUE_TYPE)
#undef VALUE_TYPE
  };

  static constexpr MachineRepresentation kMemRep[] = {
#define MEMREP(_, __, memrep) MachineRepresentation::k##memrep,
      FOREACH_STORE_TYPE(MEMREP)
#undef MEMREP
  };
};

base::Optional<wasm::ValueKind> WasmReturnTypeFromSignature(
    const FunctionSig* wasm_signature);

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_VALUE_TYPE_H_
