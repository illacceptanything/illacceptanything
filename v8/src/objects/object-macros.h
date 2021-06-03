// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note 1: Any file that includes this one should include object-macros-undef.h
// at the bottom.

// Note 2: This file is deliberately missing the include guards (the undeffing
// approach wouldn't work otherwise).
//
// PRESUBMIT_INTENTIONALLY_MISSING_INCLUDE_GUARD

// The accessors with RELAXED_, ACQUIRE_, and RELEASE_ prefixes should be used
// for fields that can be written to and read from multiple threads at the same
// time. See comments in src/base/atomicops.h for the memory ordering sematics.

#include "src/base/memory.h"

// Since this changes visibility, it should always be last in a class
// definition.
#define OBJECT_CONSTRUCTORS(Type, ...)             \
 public:                                           \
  constexpr Type() : __VA_ARGS__() {}              \
                                                   \
 protected:                                        \
  template <typename TFieldType, int kFieldOffset> \
  friend class TaggedField;                        \
                                                   \
  explicit inline Type(Address ptr)

#define OBJECT_CONSTRUCTORS_IMPL(Type, Super) \
  inline Type::Type(Address ptr) : Super(ptr) { SLOW_DCHECK(Is##Type()); }
// In these cases, we don't have our own instance type to check, so check the
// supertype instead. This happens for types denoting a NativeContext-dependent
// set of maps.
#define OBJECT_CONSTRUCTORS_IMPL_CHECK_SUPER(Type, Super) \
  inline Type::Type(Address ptr) : Super(ptr) { SLOW_DCHECK(Is##Super()); }

#define NEVER_READ_ONLY_SPACE   \
  inline Heap* GetHeap() const; \
  inline Isolate* GetIsolate() const;

// TODO(leszeks): Add checks in the factory that we never allocate these
// objects in RO space.
#define NEVER_READ_ONLY_SPACE_IMPL(Type)                                   \
  Heap* Type::GetHeap() const { return GetHeapFromWritableObject(*this); } \
  Isolate* Type::GetIsolate() const {                                      \
    return GetIsolateFromWritableObject(*this);                            \
  }

#define DECL_PRIMITIVE_GETTER(name, type) inline type name() const;

#define DECL_PRIMITIVE_SETTER(name, type) inline void set_##name(type value);

#define DECL_PRIMITIVE_ACCESSORS(name, type) \
  DECL_PRIMITIVE_GETTER(name, type)          \
  DECL_PRIMITIVE_SETTER(name, type)

#define DECL_BOOLEAN_ACCESSORS(name) DECL_PRIMITIVE_ACCESSORS(name, bool)

#define DECL_INT_ACCESSORS(name) DECL_PRIMITIVE_ACCESSORS(name, int)

#define DECL_INT32_ACCESSORS(name) DECL_PRIMITIVE_ACCESSORS(name, int32_t)

#define DECL_UINT16_ACCESSORS(name) \
  inline uint16_t name() const;     \
  inline void set_##name(int value);

#define DECL_INT16_ACCESSORS(name) \
  inline int16_t name() const;     \
  inline void set_##name(int16_t value);

#define DECL_UINT8_ACCESSORS(name) \
  inline uint8_t name() const;     \
  inline void set_##name(int value);

// TODO(ishell): eventually isolate-less getters should not be used anymore.
// For full pointer-mode the C++ compiler should optimize away unused isolate
// parameter.
#define DECL_GETTER(name, type) \
  inline type name() const;     \
  inline type name(PtrComprCageBase cage_base) const;

#define DEF_GETTER(holder, name, type)                       \
  type holder::name() const {                                \
    PtrComprCageBase cage_base = GetPtrComprCageBase(*this); \
    return holder::name(cage_base);                          \
  }                                                          \
  type holder::name(PtrComprCageBase cage_base) const

#define DECL_SETTER(name, type)      \
  inline void set_##name(type value, \
                         WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

#define DECL_ACCESSORS(name, type) \
  DECL_GETTER(name, type)          \
  DECL_SETTER(name, type)

#define DECL_ACCESSORS_LOAD_TAG(name, type, tag_type) \
  inline type name(tag_type tag) const;               \
  inline type name(PtrComprCageBase cage_base, tag_type) const;

#define DECL_ACCESSORS_STORE_TAG(name, type, tag_type) \
  inline void set_##name(type value, tag_type,         \
                         WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

#define DECL_RELAXED_GETTER(name, type) \
  DECL_ACCESSORS_LOAD_TAG(name, type, RelaxedLoadTag)

#define DECL_RELAXED_SETTER(name, type) \
  DECL_ACCESSORS_STORE_TAG(name, type, RelaxedStoreTag)

#define DECL_RELAXED_ACCESSORS(name, type) \
  DECL_RELAXED_GETTER(name, type)          \
  DECL_RELAXED_SETTER(name, type)

#define DECL_ACQUIRE_GETTER(name, type) \
  DECL_ACCESSORS_LOAD_TAG(name, type, AcquireLoadTag)

#define DECL_RELEASE_SETTER(name, type) \
  DECL_ACCESSORS_STORE_TAG(name, type, ReleaseStoreTag)

#define DECL_RELEASE_ACQUIRE_ACCESSORS(name, type) \
  DECL_ACQUIRE_GETTER(name, type)                  \
  DECL_RELEASE_SETTER(name, type)

#define DECL_RELEASE_ACQUIRE_WEAK_ACCESSORS(name) \
  DECL_ACQUIRE_GETTER(name, MaybeObject)          \
  DECL_RELEASE_SETTER(name, MaybeObject)

#define DECL_CAST(Type)                                 \
  V8_INLINE static Type cast(Object object);            \
  V8_INLINE static Type unchecked_cast(Object object) { \
    return bit_cast<Type>(object);                      \
  }

#define CAST_ACCESSOR(Type) \
  Type Type::cast(Object object) { return Type(object.ptr()); }

#define INT_ACCESSORS(holder, name, offset)                   \
  int holder::name() const { return ReadField<int>(offset); } \
  void holder::set_##name(int value) { WriteField<int>(offset, value); }

#define INT32_ACCESSORS(holder, name, offset)                         \
  int32_t holder::name() const { return ReadField<int32_t>(offset); } \
  void holder::set_##name(int32_t value) { WriteField<int32_t>(offset, value); }

#define RELAXED_INT32_ACCESSORS(holder, name, offset) \
  int32_t holder::name() const {                      \
    return RELAXED_READ_INT32_FIELD(*this, offset);   \
  }                                                   \
  void holder::set_##name(int32_t value) {            \
    RELAXED_WRITE_INT32_FIELD(*this, offset, value);  \
  }

#define UINT16_ACCESSORS(holder, name, offset)                          \
  uint16_t holder::name() const { return ReadField<uint16_t>(offset); } \
  void holder::set_##name(int value) {                                  \
    DCHECK_GE(value, 0);                                                \
    DCHECK_LE(value, static_cast<uint16_t>(-1));                        \
    WriteField<uint16_t>(offset, value);                                \
  }

#define UINT8_ACCESSORS(holder, name, offset)                         \
  uint8_t holder::name() const { return ReadField<uint8_t>(offset); } \
  void holder::set_##name(int value) {                                \
    DCHECK_GE(value, 0);                                              \
    DCHECK_LE(value, static_cast<uint8_t>(-1));                       \
    WriteField<uint8_t>(offset, value);                               \
  }

#define ACCESSORS_CHECKED2(holder, name, type, offset, get_condition, \
                           set_condition)                             \
  DEF_GETTER(holder, name, type) {                                    \
    type value = TaggedField<type, offset>::load(cage_base, *this);   \
    DCHECK(get_condition);                                            \
    return value;                                                     \
  }                                                                   \
  void holder::set_##name(type value, WriteBarrierMode mode) {        \
    DCHECK(set_condition);                                            \
    TaggedField<type, offset>::store(*this, value);                   \
    CONDITIONAL_WRITE_BARRIER(*this, offset, value, mode);            \
  }

