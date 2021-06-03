// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_HEAP_REFS_H_
#define V8_COMPILER_HEAP_REFS_H_

#include "src/base/optional.h"
#include "src/ic/call-optimization.h"
#include "src/objects/elements-kind.h"
#include "src/objects/feedback-vector.h"
#include "src/objects/instance-type.h"
#include "src/utils/boxed-float.h"

namespace v8 {

class CFunctionInfo;

namespace internal {

class BytecodeArray;
class CallHandlerInfo;
class FixedDoubleArray;
class FunctionTemplateInfo;
class HeapNumber;
class InternalizedString;
class JSBoundFunction;
class JSDataView;
class JSGlobalProxy;
class JSTypedArray;
class NativeContext;
class ScriptContextTable;
template <typename>
class Signature;

namespace wasm {
class ValueType;
struct WasmModule;
}  // namespace wasm

namespace compiler {

class CompilationDependencies;
struct FeedbackSource;
class JSHeapBroker;
class ObjectData;
class PerIsolateCompilerCache;
class PropertyAccessInfo;

// Whether we are loading a property or storing to a property.
// For a store during literal creation, do not walk up the prototype chain.
enum class AccessMode { kLoad, kStore, kStoreInLiteral, kHas };

inline bool IsAnyStore(AccessMode mode) {
  return mode == AccessMode::kStore || mode == AccessMode::kStoreInLiteral;
}

enum class SerializationPolicy { kAssumeSerialized, kSerializeIfNeeded };

enum class OddballType : uint8_t {
  kNone,     // Not an Oddball.
  kBoolean,  // True or False.
  kUndefined,
  kNull,
  kHole,
  kUninitialized,
  kOther  // Oddball, but none of the above.
};

enum class RefSerializationKind {
  // Will skip serialization when --concurrent-inlining is on. Otherwise, they
  // might get serialized. (The cake is a lie.)
  kNeverSerialized,
  // Can be serialized on demand from the background thread.
  kBackgroundSerialized,
  kSerialized,
};

// This list is sorted such that subtypes appear before their supertypes.
// DO NOT VIOLATE THIS PROPERTY!
#define HEAP_BROKER_OBJECT_LIST(V)                                        \
  /* Subtypes of JSObject */                                              \
  V(JSArray, RefSerializationKind::kSerialized)                           \
  V(JSBoundFunction, RefSerializationKind::kSerialized)                   \
  V(JSDataView, RefSerializationKind::kSerialized)                        \
  V(JSFunction, RefSerializationKind::kSerialized)                        \
  V(JSGlobalObject, RefSerializationKind::kSerialized)                    \
  V(JSGlobalProxy, RefSerializationKind::kSerialized)                     \
  V(JSTypedArray, RefSerializationKind::kSerialized)                      \
  /* Subtypes of Context */                                               \
  V(NativeContext, RefSerializationKind::kSerialized)                     \
  /* Subtypes of FixedArray */                                            \
  V(ObjectBoilerplateDescription, RefSerializationKind::kNeverSerialized) \
  V(ScriptContextTable, RefSerializationKind::kBackgroundSerialized)      \
  /* Subtypes of String */                                                \
  V(InternalizedString, RefSerializationKind::kNeverSerialized)           \
  /* Subtypes of FixedArrayBase */                                        \
  V(BytecodeArray, RefSerializationKind::kNeverSerialized)                \
  V(FixedArray, RefSerializationKind::kBackgroundSerialized)              \
  V(FixedDoubleArray, RefSerializationKind::kNeverSerialized)             \
  /* Subtypes of Name */                                                  \
  V(String, RefSerializationKind::kNeverSerialized)                       \
  V(Symbol, RefSerializationKind::kNeverSerialized)                       \
  /* Subtypes of JSReceiver */                                            \
  V(JSObject, RefSerializationKind::kSerialized)                          \
  /* Subtypes of HeapObject */                                            \
  V(AccessorInfo, RefSerializationKind::kNeverSerialized)                 \
  V(AllocationSite, RefSerializationKind::kSerialized)                    \
  V(ArrayBoilerplateDescription, RefSerializationKind::kNeverSerialized)  \
  V(BigInt, RefSerializationKind::kBackgroundSerialized)                  \
  V(CallHandlerInfo, RefSerializationKind::kNeverSerialized)              \
  V(Cell, RefSerializationKind::kNeverSerialized)                         \
  V(Code, RefSerializationKind::kNeverSerialized)                         \
  V(Context, RefSerializationKind::kSerialized)                           \
  V(DescriptorArray, RefSerializationKind::kNeverSerialized)              \
  V(FeedbackCell, RefSerializationKind::kNeverSerialized)                 \
  V(FeedbackVector, RefSerializationKind::kNeverSerialized)               \
  V(FixedArrayBase, RefSerializationKind::kBackgroundSerialized)          \
  V(FunctionTemplateInfo, RefSerializationKind::kNeverSerialized)         \
  V(HeapNumber, RefSerializationKind::kNeverSerialized)                   \
  V(JSReceiver, RefSerializationKind::kBackgroundSerialized)              \
  V(Map, RefSerializationKind::kBackgroundSerialized)                     \
  V(Name, RefSerializationKind::kNeverSerialized)                         \
  V(PropertyCell, RefSerializationKind::kBackgroundSerialized)            \
  V(RegExpBoilerplateDescription, RefSerializationKind::kNeverSerialized) \
  V(ScopeInfo, RefSerializationKind::kNeverSerialized)                    \
  V(SharedFunctionInfo, RefSerializationKind::kNeverSerialized)           \
  V(SourceTextModule, RefSerializationKind::kNeverSerialized)             \
  V(TemplateObjectDescription, RefSerializationKind::kNeverSerialized)    \
  /* Subtypes of Object */                                                \
  V(HeapObject, RefSerializationKind::kBackgroundSerialized)

#define FORWARD_DECL(Name, ...) class Name##Ref;
HEAP_BROKER_OBJECT_LIST(FORWARD_DECL)
#undef FORWARD_DECL

class ObjectRef;

template <class T>
struct ref_traits;

#define REF_TRAITS(Name, Kind)                                           \
  template <>                                                            \
  struct ref_traits<Name> {                                              \
    using ref_type = Name##Ref;                                          \
    static constexpr RefSerializationKind ref_serialization_kind = Kind; \
  };
HEAP_BROKER_OBJECT_LIST(REF_TRAITS)
#undef REF_TYPE

template <>
struct ref_traits<Object> {
  using ref_type = ObjectRef;
  // Note: While a bit awkward, this artificial ref serialization kind value is
  // okay: smis are never-serialized, and we never create raw non-smi
  // ObjectRefs (they would at least be HeapObjectRefs instead).
  static constexpr RefSerializationKind ref_serialization_kind =
      RefSerializationKind::kNeverSerialized;
};

class V8_EXPORT_PRIVATE ObjectRef {
 public:
  ObjectRef(JSHeapBroker* broker, Handle<Object> object,
            bool check_type = true);
  ObjectRef(JSHeapBroker* broker, ObjectData* data, bool check_type = true)
      : data_(data), broker_(broker) {
    CHECK_NOT_NULL(data_);
  }
  Handle<Object> object() const;

