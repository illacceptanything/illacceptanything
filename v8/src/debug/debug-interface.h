// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_DEBUG_DEBUG_INTERFACE_H_
#define V8_DEBUG_DEBUG_INTERFACE_H_

#include <memory>

#include "include/v8-util.h"
#include "include/v8.h"
#include "src/base/platform/time.h"
#include "src/common/globals.h"
#include "src/debug/interface-types.h"
#include "src/utils/vector.h"

namespace v8_inspector {
class V8Inspector;
}  // namespace v8_inspector

namespace v8 {

namespace internal {
struct CoverageBlock;
struct CoverageFunction;
struct CoverageScript;
struct TypeProfileEntry;
struct TypeProfileScript;
class Coverage;
class DisableBreak;
class PostponeInterruptsScope;
class Script;
class TypeProfile;
}  // namespace internal

namespace debug {

void SetContextId(Local<Context> context, int id);
int GetContextId(Local<Context> context);

void SetInspector(Isolate* isolate, v8_inspector::V8Inspector*);
v8_inspector::V8Inspector* GetInspector(Isolate* isolate);

// Returns the debug name for the function, which is supposed to be used
// by the debugger and the developer tools. This can thus be different from
// the name returned by the StackFrame::GetFunctionName() method. For example,
// in case of WebAssembly, the debug name is WAT-compatible and thus always
// preceeded by a dollar ('$').
Local<String> GetFunctionDebugName(Local<StackFrame> frame);

// Returns a debug string representation of the function.
Local<String> GetFunctionDescription(Local<Function> function);

// Schedule a debugger break to happen when function is called inside given
// isolate.
V8_EXPORT_PRIVATE void SetBreakOnNextFunctionCall(Isolate* isolate);

// Remove scheduled debugger break in given isolate if it has not
// happened yet.
V8_EXPORT_PRIVATE void ClearBreakOnNextFunctionCall(Isolate* isolate);

/**
 * Returns array of internal properties specific to the value type. Result has
 * the following format: [<name>, <value>,...,<name>, <value>]. Result array
 * will be allocated in the current context.
 */
MaybeLocal<Array> GetInternalProperties(Isolate* isolate, Local<Value> value);

/**
 * Returns through the out parameters names_out a vector of names
 * in v8::String for private members, including fields, methods,
 * accessors specific to the value type.
 * The values are returned through the out parameter values_out in the
 * corresponding indices. Private fields and methods are returned directly
 * while accessors are returned as v8::debug::AccessorPair. Missing components
 * in the accessor pairs are null.
 * If an exception occurs, false is returned. Otherwise true is returned.
 * Results will be allocated in the current context and handle scope.
 */
V8_EXPORT_PRIVATE bool GetPrivateMembers(Local<Context> context,
                                         Local<Object> value,
                                         std::vector<Local<Value>>* names_out,
                                         std::vector<Local<Value>>* values_out);

/**
 * Forwards to v8::Object::CreationContext, but with special handling for
 * JSGlobalProxy objects.
 */
MaybeLocal<Context> GetCreationContext(Local<Object> value);

enum ExceptionBreakState {
  NoBreakOnException = 0,
  BreakOnUncaughtException = 1,
  BreakOnAnyException = 2
};

/**
 * Defines if VM will pause on exceptions or not.
 * If BreakOnAnyExceptions is set then VM will pause on caught and uncaught
 * exception, if BreakOnUncaughtException is set then VM will pause only on
 * uncaught exception, otherwise VM won't stop on any exception.
 */
void ChangeBreakOnException(Isolate* isolate, ExceptionBreakState state);

void RemoveBreakpoint(Isolate* isolate, BreakpointId id);
void SetBreakPointsActive(Isolate* isolate, bool is_active);

enum StepAction {
  StepOut = 0,   // Step out of the current function.
  StepNext = 1,  // Step to the next statement in the current function.
  StepIn = 2     // Step into new functions invoked or the next statement
                 // in the current function.
};

void PrepareStep(Isolate* isolate, StepAction action);
void ClearStepping(Isolate* isolate);
V8_EXPORT_PRIVATE void BreakRightNow(Isolate* isolate);

// Use `SetTerminateOnResume` to indicate that an TerminateExecution interrupt
// should be set shortly before resuming, i.e. shortly before returning into
// the JavaScript stack frames on the stack. In contrast to setting the
// interrupt with `RequestTerminateExecution` directly, this flag allows
// the isolate to be entered for further JavaScript execution.
V8_EXPORT_PRIVATE void SetTerminateOnResume(Isolate* isolate);

bool CanBreakProgram(Isolate* isolate);

class Script;

struct LiveEditResult {
  enum Status {
    OK,
    COMPILE_ERROR,
    BLOCKED_BY_RUNNING_GENERATOR,
    BLOCKED_BY_ACTIVE_FUNCTION
  };
  Status status = OK;
  bool stack_changed = false;
  // Available only for OK.
  v8::Local<v8::debug::Script> script;
  // Fields below are available only for COMPILE_ERROR.
  v8::Local<v8::String> message;
  int line_number = -1;
  int column_number = -1;
};

/**
 * Native wrapper around v8::internal::Script object.
 */
class V8_EXPORT_PRIVATE Script {
 public:
  v8::Isolate* GetIsolate() const;

