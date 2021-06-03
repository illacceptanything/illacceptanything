// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_MAP_H_
#define V8_OBJECTS_MAP_H_

#include "src/base/bit-field.h"
#include "src/common/globals.h"
#include "src/objects/code.h"
#include "src/objects/heap-object.h"
#include "src/objects/internal-index.h"
#include "src/objects/objects.h"
#include "torque-generated/bit-fields.h"
#include "torque-generated/field-offsets.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

class WasmTypeInfo;

enum InstanceType : uint16_t;

#define DATA_ONLY_VISITOR_ID_LIST(V) \
  V(BigInt)                          \
  V(ByteArray)                       \
  V(CoverageInfo)                    \
  V(DataObject)                      \
  V(FeedbackMetadata)                \
  V(FixedDoubleArray)

#define POINTER_VISITOR_ID_LIST(V)      \
  V(AllocationSite)                     \
  V(BytecodeArray)                      \
  V(Cell)                               \
  V(Code)                               \
  V(CodeDataContainer)                  \
  V(DataHandler)                        \
  V(EmbedderDataArray)                  \
  V(EphemeronHashTable)                 \
  V(FeedbackCell)                       \
  V(FreeSpace)                          \
  V(JSApiObject)                        \
  V(JSArrayBuffer)                      \
  V(JSDataView)                         \
  V(JSFunction)                         \
  V(JSObject)                           \
  V(JSObjectFast)                       \
  V(JSTypedArray)                       \
  V(JSWeakRef)                          \
  V(JSWeakCollection)                   \
  V(Map)                                \
  V(NativeContext)                      \
  V(PreparseData)                       \
  V(PropertyArray)                      \
  V(PropertyCell)                       \
  V(PrototypeInfo)                      \
  V(ShortcutCandidate)                  \
  V(SmallOrderedHashMap)                \
  V(SmallOrderedHashSet)                \
  V(SmallOrderedNameDictionary)         \
  V(SourceTextModule)                   \
  V(Struct)                             \
  V(SwissNameDictionary)                \
  V(Symbol)                             \
  V(SyntheticModule)                    \
  V(TransitionArray)                    \
  IF_WASM(V, WasmArray)                 \
  IF_WASM(V, WasmExportedFunctionData)  \
  IF_WASM(V, WasmFunctionData)          \
  IF_WASM(V, WasmIndirectFunctionTable) \
  IF_WASM(V, WasmInstanceObject)        \
  IF_WASM(V, WasmJSFunctionData)        \
  IF_WASM(V, WasmStruct)                \
  IF_WASM(V, WasmTypeInfo)              \
  V(WeakCell)

#define TORQUE_VISITOR_ID_LIST(V)     \
  TORQUE_DATA_ONLY_VISITOR_ID_LIST(V) \
  TORQUE_POINTER_VISITOR_ID_LIST(V)

// Objects with the same visitor id are processed in the same way by
// the heap visitors. The visitor ids for data only objects must precede
// other visitor ids. We rely on kDataOnlyVisitorIdCount for quick check
// of whether an object contains only data or may contain pointers.
enum VisitorId {
#define VISITOR_ID_ENUM_DECL(id) kVisit##id,
  DATA_ONLY_VISITOR_ID_LIST(VISITOR_ID_ENUM_DECL)
      TORQUE_DATA_ONLY_VISITOR_ID_LIST(VISITOR_ID_ENUM_DECL)
          kDataOnlyVisitorIdCount,
  POINTER_VISITOR_ID_LIST(VISITOR_ID_ENUM_DECL)
      TORQUE_POINTER_VISITOR_ID_LIST(VISITOR_ID_ENUM_DECL)
#undef VISITOR_ID_ENUM_DECL
          kVisitorIdCount
};

enum class ObjectFields {
  kDataOnly,
  kMaybePointers,
};

using MapHandles = std::vector<Handle<Map>>;

#include "torque-generated/src/objects/map-tq.inc"

