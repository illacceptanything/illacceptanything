// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/execution/execution.h"

#include "src/api/api-inl.h"
#include "src/debug/debug.h"
#include "src/execution/frames.h"
#include "src/execution/isolate-inl.h"
#include "src/execution/vm-state-inl.h"
#include "src/logging/counters.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/compiler/wasm-compiler.h"  // Only for static asserts.
#endif                                   // V8_ENABLE_WEBASSEMBLY

namespace v8 {
namespace internal {

namespace {

Handle<Object> NormalizeReceiver(Isolate* isolate, Handle<Object> receiver) {
  // Convert calls on global objects to be calls on the global
  // receiver instead to avoid having a 'this' pointer which refers
  // directly to a global object.
  if (receiver->IsJSGlobalObject()) {
    return handle(Handle<JSGlobalObject>::cast(receiver)->global_proxy(),
                  isolate);
  }
  return receiver;
}

struct InvokeParams {
  static InvokeParams SetUpForNew(Isolate* isolate, Handle<Object> constructor,
                                  Handle<Object> new_target, int argc,
                                  Handle<Object>* argv);

  static InvokeParams SetUpForCall(Isolate* isolate, Handle<Object> callable,
                                   Handle<Object> receiver, int argc,
                                   Handle<Object>* argv);

  static InvokeParams SetUpForTryCall(
      Isolate* isolate, Handle<Object> callable, Handle<Object> receiver,
      int argc, Handle<Object>* argv,
      Execution::MessageHandling message_handling,
      MaybeHandle<Object>* exception_out, bool reschedule_terminate);

  static InvokeParams SetUpForRunMicrotasks(Isolate* isolate,
                                            MicrotaskQueue* microtask_queue,
                                            MaybeHandle<Object>* exception_out);

  Handle<Object> target;
  Handle<Object> receiver;
  int argc;
  Handle<Object>* argv;
  Handle<Object> new_target;

  MicrotaskQueue* microtask_queue;

  Execution::MessageHandling message_handling;
  MaybeHandle<Object>* exception_out;