  ScriptOriginOptions OriginOptions() const;
  bool WasCompiled() const;
  bool IsEmbedded() const;
  int Id() const;
  int LineOffset() const;
  int ColumnOffset() const;
  std::vector<int> LineEnds() const;
  MaybeLocal<String> Name() const;
  MaybeLocal<String> SourceURL() const;
  MaybeLocal<String> SourceMappingURL() const;
  Maybe<int> ContextId() const;
  MaybeLocal<String> Source() const;
  bool IsModule() const;
  bool GetPossibleBreakpoints(
      const debug::Location& start, const debug::Location& end,
      bool restrict_to_function,
      std::vector<debug::BreakLocation>* locations) const;
  int GetSourceOffset(const debug::Location& location) const;
  v8::debug::Location GetSourceLocation(int offset) const;
  bool SetScriptSource(v8::Local<v8::String> newSource, bool preview,
                       LiveEditResult* result) const;
  bool SetBreakpoint(v8::Local<v8::String> condition, debug::Location* location,
                     BreakpointId* id) const;
#if V8_ENABLE_WEBASSEMBLY
  bool IsWasm() const;
  void RemoveWasmBreakpoint(BreakpointId id);
#endif  // V8_ENABLE_WEBASSEMBLY
  bool SetBreakpointOnScriptEntry(BreakpointId* id) const;
};

#if V8_ENABLE_WEBASSEMBLY
// Specialization for wasm Scripts.
class WasmScript : public Script {
 public:
  static WasmScript* Cast(Script* script);

  enum class DebugSymbolsType { None, SourceMap, EmbeddedDWARF, ExternalDWARF };
  DebugSymbolsType GetDebugSymbolType() const;
  MemorySpan<const char> ExternalSymbolsURL() const;
  int NumFunctions() const;
  int NumImportedFunctions() const;
  MemorySpan<const uint8_t> Bytecode() const;

  std::pair<int, int> GetFunctionRange(int function_index) const;
  int GetContainingFunction(int byte_offset) const;

  uint32_t GetFunctionHash(int function_index);