// All heap objects have a Map that describes their structure.
//  A Map contains information about:
//  - Size information about the object
//  - How to iterate over an object (for garbage collection)
//
// Map layout:
// +---------------+-------------------------------------------------+
// |   _ Type _    | _ Description _                                 |
// +---------------+-------------------------------------------------+
// | TaggedPointer | map - Always a pointer to the MetaMap root      |
// +---------------+-------------------------------------------------+
// | Int           | The first int field                             |
//  `---+----------+-------------------------------------------------+
//      | Byte     | [instance_size]                                 |
//      +----------+-------------------------------------------------+
//      | Byte     | If Map for a primitive type:                    |
//      |          |   native context index for constructor fn       |
//      |          | If Map for an Object type:                      |
//      |          |   inobject properties start offset in words     |
//      +----------+-------------------------------------------------+
//      | Byte     | [used_or_unused_instance_size_in_words]         |
//      |          | For JSObject in fast mode this byte encodes     |
//      |          | the size of the object that includes only       |
//      |          | the used property fields or the slack size      |
//      |          | in properties backing store.                    |
//      +----------+-------------------------------------------------+
//      | Byte     | [visitor_id]                                    |
// +----+----------+-------------------------------------------------+
// | Int           | The second int field                            |
//  `---+----------+-------------------------------------------------+
//      | Short    | [instance_type]                                 |
//      +----------+-------------------------------------------------+
//      | Byte     | [bit_field]                                     |
//      |          |   - has_non_instance_prototype (bit 0)          |
//      |          |   - is_callable (bit 1)                         |
//      |          |   - has_named_interceptor (bit 2)               |
//      |          |   - has_indexed_interceptor (bit 3)             |
//      |          |   - is_undetectable (bit 4)                     |
//      |          |   - is_access_check_needed (bit 5)              |
//      |          |   - is_constructor (bit 6)                      |
//      |          |   - has_prototype_slot (bit 7)                  |
//      +----------+-------------------------------------------------+
//      | Byte     | [bit_field2]                                    |
//      |          |   - new_target_is_base (bit 0)                  |
//      |          |   - is_immutable_proto (bit 1)                  |
//      |          |   - elements_kind (bits 2..7)                   |
// +----+----------+-------------------------------------------------+
// | Int           | [bit_field3]                                    |
// |               |   - enum_length (bit 0..9)                      |
// |               |   - number_of_own_descriptors (bit 10..19)      |
// |               |   - is_prototype_map (bit 20)                   |
// |               |   - is_dictionary_map (bit 21)                  |
// |               |   - owns_descriptors (bit 22)                   |
// |               |   - is_in_retained_map_list (bit 23)            |
// |               |   - is_deprecated (bit 24)                      |
// |               |   - is_unstable (bit 25)                        |
// |               |   - is_migration_target (bit 26)                |
// |               |   - is_extensible (bit 28)                      |
// |               |   - may_have_interesting_symbols (bit 28)       |
// |               |   - construction_counter (bit 29..31)           |
// |               |                                                 |
// +*****************************************************************+
// | Int           | On systems with 64bit pointer types, there      |
// |               | is an unused 32bits after bit_field3            |
// +*****************************************************************+
// | TaggedPointer | [prototype]                                     |
// +---------------+-------------------------------------------------+
// | TaggedPointer | [constructor_or_back_pointer_or_native_context] |
// +---------------+-------------------------------------------------+
// | TaggedPointer | [instance_descriptors]                          |
// +*****************************************************************+
// | TaggedPointer | [dependent_code]                                |
// +---------------+-------------------------------------------------+
// | TaggedPointer | [prototype_validity_cell]                       |
// +---------------+-------------------------------------------------+
// | TaggedPointer | If Map is a prototype map:                      |
// |               |   [prototype_info]                              |
// |               | Else:                                           |
// |               |   [raw_transitions]                             |
// +---------------+-------------------------------------------------+

class Map : public HeapObject {
 public:
  // Instance size.
  // Size in bytes or kVariableSizeSentinel if instances do not have
  // a fixed size.
  DECL_INT_ACCESSORS(instance_size)
  // Size in words or kVariableSizeSentinel if instances do not have
  // a fixed size.
  DECL_INT_ACCESSORS(instance_size_in_words)

  // [inobject_properties_start_or_constructor_function_index]:
  // Provides access to the inobject properties start offset in words in case of
  // JSObject maps, or the constructor function index in case of primitive maps.
  DECL_INT_ACCESSORS(inobject_properties_start_or_constructor_function_index)

  // Get/set the in-object property area start offset in words in the object.
  inline int GetInObjectPropertiesStartInWords() const;
  inline void SetInObjectPropertiesStartInWords(int value);
  // Count of properties allocated in the object (JSObject only).
  inline int GetInObjectProperties() const;
  // Index of the constructor function in the native context (primitives only),
  // or the special sentinel value to indicate that there is no object wrapper
  // for the primitive (i.e. in case of null or undefined).
  static const int kNoConstructorFunctionIndex = 0;
  inline int GetConstructorFunctionIndex() const;
  inline void SetConstructorFunctionIndex(int value);
  static base::Optional<JSFunction> GetConstructorFunction(
      Map map, Context native_context);

  // Retrieve interceptors.
  DECL_GETTER(GetNamedInterceptor, InterceptorInfo)
  DECL_GETTER(GetIndexedInterceptor, InterceptorInfo)

  // Instance type.
  DECL_PRIMITIVE_ACCESSORS(instance_type, InstanceType)

  // Returns the size of the used in-object area including object header
  // (only used for JSObject in fast mode, for the other kinds of objects it
  // is equal to the instance size).
  inline int UsedInstanceSize() const;

  // Tells how many unused property fields (in-object or out-of object) are
  // available in the instance (only used for JSObject in fast mode).
  inline int UnusedPropertyFields() const;
  // Tells how many unused in-object property words are present.
  inline int UnusedInObjectProperties() const;
  // Updates the counters tracking unused fields in the object.
  inline void SetInObjectUnusedPropertyFields(int unused_property_fields);
  // Updates the counters tracking unused fields in the property array.
  inline void SetOutOfObjectUnusedPropertyFields(int unused_property_fields);
  inline void CopyUnusedPropertyFields(Map map);
  inline void CopyUnusedPropertyFieldsAdjustedForInstanceSize(Map map);
  inline void AccountAddedPropertyField();
  inline void AccountAddedOutOfObjectPropertyField(
      int unused_in_property_array);

  //
  // Bit field.
  //
  // The setter in this pair calls the relaxed setter if concurrent marking is
  // on, or performs the write non-atomically if it's off. The read is always
  // non-atomically. This is done to have wider TSAN coverage on the cases where
  // it's possible.
  DECL_PRIMITIVE_ACCESSORS(bit_field, byte)

  // Atomic accessors, used for allowlisting legitimate concurrent accesses.
  DECL_PRIMITIVE_ACCESSORS(relaxed_bit_field, byte)