  bool is_construct;
  Execution::Target execution_target;
  bool reschedule_terminate;
};

// static
InvokeParams InvokeParams::SetUpForNew(Isolate* isolate,
                                       Handle<Object> constructor,
                                       Handle<Object> new_target, int argc,
                                       Handle<Object>* argv) {
  InvokeParams params;
  params.target = constructor;
  params.receiver = isolate->factory()->undefined_value();
  params.argc = argc;
  params.argv = argv;
  params.new_target = new_target;
  params.microtask_queue = nullptr;
  params.message_handling = Execution::MessageHandling::kReport;
  params.exception_out = nullptr;
  params.is_construct = true;
  params.execution_target = Execution::Target::kCallable;
  params.reschedule_terminate = true;
  return params;
}

// static
InvokeParams InvokeParams::SetUpForCall(Isolate* isolate,
                                        Handle<Object> callable,
                                        Handle<Object> receiver, int argc,
                                        Handle<Object>* argv) {
  InvokeParams params;
  params.target = callable;
  params.receiver = NormalizeReceiver(isolate, receiver);
  params.argc = argc;
  params.argv = argv;
  params.new_target = isolate->factory()->undefined_value();
  params.microtask_queue = nullptr;
  params.message_handling = Execution::MessageHandling::kReport;
  params.exception_out = nullptr;
  params.is_construct = false;
  params.execution_target = Execution::Target::kCallable;
  params.reschedule_terminate = true;
  return params;
}

// static
InvokeParams InvokeParams::SetUpForTryCall(
    Isolate* isolate, Handle<Object> callable, Handle<Object> receiver,
    int argc, Handle<Object>* argv, Execution::MessageHandling message_handling,
    MaybeHandle<Object>* exception_out, bool reschedule_terminate) {
  InvokeParams params;
  params.target = callable;
  params.receiver = NormalizeReceiver(isolate, receiver);
  params.argc = argc;
  params.argv = argv;
  params.new_target = isolate->factory()->undefined_value();
  params.microtask_queue = nullptr;
  params.message_handling = message_handling;
  params.exception_out = exception_out;
  params.is_construct = false;
  params.execution_target = Execution::Target::kCallable;
  params.reschedule_terminate = reschedule_terminate;
  return params;
}

// static
InvokeParams InvokeParams::SetUpForRunMicrotasks(
    Isolate* isolate, MicrotaskQueue* microtask_queue,
    MaybeHandle<Object>* exception_out) {
  auto undefined = isolate->factory()->undefined_value();
  InvokeParams params;
  params.target = undefined;
  params.receiver = undefined;
  params.argc = 0;
  params.argv = nullptr;
  params.new_target = undefined;
  params.microtask_queue = microtask_queue;
  params.message_handling = Execution::MessageHandling::kReport;
  params.exception_out = exception_out;
  params.is_construct = false;
  params.execution_target = Execution::Target::kRunMicrotasks;
  params.reschedule_terminate = true;
  return params;
}

Handle<Code> JSEntry(Isolate* isolate, Execution::Target execution_target,
                     bool is_construct) {
  if (is_construct) {
    DCHECK_EQ(Execution::Target::kCallable, execution_target);
    return BUILTIN_CODE(isolate, JSConstructEntry);
  } else if (execution_target == Execution::Target::kCallable) {
    DCHECK(!is_construct);
    return BUILTIN_CODE(isolate, JSEntry);
  } else if (execution_target == Execution::Target::kRunMicrotasks) {
    DCHECK(!is_construct);
    return BUILTIN_CODE(isolate, JSRunMicrotasksEntry);
  }
  UNREACHABLE();
}

MaybeHandle<Context> NewScriptContext(Isolate* isolate,
                                      Handle<JSFunction> function) {
  // Creating a script context is a side effect, so abort if that's not
  // allowed.
  if (isolate->debug_execution_mode() == DebugInfo::kSideEffects) {
    isolate->Throw(*isolate->factory()->NewEvalError(
        MessageTemplate::kNoSideEffectDebugEvaluate));
    return MaybeHandle<Context>();
  }
  SaveAndSwitchContext save(isolate, function->context());
  SharedFunctionInfo sfi = function->shared();
  Handle<Script> script(Script::cast(sfi.script()), isolate);
  Handle<ScopeInfo> scope_info(sfi.scope_info(), isolate);
  Handle<NativeContext> native_context(NativeContext::cast(function->context()),
                                       isolate);
  Handle<JSGlobalObject> global_object(native_context->global_object(),
                                       isolate);
  Handle<ScriptContextTable> script_context(
      native_context->script_context_table(), isolate);

  // Find name clashes.
  for (int var = 0; var < scope_info->ContextLocalCount(); var++) {
    Handle<String> name(scope_info->ContextLocalName(var), isolate);
    VariableMode mode = scope_info->ContextLocalMode(var);
    ScriptContextTable::LookupResult lookup;
    if (ScriptContextTable::Lookup(isolate, *script_context, *name, &lookup)) {
      if (IsLexicalVariableMode(mode) || IsLexicalVariableMode(lookup.mode)) {
        Handle<Context> context = ScriptContextTable::GetContext(
            isolate, script_context, lookup.context_index);
        // If we are trying to re-declare a REPL-mode let as a let or REPL-mode
        // const as a const, allow it.
        if (!(((mode == VariableMode::kLet &&
                lookup.mode == VariableMode::kLet) ||
               (mode == VariableMode::kConst &&
                lookup.mode == VariableMode::kConst)) &&
              scope_info->IsReplModeScope() &&
              context->scope_info().IsReplModeScope())) {
          // ES#sec-globaldeclarationinstantiation 5.b:
          // If envRec.HasLexicalDeclaration(name) is true, throw a SyntaxError
          // exception.
          MessageLocation location(script, 0, 1);
          return isolate->ThrowAt<Context>(
              isolate->factory()->NewSyntaxError(
                  MessageTemplate::kVarRedeclaration, name),
              &location);
        }
      }
    }

    if (IsLexicalVariableMode(mode)) {
      LookupIterator it(isolate, global_object, name, global_object,
                        LookupIterator::OWN_SKIP_INTERCEPTOR);
      Maybe<PropertyAttributes> maybe = JSReceiver::GetPropertyAttributes(&it);
      // Can't fail since the we looking up own properties on the global object
      // skipping interceptors.
      CHECK(!maybe.IsNothing());
      if ((maybe.FromJust() & DONT_DELETE) != 0) {
        // ES#sec-globaldeclarationinstantiation 5.a:
        // If envRec.HasVarDeclaration(name) is true, throw a SyntaxError
        // exception.
        // ES#sec-globaldeclarationinstantiation 5.d:
        // If hasRestrictedGlobal is true, throw a SyntaxError exception.
        MessageLocation location(script, 0, 1);
        return isolate->ThrowAt<Context>(
            isolate->factory()->NewSyntaxError(
                MessageTemplate::kVarRedeclaration, name),
            &location);
      }

      JSGlobalObject::InvalidatePropertyCell(global_object, name);
    }
  }

  Handle<Context> result =
      isolate->factory()->NewScriptContext(native_context, scope_info);

  result->Initialize(isolate);

  Handle<ScriptContextTable> new_script_context_table =
      ScriptContextTable::Extend(script_context, result);
  native_context->synchronized_set_script_context_table(
      *new_script_context_table);
  return result;
}

V8_WARN_UNUSED_RESULT MaybeHandle<Object> Invoke(Isolate* isolate,
                                                 const InvokeParams& params) {
  RCS_SCOPE(isolate, RuntimeCallCounterId::kInvoke);
  DCHECK(!params.receiver->IsJSGlobalObject());
  DCHECK_LE(params.argc, FixedArray::kMaxLength);

#ifdef USE_SIMULATOR
  // Simulators use separate stacks for C++ and JS. JS stack overflow checks
  // are performed whenever a JS function is called. However, it can be the case
  // that the C++ stack grows faster than the JS stack, resulting in an overflow
  // there. Add a check here to make that less likely.
  StackLimitCheck check(isolate);
  if (check.HasOverflowed()) {
    isolate->StackOverflow();
    if (params.message_handling == Execution::MessageHandling::kReport) {
      isolate->ReportPendingMessages();
    }
    return MaybeHandle<Object>();
  }
#endif

  // api callbacks can be called directly, unless we want to take the detour
  // through JS to set up a frame for break-at-entry.
  if (params.target->IsJSFunction()) {
    Handle<JSFunction> function = Handle<JSFunction>::cast(params.target);
    if ((!params.is_construct || function->IsConstructor()) &&
        function->shared().IsApiFunction() &&
        !function->shared().BreakAtEntry()) {
      SaveAndSwitchContext save(isolate, function->context());
      DCHECK(function->context().global_object().IsJSGlobalObject());

      Handle<Object> receiver = params.is_construct
                                    ? isolate->factory()->the_hole_value()
                                    : params.receiver;
      auto value = Builtins::InvokeApiFunction(
          isolate, params.is_construct, function, receiver, params.argc,
          params.argv, Handle<HeapObject>::cast(params.new_target));
      bool has_exception = value.is_null();
      DCHECK(has_exception == isolate->has_pending_exception());
      if (has_exception) {
        if (params.message_handling == Execution::MessageHandling::kReport) {
          isolate->ReportPendingMessages();
        }
        return MaybeHandle<Object>();
      } else {
        isolate->clear_pending_message();
      }
      return value;
    }

    // Set up a ScriptContext when running scripts that need it.
    if (function->shared().needs_script_context()) {
      Handle<Context> context;
      if (!NewScriptContext(isolate, function).ToHandle(&context)) {
        if (params.message_handling == Execution::MessageHandling::kReport) {
          isolate->ReportPendingMessages();
        }
        return MaybeHandle<Object>();
      }

      // We mutate the context if we allocate a script context. This is
      // guaranteed to only happen once in a native context since scripts will
      // always produce name clashes with themselves.
      function->set_context(*context);
    }
  }

  // Entering JavaScript.
  VMState<JS> state(isolate);
  CHECK(AllowJavascriptExecution::IsAllowed(isolate));
  if (!ThrowOnJavascriptExecution::IsAllowed(isolate)) {
    isolate->ThrowIllegalOperation();
    if (params.message_handling == Execution::MessageHandling::kReport) {
      isolate->ReportPendingMessages();
    }
    return MaybeHandle<Object>();
  }
  if (!DumpOnJavascriptExecution::IsAllowed(isolate)) {
    V8::GetCurrentPlatform()->DumpWithoutCrashing();
    return isolate->factory()->undefined_value();
  }

  if (params.execution_target == Execution::Target::kCallable) {
    Handle<Context> context = isolate->native_context();
    if (!context->script_execution_callback().IsUndefined(isolate)) {
      v8::Context::AbortScriptExecutionCallback callback =
          v8::ToCData<v8::Context::AbortScriptExecutionCallback>(
              context->script_execution_callback());
      v8::Isolate* api_isolate = reinterpret_cast<v8::Isolate*>(isolate);
      v8::Local<v8::Context> api_context = v8::Utils::ToLocal(context);
      callback(api_isolate, api_context);
      DCHECK(!isolate->has_scheduled_exception());
      // Always throw an exception to abort execution, if callback exists.
      isolate->ThrowIllegalOperation();
      return MaybeHandle<Object>();
    }
  }

  // Placeholder for return value.
  Object value;

  Handle<Code> code =
      JSEntry(isolate, params.execution_target, params.is_construct);
  {
    // Save and restore context around invocation and block the
    // allocation of handles without explicit handle scopes.
    SaveContext save(isolate);
    SealHandleScope shs(isolate);

    if (FLAG_clear_exceptions_on_js_entry) isolate->clear_pending_exception();

    if (params.execution_target == Execution::Target::kCallable) {
      // clang-format off
      // {new_target}, {target}, {receiver}, return value: tagged pointers
      // {argv}: pointer to array of tagged pointers
      using JSEntryFunction = GeneratedCode<Address(
          Address root_register_value, Address new_target, Address target,
          Address receiver, intptr_t argc, Address** argv)>;
      // clang-format on
      JSEntryFunction stub_entry =
          JSEntryFunction::FromAddress(isolate, code->InstructionStart());

      Address orig_func = params.new_target->ptr();
      Address func = params.target->ptr();
      Address recv = params.receiver->ptr();
      Address** argv = reinterpret_cast<Address**>(params.argv);
      RCS_SCOPE(isolate, RuntimeCallCounterId::kJS_Execution);
      value = Object(stub_entry.Call(isolate->isolate_data()->isolate_root(),
                                     orig_func, func, recv, params.argc, argv));
    } else {
      DCHECK_EQ(Execution::Target::kRunMicrotasks, params.execution_target);

      // clang-format off
      // return value: tagged pointers
      // {microtask_queue}: pointer to a C++ object
      using JSEntryFunction = GeneratedCode<Address(
          Address root_register_value, MicrotaskQueue* microtask_queue)>;
      // clang-format on
      JSEntryFunction stub_entry =
          JSEntryFunction::FromAddress(isolate, code->InstructionStart());

      RCS_SCOPE(isolate, RuntimeCallCounterId::kJS_Execution);
      value = Object(stub_entry.Call(isolate->isolate_data()->isolate_root(),
                                     params.microtask_queue));
    }
  }

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    value.ObjectVerify(isolate);
  }
#endif