  int CodeOffset() const;
  int CodeLength() const;
};
#endif  // V8_ENABLE_WEBASSEMBLY

V8_EXPORT_PRIVATE void GetLoadedScripts(Isolate* isolate,
                                        PersistentValueVector<Script>& scripts);

MaybeLocal<UnboundScript> CompileInspectorScript(Isolate* isolate,
                                                 Local<String> source);

enum ExceptionType { kException, kPromiseRejection };

class DebugDelegate {
 public:
  virtual ~DebugDelegate() = default;
  virtual void ScriptCompiled(v8::Local<Script> script, bool is_live_edited,
                              bool has_compile_error) {}
  // |inspector_break_points_hit| contains id of breakpoints installed with
  // debug::Script::SetBreakpoint API.
  virtual void BreakProgramRequested(
      v8::Local<v8::Context> paused_context,
      const std::vector<debug::BreakpointId>& inspector_break_points_hit) {}
  virtual void ExceptionThrown(v8::Local<v8::Context> paused_context,
                               v8::Local<v8::Value> exception,
                               v8::Local<v8::Value> promise, bool is_uncaught,
                               ExceptionType exception_type) {}
  virtual bool IsFunctionBlackboxed(v8::Local<debug::Script> script,
                                    const debug::Location& start,
                                    const debug::Location& end) {
    return false;
  }
  virtual bool ShouldBeSkipped(v8::Local<v8::debug::Script> script, int line,
                               int column) {
    return false;
  }
};

V8_EXPORT_PRIVATE void SetDebugDelegate(Isolate* isolate,
                                        DebugDelegate* listener);

#if V8_ENABLE_WEBASSEMBLY
V8_EXPORT_PRIVATE void TierDownAllModulesPerIsolate(Isolate* isolate);
V8_EXPORT_PRIVATE void TierUpAllModulesPerIsolate(Isolate* isolate);
#endif  // V8_ENABLE_WEBASSEMBLY

class AsyncEventDelegate {
 public:
  virtual ~AsyncEventDelegate() = default;
  virtual void AsyncEventOccurred(debug::DebugAsyncActionType type, int id,
                                  bool is_blackboxed) = 0;
};

void SetAsyncEventDelegate(Isolate* isolate, AsyncEventDelegate* delegate);

void ResetBlackboxedStateCache(Isolate* isolate,
                               v8::Local<debug::Script> script);

int EstimatedValueSize(Isolate* isolate, v8::Local<v8::Value> value);

enum Builtin { kStringToLowerCase };

Local<Function> GetBuiltin(Isolate* isolate, Builtin builtin);

V8_EXPORT_PRIVATE void SetConsoleDelegate(Isolate* isolate,
                                          ConsoleDelegate* delegate);

V8_DEPRECATED("See http://crbug.com/v8/10566.")
int GetStackFrameId(v8::Local<v8::StackFrame> frame);

v8::Local<v8::StackTrace> GetDetailedStackTrace(Isolate* isolate,
                                                v8::Local<v8::Object> error);

/**
 * Native wrapper around v8::internal::JSGeneratorObject object.
 */
class GeneratorObject {
 public:
  v8::MaybeLocal<debug::Script> Script();
  v8::Local<v8::Function> Function();
  debug::Location SuspendedLocation();
  bool IsSuspended();

  static v8::Local<debug::GeneratorObject> Cast(v8::Local<v8::Value> value);
};

/*
 * Provide API layer between inspector and code coverage.
 */
class V8_EXPORT_PRIVATE Coverage {
 public:
  MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(Coverage);

  // Forward declarations.
  class ScriptData;
  class FunctionData;

  class V8_EXPORT_PRIVATE BlockData {
   public:
    MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(BlockData);

    int StartOffset() const;
    int EndOffset() const;
    uint32_t Count() const;

   private:
    explicit BlockData(i::CoverageBlock* block,
                       std::shared_ptr<i::Coverage> coverage)
        : block_(block), coverage_(std::move(coverage)) {}

    i::CoverageBlock* block_;
    std::shared_ptr<i::Coverage> coverage_;

    friend class v8::debug::Coverage::FunctionData;
  };

  class V8_EXPORT_PRIVATE FunctionData {
   public:
    MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(FunctionData);

    int StartOffset() const;
    int EndOffset() const;
    uint32_t Count() const;
    MaybeLocal<String> Name() const;
    size_t BlockCount() const;
    bool HasBlockCoverage() const;
    BlockData GetBlockData(size_t i) const;

   private:
    explicit FunctionData(i::CoverageFunction* function,
                          std::shared_ptr<i::Coverage> coverage)
        : function_(function), coverage_(std::move(coverage)) {}