#define ACCESSORS_CHECKED(holder, name, type, offset, condition) \
  ACCESSORS_CHECKED2(holder, name, type, offset, condition, condition)

#define ACCESSORS(holder, name, type, offset) \
  ACCESSORS_CHECKED(holder, name, type, offset, true)

#define RENAME_TORQUE_ACCESSORS(holder, name, torque_name, type)      \
  inline type holder::name() const {                                  \
    return TorqueGeneratedClass::torque_name();                       \
  }                                                                   \
  inline void holder::set_##name(type value, WriteBarrierMode mode) { \
    TorqueGeneratedClass::set_##torque_name(value, mode);             \
  }

#define RENAME_UINT16_TORQUE_ACCESSORS(holder, name, torque_name) \
  uint16_t holder::name() const {                                 \
    return TorqueGeneratedClass::torque_name();                   \
  }                                                               \
  void holder::set_##name(int value) {                            \
    DCHECK_EQ(value, static_cast<uint16_t>(value));               \
    TorqueGeneratedClass::set_##torque_name(value);               \
  }

#define RELAXED_ACCESSORS_CHECKED2(holder, name, type, offset, get_condition, \
                                   set_condition)                             \
  type holder::name(RelaxedLoadTag tag) const {                               \
    PtrComprCageBase cage_base = GetPtrComprCageBase(*this);                  \
    return holder::name(cage_base, tag);                                      \
  }                                                                           \
  type holder::name(PtrComprCageBase cage_base, RelaxedLoadTag) const {       \
    type value = TaggedField<type, offset>::Relaxed_Load(cage_base, *this);   \
    DCHECK(get_condition);                                                    \
    return value;                                                             \
  }                                                                           \
  void holder::set_##name(type value, RelaxedStoreTag,                        \
                          WriteBarrierMode mode) {                            \
    DCHECK(set_condition);                                                    \
    TaggedField<type, offset>::Relaxed_Store(*this, value);                   \
    CONDITIONAL_WRITE_BARRIER(*this, offset, value, mode);                    \
  }