  bool equals(const ObjectRef& other) const;
  bool ShouldHaveBeenSerialized() const;

  bool IsSmi() const;
  int AsSmi() const;

#define HEAP_IS_METHOD_DECL(Name, ...) bool Is##Name() const;
  HEAP_BROKER_OBJECT_LIST(HEAP_IS_METHOD_DECL)
#undef HEAP_IS_METHOD_DECL

#define HEAP_AS_METHOD_DECL(Name, ...) Name##Ref As##Name() const;
  HEAP_BROKER_OBJECT_LIST(HEAP_AS_METHOD_DECL)
#undef HEAP_AS_METHOD_DECL

  bool IsNull() const;
  bool IsNullOrUndefined() const;
  bool IsTheHole() const;

  base::Optional<bool> TryGetBooleanValue() const;
  Maybe<double> OddballToNumber() const;

  Isolate* isolate() const;

  struct Hash {
    size_t operator()(const ObjectRef& ref) const {
      return base::hash_combine(ref.object().address());
    }
  };
  struct Equal {
    bool operator()(const ObjectRef& lhs, const ObjectRef& rhs) const {
      return lhs.equals(rhs);
    }
  };

#ifdef DEBUG
  bool IsNeverSerializedHeapObject() const;
#endif  // DEBUG

 protected:
  JSHeapBroker* broker() const;
  ObjectData* data() const;
  ObjectData* data_;  // Should be used only by object() getters.

 private:
  friend class FunctionTemplateInfoRef;
  friend class JSArrayData;
  friend class JSGlobalObjectData;
  friend class JSGlobalObjectRef;
  friend class JSHeapBroker;
  friend class JSObjectData;
  friend class StringData;