    i::CoverageFunction* function_;
    std::shared_ptr<i::Coverage> coverage_;

    friend class v8::debug::Coverage::ScriptData;
  };

  class V8_EXPORT_PRIVATE ScriptData {
   public:
    MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(ScriptData);

    Local<debug::Script> GetScript() const;
    size_t FunctionCount() const;
    FunctionData GetFunctionData(size_t i) const;

   private:
    explicit ScriptData(size_t index, std::shared_ptr<i::Coverage> c);

    i::CoverageScript* script_;
    std::shared_ptr<i::Coverage> coverage_;

    friend class v8::debug::Coverage;
  };

  static Coverage CollectPrecise(Isolate* isolate);
  static Coverage CollectBestEffort(Isolate* isolate);

  static void SelectMode(Isolate* isolate, CoverageMode mode);

  size_t ScriptCount() const;
  ScriptData GetScriptData(size_t i) const;
  bool IsEmpty() const { return coverage_ == nullptr; }

 private:
  explicit Coverage(std::shared_ptr<i::Coverage> coverage)
      : coverage_(std::move(coverage)) {}
  std::shared_ptr<i::Coverage> coverage_;
};

/*
 * Provide API layer between inspector and type profile.
 */
class V8_EXPORT_PRIVATE TypeProfile {
 public:
  MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(TypeProfile);

  class ScriptData;  // Forward declaration.

  class V8_EXPORT_PRIVATE Entry {
   public:
    MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(Entry);

    int SourcePosition() const;
    std::vector<MaybeLocal<String>> Types() const;

   private:
    explicit Entry(const i::TypeProfileEntry* entry,
                   std::shared_ptr<i::TypeProfile> type_profile)
        : entry_(entry), type_profile_(std::move(type_profile)) {}

    const i::TypeProfileEntry* entry_;
    std::shared_ptr<i::TypeProfile> type_profile_;

    friend class v8::debug::TypeProfile::ScriptData;
  };

  class V8_EXPORT_PRIVATE ScriptData {
   public:
    MOVE_ONLY_NO_DEFAULT_CONSTRUCTOR(ScriptData);

    Local<debug::Script> GetScript() const;
    std::vector<Entry> Entries() const;

   private:
    explicit ScriptData(size_t index,
                        std::shared_ptr<i::TypeProfile> type_profile);

    i::TypeProfileScript* script_;
    std::shared_ptr<i::TypeProfile> type_profile_;

    friend class v8::debug::TypeProfile;
  };

  static TypeProfile Collect(Isolate* isolate);

  static void SelectMode(Isolate* isolate, TypeProfileMode mode);

  size_t ScriptCount() const;
  ScriptData GetScriptData(size_t i) const;

 private:
  explicit TypeProfile(std::shared_ptr<i::TypeProfile> type_profile)
      : type_profile_(std::move(type_profile)) {}

  std::shared_ptr<i::TypeProfile> type_profile_;
};

class V8_EXPORT_PRIVATE ScopeIterator {
 public:
  static std::unique_ptr<ScopeIterator> CreateForFunction(
      v8::Isolate* isolate, v8::Local<v8::Function> func);
  static std::unique_ptr<ScopeIterator> CreateForGeneratorObject(
      v8::Isolate* isolate, v8::Local<v8::Object> generator);

  ScopeIterator() = default;
  virtual ~ScopeIterator() = default;
  ScopeIterator(const ScopeIterator&) = delete;
  ScopeIterator& operator=(const ScopeIterator&) = delete;

  enum ScopeType {
    ScopeTypeGlobal = 0,
    ScopeTypeLocal,
    ScopeTypeWith,
    ScopeTypeClosure,
    ScopeTypeCatch,
    ScopeTypeBlock,
    ScopeTypeScript,
    ScopeTypeEval,
    ScopeTypeModule,
    ScopeTypeWasmExpressionStack
  };