#define RELAXED_ACCESSORS_CHECKED(holder, name, type, offset, condition) \
  RELAXED_ACCESSORS_CHECKED2(holder, name, type, offset, condition, condition)

#define RELAXED_ACCESSORS(holder, name, type, offset) \
  RELAXED_ACCESSORS_CHECKED(holder, name, type, offset, true)

#define RELEASE_ACQUIRE_ACCESSORS_CHECKED2(holder, name, type, offset,      \
                                           get_condition, set_condition)    \
  type holder::name(AcquireLoadTag tag) const {                             \
    PtrComprCageBase cage_base = GetPtrComprCageBase(*this);                \
    return holder::name(cage_base, tag);                                    \
  }                                                                         \
  type holder::name(PtrComprCageBase cage_base, AcquireLoadTag) const {     \
    type value = TaggedField<type, offset>::Acquire_Load(cage_base, *this); \
    DCHECK(get_condition);                                                  \
    return value;                                                           \
  }                                                                         \
  void holder::set_##name(type value, ReleaseStoreTag,                      \
                          WriteBarrierMode mode) {                          \
    DCHECK(set_condition);                                                  \
    TaggedField<type, offset>::Release_Store(*this, value);                 \
    CONDITIONAL_WRITE_BARRIER(*this, offset, value, mode);                  \
  }

#define RELEASE_ACQUIRE_ACCESSORS_CHECKED(holder, name, type, offset,       \
                                          condition)                        \
  RELEASE_ACQUIRE_ACCESSORS_CHECKED2(holder, name, type, offset, condition, \
                                     condition)

#define RELEASE_ACQUIRE_ACCESSORS(holder, name, type, offset) \
  RELEASE_ACQUIRE_ACCESSORS_CHECKED(holder, name, type, offset, true)