  friend std::ostream& operator<<(std::ostream& os, const ObjectRef& ref);

  JSHeapBroker* broker_;
};

// Temporary class that carries information from a Map. We'd like to remove
// this class and use MapRef instead, but we can't as long as we support the
// kDisabled broker mode. That's because obtaining the MapRef via
// HeapObjectRef::map() requires a HandleScope when the broker is disabled.
// During OptimizeGraph we generally don't have a HandleScope, however. There
// are two places where we therefore use GetHeapObjectType() instead. Both that
// function and this class should eventually be removed.
class HeapObjectType {
 public:
  enum Flag : uint8_t { kUndetectable = 1 << 0, kCallable = 1 << 1 };

  using Flags = base::Flags<Flag>;

  HeapObjectType(InstanceType instance_type, Flags flags,
                 OddballType oddball_type)
      : instance_type_(instance_type),
        oddball_type_(oddball_type),
        flags_(flags) {
    DCHECK_EQ(instance_type == ODDBALL_TYPE,
              oddball_type != OddballType::kNone);
  }

  OddballType oddball_type() const { return oddball_type_; }
  InstanceType instance_type() const { return instance_type_; }
  Flags flags() const { return flags_; }

  bool is_callable() const { return flags_ & kCallable; }
  bool is_undetectable() const { return flags_ & kUndetectable; }

 private:
  InstanceType const instance_type_;
  OddballType const oddball_type_;
  Flags const flags_;
};

// Constructors are carefully defined such that we do a type check on
// the outermost Ref class in the inheritance chain only.
#define DEFINE_REF_CONSTRUCTOR(Name, Base)                                  \
  Name##Ref(JSHeapBroker* broker, ObjectData* data, bool check_type = true) \
      : Base(broker, data, false) {                                         \
    if (check_type) {                                                       \
      CHECK(Is##Name());                                                    \
    }                                                                       \
  }

class HeapObjectRef : public ObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(HeapObject, ObjectRef)

  Handle<HeapObject> object() const;

  MapRef map() const;

  // See the comment on the HeapObjectType class.
  HeapObjectType GetHeapObjectType() const;
};

class PropertyCellRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(PropertyCell, HeapObjectRef)

  Handle<PropertyCell> object() const;

  // Can be called from a background thread.
  V8_WARN_UNUSED_RESULT bool Serialize() const;
  void SerializeAsProtector() const {
    bool serialized = Serialize();
    // A protector always holds a Smi value and its cell type never changes, so
    // Serialize can't fail.
    CHECK(serialized);
  }

  PropertyDetails property_details() const;
  ObjectRef value() const;
};

class JSReceiverRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSReceiver, HeapObjectRef)

  Handle<JSReceiver> object() const;
};

class JSObjectRef : public JSReceiverRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSObject, JSReceiverRef)

  Handle<JSObject> object() const;

  ObjectRef RawFastPropertyAt(FieldIndex index) const;

  // Return the element at key {index} if {index} is known to be an own data
  // property of the object that is non-writable and non-configurable.
  base::Optional<ObjectRef> GetOwnConstantElement(
      uint32_t index, SerializationPolicy policy =
                          SerializationPolicy::kAssumeSerialized) const;

  // Return the value of the property identified by the field {index}
  // if {index} is known to be an own data property of the object.
  base::Optional<ObjectRef> GetOwnFastDataProperty(
      Representation field_representation, FieldIndex index,
      SerializationPolicy policy =
          SerializationPolicy::kAssumeSerialized) const;

  // Return the value of the dictionary property at {index} in the dictionary
  // if {index} is known to be an own data property of the object.
  ObjectRef GetOwnDictionaryProperty(
      InternalIndex index, SerializationPolicy policy =
                               SerializationPolicy::kAssumeSerialized) const;

  base::Optional<FixedArrayBaseRef> elements() const;
  void SerializeElements();
  void EnsureElementsTenured();
  ElementsKind GetElementsKind() const;

  void SerializeObjectCreateMap();
  base::Optional<MapRef> GetObjectCreateMap() const;
};

class JSDataViewRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSDataView, JSObjectRef)

  Handle<JSDataView> object() const;

  size_t byte_length() const;
};

class JSBoundFunctionRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSBoundFunction, JSObjectRef)

  Handle<JSBoundFunction> object() const;

  bool Serialize();
  bool serialized() const;

  // The following are available only after calling Serialize().
  JSReceiverRef bound_target_function() const;
  ObjectRef bound_this() const;
  FixedArrayRef bound_arguments() const;
};