  virtual bool Done() = 0;
  virtual void Advance() = 0;
  virtual ScopeType GetType() = 0;
  virtual v8::Local<v8::Object> GetObject() = 0;
  virtual v8::Local<v8::Value> GetFunctionDebugName() = 0;
  virtual int GetScriptId() = 0;
  virtual bool HasLocationInfo() = 0;
  virtual debug::Location GetStartLocation() = 0;
  virtual debug::Location GetEndLocation() = 0;

  virtual bool SetVariableValue(v8::Local<v8::String> name,
                                v8::Local<v8::Value> value) = 0;
};

class V8_EXPORT_PRIVATE StackTraceIterator {
 public:
  static std::unique_ptr<StackTraceIterator> Create(Isolate* isolate,
                                                    int index = 0);
  StackTraceIterator() = default;
  virtual ~StackTraceIterator() = default;
  StackTraceIterator(const StackTraceIterator&) = delete;
  StackTraceIterator& operator=(const StackTraceIterator&) = delete;

  virtual bool Done() const = 0;
  virtual void Advance() = 0;

  virtual int GetContextId() const = 0;
  virtual v8::MaybeLocal<v8::Value> GetReceiver() const = 0;
  virtual v8::Local<v8::Value> GetReturnValue() const = 0;
  virtual v8::Local<v8::String> GetFunctionDebugName() const = 0;
  virtual v8::Local<v8::debug::Script> GetScript() const = 0;
  virtual debug::Location GetSourceLocation() const = 0;
  virtual v8::Local<v8::Function> GetFunction() const = 0;
  virtual std::unique_ptr<ScopeIterator> GetScopeIterator() const = 0;

  virtual v8::MaybeLocal<v8::Value> Evaluate(v8::Local<v8::String> source,
                                             bool throw_on_side_effect) = 0;
};

class QueryObjectPredicate {
 public:
  virtual ~QueryObjectPredicate() = default;
  virtual bool Filter(v8::Local<v8::Object> object) = 0;
};

void QueryObjects(v8::Local<v8::Context> context,
                  QueryObjectPredicate* predicate,
                  v8::PersistentValueVector<v8::Object>* objects);

void GlobalLexicalScopeNames(v8::Local<v8::Context> context,
                             v8::PersistentValueVector<v8::String>* names);

void SetReturnValue(v8::Isolate* isolate, v8::Local<v8::Value> value);

enum class NativeAccessorType {
  None = 0,
  HasGetter = 1 << 0,
  HasSetter = 1 << 1
};

int64_t GetNextRandomInt64(v8::Isolate* isolate);

using RuntimeCallCounterCallback =
    std::function<void(const char* name, int64_t count, base::TimeDelta time)>;
void EnumerateRuntimeCallCounters(v8::Isolate* isolate,
                                  RuntimeCallCounterCallback callback);

enum class EvaluateGlobalMode {
  kDefault,
  kDisableBreaks,
  kDisableBreaksAndThrowOnSideEffect
};

V8_EXPORT_PRIVATE v8::MaybeLocal<v8::Value> EvaluateGlobal(
    v8::Isolate* isolate, v8::Local<v8::String> source, EvaluateGlobalMode mode,
    bool repl_mode = false);

V8_EXPORT_PRIVATE v8::MaybeLocal<v8::Value> EvaluateGlobalForTesting(
    v8::Isolate* isolate, v8::Local<v8::Script> function,
    v8::debug::EvaluateGlobalMode mode, bool repl);

int GetDebuggingId(v8::Local<v8::Function> function);

bool SetFunctionBreakpoint(v8::Local<v8::Function> function,
                           v8::Local<v8::String> condition, BreakpointId* id);

v8::Platform* GetCurrentPlatform();

void ForceGarbageCollection(
    v8::Isolate* isolate,
    v8::EmbedderHeapTracer::EmbedderStackState embedder_stack_state);

class V8_NODISCARD PostponeInterruptsScope {
 public:
  explicit PostponeInterruptsScope(v8::Isolate* isolate);
  ~PostponeInterruptsScope();