#define WEAK_ACCESSORS_CHECKED2(holder, name, offset, get_condition,  \
                                set_condition)                        \
  DEF_GETTER(holder, name, MaybeObject) {                             \
    MaybeObject value =                                               \
        TaggedField<MaybeObject, offset>::load(cage_base, *this);     \
    DCHECK(get_condition);                                            \
    return value;                                                     \
  }                                                                   \
  void holder::set_##name(MaybeObject value, WriteBarrierMode mode) { \
    DCHECK(set_condition);                                            \
    TaggedField<MaybeObject, offset>::store(*this, value);            \
    CONDITIONAL_WEAK_WRITE_BARRIER(*this, offset, value, mode);       \
  }

#define WEAK_ACCESSORS_CHECKED(holder, name, offset, condition) \
  WEAK_ACCESSORS_CHECKED2(holder, name, offset, condition, condition)

#define WEAK_ACCESSORS(holder, name, offset) \
  WEAK_ACCESSORS_CHECKED(holder, name, offset, true)

#define RELEASE_ACQUIRE_WEAK_ACCESSORS_CHECKED2(holder, name, offset,          \
                                                get_condition, set_condition)  \
  MaybeObject holder::name(AcquireLoadTag tag) const {                         \
    PtrComprCageBase cage_base = GetPtrComprCageBase(*this);                   \
    return holder::name(cage_base, tag);                                       \
  }                                                                            \
  MaybeObject holder::name(PtrComprCageBase cage_base, AcquireLoadTag) const { \
    MaybeObject value =                                                        \
        TaggedField<MaybeObject, offset>::Acquire_Load(cage_base, *this);      \
    DCHECK(get_condition);                                                     \
    return value;                                                              \
  }                                                                            \
  void holder::set_##name(MaybeObject value, ReleaseStoreTag,                  \
                          WriteBarrierMode mode) {                             \
    DCHECK(set_condition);                                                     \
    TaggedField<MaybeObject, offset>::Release_Store(*this, value);             \
    CONDITIONAL_WEAK_WRITE_BARRIER(*this, offset, value, mode);                \
  }

#define RELEASE_ACQUIRE_WEAK_ACCESSORS_CHECKED(holder, name, offset,       \
                                               condition)                  \
  RELEASE_ACQUIRE_WEAK_ACCESSORS_CHECKED2(holder, name, offset, condition, \
                                          condition)

#define RELEASE_ACQUIRE_WEAK_ACCESSORS(holder, name, offset) \
  RELEASE_ACQUIRE_WEAK_ACCESSORS_CHECKED(holder, name, offset, true)

// Getter that returns a Smi as an int and writes an int as a Smi.
#define SMI_ACCESSORS_CHECKED(holder, name, offset, condition)   \
  int holder::name() const {                                     \
    DCHECK(condition);                                           \
    Smi value = TaggedField<Smi, offset>::load(*this);           \
    return value.value();                                        \
  }                                                              \
  void holder::set_##name(int value) {                           \
    DCHECK(condition);                                           \
    TaggedField<Smi, offset>::store(*this, Smi::FromInt(value)); \
  }

#define SMI_ACCESSORS(holder, name, offset) \
  SMI_ACCESSORS_CHECKED(holder, name, offset, true)

#define DECL_RELEASE_ACQUIRE_INT_ACCESSORS(name) \
  inline int name(AcquireLoadTag) const;         \
  inline void set_##name(int value, ReleaseStoreTag);

#define RELEASE_ACQUIRE_SMI_ACCESSORS(holder, name, offset)              \
  int holder::name(AcquireLoadTag) const {                               \
    Smi value = TaggedField<Smi, offset>::Acquire_Load(*this);           \
    return value.value();                                                \
  }                                                                      \
  void holder::set_##name(int value, ReleaseStoreTag) {                  \
    TaggedField<Smi, offset>::Release_Store(*this, Smi::FromInt(value)); \
  }

#define DECL_RELAXED_SMI_ACCESSORS(name) \
  inline int name(RelaxedLoadTag) const; \
  inline void set_##name(int value, RelaxedStoreTag);