  // Bit positions for |bit_field|.
  struct Bits1 {
    DEFINE_TORQUE_GENERATED_MAP_BIT_FIELDS1()
  };

  //
  // Bit field 2.
  //
  DECL_PRIMITIVE_ACCESSORS(bit_field2, byte)

  // Bit positions for |bit_field2|.
  struct Bits2 {
    DEFINE_TORQUE_GENERATED_MAP_BIT_FIELDS2()
  };

  //
  // Bit field 3.
  //
  // {bit_field3} calls the relaxed accessors if concurrent marking is on, or
  // performs the read/write non-atomically if it's off. This is done to have
  // wider TSAN coverage on the cases where it's possible.
  DECL_PRIMITIVE_ACCESSORS(bit_field3, uint32_t)

  DECL_PRIMITIVE_ACCESSORS(relaxed_bit_field3, uint32_t)
  DECL_PRIMITIVE_ACCESSORS(release_acquire_bit_field3, uint32_t)

  // Clear uninitialized padding space. This ensures that the snapshot content
  // is deterministic. Depending on the V8 build mode there could be no padding.
  V8_INLINE void clear_padding();

  // Bit positions for |bit_field3|.
  struct Bits3 {
    DEFINE_TORQUE_GENERATED_MAP_BIT_FIELDS3()
  };

  // Ensure that Torque-defined bit widths for |bit_field3| are as expected.
  STATIC_ASSERT(Bits3::EnumLengthBits::kSize == kDescriptorIndexBitCount);
  STATIC_ASSERT(Bits3::NumberOfOwnDescriptorsBits::kSize ==
                kDescriptorIndexBitCount);

  STATIC_ASSERT(Bits3::NumberOfOwnDescriptorsBits::kMax >=
                kMaxNumberOfDescriptors);

  static const int kSlackTrackingCounterStart = 7;
  static const int kSlackTrackingCounterEnd = 1;
  static const int kNoSlackTracking = 0;
  STATIC_ASSERT(kSlackTrackingCounterStart <=
                Bits3::ConstructionCounterBits::kMax);

  // Inobject slack tracking is the way to reclaim unused inobject space.
  //
  // The instance size is initially determined by adding some slack to
  // expected_nof_properties (to allow for a few extra properties added
  // after the constructor). There is no guarantee that the extra space
  // will not be wasted.
  //
  // Here is the algorithm to reclaim the unused inobject space:
  // - Detect the first constructor call for this JSFunction.
  //   When it happens enter the "in progress" state: initialize construction
  //   counter in the initial_map.
  // - While the tracking is in progress initialize unused properties of a new
  //   object with one_pointer_filler_map instead of undefined_value (the "used"
  //   part is initialized with undefined_value as usual). This way they can
  //   be resized quickly and safely.
  // - Once enough objects have been created  compute the 'slack'
  //   (traverse the map transition tree starting from the
  //   initial_map and find the lowest value of unused_property_fields).
  // - Traverse the transition tree again and decrease the instance size
  //   of every map. Existing objects will resize automatically (they are
  //   filled with one_pointer_filler_map). All further allocations will
  //   use the adjusted instance size.
  // - SharedFunctionInfo's expected_nof_properties left unmodified since
  //   allocations made using different closures could actually create different
  //   kind of objects (see prototype inheritance pattern).
  //
  //  Important: inobject slack tracking is not attempted during the snapshot
  //  creation.

  static const int kGenerousAllocationCount =
      kSlackTrackingCounterStart - kSlackTrackingCounterEnd + 1;

  // Starts the tracking by initializing object constructions countdown counter.
  void StartInobjectSlackTracking();

  // True if the object constructions countdown counter is a range
  // [kSlackTrackingCounterEnd, kSlackTrackingCounterStart].
  inline bool IsInobjectSlackTrackingInProgress() const;

  // Does the tracking step.
  inline void InobjectSlackTrackingStep(Isolate* isolate);

  // Computes inobject slack for the transition tree starting at this initial
  // map.
  int ComputeMinObjectSlack(Isolate* isolate);
  inline int InstanceSizeFromSlack(int slack) const;

  // Completes inobject slack tracking for the transition tree starting at this
  // initial map.
  V8_EXPORT_PRIVATE void CompleteInobjectSlackTracking(Isolate* isolate);

  // Tells whether the object in the prototype property will be used
  // for instances created from this function.  If the prototype
  // property is set to a value that is not a JSObject, the prototype
  // property will not be used to create instances of the function.
  // See ECMA-262, 13.2.2.
  DECL_BOOLEAN_ACCESSORS(has_non_instance_prototype)

  // Tells whether the instance has a [[Construct]] internal method.
  // This property is implemented according to ES6, section 7.2.4.
  DECL_BOOLEAN_ACCESSORS(is_constructor)

  // Tells whether the instance with this map may have properties for
  // interesting symbols on it.
  // An "interesting symbol" is one for which Name::IsInterestingSymbol()
  // returns true, i.e. a well-known symbol like @@toStringTag.
  DECL_BOOLEAN_ACCESSORS(may_have_interesting_symbols)

  DECL_BOOLEAN_ACCESSORS(has_prototype_slot)

  // Records and queries whether the instance has a named interceptor.
  DECL_BOOLEAN_ACCESSORS(has_named_interceptor)

  // Records and queries whether the instance has an indexed interceptor.
  DECL_BOOLEAN_ACCESSORS(has_indexed_interceptor)