  // Update the pending exception flag and return the value.
  bool has_exception = value.IsException(isolate);
  DCHECK(has_exception == isolate->has_pending_exception());
  if (has_exception) {
    if (params.message_handling == Execution::MessageHandling::kReport) {
      isolate->ReportPendingMessages();
    }
    return MaybeHandle<Object>();
  } else {
    isolate->clear_pending_message();
  }

  return Handle<Object>(value, isolate);
}

MaybeHandle<Object> InvokeWithTryCatch(Isolate* isolate,
                                       const InvokeParams& params) {
  bool is_termination = false;
  MaybeHandle<Object> maybe_result;
  if (params.exception_out != nullptr) {
    *params.exception_out = MaybeHandle<Object>();
  }
  DCHECK_IMPLIES(
      params.message_handling == Execution::MessageHandling::kKeepPending,
      params.exception_out == nullptr);
  // Enter a try-block while executing the JavaScript code. To avoid
  // duplicate error printing it must be non-verbose.  Also, to avoid
  // creating message objects during stack overflow we shouldn't
  // capture messages.
  {
    v8::TryCatch catcher(reinterpret_cast<v8::Isolate*>(isolate));
    catcher.SetVerbose(false);
    catcher.SetCaptureMessage(false);

    maybe_result = Invoke(isolate, params);

    if (maybe_result.is_null()) {
      DCHECK(isolate->has_pending_exception());
      if (isolate->pending_exception() ==
          ReadOnlyRoots(isolate).termination_exception()) {
        is_termination = true;
      } else {
        if (params.exception_out != nullptr) {
          DCHECK(catcher.HasCaught());
          DCHECK(isolate->external_caught_exception());
          *params.exception_out = v8::Utils::OpenHandle(*catcher.Exception());
        }
        if (params.message_handling == Execution::MessageHandling::kReport) {
          isolate->OptionalRescheduleException(true);
        }
      }
    }
  }

  if (is_termination && params.reschedule_terminate) {
    // Reschedule terminate execution exception.
    isolate->OptionalRescheduleException(false);
  }

  return maybe_result;
}

}  // namespace

// static
MaybeHandle<Object> Execution::Call(Isolate* isolate, Handle<Object> callable,
                                    Handle<Object> receiver, int argc,
                                    Handle<Object> argv[]) {
  return Invoke(isolate, InvokeParams::SetUpForCall(isolate, callable, receiver,
                                                    argc, argv));
}

MaybeHandle<Object> Execution::CallBuiltin(Isolate* isolate,
                                           Handle<JSFunction> builtin,
                                           Handle<Object> receiver, int argc,
                                           Handle<Object> argv[]) {
  DCHECK(builtin->code().is_builtin());
  DisableBreak no_break(isolate->debug());
  return Invoke(isolate, InvokeParams::SetUpForCall(isolate, builtin, receiver,
                                                    argc, argv));
}

// static
MaybeHandle<Object> Execution::New(Isolate* isolate, Handle<Object> constructor,
                                   int argc, Handle<Object> argv[]) {
  return New(isolate, constructor, constructor, argc, argv);
}

// static
MaybeHandle<Object> Execution::New(Isolate* isolate, Handle<Object> constructor,
                                   Handle<Object> new_target, int argc,
                                   Handle<Object> argv[]) {
  return Invoke(isolate, InvokeParams::SetUpForNew(isolate, constructor,
                                                   new_target, argc, argv));
}

// static
MaybeHandle<Object> Execution::TryCall(
    Isolate* isolate, Handle<Object> callable, Handle<Object> receiver,
    int argc, Handle<Object> argv[], MessageHandling message_handling,
    MaybeHandle<Object>* exception_out, bool reschedule_terminate) {
  return InvokeWithTryCatch(
      isolate, InvokeParams::SetUpForTryCall(
                   isolate, callable, receiver, argc, argv, message_handling,
                   exception_out, reschedule_terminate));
}

// static
MaybeHandle<Object> Execution::TryRunMicrotasks(
    Isolate* isolate, MicrotaskQueue* microtask_queue,
    MaybeHandle<Object>* exception_out) {
  return InvokeWithTryCatch(
      isolate, InvokeParams::SetUpForRunMicrotasks(isolate, microtask_queue,
                                                   exception_out));
}

struct StackHandlerMarker {
  Address next;
  Address padding;
};
STATIC_ASSERT(offsetof(StackHandlerMarker, next) ==
              StackHandlerConstants::kNextOffset);
STATIC_ASSERT(offsetof(StackHandlerMarker, padding) ==
              StackHandlerConstants::kPaddingOffset);
STATIC_ASSERT(sizeof(StackHandlerMarker) == StackHandlerConstants::kSize);

#if V8_ENABLE_WEBASSEMBLY
void Execution::CallWasm(Isolate* isolate, Handle<Code> wrapper_code,
                         Address wasm_call_target, Handle<Object> object_ref,
                         Address packed_args) {
  using WasmEntryStub = GeneratedCode<Address(
      Address target, Address object_ref, Address argv, Address c_entry_fp)>;
  WasmEntryStub stub_entry =
      WasmEntryStub::FromAddress(isolate, wrapper_code->InstructionStart());

  // Save and restore context around invocation and block the
  // allocation of handles without explicit handle scopes.
  SaveContext save(isolate);
  SealHandleScope shs(isolate);

  Address saved_c_entry_fp = *isolate->c_entry_fp_address();
  Address saved_js_entry_sp = *isolate->js_entry_sp_address();
  if (saved_js_entry_sp == kNullAddress) {
    *isolate->js_entry_sp_address() = GetCurrentStackPosition();
  }
  StackHandlerMarker stack_handler;
  stack_handler.next = isolate->thread_local_top()->handler_;
#ifdef V8_USE_ADDRESS_SANITIZER
  stack_handler.padding = GetCurrentStackPosition();
#else
  stack_handler.padding = 0;
#endif
  isolate->thread_local_top()->handler_ =
      reinterpret_cast<Address>(&stack_handler);
  trap_handler::SetThreadInWasm();

  {
    RCS_SCOPE(isolate, RuntimeCallCounterId::kJS_Execution);
    STATIC_ASSERT(compiler::CWasmEntryParameters::kCodeEntry == 0);
    STATIC_ASSERT(compiler::CWasmEntryParameters::kObjectRef == 1);
    STATIC_ASSERT(compiler::CWasmEntryParameters::kArgumentsBuffer == 2);
    STATIC_ASSERT(compiler::CWasmEntryParameters::kCEntryFp == 3);
    Address result = stub_entry.Call(wasm_call_target, object_ref->ptr(),
                                     packed_args, saved_c_entry_fp);
    if (result != kNullAddress) {
      isolate->set_pending_exception(Object(result));
    }
  }

  // If there was an exception, then the thread-in-wasm flag is cleared
  // already.
  if (trap_handler::IsThreadInWasm()) {
    trap_handler::ClearThreadInWasm();
  }
  isolate->thread_local_top()->handler_ = stack_handler.next;
  if (saved_js_entry_sp == kNullAddress) {
    *isolate->js_entry_sp_address() = saved_js_entry_sp;
  }
  *isolate->c_entry_fp_address() = saved_c_entry_fp;
}
#endif  // V8_ENABLE_WEBASSEMBLY

}  // namespace internal
}  // namespace v8