#define RELAXED_SMI_ACCESSORS(holder, name, offset)                      \
  int holder::name(RelaxedLoadTag) const {                               \
    Smi value = TaggedField<Smi, offset>::Relaxed_Load(*this);           \
    return value.value();                                                \
  }                                                                      \
  void holder::set_##name(int value, RelaxedStoreTag) {                  \
    TaggedField<Smi, offset>::Relaxed_Store(*this, Smi::FromInt(value)); \
  }

#define BOOL_GETTER(holder, field, name, offset) \
  bool holder::name() const { return BooleanBit::get(field(), offset); }

#define BOOL_ACCESSORS(holder, field, name, offset)                      \
  bool holder::name() const { return BooleanBit::get(field(), offset); } \
  void holder::set_##name(bool value) {                                  \
    set_##field(BooleanBit::set(field(), offset, value));                \
  }

#define DECL_RELAXED_BOOL_ACCESSORS(name) \
  inline bool name(RelaxedLoadTag) const; \
  inline void set_##name(bool value, RelaxedStoreTag);

#define RELAXED_BOOL_ACCESSORS(holder, field, name, offset)          \
  bool holder::name(RelaxedLoadTag) const {                          \
    return BooleanBit::get(field(kRelaxedLoad), offset);             \
  }                                                                  \
  void holder::set_##name(bool value, RelaxedStoreTag) {             \
    set_##field(BooleanBit::set(field(kRelaxedLoad), offset, value), \
                kRelaxedStore);                                      \
  }

#define BIT_FIELD_ACCESSORS2(holder, get_field, set_field, name, BitField) \
  typename BitField::FieldType holder::name() const {                      \
    return BitField::decode(get_field());                                  \
  }                                                                        \
  void holder::set_##name(typename BitField::FieldType value) {            \
    set_##set_field(BitField::update(set_field(), value));                 \
  }

#define BIT_FIELD_ACCESSORS(holder, field, name, BitField) \
  BIT_FIELD_ACCESSORS2(holder, field, field, name, BitField)

#define INSTANCE_TYPE_CHECKER(type, forinstancetype)    \
  V8_INLINE bool Is##type(InstanceType instance_type) { \
    return instance_type == forinstancetype;            \
  }

#define TYPE_CHECKER(type, ...)                                           \
  DEF_GETTER(HeapObject, Is##type, bool) {                                \
    return InstanceTypeChecker::Is##type(map(cage_base).instance_type()); \
  }

#define RELAXED_INT16_ACCESSORS(holder, name, offset) \
  int16_t holder::name() const {                      \
    return RELAXED_READ_INT16_FIELD(*this, offset);   \
  }                                                   \
  void holder::set_##name(int16_t value) {            \
    RELAXED_WRITE_INT16_FIELD(*this, offset, value);  \
  }

#define FIELD_ADDR(p, offset) ((p).ptr() + offset - kHeapObjectTag)

#define ACQUIRE_READ_FIELD(p, offset) \
  TaggedField<Object>::Acquire_Load(p, offset)

#define RELAXED_READ_FIELD(p, offset) \
  TaggedField<Object>::Relaxed_Load(p, offset)

#define RELAXED_READ_WEAK_FIELD(p, offset) \
  TaggedField<MaybeObject>::Relaxed_Load(p, offset)

#define WRITE_FIELD(p, offset, value) \
  TaggedField<Object>::store(p, offset, value)

#define RELEASE_WRITE_FIELD(p, offset, value) \
  TaggedField<Object>::Release_Store(p, offset, value)

#define RELAXED_WRITE_FIELD(p, offset, value) \
  TaggedField<Object>::Relaxed_Store(p, offset, value)

#define RELAXED_WRITE_WEAK_FIELD(p, offset, value) \
  TaggedField<MaybeObject>::Relaxed_Store(p, offset, value)