  // Tells whether the instance is undetectable.
  // An undetectable object is a special class of JSObject: 'typeof' operator
  // returns undefined, ToBoolean returns false. Otherwise it behaves like
  // a normal JS object.  It is useful for implementing undetectable
  // document.all in Firefox & Safari.
  // See https://bugzilla.mozilla.org/show_bug.cgi?id=248549.
  DECL_BOOLEAN_ACCESSORS(is_undetectable)

  // Tells whether the instance has a [[Call]] internal method.
  // This property is implemented according to ES6, section 7.2.3.
  DECL_BOOLEAN_ACCESSORS(is_callable)

  DECL_BOOLEAN_ACCESSORS(new_target_is_base)
  DECL_BOOLEAN_ACCESSORS(is_extensible)
  DECL_BOOLEAN_ACCESSORS(is_prototype_map)
  inline bool is_abandoned_prototype_map() const;

  // Whether the instance has been added to the retained map list by
  // Heap::AddRetainedMap.
  DECL_BOOLEAN_ACCESSORS(is_in_retained_map_list)

  DECL_PRIMITIVE_ACCESSORS(elements_kind, ElementsKind)

  // Tells whether the instance has fast elements that are only Smis.
  inline bool has_fast_smi_elements() const;

  // Tells whether the instance has fast elements.
  inline bool has_fast_object_elements() const;
  inline bool has_fast_smi_or_object_elements() const;
  inline bool has_fast_double_elements() const;
  inline bool has_fast_elements() const;
  inline bool has_sloppy_arguments_elements() const;
  inline bool has_fast_sloppy_arguments_elements() const;
  inline bool has_fast_string_wrapper_elements() const;
  inline bool has_typed_array_elements() const;
  inline bool has_rab_gsab_typed_array_elements() const;
  inline bool has_typed_array_or_rab_gsab_typed_array_elements() const;
  inline bool has_dictionary_elements() const;
  inline bool has_any_nonextensible_elements() const;
  inline bool has_nonextensible_elements() const;
  inline bool has_sealed_elements() const;
  inline bool has_frozen_elements() const;

  // Weakly checks whether a map is detached from all transition trees. If this
  // returns true, the map is guaranteed to be detached. If it returns false,
  // there is no guarantee it is attached.
  inline bool IsDetached(Isolate* isolate) const;

  // Returns true if there is an object with potentially read-only elements
  // in the prototype chain. It could be a Proxy, a string wrapper,
  // an object with DICTIONARY_ELEMENTS potentially containing read-only
  // elements or an object with any frozen elements, or a slow arguments object.
  bool MayHaveReadOnlyElementsInPrototypeChain(Isolate* isolate);

  inline Map ElementsTransitionMap(Isolate* isolate);

  inline FixedArrayBase GetInitialElements() const;

  // [raw_transitions]: Provides access to the transitions storage field.
  // Don't call set_raw_transitions() directly to overwrite transitions, use
  // the TransitionArray::ReplaceTransitions() wrapper instead!
  DECL_ACCESSORS(raw_transitions, MaybeObject)
  DECL_RELEASE_ACQUIRE_WEAK_ACCESSORS(raw_transitions)
  // [prototype_info]: Per-prototype metadata. Aliased with transitions
  // (which prototype maps don't have).
  DECL_ACCESSORS(prototype_info, Object)
  // PrototypeInfo is created lazily using this helper (which installs it on
  // the given prototype's map).
  static Handle<PrototypeInfo> GetOrCreatePrototypeInfo(
      Handle<JSObject> prototype, Isolate* isolate);
  static Handle<PrototypeInfo> GetOrCreatePrototypeInfo(
      Handle<Map> prototype_map, Isolate* isolate);
  inline bool should_be_fast_prototype_map() const;
  static void SetShouldBeFastPrototypeMap(Handle<Map> map, bool value,
                                          Isolate* isolate);

  // [prototype chain validity cell]: Associated with a prototype object,
  // stored in that object's map, indicates that prototype chains through this
  // object are currently valid. The cell will be invalidated and replaced when
  // the prototype chain changes. When there's nothing to guard (for example,
  // when direct prototype is null or Proxy) this function returns Smi with
  // |kPrototypeChainValid| sentinel value.
  static Handle<Object> GetOrCreatePrototypeChainValidityCell(Handle<Map> map,
                                                              Isolate* isolate);
  static const int kPrototypeChainValid = 0;
  static const int kPrototypeChainInvalid = 1;

  static bool IsPrototypeChainInvalidated(Map map);

  // Return the map of the root of object's prototype chain.
  Map GetPrototypeChainRootMap(Isolate* isolate) const;

  V8_EXPORT_PRIVATE Map FindRootMap(Isolate* isolate) const;
  V8_EXPORT_PRIVATE Map FindFieldOwner(Isolate* isolate,
                                       InternalIndex descriptor) const;

  inline int GetInObjectPropertyOffset(int index) const;

  class FieldCounts {
   public:
    FieldCounts(int mutable_count, int const_count)
        : mutable_count_(mutable_count), const_count_(const_count) {}

    int GetTotal() const { return mutable_count() + const_count(); }

    int mutable_count() const { return mutable_count_; }
    int const_count() const { return const_count_; }

   private:
    int mutable_count_;
    int const_count_;
  };

  FieldCounts GetFieldCounts() const;
  int NumberOfFields() const;

  bool HasOutOfObjectProperties() const;