 private:
  std::unique_ptr<i::PostponeInterruptsScope> scope_;
};

class V8_NODISCARD DisableBreakScope {
 public:
  explicit DisableBreakScope(v8::Isolate* isolate);
  ~DisableBreakScope();

 private:
  std::unique_ptr<i::DisableBreak> scope_;
};

class WeakMap : public v8::Object {
 public:
  WeakMap() = delete;
  V8_EXPORT_PRIVATE V8_WARN_UNUSED_RESULT v8::MaybeLocal<v8::Value> Get(
      v8::Local<v8::Context> context, v8::Local<v8::Value> key);
  V8_EXPORT_PRIVATE V8_WARN_UNUSED_RESULT v8::MaybeLocal<WeakMap> Set(
      v8::Local<v8::Context> context, v8::Local<v8::Value> key,
      v8::Local<v8::Value> value);

  V8_EXPORT_PRIVATE static Local<WeakMap> New(v8::Isolate* isolate);
  V8_INLINE static WeakMap* Cast(Value* obj);
};

/**
 * Pairs of accessors.
 *
 * In the case of private accessors, getters and setters are either null or
 * Functions.
 */
class V8_EXPORT_PRIVATE AccessorPair : public v8::Value {
 public:
  AccessorPair() = delete;
  v8::Local<v8::Value> getter();
  v8::Local<v8::Value> setter();

  static bool IsAccessorPair(v8::Local<v8::Value> obj);
  V8_INLINE static AccessorPair* Cast(v8::Value* obj);

 private:
  static void CheckCast(v8::Value* obj);
};

struct PropertyDescriptor {
  bool enumerable : 1;
  bool has_enumerable : 1;
  bool configurable : 1;
  bool has_configurable : 1;
  bool writable : 1;
  bool has_writable : 1;
  v8::Local<v8::Value> value;
  v8::Local<v8::Value> get;
  v8::Local<v8::Value> set;
};

class PropertyIterator {
 public:
  // Creating a PropertyIterator can potentially throw an exception.
  // The returned std::unique_ptr is empty iff that happens.
  V8_WARN_UNUSED_RESULT static std::unique_ptr<PropertyIterator> Create(
      v8::Local<v8::Context> context, v8::Local<v8::Object> object);

  virtual ~PropertyIterator() = default;

  virtual bool Done() const = 0;
  // Returns |Nothing| should |Advance| throw an exception,
  // |true| otherwise.
  V8_WARN_UNUSED_RESULT virtual Maybe<bool> Advance() = 0;

  virtual v8::Local<v8::Name> name() const = 0;

  virtual bool is_native_accessor() = 0;
  virtual bool has_native_getter() = 0;
  virtual bool has_native_setter() = 0;
  virtual Maybe<PropertyAttribute> attributes() = 0;
  virtual Maybe<PropertyDescriptor> descriptor() = 0;

  virtual bool is_own() = 0;
  virtual bool is_array_index() = 0;
};

#if V8_ENABLE_WEBASSEMBLY
class V8_EXPORT_PRIVATE WasmValueObject : public v8::Object {
 public:
  WasmValueObject() = delete;
  static bool IsWasmValueObject(v8::Local<v8::Value> obj);
  static WasmValueObject* Cast(v8::Value* value) {
#ifdef V8_ENABLE_CHECKS
    CheckCast(value);
#endif
    return static_cast<WasmValueObject*>(value);
  }

  v8::Local<v8::String> type() const;

 private:
  static void CheckCast(v8::Value* obj);
};
#endif  // V8_ENABLE_WEBASSEMBLY

AccessorPair* AccessorPair::Cast(v8::Value* value) {
#ifdef V8_ENABLE_CHECKS
  CheckCast(value);
#endif
  return static_cast<AccessorPair*>(value);
}

MaybeLocal<Message> GetMessageFromPromise(Local<Promise> promise);

}  // namespace debug
}  // namespace v8

#endif  // V8_DEBUG_DEBUG_INTERFACE_H_