#ifdef V8_DISABLE_WRITE_BARRIERS
#define WRITE_BARRIER(object, offset, value)
#else
#define WRITE_BARRIER(object, offset, value)                         \
  do {                                                               \
    DCHECK_NOT_NULL(GetHeapFromWritableObject(object));              \
    WriteBarrier::Marking(object, (object).RawField(offset), value); \
    GenerationalBarrier(object, (object).RawField(offset), value);   \
  } while (false)
#endif

#ifdef V8_DISABLE_WRITE_BARRIERS
#define WEAK_WRITE_BARRIER(object, offset, value)
#else
#define WEAK_WRITE_BARRIER(object, offset, value)                             \
  do {                                                                        \
    DCHECK_NOT_NULL(GetHeapFromWritableObject(object));                       \
    WriteBarrier::Marking(object, (object).RawMaybeWeakField(offset), value); \
    GenerationalBarrier(object, (object).RawMaybeWeakField(offset), value);   \
  } while (false)
#endif

#ifdef V8_DISABLE_WRITE_BARRIERS
#define EPHEMERON_KEY_WRITE_BARRIER(object, offset, value)
#elif V8_ENABLE_UNCONDITIONAL_WRITE_BARRIERS
#define EPHEMERON_KEY_WRITE_BARRIER(object, offset, value) \
  WRITE_BARRIER(object, offset, value)
#else
#define EPHEMERON_KEY_WRITE_BARRIER(object, offset, value)                    \
  do {                                                                        \
    DCHECK_NOT_NULL(GetHeapFromWritableObject(object));                       \
    EphemeronHashTable table = EphemeronHashTable::cast(object);              \
    WriteBarrier::Marking(object, (object).RawField(offset), value);          \
    GenerationalEphemeronKeyBarrier(table, (object).RawField(offset), value); \
  } while (false)
#endif

#ifdef V8_DISABLE_WRITE_BARRIERS
#define CONDITIONAL_WRITE_BARRIER(object, offset, value, mode)
#elif V8_ENABLE_UNCONDITIONAL_WRITE_BARRIERS
#define CONDITIONAL_WRITE_BARRIER(object, offset, value, mode) \
  WRITE_BARRIER(object, offset, value)
#else
#define CONDITIONAL_WRITE_BARRIER(object, offset, value, mode)           \
  do {                                                                   \
    DCHECK_NOT_NULL(GetHeapFromWritableObject(object));                  \
    DCHECK_NE(mode, UPDATE_EPHEMERON_KEY_WRITE_BARRIER);                 \
    if (mode != SKIP_WRITE_BARRIER) {                                    \
      if (mode == UPDATE_WRITE_BARRIER) {                                \
        WriteBarrier::Marking(object, (object).RawField(offset), value); \
      }                                                                  \
      GenerationalBarrier(object, (object).RawField(offset), value);     \
    }                                                                    \
  } while (false)
#endif

#ifdef V8_DISABLE_WRITE_BARRIERS
#define CONDITIONAL_WEAK_WRITE_BARRIER(object, offset, value, mode)
#elif V8_ENABLE_UNCONDITIONAL_WRITE_BARRIERS
#define CONDITIONAL_WEAK_WRITE_BARRIER(object, offset, value, mode) \
  WRITE_BARRIER(object, offset, value)
#else
#define CONDITIONAL_WEAK_WRITE_BARRIER(object, offset, value, mode)           \
  do {                                                                        \
    DCHECK_NOT_NULL(GetHeapFromWritableObject(object));                       \
    DCHECK_NE(mode, UPDATE_EPHEMERON_KEY_WRITE_BARRIER);                      \
    if (mode != SKIP_WRITE_BARRIER) {                                         \
      if (mode == UPDATE_WRITE_BARRIER) {                                     \
        WriteBarrier::Marking(object, (object).RawMaybeWeakField(offset),     \
                              value);                                         \
      }                                                                       \
      GenerationalBarrier(object, (object).RawMaybeWeakField(offset), value); \
    }                                                                         \
  } while (false)
#endif