  // TODO(ishell): candidate with JSObject::MigrateToMap().
  bool InstancesNeedRewriting(Map target) const;
  bool InstancesNeedRewriting(Map target, int target_number_of_fields,
                              int target_inobject, int target_unused,
                              int* old_number_of_fields) const;
  // Returns true if the |field_type| is the most general one for
  // given |representation|.
  static inline bool IsMostGeneralFieldType(Representation representation,
                                            FieldType field_type);
  static inline bool FieldTypeIsCleared(Representation rep, FieldType type);

  // Generalizes representation and field_type if objects with given
  // instance type can have fast elements that can be transitioned by
  // stubs or optimized code to more general elements kind.
  // This generalization is necessary in order to ensure that elements kind
  // transitions performed by stubs / optimized code don't silently transition
  // fields with representation "Tagged" back to "Smi" or "HeapObject" or
  // fields with HeapObject representation and "Any" type back to "Class" type.
  static inline void GeneralizeIfCanHaveTransitionableFastElementsKind(
      Isolate* isolate, InstanceType instance_type,
      Representation* representation, Handle<FieldType>* field_type);

  V8_EXPORT_PRIVATE static Handle<Map> PrepareForDataProperty(
      Isolate* isolate, Handle<Map> old_map, InternalIndex descriptor_number,
      PropertyConstness constness, Handle<Object> value);

  V8_EXPORT_PRIVATE static Handle<Map> Normalize(Isolate* isolate,
                                                 Handle<Map> map,
                                                 ElementsKind new_elements_kind,
                                                 PropertyNormalizationMode mode,
                                                 const char* reason);

  inline static Handle<Map> Normalize(Isolate* isolate, Handle<Map> fast_map,
                                      PropertyNormalizationMode mode,
                                      const char* reason);

  // Tells whether the map is used for JSObjects in dictionary mode (ie
  // normalized objects, ie objects for which HasFastProperties returns false).
  // A map can never be used for both dictionary mode and fast mode JSObjects.
  // False by default and for HeapObjects that are not JSObjects.
  DECL_BOOLEAN_ACCESSORS(is_dictionary_map)

  // Tells whether the instance needs security checks when accessing its
  // properties.
  DECL_BOOLEAN_ACCESSORS(is_access_check_needed)

  // [prototype]: implicit prototype object.
  DECL_ACCESSORS(prototype, HeapObject)
  // TODO(jkummerow): make set_prototype private.
  V8_EXPORT_PRIVATE static void SetPrototype(
      Isolate* isolate, Handle<Map> map, Handle<HeapObject> prototype,
      bool enable_prototype_setup_mode = true);