class V8_EXPORT_PRIVATE JSFunctionRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSFunction, JSObjectRef)

  Handle<JSFunction> object() const;

  bool has_feedback_vector() const;
  bool has_initial_map() const;
  bool has_prototype() const;
  bool PrototypeRequiresRuntimeLookup() const;

  void Serialize();
  bool serialized() const;

  // The following are available only after calling Serialize().
  ObjectRef prototype() const;
  MapRef initial_map() const;
  ContextRef context() const;
  NativeContextRef native_context() const;
  SharedFunctionInfoRef shared() const;
  int InitialMapInstanceSizeWithMinSlack() const;

  void SerializeCodeAndFeedback();
  bool serialized_code_and_feedback() const;

  // The following are available only after calling SerializeCodeAndFeedback().
  // TODO(mvstanton): Once we allow inlining of functions we didn't see
  // during serialization, we do need to ensure that any feedback vector
  // we read here has been fully initialized (ie, store-ordered into the
  // cell).
  FeedbackVectorRef feedback_vector() const;
  FeedbackCellRef raw_feedback_cell() const;
  CodeRef code() const;
};

class RegExpBoilerplateDescriptionRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(RegExpBoilerplateDescription, HeapObjectRef)

  Handle<RegExpBoilerplateDescription> object() const;

  void Serialize();

  FixedArrayRef data() const;
  StringRef source() const;
  int flags() const;
};

// HeapNumberRef is only created for immutable HeapNumbers. Mutable
// HeapNumbers (those owned by in-object or backing store fields with
// representation type Double are not exposed to the compiler through
// HeapNumberRef. Instead, we read their value, and protect that read
// with a field-constness Dependency.
class HeapNumberRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(HeapNumber, HeapObjectRef)

  Handle<HeapNumber> object() const;

  double value() const;
};

class ContextRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(Context, HeapObjectRef)

  Handle<Context> object() const;

  // {previous} decrements {depth} by 1 for each previous link successfully
  // followed. If {depth} != 0 on function return, then it only got partway to
  // the desired depth. If {serialize} is true, then {previous} will cache its
  // findings (unless concurrent inlining is enabled).
  ContextRef previous(size_t* depth,
                      SerializationPolicy policy =
                          SerializationPolicy::kAssumeSerialized) const;

  // Only returns a value if the index is valid for this ContextRef.
  base::Optional<ObjectRef> get(
      int index, SerializationPolicy policy =
                     SerializationPolicy::kAssumeSerialized) const;

  SourceTextModuleRef GetModule(SerializationPolicy policy) const;
};

// TODO(jgruber): Don't serialize NativeContext fields once all refs can be
// created concurrently.

#define BROKER_COMPULSORY_NATIVE_CONTEXT_FIELDS(V) \
  V(JSFunction, array_function)                    \
  V(JSFunction, function_prototype_apply)          \
  V(JSFunction, boolean_function)                  \
  V(JSFunction, bigint_function)                   \
  V(JSFunction, number_function)                   \
  V(JSFunction, object_function)                   \
  V(JSFunction, promise_function)                  \
  V(JSFunction, promise_then)                      \
  V(JSFunction, regexp_function)                   \
  V(JSFunction, string_function)                   \
  V(JSFunction, symbol_function)                   \
  V(JSGlobalObject, global_object)                 \
  V(JSGlobalProxy, global_proxy_object)            \
  V(JSObject, promise_prototype)                   \
  V(Map, bound_function_with_constructor_map)      \
  V(Map, bound_function_without_constructor_map)   \
  V(Map, js_array_holey_double_elements_map)       \
  V(Map, js_array_holey_elements_map)              \
  V(Map, js_array_holey_smi_elements_map)          \
  V(Map, js_array_packed_double_elements_map)      \
  V(Map, js_array_packed_elements_map)             \
  V(Map, js_array_packed_smi_elements_map)         \
  V(ScriptContextTable, script_context_table)

#define BROKER_OPTIONAL_NATIVE_CONTEXT_FIELDS(V) \
  V(JSFunction, regexp_exec_function)