#ifdef V8_DISABLE_WRITE_BARRIERS
#define CONDITIONAL_EPHEMERON_KEY_WRITE_BARRIER(object, offset, value, mode)
#else
#define CONDITIONAL_EPHEMERON_KEY_WRITE_BARRIER(object, offset, value, mode) \
  do {                                                                       \
    DCHECK_NOT_NULL(GetHeapFromWritableObject(object));                      \
    DCHECK_NE(mode, UPDATE_EPHEMERON_KEY_WRITE_BARRIER);                     \
    EphemeronHashTable table = EphemeronHashTable::cast(object);             \
    if (mode != SKIP_WRITE_BARRIER) {                                        \
      if (mode == UPDATE_WRITE_BARRIER) {                                    \
        WriteBarrier::Marking(object, (object).RawField(offset), value);     \
      }                                                                      \
      GenerationalEphemeronKeyBarrier(table, (object).RawField(offset),      \
                                      value);                                \
    }                                                                        \
  } while (false)
#endif

#define ACQUIRE_READ_INT32_FIELD(p, offset) \
  static_cast<int32_t>(base::Acquire_Load(  \
      reinterpret_cast<const base::Atomic32*>(FIELD_ADDR(p, offset))))

#define RELAXED_WRITE_INT8_FIELD(p, offset, value)                             \
  base::Relaxed_Store(reinterpret_cast<base::Atomic8*>(FIELD_ADDR(p, offset)), \
                      static_cast<base::Atomic8>(value));
#define RELAXED_READ_INT8_FIELD(p, offset) \
  static_cast<int8_t>(base::Relaxed_Load(  \
      reinterpret_cast<const base::Atomic8*>(FIELD_ADDR(p, offset))))

#define RELAXED_READ_UINT16_FIELD(p, offset) \
  static_cast<uint16_t>(base::Relaxed_Load(  \
      reinterpret_cast<const base::Atomic16*>(FIELD_ADDR(p, offset))))

#define RELAXED_WRITE_UINT16_FIELD(p, offset, value)            \
  base::Relaxed_Store(                                          \
      reinterpret_cast<base::Atomic16*>(FIELD_ADDR(p, offset)), \
      static_cast<base::Atomic16>(value));

#define RELAXED_READ_INT16_FIELD(p, offset) \
  static_cast<int16_t>(base::Relaxed_Load(  \
      reinterpret_cast<const base::Atomic16*>(FIELD_ADDR(p, offset))))

#define RELAXED_WRITE_INT16_FIELD(p, offset, value)             \
  base::Relaxed_Store(                                          \
      reinterpret_cast<base::Atomic16*>(FIELD_ADDR(p, offset)), \
      static_cast<base::Atomic16>(value));

#define RELAXED_READ_UINT32_FIELD(p, offset) \
  static_cast<uint32_t>(base::Relaxed_Load(  \
      reinterpret_cast<const base::Atomic32*>(FIELD_ADDR(p, offset))))

#define ACQUIRE_READ_UINT32_FIELD(p, offset) \
  static_cast<uint32_t>(base::Acquire_Load(  \
      reinterpret_cast<const base::Atomic32*>(FIELD_ADDR(p, offset))))

#define RELAXED_WRITE_UINT32_FIELD(p, offset, value)            \
  base::Relaxed_Store(                                          \
      reinterpret_cast<base::Atomic32*>(FIELD_ADDR(p, offset)), \
      static_cast<base::Atomic32>(value));

#define RELEASE_WRITE_UINT32_FIELD(p, offset, value)            \
  base::Release_Store(                                          \
      reinterpret_cast<base::Atomic32*>(FIELD_ADDR(p, offset)), \
      static_cast<base::Atomic32>(value));

#define RELAXED_READ_INT32_FIELD(p, offset) \
  static_cast<int32_t>(base::Relaxed_Load(  \
      reinterpret_cast<const base::Atomic32*>(FIELD_ADDR(p, offset))))

