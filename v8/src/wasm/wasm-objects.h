// Copyright 2016 the V8 project authors. All rights reserved.  Use of
// this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_WASM_OBJECTS_H_
#define V8_WASM_WASM_OBJECTS_H_

#include <memory>

#include "src/base/bit-field.h"
#include "src/base/bits.h"
#include "src/codegen/signature.h"
#include "src/debug/debug.h"
#include "src/heap/heap.h"
#include "src/objects/js-function.h"
#include "src/objects/objects.h"
#include "src/wasm/struct-types.h"
#include "src/wasm/value-type.h"

// Has to be the last include (doesn't have include guards)
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {
namespace wasm {
class InterpretedFrame;
class NativeModule;
class WasmCode;
struct WasmException;
struct WasmGlobal;
struct WasmModule;
class WasmValue;
class WireBytesRef;
}  // namespace wasm

class BreakPoint;
class JSArrayBuffer;
class SeqOneByteString;
class WasmCapiFunction;
class WasmExceptionTag;
class WasmExportedFunction;
class WasmExternalFunction;
class WasmInstanceObject;
class WasmJSFunction;
class WasmModuleObject;

enum class SharedFlag : uint8_t;

template <class CppType>
class Managed;

#include "torque-generated/src/wasm/wasm-objects-tq.inc"

#define DECL_OPTIONAL_ACCESSORS(name, type) \
  DECL_GETTER(has_##name, bool)             \
  DECL_ACCESSORS(name, type)

// A helper for an entry in an indirect function table (IFT).
// The underlying storage in the instance is used by generated code to
// call functions indirectly at runtime.
// Each entry has the following fields:
// - object = target instance, if a Wasm function, tuple if imported
// - sig_id = signature id of function
// - target = entrypoint to Wasm code or import wrapper code
class V8_EXPORT_PRIVATE IndirectFunctionTableEntry {
 public:
  inline IndirectFunctionTableEntry(Handle<WasmInstanceObject>, int table_index,
                                    int entry_index);

  inline IndirectFunctionTableEntry(Handle<WasmIndirectFunctionTable> table,
                                    int entry_index);

  void clear();
  void Set(int sig_id, Handle<WasmInstanceObject> target_instance,
           int target_func_index);
  void Set(int sig_id, Address call_target, Object ref);

  Object object_ref() const;
  int sig_id() const;
  Address target() const;

 private:
  Handle<WasmInstanceObject> const instance_;
  Handle<WasmIndirectFunctionTable> const table_;
  int const index_;
};

// A helper for an entry for an imported function, indexed statically.
// The underlying storage in the instance is used by generated code to
// call imported functions at runtime.
// Each entry is either:
//   - Wasm to JS, which has fields
//      - object = a Tuple2 of the importing instance and the callable
//      - target = entrypoint to import wrapper code
//   - Wasm to Wasm, which has fields
//      - object = target instance
//      - target = entrypoint for the function
class ImportedFunctionEntry {
 public:
  inline ImportedFunctionEntry(Handle<WasmInstanceObject>, int index);

  // Initialize this entry as a Wasm to JS call. This accepts the isolate as a
  // parameter, since it must allocate a tuple.
  V8_EXPORT_PRIVATE void SetWasmToJs(Isolate*, Handle<JSReceiver> callable,
                                     const wasm::WasmCode* wasm_to_js_wrapper);
  // Initialize this entry as a Wasm to Wasm call.
  void SetWasmToWasm(WasmInstanceObject target_instance, Address call_target);

  WasmInstanceObject instance();
  JSReceiver callable();
  Object maybe_callable();
  Object object_ref();
  Address target();

 private:
  Handle<WasmInstanceObject> const instance_;
  int const index_;
};

enum InternalizeString : bool { kInternalize = true, kNoInternalize = false };

// Representation of a WebAssembly.Module JavaScript-level object.
class WasmModuleObject : public JSObject {
 public:
  DECL_CAST(WasmModuleObject)

  DECL_ACCESSORS(managed_native_module, Managed<wasm::NativeModule>)
  DECL_ACCESSORS(export_wrappers, FixedArray)
  DECL_ACCESSORS(script, Script)
  inline wasm::NativeModule* native_module() const;
  inline const std::shared_ptr<wasm::NativeModule>& shared_native_module()
      const;
  inline const wasm::WasmModule* module() const;

  // Dispatched behavior.
  DECL_PRINTER(WasmModuleObject)
  DECL_VERIFIER(WasmModuleObject)

  DEFINE_FIELD_OFFSET_CONSTANTS(JSObject::kHeaderSize,
                                TORQUE_GENERATED_WASM_MODULE_OBJECT_FIELDS)

  // Creates a new {WasmModuleObject} for an existing {NativeModule} that is
  // reference counted and might be shared between multiple Isolates.
  V8_EXPORT_PRIVATE static Handle<WasmModuleObject> New(
      Isolate* isolate, std::shared_ptr<wasm::NativeModule> native_module,
      Handle<Script> script);
  V8_EXPORT_PRIVATE static Handle<WasmModuleObject> New(
      Isolate* isolate, std::shared_ptr<wasm::NativeModule> native_module,
      Handle<Script> script, Handle<FixedArray> export_wrappers);

  // Check whether this module was generated from asm.js source.
  inline bool is_asm_js();

  // Get the module name, if set. Returns an empty handle otherwise.
  static MaybeHandle<String> GetModuleNameOrNull(Isolate*,
                                                 Handle<WasmModuleObject>);

  // Get the function name of the function identified by the given index.
  // Returns a null handle if the function is unnamed or the name is not a valid
  // UTF-8 string.
  static MaybeHandle<String> GetFunctionNameOrNull(Isolate*,
                                                   Handle<WasmModuleObject>,
                                                   uint32_t func_index);

  // Get the raw bytes of the function name of the function identified by the
  // given index.
  // Meant to be used for debugging or frame printing.
  // Does not allocate, hence gc-safe.
  Vector<const uint8_t> GetRawFunctionName(int func_index);

  // Extract a portion of the wire bytes as UTF-8 string, optionally
  // internalized. (Prefer to internalize early if the string will be used for a
  // property lookup anyway.)
  static Handle<String> ExtractUtf8StringFromModuleBytes(
      Isolate*, Handle<WasmModuleObject>, wasm::WireBytesRef,
      InternalizeString);
  static Handle<String> ExtractUtf8StringFromModuleBytes(
      Isolate*, Vector<const uint8_t> wire_byte, wasm::WireBytesRef,
      InternalizeString);

  OBJECT_CONSTRUCTORS(WasmModuleObject, JSObject);
};

// Representation of a WebAssembly.Table JavaScript-level object.
class V8_EXPORT_PRIVATE WasmTableObject : public JSObject {
 public:
  DECL_CAST(WasmTableObject)

  // The instance in which this WasmTableObject is defined.
  // This field is undefined if the global is defined outside any Wasm module,
  // i.e., through the JS API (WebAssembly.Table).
  // Because it might be undefined, we declare it as a HeapObject.
  DECL_ACCESSORS(instance, HeapObject)
  // The entries array is at least as big as {current_length()}, but might be
  // bigger to make future growth more efficient.
  DECL_ACCESSORS(entries, FixedArray)
  DECL_INT_ACCESSORS(current_length)
  // TODO(titzer): introduce DECL_I64_ACCESSORS macro
  DECL_ACCESSORS(maximum_length, Object)
  DECL_ACCESSORS(dispatch_tables, FixedArray)
  DECL_INT_ACCESSORS(raw_type)

  // Dispatched behavior.
  DECL_PRINTER(WasmTableObject)
  DECL_VERIFIER(WasmTableObject)

  DEFINE_FIELD_OFFSET_CONSTANTS(JSObject::kHeaderSize,
                                TORQUE_GENERATED_WASM_TABLE_OBJECT_FIELDS)

  inline wasm::ValueType type();

  static int Grow(Isolate* isolate, Handle<WasmTableObject> table,
                  uint32_t count, Handle<Object> init_value);

  static Handle<WasmTableObject> New(Isolate* isolate,
                                     Handle<WasmInstanceObject> instance,
                                     wasm::ValueType type, uint32_t initial,
                                     bool has_maximum, uint32_t maximum,
                                     Handle<FixedArray>* entries);

  static void AddDispatchTable(Isolate* isolate, Handle<WasmTableObject> table,
                               Handle<WasmInstanceObject> instance,
                               int table_index);

  static bool IsInBounds(Isolate* isolate, Handle<WasmTableObject> table,
                         uint32_t entry_index);

  static bool IsValidElement(Isolate* isolate, Handle<WasmTableObject> table,
                             Handle<Object> entry);

  static void Set(Isolate* isolate, Handle<WasmTableObject> table,
                  uint32_t index, Handle<Object> entry);

  static Handle<Object> Get(Isolate* isolate, Handle<WasmTableObject> table,
                            uint32_t index);

  static void Fill(Isolate* isolate, Handle<WasmTableObject> table,
                   uint32_t start, Handle<Object> entry, uint32_t count);

  // TODO(wasm): Unify these three methods into one.
  static void UpdateDispatchTables(Isolate* isolate,
                                   Handle<WasmTableObject> table,
                                   int entry_index,
                                   const wasm::FunctionSig* sig,
                                   Handle<WasmInstanceObject> target_instance,
                                   int target_func_index);
  static void UpdateDispatchTables(Isolate* isolate,
                                   Handle<WasmTableObject> table,
                                   int entry_index,
                                   Handle<WasmJSFunction> function);
  static void UpdateDispatchTables(Isolate* isolate,
                                   Handle<WasmTableObject> table,
                                   int entry_index,
                                   Handle<WasmCapiFunction> capi_function);

  static void ClearDispatchTables(Isolate* isolate,
                                  Handle<WasmTableObject> table, int index);

  static void SetFunctionTablePlaceholder(Isolate* isolate,
                                          Handle<WasmTableObject> table,
                                          int entry_index,
                                          Handle<WasmInstanceObject> instance,
                                          int func_index);

  // This function reads the content of a function table entry and returns it
  // through the out parameters {is_valid}, {is_null}, {instance},
  // {function_index}, and {maybe_js_function}.
  static void GetFunctionTableEntry(
      Isolate* isolate, const wasm::WasmModule* module,
      Handle<WasmTableObject> table, int entry_index, bool* is_valid,
      bool* is_null, MaybeHandle<WasmInstanceObject>* instance,
      int* function_index, MaybeHandle<WasmJSFunction>* maybe_js_function);

 private:
  static void SetFunctionTableEntry(Isolate* isolate,
                                    Handle<WasmTableObject> table,
                                    Handle<FixedArray> entries, int entry_index,
                                    Handle<Object> entry);

  OBJECT_CONSTRUCTORS(WasmTableObject, JSObject);
};

// Representation of a WebAssembly.Memory JavaScript-level object.
class WasmMemoryObject : public JSObject {
 public:
  DECL_CAST(WasmMemoryObject)

  DECL_ACCESSORS(array_buffer, JSArrayBuffer)
  DECL_INT_ACCESSORS(maximum_pages)
  DECL_OPTIONAL_ACCESSORS(instances, WeakArrayList)

  // Dispatched behavior.
  DECL_PRINTER(WasmMemoryObject)
  DECL_VERIFIER(WasmMemoryObject)

  DEFINE_FIELD_OFFSET_CONSTANTS(JSObject::kHeaderSize,
                                TORQUE_GENERATED_WASM_MEMORY_OBJECT_FIELDS)

  // Add an instance to the internal (weak) list.
  V8_EXPORT_PRIVATE static void AddInstance(Isolate* isolate,
                                            Handle<WasmMemoryObject> memory,
                                            Handle<WasmInstanceObject> object);
  inline bool has_maximum_pages();

  V8_EXPORT_PRIVATE static Handle<WasmMemoryObject> New(
      Isolate* isolate, MaybeHandle<JSArrayBuffer> buffer, int maximum);

  V8_EXPORT_PRIVATE static MaybeHandle<WasmMemoryObject> New(Isolate* isolate,
                                                             int initial,
                                                             int maximum,
                                                             SharedFlag shared);

  static constexpr int kNoMaximum = -1;

  void update_instances(Isolate* isolate, Handle<JSArrayBuffer> buffer);

  V8_EXPORT_PRIVATE static int32_t Grow(Isolate*, Handle<WasmMemoryObject>,
                                        uint32_t pages);

  OBJECT_CONSTRUCTORS(WasmMemoryObject, JSObject);
};

// Representation of a WebAssembly.Global JavaScript-level object.
class WasmGlobalObject : public JSObject {
 public:
  DECL_CAST(WasmGlobalObject)

  // The instance in which this WasmGlobalObject is defined.
  // This field is undefined if the global is defined outside any Wasm module,
  // i.e., through the JS API (WebAssembly.Global).
  // Because it might be undefined, we declare it as a HeapObject.
  DECL_ACCESSORS(instance, HeapObject)
  DECL_ACCESSORS(untagged_buffer, JSArrayBuffer)
  DECL_ACCESSORS(tagged_buffer, FixedArray)
  DECL_INT32_ACCESSORS(offset)
  DECL_INT_ACCESSORS(raw_type)
  DECL_PRIMITIVE_ACCESSORS(type, wasm::ValueType)
  // TODO(7748): If we encode mutability in raw_type, turn this into a boolean
  // accessor.
  DECL_INT_ACCESSORS(is_mutable)

  // Dispatched behavior.
  DECL_PRINTER(WasmGlobalObject)
  DECL_VERIFIER(WasmGlobalObject)

  DEFINE_FIELD_OFFSET_CONSTANTS(JSObject::kHeaderSize,
                                TORQUE_GENERATED_WASM_GLOBAL_OBJECT_FIELDS)

  V8_EXPORT_PRIVATE static MaybeHandle<WasmGlobalObject> New(
      Isolate* isolate, Handle<WasmInstanceObject> instance,
      MaybeHandle<JSArrayBuffer> maybe_untagged_buffer,
      MaybeHandle<FixedArray> maybe_tagged_buffer, wasm::ValueType type,
      int32_t offset, bool is_mutable);

  inline int type_size() const;

  inline int32_t GetI32();
  inline int64_t GetI64();
  inline float GetF32();
  inline double GetF64();
  inline Handle<Object> GetRef();

  inline void SetI32(int32_t value);
  inline void SetI64(int64_t value);
  inline void SetF32(float value);
  inline void SetF64(double value);
  inline void SetExternRef(Handle<Object> value);
  inline bool SetFuncRef(Isolate* isolate, Handle<Object> value);

 private:
  // This function returns the address of the global's data in the
  // JSArrayBuffer. This buffer may be allocated on-heap, in which case it may
  // not have a fixed address.
  inline Address address() const;

  OBJECT_CONSTRUCTORS(WasmGlobalObject, JSObject);
};

// Representation of a WebAssembly.Instance JavaScript-level object.
class V8_EXPORT_PRIVATE WasmInstanceObject : public JSObject {
 public:
  DECL_CAST(WasmInstanceObject)

  DECL_ACCESSORS(module_object, WasmModuleObject)
  DECL_ACCESSORS(exports_object, JSObject)
  DECL_ACCESSORS(native_context, Context)
  DECL_OPTIONAL_ACCESSORS(memory_object, WasmMemoryObject)
  DECL_OPTIONAL_ACCESSORS(untagged_globals_buffer, JSArrayBuffer)
  DECL_OPTIONAL_ACCESSORS(tagged_globals_buffer, FixedArray)
  DECL_OPTIONAL_ACCESSORS(imported_mutable_globals_buffers, FixedArray)
  DECL_OPTIONAL_ACCESSORS(tables, FixedArray)
  DECL_OPTIONAL_ACCESSORS(indirect_function_tables, FixedArray)
  DECL_ACCESSORS(imported_function_refs, FixedArray)
  DECL_OPTIONAL_ACCESSORS(indirect_function_table_refs, FixedArray)
  DECL_OPTIONAL_ACCESSORS(managed_native_allocations, Foreign)
  DECL_OPTIONAL_ACCESSORS(exceptions_table, FixedArray)
  DECL_OPTIONAL_ACCESSORS(wasm_external_functions, FixedArray)
  DECL_ACCESSORS(managed_object_maps, FixedArray)
  DECL_PRIMITIVE_ACCESSORS(memory_start, byte*)
  DECL_PRIMITIVE_ACCESSORS(memory_size, size_t)
  DECL_PRIMITIVE_ACCESSORS(memory_mask, size_t)
  DECL_PRIMITIVE_ACCESSORS(isolate_root, Address)
  DECL_PRIMITIVE_ACCESSORS(stack_limit_address, Address)
  DECL_PRIMITIVE_ACCESSORS(real_stack_limit_address, Address)
  DECL_PRIMITIVE_ACCESSORS(imported_function_targets, Address*)
  DECL_PRIMITIVE_ACCESSORS(globals_start, byte*)
  DECL_PRIMITIVE_ACCESSORS(imported_mutable_globals, Address*)
  DECL_PRIMITIVE_ACCESSORS(indirect_function_table_size, uint32_t)
  DECL_PRIMITIVE_ACCESSORS(indirect_function_table_sig_ids, uint32_t*)
  DECL_PRIMITIVE_ACCESSORS(indirect_function_table_targets, Address*)
  DECL_PRIMITIVE_ACCESSORS(jump_table_start, Address)
  DECL_PRIMITIVE_ACCESSORS(data_segment_starts, Address*)
  DECL_PRIMITIVE_ACCESSORS(data_segment_sizes, uint32_t*)
  DECL_PRIMITIVE_ACCESSORS(dropped_elem_segments, byte*)
  DECL_PRIMITIVE_ACCESSORS(hook_on_function_call_address, Address)
  DECL_PRIMITIVE_ACCESSORS(num_liftoff_function_calls_array, uint32_t*)
  DECL_PRIMITIVE_ACCESSORS(break_on_entry, uint8_t)

  // Clear uninitialized padding space. This ensures that the snapshot content
  // is deterministic. Depending on the V8 build mode there could be no padding.
  V8_INLINE void clear_padding();

  // Dispatched behavior.
  DECL_PRINTER(WasmInstanceObject)
  DECL_VERIFIER(WasmInstanceObject)

// Layout description.
#define WASM_INSTANCE_OBJECT_FIELDS(V)                                    \
  /* Often-accessed fields go first to minimize generated code size. */   \
  V(kMemoryStartOffset, kSystemPointerSize)                               \
  V(kMemorySizeOffset, kSizetSize)                                        \
  V(kMemoryMaskOffset, kSizetSize)                                        \
  V(kStackLimitAddressOffset, kSystemPointerSize)                         \
  V(kImportedFunctionRefsOffset, kTaggedSize)                             \
  V(kImportedFunctionTargetsOffset, kSystemPointerSize)                   \
  V(kIndirectFunctionTableRefsOffset, kTaggedSize)                        \
  V(kIndirectFunctionTableTargetsOffset, kSystemPointerSize)              \
  V(kIndirectFunctionTableSigIdsOffset, kSystemPointerSize)               \
  V(kIndirectFunctionTableSizeOffset, kUInt32Size)                        \
  /* Optional padding to align system pointer size fields */              \
  V(kOptionalPaddingOffset, POINTER_SIZE_PADDING(kOptionalPaddingOffset)) \
  V(kGlobalsStartOffset, kSystemPointerSize)                              \
  V(kImportedMutableGlobalsOffset, kSystemPointerSize)                    \
  V(kIsolateRootOffset, kSystemPointerSize)                               \
  V(kJumpTableStartOffset, kSystemPointerSize)                            \
  /* End of often-accessed fields. */                                     \
  V(kModuleObjectOffset, kTaggedSize)                                     \
  V(kExportsObjectOffset, kTaggedSize)                                    \
  V(kNativeContextOffset, kTaggedSize)                                    \
  V(kMemoryObjectOffset, kTaggedSize)                                     \
  V(kUntaggedGlobalsBufferOffset, kTaggedSize)                            \
  V(kTaggedGlobalsBufferOffset, kTaggedSize)                              \
  V(kImportedMutableGlobalsBuffersOffset, kTaggedSize)                    \
  V(kTablesOffset, kTaggedSize)                                           \
  V(kIndirectFunctionTablesOffset, kTaggedSize)                           \
  V(kManagedNativeAllocationsOffset, kTaggedSize)                         \
  V(kExceptionsTableOffset, kTaggedSize)                                  \
  V(kWasmExternalFunctionsOffset, kTaggedSize)                            \
  V(kManagedObjectMapsOffset, kTaggedSize)                                \
  V(kRealStackLimitAddressOffset, kSystemPointerSize)                     \
  V(kDataSegmentStartsOffset, kSystemPointerSize)                         \
  V(kDataSegmentSizesOffset, kSystemPointerSize)                          \
  V(kDroppedElemSegmentsOffset, kSystemPointerSize)                       \
  V(kHookOnFunctionCallAddressOffset, kSystemPointerSize)                 \
  V(kNumLiftoffFunctionCallsArrayOffset, kSystemPointerSize)              \
  V(kBreakOnEntryOffset, kUInt8Size)                                      \
  /* More padding to make the header pointer-size aligned */              \
  V(kHeaderPaddingOffset, POINTER_SIZE_PADDING(kHeaderPaddingOffset))     \
  V(kHeaderSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(JSObject::kHeaderSize,
                                WASM_INSTANCE_OBJECT_FIELDS)
  STATIC_ASSERT(IsAligned(kHeaderSize, kTaggedSize));
  // TODO(ishell, v8:8875): When pointer compression is enabled 8-byte size
  // fields (external pointers, doubles and BigInt data) are only kTaggedSize
  // aligned so checking for alignments of fields bigger than kTaggedSize
  // doesn't make sense until v8:8875 is fixed.
#define ASSERT_FIELD_ALIGNED(offset, size)                                 \
  STATIC_ASSERT(size == 0 || IsAligned(offset, size) ||                    \
                (COMPRESS_POINTERS_BOOL && (size == kSystemPointerSize) && \
                 IsAligned(offset, kTaggedSize)));
  WASM_INSTANCE_OBJECT_FIELDS(ASSERT_FIELD_ALIGNED)
#undef ASSERT_FIELD_ALIGNED
#undef WASM_INSTANCE_OBJECT_FIELDS

  static constexpr uint16_t kTaggedFieldOffsets[] = {
      kImportedFunctionRefsOffset,
      kIndirectFunctionTableRefsOffset,
      kModuleObjectOffset,
      kExportsObjectOffset,
      kNativeContextOffset,
      kMemoryObjectOffset,
      kUntaggedGlobalsBufferOffset,
      kTaggedGlobalsBufferOffset,
      kImportedMutableGlobalsBuffersOffset,
      kTablesOffset,
      kIndirectFunctionTablesOffset,
      kManagedNativeAllocationsOffset,
      kExceptionsTableOffset,
      kWasmExternalFunctionsOffset,
      kManagedObjectMapsOffset};

  const wasm::WasmModule* module();

  static bool EnsureIndirectFunctionTableWithMinimumSize(
      Handle<WasmInstanceObject> instance, int table_index,
      uint32_t minimum_size);

  void SetRawMemory(byte* mem_start, size_t mem_size);

  static Handle<WasmInstanceObject> New(Isolate*, Handle<WasmModuleObject>);

  Address GetCallTarget(uint32_t func_index);

  static int IndirectFunctionTableSize(Isolate* isolate,
                                       Handle<WasmInstanceObject> instance,
                                       uint32_t table_index);

  // Copies table entries. Returns {false} if the ranges are out-of-bounds.
  static bool CopyTableEntries(Isolate* isolate,
                               Handle<WasmInstanceObject> instance,
                               uint32_t table_dst_index,
                               uint32_t table_src_index, uint32_t dst,
                               uint32_t src,
                               uint32_t count) V8_WARN_UNUSED_RESULT;

  // Copy table entries from an element segment. Returns {false} if the ranges
  // are out-of-bounds.
  static bool InitTableEntries(Isolate* isolate,
                               Handle<WasmInstanceObject> instance,
                               uint32_t table_index, uint32_t segment_index,
                               uint32_t dst, uint32_t src,
                               uint32_t count) V8_WARN_UNUSED_RESULT;

  // Iterates all fields in the object except the untagged fields.
  class BodyDescriptor;

  static MaybeHandle<WasmExternalFunction> GetWasmExternalFunction(
      Isolate* isolate, Handle<WasmInstanceObject> instance, int index);

  // Acquires the {WasmExternalFunction} for a given {function_index} from the
  // cache of the given {instance}, or creates a new {WasmExportedFunction} if
  // it does not exist yet. The new {WasmExportedFunction} is added to the
  // cache of the {instance} immediately.
  static Handle<WasmExternalFunction> GetOrCreateWasmExternalFunction(
      Isolate* isolate, Handle<WasmInstanceObject> instance,
      int function_index);

  static void SetWasmExternalFunction(Isolate* isolate,
                                      Handle<WasmInstanceObject> instance,
                                      int index,
                                      Handle<WasmExternalFunction> val);

  // Imports a constructed {WasmJSFunction} into the indirect function table of
  // this instance. Note that this might trigger wrapper compilation, since a
  // {WasmJSFunction} is instance-independent and just wraps a JS callable.
  static void ImportWasmJSFunctionIntoTable(Isolate* isolate,
                                            Handle<WasmInstanceObject> instance,
                                            int table_index, int entry_index,
                                            Handle<WasmJSFunction> js_function);

  // Get a raw pointer to the location where the given global is stored.
  // {global} must not be a reference type.
  static uint8_t* GetGlobalStorage(Handle<WasmInstanceObject>,
                                   const wasm::WasmGlobal&);

  // Get the FixedArray and the index in that FixedArray for the given global,
  // which must be a reference type.
  static std::pair<Handle<FixedArray>, uint32_t> GetGlobalBufferAndIndex(
      Handle<WasmInstanceObject>, const wasm::WasmGlobal&);

  // Get the value of a global in the given instance.
  static wasm::WasmValue GetGlobalValue(Handle<WasmInstanceObject>,
                                        const wasm::WasmGlobal&);

  OBJECT_CONSTRUCTORS(WasmInstanceObject, JSObject);

 private:
  static void InitDataSegmentArrays(Handle<WasmInstanceObject>,
                                    Handle<WasmModuleObject>);
  static void InitElemSegmentArrays(Handle<WasmInstanceObject>,
                                    Handle<WasmModuleObject>);
};

// Representation of WebAssembly.Exception JavaScript-level object.
class WasmExceptionObject : public JSObject {
 public:
  DECL_CAST(WasmExceptionObject)

  DECL_ACCESSORS(serialized_signature, PodArray<wasm::ValueType>)
  DECL_ACCESSORS(exception_tag, HeapObject)

  // Dispatched behavior.
  DECL_PRINTER(WasmExceptionObject)
  DECL_VERIFIER(WasmExceptionObject)

  DEFINE_FIELD_OFFSET_CONSTANTS(JSObject::kHeaderSize,
                                TORQUE_GENERATED_WASM_EXCEPTION_OBJECT_FIELDS)

  // Checks whether the given {sig} has the same parameter types as the
  // serialized signature stored within this exception object.
  bool MatchesSignature(const wasm::FunctionSig* sig);

  static Handle<WasmExceptionObject> New(Isolate* isolate,
                                         const wasm::FunctionSig* sig,
                                         Handle<HeapObject> exception_tag);

  OBJECT_CONSTRUCTORS(WasmExceptionObject, JSObject);
};

// A Wasm exception that has been thrown out of Wasm code.
class V8_EXPORT_PRIVATE WasmExceptionPackage : public JSReceiver {
 public:
  static Handle<WasmExceptionPackage> New(
      Isolate* isolate, Handle<WasmExceptionTag> exception_tag,
      int encoded_size);

  // The below getters return {undefined} in case the given exception package
  // does not carry the requested values (i.e. is of a different type).
  static Handle<Object> GetExceptionTag(
      Isolate* isolate, Handle<WasmExceptionPackage> exception_package);
  static Handle<Object> GetExceptionValues(
      Isolate* isolate, Handle<WasmExceptionPackage> exception_package);

  // Determines the size of the array holding all encoded exception values.
  static uint32_t GetEncodedSize(const wasm::WasmException* exception);

  DECL_CAST(WasmExceptionPackage)
  OBJECT_CONSTRUCTORS(WasmExceptionPackage, JSReceiver);
};

// A Wasm function that is wrapped and exported to JavaScript.
// Representation of WebAssembly.Function JavaScript-level object.
class WasmExportedFunction : public JSFunction {
 public:
  WasmInstanceObject instance();
  V8_EXPORT_PRIVATE int function_index();

  V8_EXPORT_PRIVATE static bool IsWasmExportedFunction(Object object);

  V8_EXPORT_PRIVATE static Handle<WasmExportedFunction> New(
      Isolate* isolate, Handle<WasmInstanceObject> instance, int func_index,
      int arity, Handle<Code> export_wrapper);

  Address GetWasmCallTarget();

  V8_EXPORT_PRIVATE const wasm::FunctionSig* sig();

  bool MatchesSignature(const wasm::WasmModule* other_module,
                        const wasm::FunctionSig* other_sig);

  // Return a null-terminated string with the debug name in the form
  // 'js-to-wasm:<sig>'.
  static std::unique_ptr<char[]> GetDebugName(const wasm::FunctionSig* sig);

  DECL_CAST(WasmExportedFunction)
  OBJECT_CONSTRUCTORS(WasmExportedFunction, JSFunction);
};

// A Wasm function that was created by wrapping a JavaScript callable.
// Representation of WebAssembly.Function JavaScript-level object.
class WasmJSFunction : public JSFunction {
 public:
  static bool IsWasmJSFunction(Object object);

  static Handle<WasmJSFunction> New(Isolate* isolate,
                                    const wasm::FunctionSig* sig,
                                    Handle<JSReceiver> callable);

  JSReceiver GetCallable() const;
  // Deserializes the signature of this function using the provided zone. Note
  // that lifetime of the signature is hence directly coupled to the zone.
  const wasm::FunctionSig* GetSignature(Zone* zone);
  bool MatchesSignature(const wasm::FunctionSig* sig);

  DECL_CAST(WasmJSFunction)
  OBJECT_CONSTRUCTORS(WasmJSFunction, JSFunction);
};

// An external function exposed to Wasm via the C/C++ API.
class WasmCapiFunction : public JSFunction {
 public:
  static bool IsWasmCapiFunction(Object object);

  static Handle<WasmCapiFunction> New(
      Isolate* isolate, Address call_target, Handle<Foreign> embedder_data,
      Handle<PodArray<wasm::ValueType>> serialized_signature);

  Address GetHostCallTarget() const;
  PodArray<wasm::ValueType> GetSerializedSignature() const;
  // Checks whether the given {sig} has the same parameter types as the
  // serialized signature stored within this C-API function object.
  bool MatchesSignature(const wasm::FunctionSig* sig) const;

  DECL_CAST(WasmCapiFunction)
  OBJECT_CONSTRUCTORS(WasmCapiFunction, JSFunction);
};

// Any external function that can be imported/exported in modules. This abstract
// class just dispatches to the following concrete classes:
//  - {WasmExportedFunction}: A proper Wasm function exported from a module.
//  - {WasmJSFunction}: A function constructed via WebAssembly.Function in JS.
// // TODO(wasm): Potentially {WasmCapiFunction} will be added here as well.
class WasmExternalFunction : public JSFunction {
 public:
  static bool IsWasmExternalFunction(Object object);

  DECL_CAST(WasmExternalFunction)
  OBJECT_CONSTRUCTORS(WasmExternalFunction, JSFunction);
};

class WasmIndirectFunctionTable : public Struct {
 public:
  DECL_PRIMITIVE_ACCESSORS(size, uint32_t)
  DECL_PRIMITIVE_ACCESSORS(sig_ids, uint32_t*)
  DECL_PRIMITIVE_ACCESSORS(targets, Address*)
  DECL_OPTIONAL_ACCESSORS(managed_native_allocations, Foreign)
  DECL_ACCESSORS(refs, FixedArray)

  V8_EXPORT_PRIVATE static Handle<WasmIndirectFunctionTable> New(
      Isolate* isolate, uint32_t size);
  static void Resize(Isolate* isolate, Handle<WasmIndirectFunctionTable> table,
                     uint32_t new_size);

  DECL_CAST(WasmIndirectFunctionTable)

  DECL_PRINTER(WasmIndirectFunctionTable)
  DECL_VERIFIER(WasmIndirectFunctionTable)

  DEFINE_FIELD_OFFSET_CONSTANTS(
      HeapObject::kHeaderSize,
      TORQUE_GENERATED_WASM_INDIRECT_FUNCTION_TABLE_FIELDS)

  STATIC_ASSERT(kStartOfStrongFieldsOffset == kManagedNativeAllocationsOffset);
  using BodyDescriptor = FlexibleBodyDescriptor<kStartOfStrongFieldsOffset>;

  OBJECT_CONSTRUCTORS(WasmIndirectFunctionTable, Struct);
};

class WasmFunctionData
    : public TorqueGeneratedWasmFunctionData<WasmFunctionData, Foreign> {
 public:
  DECL_ACCESSORS(ref, Object)

  DECL_CAST(WasmFunctionData)
  DECL_PRINTER(WasmFunctionData)

  TQ_OBJECT_CONSTRUCTORS(WasmFunctionData)
};

// Information for a WasmExportedFunction which is referenced as the function
// data of the SharedFunctionInfo underlying the function. For details please
// see the {SharedFunctionInfo::HasWasmExportedFunctionData} predicate.
class WasmExportedFunctionData : public WasmFunctionData {
 public:
  DECL_ACCESSORS(wrapper_code, Code)
  // This is the instance that exported the function (which in case of
  // imported and re-exported functions is different from the instance
  // where the function is defined -- for the latter see WasmFunctionData::ref).
  DECL_ACCESSORS(instance, WasmInstanceObject)
  DECL_INT_ACCESSORS(function_index)
  DECL_ACCESSORS(signature, Foreign)
  DECL_INT_ACCESSORS(wrapper_budget)
  DECL_ACCESSORS(c_wrapper_code, Object)
  DECL_INT_ACCESSORS(packed_args_size)

  inline wasm::FunctionSig* sig() const;

  DECL_CAST(WasmExportedFunctionData)

  // Dispatched behavior.
  DECL_PRINTER(WasmExportedFunctionData)
  DECL_VERIFIER(WasmExportedFunctionData)

  // Layout description.
  DEFINE_FIELD_OFFSET_CONSTANTS(
      WasmFunctionData::kSize,
      TORQUE_GENERATED_WASM_EXPORTED_FUNCTION_DATA_FIELDS)

  class BodyDescriptor;

  OBJECT_CONSTRUCTORS(WasmExportedFunctionData, WasmFunctionData);
};

// Information for a WasmJSFunction which is referenced as the function data of
// the SharedFunctionInfo underlying the function. For details please see the
// {SharedFunctionInfo::HasWasmJSFunctionData} predicate.
class WasmJSFunctionData : public WasmFunctionData {
 public:
  DECL_INT_ACCESSORS(serialized_return_count)
  DECL_INT_ACCESSORS(serialized_parameter_count)
  DECL_ACCESSORS(serialized_signature, PodArray<wasm::ValueType>)
  DECL_ACCESSORS(wrapper_code, Code)
  DECL_ACCESSORS(wasm_to_js_wrapper_code, Code)

  DECL_CAST(WasmJSFunctionData)

  // Dispatched behavior.
  DECL_PRINTER(WasmJSFunctionData)
  DECL_VERIFIER(WasmJSFunctionData)

  // Layout description.
  DEFINE_FIELD_OFFSET_CONSTANTS(WasmFunctionData::kSize,
                                TORQUE_GENERATED_WASM_JS_FUNCTION_DATA_FIELDS)

  class BodyDescriptor;

  OBJECT_CONSTRUCTORS(WasmJSFunctionData, WasmFunctionData);
};

class WasmScript : public AllStatic {
 public:
  // Position used for storing "on entry" breakpoints (a.k.a. instrumentation
  // breakpoints). This would be an illegal position for any other breakpoint.
  static constexpr int kOnEntryBreakpointPosition = -1;

  // Set a breakpoint on the given byte position inside the given module.
  // This will affect all live and future instances of the module.
  // The passed position might be modified to point to the next breakable
  // location inside the same function.
  // If it points outside a function, or behind the last breakable location,
  // this function returns false and does not set any breakpoint.
  V8_EXPORT_PRIVATE static bool SetBreakPoint(Handle<Script>, int* position,
                                              Handle<BreakPoint> break_point);

  // Set a breakpoint on first breakable position of the given function index
  // inside the given module. This will affect all live and future instances of
  // the module.
  V8_EXPORT_PRIVATE static bool SetBreakPointOnFirstBreakableForFunction(
      Handle<Script>, int function_index, Handle<BreakPoint> break_point);

  // Set a breakpoint at the breakable offset of the given function index
  // inside the given module. This will affect all live and future instances of
  // the module.
  V8_EXPORT_PRIVATE static bool SetBreakPointForFunction(
      Handle<Script>, int function_index, int breakable_offset,
      Handle<BreakPoint> break_point);

  // Remove a previously set breakpoint at the given byte position inside the
  // given module. If this breakpoint is not found this function returns false.
  V8_EXPORT_PRIVATE static bool ClearBreakPoint(Handle<Script>, int position,
                                                Handle<BreakPoint> break_point);

  // Remove a previously set breakpoint by id. If this breakpoint is not found,
  // returns false.
  V8_EXPORT_PRIVATE static bool ClearBreakPointById(Handle<Script>,
                                                    int breakpoint_id);

  // Remove all set breakpoints.
  static void ClearAllBreakpoints(Script);

  // Get a list of all possible breakpoints within a given range of this module.
  V8_EXPORT_PRIVATE static bool GetPossibleBreakpoints(
      wasm::NativeModule* native_module, const debug::Location& start,
      const debug::Location& end, std::vector<debug::BreakLocation>* locations);

  // Return an empty handle if no breakpoint is hit at that location, or a
  // FixedArray with all hit breakpoint objects.
  static MaybeHandle<FixedArray> CheckBreakPoints(Isolate*, Handle<Script>,
                                                  int position,
                                                  StackFrameId stack_frame_id);

 private:
  // Helper functions that update the breakpoint info list.
  static void AddBreakpointToInfo(Handle<Script>, int position,
                                  Handle<BreakPoint> break_point);
};

// Tags provide an object identity for each exception defined in a wasm module
// header. They are referenced by the following fields:
//  - {WasmExceptionObject::exception_tag}  : The tag of the exception object.
//  - {WasmInstanceObject::exceptions_table}: List of tags used by an instance.
class WasmExceptionTag
    : public TorqueGeneratedWasmExceptionTag<WasmExceptionTag, Struct> {
 public:
  V8_EXPORT_PRIVATE static Handle<WasmExceptionTag> New(Isolate* isolate,
                                                        int index);

  DECL_PRINTER(WasmExceptionTag)

  TQ_OBJECT_CONSTRUCTORS(WasmExceptionTag)
};

// Data annotated to the asm.js Module function. Used for later instantiation of
// that function.
class AsmWasmData : public Struct {
 public:
  static Handle<AsmWasmData> New(
      Isolate* isolate, std::shared_ptr<wasm::NativeModule> native_module,
      Handle<FixedArray> export_wrappers, Handle<HeapNumber> uses_bitset);

  DECL_ACCESSORS(managed_native_module, Managed<wasm::NativeModule>)
  DECL_ACCESSORS(export_wrappers, FixedArray)
  DECL_ACCESSORS(uses_bitset, HeapNumber)

  DECL_CAST(AsmWasmData)
  DECL_PRINTER(AsmWasmData)
  DECL_VERIFIER(AsmWasmData)

  DEFINE_FIELD_OFFSET_CONSTANTS(Struct::kHeaderSize,
                                TORQUE_GENERATED_ASM_WASM_DATA_FIELDS)

  OBJECT_CONSTRUCTORS(AsmWasmData, Struct);
};

class WasmTypeInfo : public TorqueGeneratedWasmTypeInfo<WasmTypeInfo, Foreign> {
 public:
  inline void clear_foreign_address(Isolate* isolate);

  DECL_CAST(WasmTypeInfo)
  DECL_PRINTER(WasmTypeInfo)

  class BodyDescriptor;

  TQ_OBJECT_CONSTRUCTORS(WasmTypeInfo)
};

class WasmStruct : public TorqueGeneratedWasmStruct<WasmStruct, HeapObject> {
 public:
  static inline wasm::StructType* type(Map map);
  inline wasm::StructType* type() const;
  static inline wasm::StructType* GcSafeType(Map map);
  static inline int Size(const wasm::StructType* type);
  static inline int GcSafeSize(Map map);

  inline ObjectSlot RawField(int raw_offset);

  wasm::WasmValue GetFieldValue(uint32_t field_index);

  DECL_CAST(WasmStruct)
  DECL_PRINTER(WasmStruct)

  class BodyDescriptor;

  TQ_OBJECT_CONSTRUCTORS(WasmStruct)
};

class WasmArray : public TorqueGeneratedWasmArray<WasmArray, HeapObject> {
 public:
  static inline wasm::ArrayType* type(Map map);
  inline wasm::ArrayType* type() const;
  static inline wasm::ArrayType* GcSafeType(Map map);

  wasm::WasmValue GetElement(uint32_t index);

  static inline int SizeFor(Map map, int length);
  static inline int GcSafeSizeFor(Map map, int length);

  DECL_CAST(WasmArray)
  DECL_PRINTER(WasmArray)

  class BodyDescriptor;

  TQ_OBJECT_CONSTRUCTORS(WasmArray)
};

#undef DECL_OPTIONAL_ACCESSORS

namespace wasm {

Handle<Map> CreateStructMap(Isolate* isolate, const WasmModule* module,
                            int struct_index, MaybeHandle<Map> rtt_parent);
Handle<Map> CreateArrayMap(Isolate* isolate, const WasmModule* module,
                           int array_index, MaybeHandle<Map> rtt_parent);
Handle<Map> AllocateSubRtt(Isolate* isolate,
                           Handle<WasmInstanceObject> instance, uint32_t type,
                           Handle<Map> parent);

bool TypecheckJSObject(Isolate* isolate, const WasmModule* module,
                       Handle<Object> value, ValueType expected,
                       const char** error_message);
}  // namespace wasm

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_WASM_WASM_OBJECTS_H_