  // [constructor]: points back to the function or FunctionTemplateInfo
  // responsible for this map.
  // The field overlaps with the back pointer. All maps in a transition tree
  // have the same constructor, so maps with back pointers can walk the
  // back pointer chain until they find the map holding their constructor.
  // Returns null_value if there's neither a constructor function nor a
  // FunctionTemplateInfo available.
  // The field also overlaps with the native context pointer for context maps,
  // and with the Wasm type info for WebAssembly object maps.
  DECL_ACCESSORS(constructor_or_back_pointer, Object)
  DECL_ACCESSORS(native_context, NativeContext)
  DECL_ACCESSORS(native_context_or_null, Object)
  DECL_ACCESSORS(wasm_type_info, WasmTypeInfo)
  DECL_GETTER(GetConstructor, Object)
  DECL_GETTER(GetFunctionTemplateInfo, FunctionTemplateInfo)
  inline void SetConstructor(Object constructor,
                             WriteBarrierMode mode = UPDATE_WRITE_BARRIER);
  // Constructor getter that performs at most the given number of steps
  // in the transition tree. Returns either the constructor or the map at
  // which the walk has stopped.
  inline Object TryGetConstructor(Isolate* isolate, int max_steps);
  // [back pointer]: points back to the parent map from which a transition
  // leads to this map. The field overlaps with the constructor (see above).
  DECL_GETTER(GetBackPointer, HeapObject)
  inline void SetBackPointer(HeapObject value,
                             WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

  // [instance descriptors]: describes the object.
  DECL_ACCESSORS(instance_descriptors, DescriptorArray)
  DECL_RELAXED_ACCESSORS(instance_descriptors, DescriptorArray)
  DECL_ACQUIRE_GETTER(instance_descriptors, DescriptorArray)
  V8_EXPORT_PRIVATE void SetInstanceDescriptors(Isolate* isolate,
                                                DescriptorArray descriptors,
                                                int number_of_own_descriptors);

  inline void UpdateDescriptors(Isolate* isolate, DescriptorArray descriptors,
                                int number_of_own_descriptors);
  inline void InitializeDescriptors(Isolate* isolate,
                                    DescriptorArray descriptors);

  // [dependent code]: list of optimized codes that weakly embed this map.
  DECL_ACCESSORS(dependent_code, DependentCode)

  // [prototype_validity_cell]: Cell containing the validity bit for prototype
  // chains or Smi(0) if uninitialized.
  // The meaning of this validity cell is different for prototype maps and
  // non-prototype maps.
  // For prototype maps the validity bit "guards" modifications of prototype
  // chains going through this object. When a prototype object changes, both its
  // own validity cell and those of all "downstream" prototypes are invalidated;
  // handlers for a given receiver embed the currently valid cell for that
  // receiver's prototype during their creation and check it on execution.
  // For non-prototype maps which are used as transitioning store handlers this
  // field contains the validity cell which guards modifications of this map's
  // prototype.
  DECL_ACCESSORS(prototype_validity_cell, Object)

  // Returns true if prototype validity cell value represents "valid" prototype
  // chain state.
  inline bool IsPrototypeValidityCellValid() const;

  inline Name GetLastDescriptorName(Isolate* isolate) const;
  inline PropertyDetails GetLastDescriptorDetails(Isolate* isolate) const;

  inline InternalIndex LastAdded() const;

  inline int NumberOfOwnDescriptors() const;
  inline void SetNumberOfOwnDescriptors(int number);
  inline InternalIndex::Range IterateOwnDescriptors() const;

  inline Cell RetrieveDescriptorsPointer();

  // Checks whether all properties are stored either in the map or on the object
  // (inobject, properties, or elements backing store), requiring no special
  // checks.
  bool OnlyHasSimpleProperties() const;
  inline int EnumLength() const;
  inline void SetEnumLength(int length);

  DECL_BOOLEAN_ACCESSORS(owns_descriptors)

  inline void mark_unstable();
  inline bool is_stable() const;

  DECL_BOOLEAN_ACCESSORS(is_migration_target)

  DECL_BOOLEAN_ACCESSORS(is_immutable_proto)

  // This counter is used for in-object slack tracking.
  // The in-object slack tracking is considered enabled when the counter is
  // non zero. The counter only has a valid count for initial maps. For
  // transitioned maps only kNoSlackTracking has a meaning, namely that inobject
  // slack tracking already finished for the transition tree. Any other value
  // indicates that either inobject slack tracking is still in progress, or that
  // the map isn't part of the transition tree anymore.
  DECL_INT_ACCESSORS(construction_counter)

  DECL_BOOLEAN_ACCESSORS(is_deprecated)
  inline bool CanBeDeprecated() const;
  // Returns a non-deprecated version of the input. If the input was not
  // deprecated, it is directly returned. Otherwise, the non-deprecated version
  // is found by re-transitioning from the root of the transition tree using the
  // descriptor array of the map. Returns MaybeHandle<Map>() if no updated map
  // is found.
  V8_EXPORT_PRIVATE static MaybeHandle<Map> TryUpdate(
      Isolate* isolate, Handle<Map> map) V8_WARN_UNUSED_RESULT;
  V8_EXPORT_PRIVATE static Map TryUpdateSlow(Isolate* isolate,
                                             Map map) V8_WARN_UNUSED_RESULT;

  // Returns a non-deprecated version of the input. This method may deprecate
  // existing maps along the way if encodings conflict. Not for use while
  // gathering type feedback. Use TryUpdate in those cases instead.
  V8_EXPORT_PRIVATE static Handle<Map> Update(Isolate* isolate,
                                              Handle<Map> map);

  static inline Handle<Map> CopyInitialMap(Isolate* isolate, Handle<Map> map);
  V8_EXPORT_PRIVATE static Handle<Map> CopyInitialMap(
      Isolate* isolate, Handle<Map> map, int instance_size,
      int in_object_properties, int unused_property_fields);
  static Handle<Map> CopyInitialMapNormalized(
      Isolate* isolate, Handle<Map> map,
      PropertyNormalizationMode mode = CLEAR_INOBJECT_PROPERTIES);
  static Handle<Map> CopyDropDescriptors(Isolate* isolate, Handle<Map> map);
  V8_EXPORT_PRIVATE static Handle<Map> CopyInsertDescriptor(
      Isolate* isolate, Handle<Map> map, Descriptor* descriptor,
      TransitionFlag flag);

  static MaybeObjectHandle WrapFieldType(Isolate* isolate,
                                         Handle<FieldType> type);
  V8_EXPORT_PRIVATE static FieldType UnwrapFieldType(MaybeObject wrapped_type);

  V8_EXPORT_PRIVATE V8_WARN_UNUSED_RESULT static MaybeHandle<Map> CopyWithField(
      Isolate* isolate, Handle<Map> map, Handle<Name> name,
      Handle<FieldType> type, PropertyAttributes attributes,
      PropertyConstness constness, Representation representation,
      TransitionFlag flag);

  V8_EXPORT_PRIVATE V8_WARN_UNUSED_RESULT static MaybeHandle<Map>
  CopyWithConstant(Isolate* isolate, Handle<Map> map, Handle<Name> name,
                   Handle<Object> constant, PropertyAttributes attributes,
                   TransitionFlag flag);

  // Returns a new map with all transitions dropped from the given map and
  // the ElementsKind set.
  static Handle<Map> TransitionElementsTo(Isolate* isolate, Handle<Map> map,
                                          ElementsKind to_kind);

  V8_EXPORT_PRIVATE static Handle<Map> AsElementsKind(Isolate* isolate,
                                                      Handle<Map> map,
                                                      ElementsKind kind);

  static Handle<Map> CopyAsElementsKind(Isolate* isolate, Handle<Map> map,
                                        ElementsKind kind, TransitionFlag flag);

  static Handle<Map> AsLanguageMode(Isolate* isolate, Handle<Map> initial_map,
                                    Handle<SharedFunctionInfo> shared_info);

  V8_EXPORT_PRIVATE static Handle<Map> CopyForPreventExtensions(
      Isolate* isolate, Handle<Map> map, PropertyAttributes attrs_to_add,
      Handle<Symbol> transition_marker, const char* reason,
      bool old_map_is_dictionary_elements_kind = false);

  // Maximal number of fast properties. Used to restrict the number of map
  // transitions to avoid an explosion in the number of maps for objects used as
  // dictionaries.
  inline bool TooManyFastProperties(StoreOrigin store_origin) const;
  V8_EXPORT_PRIVATE static Handle<Map> TransitionToDataProperty(
      Isolate* isolate, Handle<Map> map, Handle<Name> name,
      Handle<Object> value, PropertyAttributes attributes,
      PropertyConstness constness, StoreOrigin store_origin);
  V8_EXPORT_PRIVATE static Handle<Map> TransitionToAccessorProperty(
      Isolate* isolate, Handle<Map> map, Handle<Name> name,
      InternalIndex descriptor, Handle<Object> getter, Handle<Object> setter,
      PropertyAttributes attributes);

  inline void AppendDescriptor(Isolate* isolate, Descriptor* desc);

  // Returns a copy of the map, prepared for inserting into the transition
  // tree (if the |map| owns descriptors then the new one will share
  // descriptors with |map|).
  static Handle<Map> CopyForElementsTransition(Isolate* isolate,
                                               Handle<Map> map);

  // Returns a copy of the map, with all transitions dropped from the
  // instance descriptors.
  static Handle<Map> Copy(Isolate* isolate, Handle<Map> map,
                          const char* reason);
  V8_EXPORT_PRIVATE static Handle<Map> Create(Isolate* isolate,
                                              int inobject_properties);

  // Returns the next free property index (only valid for FAST MODE).
  int NextFreePropertyIndex() const;

  // Returns the number of enumerable properties.
  int NumberOfEnumerableProperties() const;

  DECL_CAST(Map)

  static inline int SlackForArraySize(int old_size, int size_limit);

  V8_EXPORT_PRIVATE static void EnsureDescriptorSlack(Isolate* isolate,
                                                      Handle<Map> map,
                                                      int slack);

  // Returns the map to be used for instances when the given {prototype} is
  // passed to an Object.create call. Might transition the given {prototype}.
  static Handle<Map> GetObjectCreateMap(Isolate* isolate,
                                        Handle<HeapObject> prototype);

  // Similar to {GetObjectCreateMap} but does not transition {prototype} and
  // fails gracefully by returning an empty handle instead.
  static MaybeHandle<Map> TryGetObjectCreateMap(Isolate* isolate,
                                                Handle<HeapObject> prototype);

  // Computes a hash value for this map, to be used in HashTables and such.
  int Hash();

  // Returns the transitioned map for this map with the most generic
  // elements_kind that's found in |candidates|, or |nullptr| if no match is
  // found at all.
  V8_EXPORT_PRIVATE Map FindElementsKindTransitionedMap(
      Isolate* isolate, MapHandles const& candidates);

  inline bool CanTransition() const;

  static Map GetInstanceTypeMap(ReadOnlyRoots roots, InstanceType type);

#define DECL_TESTER(Type, ...) inline bool Is##Type##Map() const;
  INSTANCE_TYPE_CHECKERS(DECL_TESTER)
#undef DECL_TESTER
  inline bool IsBooleanMap() const;
  inline bool IsNullOrUndefinedMap() const;
  inline bool IsPrimitiveMap() const;
  inline bool IsSpecialReceiverMap() const;
  inline bool IsCustomElementsReceiverMap() const;

  bool IsMapInArrayPrototypeChain(Isolate* isolate) const;

  // Dispatched behavior.
  void MapPrint(std::ostream& os);
  DECL_VERIFIER(Map)

#ifdef VERIFY_HEAP
  void DictionaryMapVerify(Isolate* isolate);
#endif

  DECL_PRIMITIVE_ACCESSORS(visitor_id, VisitorId)

  static ObjectFields ObjectFieldsFrom(VisitorId visitor_id) {
    return (visitor_id < kDataOnlyVisitorIdCount)
               ? ObjectFields::kDataOnly
               : ObjectFields::kMaybePointers;
  }

  V8_EXPORT_PRIVATE static Handle<Map> TransitionToPrototype(
      Isolate* isolate, Handle<Map> map, Handle<HeapObject> prototype);

  static Handle<Map> TransitionToImmutableProto(Isolate* isolate,
                                                Handle<Map> map);

  static const int kMaxPreAllocatedPropertyFields = 255;

  DEFINE_FIELD_OFFSET_CONSTANTS(HeapObject::kHeaderSize,
                                TORQUE_GENERATED_MAP_FIELDS)

  STATIC_ASSERT(kInstanceTypeOffset == Internals::kMapInstanceTypeOffset);

  class BodyDescriptor;

  // Compares this map to another to see if they describe equivalent objects,
  // up to the given |elements_kind|.
  // If |mode| is set to CLEAR_INOBJECT_PROPERTIES, |other| is treated as if
  // it had exactly zero inobject properties.
  // The "shared" flags of both this map and |other| are ignored.
  bool EquivalentToForNormalization(const Map other, ElementsKind elements_kind,
                                    PropertyNormalizationMode mode) const;
  inline bool EquivalentToForNormalization(
      const Map other, PropertyNormalizationMode mode) const;

  void PrintMapDetails(std::ostream& os);

  static inline Handle<Map> AddMissingTransitionsForTesting(
      Isolate* isolate, Handle<Map> split_map,
      Handle<DescriptorArray> descriptors);

  // Fires when the layout of an object with a leaf map changes.
  // This includes adding transitions to the leaf map or changing
  // the descriptor array.
  inline void NotifyLeafMapLayoutChange(Isolate* isolate);

  V8_EXPORT_PRIVATE static VisitorId GetVisitorId(Map map);

  // Returns true if objects with given instance type are allowed to have
  // fast transitionable elements kinds. This predicate is used to ensure
  // that objects that can have transitionable fast elements kind will not
  // get in-place generalizable fields because the elements kind transition
  // performed by stubs or optimized code can't properly generalize such
  // fields.
  static inline bool CanHaveFastTransitionableElementsKind(
      InstanceType instance_type);
  inline bool CanHaveFastTransitionableElementsKind() const;

 private:
  // This byte encodes either the instance size without the in-object slack or
  // the slack size in properties backing store.
  // Let H be JSObject::kHeaderSize / kTaggedSize.
  // If value >= H then:
  //     - all field properties are stored in the object.
  //     - there is no property array.
  //     - value * kTaggedSize is the actual object size without the slack.
  // Otherwise:
  //     - there is no slack in the object.
  //     - the property array has value slack slots.
  // Note that this encoding requires that H = JSObject::kFieldsAdded.
  DECL_INT_ACCESSORS(used_or_unused_instance_size_in_words)

  // Returns the map that this (root) map transitions to if its elements_kind
  // is changed to |elements_kind|, or |nullptr| if no such map is cached yet.
  Map LookupElementsTransitionMap(Isolate* isolate, ElementsKind elements_kind);

  // Tries to replay property transitions starting from this (root) map using
  // the descriptor array of the |map|. The |root_map| is expected to have
  // proper elements kind and therefore elements kinds transitions are not
  // taken by this function. Returns |nullptr| if matching transition map is
  // not found.
  Map TryReplayPropertyTransitions(Isolate* isolate, Map map);

  static void ConnectTransition(Isolate* isolate, Handle<Map> parent,
                                Handle<Map> child, Handle<Name> name,
                                SimpleTransitionFlag flag);

  bool EquivalentToForTransition(const Map other) const;
  bool EquivalentToForElementsKindTransition(const Map other) const;
  static Handle<Map> RawCopy(Isolate* isolate, Handle<Map> map,
                             int instance_size, int inobject_properties);
  static Handle<Map> ShareDescriptor(Isolate* isolate, Handle<Map> map,
                                     Handle<DescriptorArray> descriptors,
                                     Descriptor* descriptor);
  V8_EXPORT_PRIVATE static Handle<Map> AddMissingTransitions(
      Isolate* isolate, Handle<Map> map, Handle<DescriptorArray> descriptors);
  static void InstallDescriptors(Isolate* isolate, Handle<Map> parent_map,
                                 Handle<Map> child_map,
                                 InternalIndex new_descriptor,
                                 Handle<DescriptorArray> descriptors);
  static Handle<Map> CopyAddDescriptor(Isolate* isolate, Handle<Map> map,
                                       Descriptor* descriptor,
                                       TransitionFlag flag);
  static Handle<Map> CopyReplaceDescriptors(Isolate* isolate, Handle<Map> map,
                                            Handle<DescriptorArray> descriptors,
                                            TransitionFlag flag,
                                            MaybeHandle<Name> maybe_name,
                                            const char* reason,
                                            SimpleTransitionFlag simple_flag);

  static Handle<Map> CopyReplaceDescriptor(Isolate* isolate, Handle<Map> map,
                                           Handle<DescriptorArray> descriptors,
                                           Descriptor* descriptor,
                                           InternalIndex index,
                                           TransitionFlag flag);
  static Handle<Map> CopyNormalized(Isolate* isolate, Handle<Map> map,
                                    PropertyNormalizationMode mode);

  void DeprecateTransitionTree(Isolate* isolate);

  void ReplaceDescriptors(Isolate* isolate, DescriptorArray new_descriptors);

  // This is the equivalent of IsMap() but avoids reading the instance type so
  // it can be used concurrently without acquire load.
  V8_INLINE bool ConcurrentIsMap(PtrComprCageBase cage_base,
                                 const Object& object) const;

  // Use the high-level instance_descriptors/SetInstanceDescriptors instead.
  DECL_RELEASE_SETTER(instance_descriptors, DescriptorArray)

  static const int kFastPropertiesSoftLimit = 12;
  static const int kMaxFastProperties = 128;

  friend class MapUpdater;
  template <typename ConcreteVisitor, typename MarkingState>
  friend class MarkingVisitorBase;

  OBJECT_CONSTRUCTORS(Map, HeapObject);
};

// The cache for maps used by normalized (dictionary mode) objects.
// Such maps do not have property descriptors, so a typical program
// needs very limited number of distinct normalized maps.
class NormalizedMapCache : public WeakFixedArray {
 public:
  NEVER_READ_ONLY_SPACE
  static Handle<NormalizedMapCache> New(Isolate* isolate);

  V8_WARN_UNUSED_RESULT MaybeHandle<Map> Get(Handle<Map> fast_map,
                                             ElementsKind elements_kind,
                                             PropertyNormalizationMode mode);
  void Set(Handle<Map> fast_map, Handle<Map> normalized_map);

  DECL_CAST(NormalizedMapCache)
  DECL_VERIFIER(NormalizedMapCache)

 private:
  friend bool HeapObject::IsNormalizedMapCache(
      PtrComprCageBase cage_base) const;

  static const int kEntries = 64;

  static inline int GetIndex(Handle<Map> map);

  // The following declarations hide base class methods.
  Object get(int index);
  void set(int index, Object value);

  OBJECT_CONSTRUCTORS(NormalizedMapCache, WeakFixedArray);
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_MAP_H_