#define RELEASE_WRITE_INT32_FIELD(p, offset, value)             \
  base::Release_Store(                                          \
      reinterpret_cast<base::Atomic32*>(FIELD_ADDR(p, offset)), \
      static_cast<base::Atomic32>(value))

#define RELAXED_WRITE_INT32_FIELD(p, offset, value)             \
  base::Relaxed_Store(                                          \
      reinterpret_cast<base::Atomic32*>(FIELD_ADDR(p, offset)), \
      static_cast<base::Atomic32>(value))

static_assert(sizeof(int) == sizeof(int32_t),
              "sizeof int must match sizeof int32_t");

#define RELAXED_READ_INT_FIELD(p, offset) RELAXED_READ_INT32_FIELD(p, offset)

#define RELAXED_WRITE_INT_FIELD(p, offset, value) \
  RELAXED_WRITE_INT32_FIELD(p, offset, value)

static_assert(sizeof(unsigned) == sizeof(uint32_t),
              "sizeof unsigned must match sizeof uint32_t");

#define RELAXED_READ_UINT_FIELD(p, offset) RELAXED_READ_UINT32_FIELD(p, offset)

#define RELAXED_WRITE_UINT_FIELD(p, offset, value) \
  RELAXED_WRITE_UINT32_FIELD(p, offset, value)

#define RELAXED_READ_BYTE_FIELD(p, offset) \
  static_cast<byte>(base::Relaxed_Load(    \
      reinterpret_cast<const base::Atomic8*>(FIELD_ADDR(p, offset))))

#define ACQUIRE_READ_BYTE_FIELD(p, offset) \
  static_cast<byte>(base::Acquire_Load(    \
      reinterpret_cast<const base::Atomic8*>(FIELD_ADDR(p, offset))))

#define RELAXED_WRITE_BYTE_FIELD(p, offset, value)                             \
  base::Relaxed_Store(reinterpret_cast<base::Atomic8*>(FIELD_ADDR(p, offset)), \
                      static_cast<base::Atomic8>(value));

#define RELEASE_WRITE_BYTE_FIELD(p, offset, value)                             \
  base::Release_Store(reinterpret_cast<base::Atomic8*>(FIELD_ADDR(p, offset)), \
                      static_cast<base::Atomic8>(value));

#ifdef OBJECT_PRINT
#define DECL_PRINTER(Name) void Name##Print(std::ostream& os);
#else
#define DECL_PRINTER(Name)
#endif

#ifdef VERIFY_HEAP
#define DECL_VERIFIER(Name) void Name##Verify(Isolate* isolate);
#define EXPORT_DECL_VERIFIER(Name) \
  V8_EXPORT_PRIVATE void Name##Verify(Isolate* isolate);
#else
#define DECL_VERIFIER(Name)
#define EXPORT_DECL_VERIFIER(Name)
#endif

#define DEFINE_DEOPT_ELEMENT_ACCESSORS(name, type) \
  type DeoptimizationData::name() const {          \
    return type::cast(get(k##name##Index));        \
  }                                                \
  void DeoptimizationData::Set##name(type value) { set(k##name##Index, value); }

#define DEFINE_DEOPT_ENTRY_ACCESSORS(name, type)                \
  type DeoptimizationData::name(int i) const {                  \
    return type::cast(get(IndexForEntry(i) + k##name##Offset)); \
  }                                                             \
  void DeoptimizationData::Set##name(int i, type value) {       \
    set(IndexForEntry(i) + k##name##Offset, value);             \
  }

#define TQ_OBJECT_CONSTRUCTORS(Type)               \
 public:                                           \
  constexpr Type() = default;                      \
                                                   \
 protected:                                        \
  template <typename TFieldType, int kFieldOffset> \
  friend class TaggedField;                        \
                                                   \
  inline explicit Type(Address ptr);               \
  friend class TorqueGenerated##Type<Type, Super>;

#define TQ_OBJECT_CONSTRUCTORS_IMPL(Type) \
  inline Type::Type(Address ptr)          \
      : TorqueGenerated##Type<Type, Type::Super>(ptr) {}