#define BROKER_COMPULSORY_BACKGROUND_NATIVE_CONTEXT_FIELDS(V) \
  V(Map, block_context_map)                                   \
  V(Map, catch_context_map)                                   \
  V(Map, eval_context_map)                                    \
  V(Map, fast_aliased_arguments_map)                          \
  V(Map, function_context_map)                                \
  V(Map, initial_array_iterator_map)                          \
  V(Map, initial_string_iterator_map)                         \
  V(Map, iterator_result_map)                                 \
  V(Map, sloppy_arguments_map)                                \
  V(Map, slow_object_with_null_prototype_map)                 \
  V(Map, strict_arguments_map)                                \
  V(Map, with_context_map)

// Those are set by Bootstrapper::ExportFromRuntime, which may not yet have
// happened when Turbofan is invoked via --always-opt.
#define BROKER_OPTIONAL_BACKGROUND_NATIVE_CONTEXT_FIELDS(V) \
  V(Map, async_function_object_map)                         \
  V(Map, map_key_iterator_map)                              \
  V(Map, map_key_value_iterator_map)                        \
  V(Map, map_value_iterator_map)                            \
  V(Map, set_key_value_iterator_map)                        \
  V(Map, set_value_iterator_map)

#define BROKER_NATIVE_CONTEXT_FIELDS(V)                 \
  BROKER_COMPULSORY_NATIVE_CONTEXT_FIELDS(V)            \
  BROKER_OPTIONAL_NATIVE_CONTEXT_FIELDS(V)              \
  BROKER_COMPULSORY_BACKGROUND_NATIVE_CONTEXT_FIELDS(V) \
  BROKER_OPTIONAL_BACKGROUND_NATIVE_CONTEXT_FIELDS(V)

class NativeContextRef : public ContextRef {
 public:
  DEFINE_REF_CONSTRUCTOR(NativeContext, ContextRef)

  bool is_unserialized_heap_object() const;

  Handle<NativeContext> object() const;

  void Serialize();
  void SerializeOnBackground();

#define DECL_ACCESSOR(type, name) type##Ref name() const;
  BROKER_NATIVE_CONTEXT_FIELDS(DECL_ACCESSOR)
#undef DECL_ACCESSOR

  ScopeInfoRef scope_info() const;
  MapRef GetFunctionMapFromIndex(int index) const;
  MapRef GetInitialJSArrayMap(ElementsKind kind) const;
  base::Optional<JSFunctionRef> GetConstructorFunction(const MapRef& map) const;
};

class NameRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(Name, HeapObjectRef)

  Handle<Name> object() const;

  bool IsUniqueName() const;
};

class DescriptorArrayRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(DescriptorArray, HeapObjectRef)

  Handle<DescriptorArray> object() const;

  PropertyDetails GetPropertyDetails(InternalIndex descriptor_index) const;
  NameRef GetPropertyKey(InternalIndex descriptor_index) const;
  ObjectRef GetFieldType(InternalIndex descriptor_index) const;
  base::Optional<ObjectRef> GetStrongValue(
      InternalIndex descriptor_index) const;
};

class FeedbackCellRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(FeedbackCell, HeapObjectRef)

  Handle<FeedbackCell> object() const;
  base::Optional<SharedFunctionInfoRef> shared_function_info() const;

  // TODO(mvstanton): Once we allow inlining of functions we didn't see
  // during serialization, we do need to ensure that any feedback vector
  // we read here has been fully initialized (ie, store-ordered into the
  // cell).
  base::Optional<FeedbackVectorRef> value() const;
};

class FeedbackVectorRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(FeedbackVector, HeapObjectRef)

  Handle<FeedbackVector> object() const;

  SharedFunctionInfoRef shared_function_info() const;
  double invocation_count() const;

  void Serialize();
  bool serialized() const;
  FeedbackCellRef GetClosureFeedbackCell(int index) const;
};

class CallHandlerInfoRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(CallHandlerInfo, HeapObjectRef)

  Handle<CallHandlerInfo> object() const;

  Address callback() const;
  ObjectRef data() const;
};

class AccessorInfoRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(AccessorInfo, HeapObjectRef)

  Handle<AccessorInfo> object() const;
};

class AllocationSiteRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(AllocationSite, HeapObjectRef)

  Handle<AllocationSite> object() const;

  bool PointsToLiteral() const;
  AllocationType GetAllocationType() const;
  ObjectRef nested_site() const;

  // {IsFastLiteral} determines whether the given array or object literal
  // boilerplate satisfies all limits to be considered for fast deep-copying
  // and computes the total size of all objects that are part of the graph.
  //
  // If PointsToLiteral() is false, then IsFastLiteral() is also false.
  bool IsFastLiteral() const;

  void SerializeBoilerplate();

  // We only serialize boilerplate if IsFastLiteral is true.
  base::Optional<JSObjectRef> boilerplate() const;

  ElementsKind GetElementsKind() const;
  bool CanInlineCall() const;
};

class BigIntRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(BigInt, HeapObjectRef)

  Handle<BigInt> object() const;

  uint64_t AsUint64() const;
};

class V8_EXPORT_PRIVATE MapRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(Map, HeapObjectRef)

  Handle<Map> object() const;

  int instance_size() const;
  InstanceType instance_type() const;
  int GetInObjectProperties() const;
  int GetInObjectPropertiesStartInWords() const;
  int NumberOfOwnDescriptors() const;
  int GetInObjectPropertyOffset(int index) const;
  int constructor_function_index() const;
  int NextFreePropertyIndex() const;
  int UnusedPropertyFields() const;
  ElementsKind elements_kind() const;
  bool is_stable() const;
  bool is_constructor() const;
  bool has_prototype_slot() const;
  bool is_access_check_needed() const;
  bool is_deprecated() const;
  bool CanBeDeprecated() const;
  bool CanTransition() const;
  bool IsInobjectSlackTrackingInProgress() const;
  bool is_dictionary_map() const;
  bool IsFixedCowArrayMap() const;
  bool IsPrimitiveMap() const;
  bool is_undetectable() const;
  bool is_callable() const;
  bool has_indexed_interceptor() const;
  bool is_migration_target() const;
  bool supports_fast_array_iteration() const;
  bool supports_fast_array_resize() const;
  bool is_abandoned_prototype_map() const;

  OddballType oddball_type() const;

#define DEF_TESTER(Type, ...) bool Is##Type##Map() const;
  INSTANCE_TYPE_CHECKERS(DEF_TESTER)
#undef DEF_TESTER

  void SerializeBackPointer();
  HeapObjectRef GetBackPointer() const;

  void SerializePrototype();
  // TODO(neis): We should be able to remove TrySerializePrototype once
  // concurrent-inlining is always on. Then we can also change the return type
  // of prototype() back to HeapObjectRef.
  bool TrySerializePrototype();
  base::Optional<HeapObjectRef> prototype() const;

  void SerializeForElementLoad();

  void SerializeForElementStore();
  bool HasOnlyStablePrototypesWithFastElements(
      ZoneVector<MapRef>* prototype_maps);

  // Concerning the underlying instance_descriptors:
  bool TrySerializeOwnDescriptor(InternalIndex descriptor_index);
  void SerializeOwnDescriptor(InternalIndex descriptor_index);
  bool serialized_own_descriptor(InternalIndex descriptor_index) const;
  MapRef FindFieldOwner(InternalIndex descriptor_index) const;
  PropertyDetails GetPropertyDetails(InternalIndex descriptor_index) const;
  NameRef GetPropertyKey(InternalIndex descriptor_index) const;
  FieldIndex GetFieldIndexFor(InternalIndex descriptor_index) const;
  ObjectRef GetFieldType(InternalIndex descriptor_index) const;
  base::Optional<ObjectRef> GetStrongValue(
      InternalIndex descriptor_number) const;

  DescriptorArrayRef instance_descriptors() const;

  void SerializeRootMap();
  base::Optional<MapRef> FindRootMap() const;

  // Available after calling JSFunctionRef::Serialize on a function that has
  // this map as initial map.
  ObjectRef GetConstructor() const;
  base::Optional<MapRef> AsElementsKind(ElementsKind kind) const;
};

struct HolderLookupResult {
  HolderLookupResult(CallOptimization::HolderLookup lookup_ =
                         CallOptimization::kHolderNotFound,
                     base::Optional<JSObjectRef> holder_ = base::nullopt)
      : lookup(lookup_), holder(holder_) {}
  CallOptimization::HolderLookup lookup;
  base::Optional<JSObjectRef> holder;
};

class FunctionTemplateInfoRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(FunctionTemplateInfo, HeapObjectRef)

  Handle<FunctionTemplateInfo> object() const;

  bool is_signature_undefined() const;
  bool accept_any_receiver() const;
  // The following returns true if the CallHandlerInfo is present.
  bool has_call_code() const;

  void SerializeCallCode();
  base::Optional<CallHandlerInfoRef> call_code() const;
  ZoneVector<Address> c_functions() const;
  ZoneVector<const CFunctionInfo*> c_signatures() const;

  HolderLookupResult LookupHolderOfExpectedType(
      MapRef receiver_map,
      SerializationPolicy policy = SerializationPolicy::kAssumeSerialized);
};

class FixedArrayBaseRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(FixedArrayBase, HeapObjectRef)

  Handle<FixedArrayBase> object() const;

  int length() const;
};

class ArrayBoilerplateDescriptionRef : public HeapObjectRef {
 public:
  using HeapObjectRef::HeapObjectRef;
  Handle<ArrayBoilerplateDescription> object() const;

  int constants_elements_length() const;
};

class FixedArrayRef : public FixedArrayBaseRef {
 public:
  DEFINE_REF_CONSTRUCTOR(FixedArray, FixedArrayBaseRef)

  Handle<FixedArray> object() const;

  ObjectRef get(int i) const;

  // As above but may fail if Ref construction is not possible (e.g. for
  // serialized types on the background thread).
  // TODO(jgruber): Remove once all Ref types are never-serialized or
  // background-serialized and can thus be created on background threads.
  base::Optional<ObjectRef> TryGet(int i) const;
};

class FixedDoubleArrayRef : public FixedArrayBaseRef {
 public:
  DEFINE_REF_CONSTRUCTOR(FixedDoubleArray, FixedArrayBaseRef)

  Handle<FixedDoubleArray> object() const;

  // Due to 64-bit unaligned reads, only usable for
  // immutable-after-initialization FixedDoubleArrays protected by
  // acquire-release semantics (such as boilerplate elements).
  Float64 GetFromImmutableFixedDoubleArray(int i) const;
};

class BytecodeArrayRef : public FixedArrayBaseRef {
 public:
  DEFINE_REF_CONSTRUCTOR(BytecodeArray, FixedArrayBaseRef)

  Handle<BytecodeArray> object() const;

  // NOTE: Concurrent reads of the actual bytecodes as well as the constant pool
  // (both immutable) do not go through BytecodeArrayRef but are performed
  // directly through the handle by BytecodeArrayIterator.

  int register_count() const;
  int parameter_count() const;
  interpreter::Register incoming_new_target_or_generator_register() const;

  Handle<ByteArray> SourcePositionTable() const;

  // Exception handler table.
  Address handler_table_address() const;
  int handler_table_size() const;
};

class ScriptContextTableRef : public FixedArrayRef {
 public:
  DEFINE_REF_CONSTRUCTOR(ScriptContextTable, FixedArrayRef)

  Handle<ScriptContextTable> object() const;
};

class ObjectBoilerplateDescriptionRef : public FixedArrayRef {
 public:
  DEFINE_REF_CONSTRUCTOR(ObjectBoilerplateDescription, FixedArrayRef)

  Handle<ObjectBoilerplateDescription> object() const;

  int size() const;
};

class JSArrayRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSArray, JSObjectRef)

  Handle<JSArray> object() const;

  // The `length` property of boilerplate JSArray objects. Boilerplates are
  // immutable after initialization. Must not be used for non-boilerplate
  // JSArrays.
  ObjectRef GetBoilerplateLength() const;

  // Return the element at key {index} if the array has a copy-on-write elements
  // storage and {index} is known to be an own data property.
  // Note the value returned by this function is only valid if we ensure at
  // runtime that the backing store has not changed.
  base::Optional<ObjectRef> GetOwnCowElement(
      FixedArrayBaseRef elements_ref, uint32_t index,
      SerializationPolicy policy =
          SerializationPolicy::kAssumeSerialized) const;

  // The `JSArray::length` property; not safe to use in general, but can be
  // used in some special cases that guarantee a valid `length` value despite
  // concurrent reads.
  ObjectRef length_unsafe() const;
};

class ScopeInfoRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(ScopeInfo, HeapObjectRef)

  Handle<ScopeInfo> object() const;

  int ContextLength() const;
  bool HasOuterScopeInfo() const;
  bool HasContextExtensionSlot() const;

  // Only serialized via SerializeScopeInfoChain.
  ScopeInfoRef OuterScopeInfo() const;
  void SerializeScopeInfoChain();
};

#define BROKER_SFI_FIELDS(V)                       \
  V(int, internal_formal_parameter_count)          \
  V(bool, has_duplicate_parameters)                \
  V(int, function_map_index)                       \
  V(FunctionKind, kind)                            \
  V(LanguageMode, language_mode)                   \
  V(bool, native)                                  \
  V(bool, HasBreakInfo)                            \
  V(bool, HasBuiltinId)                            \
  V(bool, construct_as_builtin)                    \
  V(bool, HasBytecodeArray)                        \
  V(int, StartPosition)                            \
  V(bool, is_compiled)                             \
  V(bool, IsUserJavaScript)                        \
  IF_WASM(V, const wasm::WasmModule*, wasm_module) \
  IF_WASM(V, const wasm::FunctionSig*, wasm_function_signature)

class V8_EXPORT_PRIVATE SharedFunctionInfoRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(SharedFunctionInfo, HeapObjectRef)

  Handle<SharedFunctionInfo> object() const;

  int builtin_id() const;
  int context_header_size() const;
  BytecodeArrayRef GetBytecodeArray() const;
  SharedFunctionInfo::Inlineability GetInlineability() const;

#define DECL_ACCESSOR(type, name) type name() const;
  BROKER_SFI_FIELDS(DECL_ACCESSOR)
#undef DECL_ACCESSOR

  bool IsInlineable() const {
    return GetInlineability() == SharedFunctionInfo::kIsInlineable;
  }

  void SerializeFunctionTemplateInfo();
  base::Optional<FunctionTemplateInfoRef> function_template_info() const;

  void SerializeScopeInfoChain();
  ScopeInfoRef scope_info() const;
};

class StringRef : public NameRef {
 public:
  DEFINE_REF_CONSTRUCTOR(String, NameRef)

  Handle<String> object() const;

  // With concurrent inlining on, we return base::nullopt due to not being able
  // to use LookupIterator in a thread-safe way.
  base::Optional<ObjectRef> GetCharAsStringOrUndefined(
      uint32_t index, SerializationPolicy policy =
                          SerializationPolicy::kAssumeSerialized) const;

  // When concurrently accessing non-read-only non-internalized strings, we
  // return base::nullopt for these methods.
  base::Optional<int> length() const;
  base::Optional<uint16_t> GetFirstChar();
  base::Optional<double> ToNumber();

  bool IsSeqString() const;
  bool IsExternalString() const;
};

class SymbolRef : public NameRef {
 public:
  DEFINE_REF_CONSTRUCTOR(Symbol, NameRef)

  Handle<Symbol> object() const;
};

class JSTypedArrayRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSTypedArray, JSObjectRef)

  Handle<JSTypedArray> object() const;

  bool is_on_heap() const;
  size_t length() const;
  void* data_ptr() const;

  void Serialize();
  bool serialized() const;
  bool ShouldHaveBeenSerialized() const;

  HeapObjectRef buffer() const;
};

class SourceTextModuleRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(SourceTextModule, HeapObjectRef)

  Handle<SourceTextModule> object() const;

  void Serialize();

  base::Optional<CellRef> GetCell(int cell_index) const;
  base::Optional<ObjectRef> import_meta() const;
};

class TemplateObjectDescriptionRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(TemplateObjectDescription, HeapObjectRef)

  Handle<TemplateObjectDescription> object() const;
};

class CellRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(Cell, HeapObjectRef)

  Handle<Cell> object() const;
};

class JSGlobalObjectRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSGlobalObject, JSObjectRef)

  Handle<JSGlobalObject> object() const;

  bool IsDetached() const;

  // If {serialize} is false:
  //   If the property is known to exist as a property cell (on the global
  //   object), return that property cell. Otherwise (not known to exist as a
  //   property cell or known not to exist as a property cell) return nothing.
  // If {serialize} is true:
  //   Like above but potentially access the heap and serialize the necessary
  //   information.
  base::Optional<PropertyCellRef> GetPropertyCell(
      NameRef const& name, SerializationPolicy policy =
                               SerializationPolicy::kAssumeSerialized) const;
};

class JSGlobalProxyRef : public JSObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(JSGlobalProxy, JSObjectRef)

  Handle<JSGlobalProxy> object() const;
};

class CodeRef : public HeapObjectRef {
 public:
  DEFINE_REF_CONSTRUCTOR(Code, HeapObjectRef)

  Handle<Code> object() const;

  unsigned GetInlinedBytecodeSize() const;
};

class InternalizedStringRef : public StringRef {
 public:
  DEFINE_REF_CONSTRUCTOR(InternalizedString, StringRef)

  Handle<InternalizedString> object() const;
};

#undef DEFINE_REF_CONSTRUCTOR

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_HEAP_REFS_H_
