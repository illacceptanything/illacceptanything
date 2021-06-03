// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/execution/messages.h"

#include <memory>

#include "src/api/api-inl.h"
#include "src/ast/ast.h"
#include "src/ast/prettyprinter.h"
#include "src/base/v8-fallthrough.h"
#include "src/execution/execution.h"
#include "src/execution/frames-inl.h"
#include "src/execution/frames.h"
#include "src/execution/isolate-inl.h"
#include "src/logging/counters.h"
#include "src/objects/foreign-inl.h"
#include "src/objects/js-array-inl.h"
#include "src/objects/stack-frame-info-inl.h"
#include "src/objects/struct-inl.h"
#include "src/parsing/parse-info.h"
#include "src/parsing/parsing.h"
#include "src/roots/roots.h"
#include "src/strings/string-builder-inl.h"

namespace v8 {
namespace internal {

MessageLocation::MessageLocation(Handle<Script> script, int start_pos,
                                 int end_pos)
    : script_(script),
      start_pos_(start_pos),
      end_pos_(end_pos),
      bytecode_offset_(-1) {}

MessageLocation::MessageLocation(Handle<Script> script, int start_pos,
                                 int end_pos, Handle<SharedFunctionInfo> shared)
    : script_(script),
      start_pos_(start_pos),
      end_pos_(end_pos),
      bytecode_offset_(-1),
      shared_(shared) {}

MessageLocation::MessageLocation(Handle<Script> script,
                                 Handle<SharedFunctionInfo> shared,
                                 int bytecode_offset)
    : script_(script),
      start_pos_(-1),
      end_pos_(-1),
      bytecode_offset_(bytecode_offset),
      shared_(shared) {}

MessageLocation::MessageLocation()
    : start_pos_(-1), end_pos_(-1), bytecode_offset_(-1) {}

// If no message listeners have been registered this one is called
// by default.
void MessageHandler::DefaultMessageReport(Isolate* isolate,
                                          const MessageLocation* loc,
                                          Handle<Object> message_obj) {
  std::unique_ptr<char[]> str = GetLocalizedMessage(isolate, message_obj);
  if (loc == nullptr) {
    PrintF("%s\n", str.get());
  } else {
    HandleScope scope(isolate);
    Handle<Object> data(loc->script()->name(), isolate);
    std::unique_ptr<char[]> data_str;
    if (data->IsString())
      data_str = Handle<String>::cast(data)->ToCString(DISALLOW_NULLS);
    PrintF("%s:%i: %s\n", data_str.get() ? data_str.get() : "<unknown>",
           loc->start_pos(), str.get());
  }
}

Handle<JSMessageObject> MessageHandler::MakeMessageObject(
    Isolate* isolate, MessageTemplate message, const MessageLocation* location,
    Handle<Object> argument, Handle<FixedArray> stack_frames) {
  Factory* factory = isolate->factory();

  int start = -1;
  int end = -1;
  int bytecode_offset = -1;
  Handle<Script> script_handle = isolate->factory()->empty_script();
  Handle<SharedFunctionInfo> shared_info;
  if (location != nullptr && !FLAG_correctness_fuzzer_suppressions) {
    start = location->start_pos();
    end = location->end_pos();
    script_handle = location->script();
    bytecode_offset = location->bytecode_offset();
    shared_info = location->shared();
  }

  Handle<Object> stack_frames_handle =
      stack_frames.is_null() ? Handle<Object>::cast(factory->undefined_value())
                             : Handle<Object>::cast(stack_frames);

  Handle<JSMessageObject> message_obj = factory->NewJSMessageObject(
      message, argument, start, end, shared_info, bytecode_offset,
      script_handle, stack_frames_handle);

  return message_obj;
}

void MessageHandler::ReportMessage(Isolate* isolate, const MessageLocation* loc,
                                   Handle<JSMessageObject> message) {
  v8::Local<v8::Message> api_message_obj = v8::Utils::MessageToLocal(message);

  if (api_message_obj->ErrorLevel() == v8::Isolate::kMessageError) {
    // We are calling into embedder's code which can throw exceptions.
    // Thus we need to save current exception state, reset it to the clean one
    // and ignore scheduled exceptions callbacks can throw.

    // We pass the exception object into the message handler callback though.
    Object exception_object = ReadOnlyRoots(isolate).undefined_value();
    if (isolate->has_pending_exception()) {
      exception_object = isolate->pending_exception();
    }
    Handle<Object> exception(exception_object, isolate);

    Isolate::ExceptionScope exception_scope(isolate);
    isolate->clear_pending_exception();
    isolate->set_external_caught_exception(false);

    // Turn the exception on the message into a string if it is an object.
    if (message->argument().IsJSObject()) {
      HandleScope scope(isolate);
      Handle<Object> argument(message->argument(), isolate);

      MaybeHandle<Object> maybe_stringified;
      Handle<Object> stringified;
      // Make sure we don't leak uncaught internally generated Error objects.
      if (argument->IsJSError()) {
        maybe_stringified = Object::NoSideEffectsToString(isolate, argument);
      } else {
        v8::TryCatch catcher(reinterpret_cast<v8::Isolate*>(isolate));
        catcher.SetVerbose(false);
        catcher.SetCaptureMessage(false);

        maybe_stringified = Object::ToString(isolate, argument);
      }

      if (!maybe_stringified.ToHandle(&stringified)) {
        DCHECK(isolate->has_pending_exception());
        isolate->clear_pending_exception();
        isolate->set_external_caught_exception(false);
        stringified =
            isolate->factory()->NewStringFromAsciiChecked("exception");
      }
      message->set_argument(*stringified);
    }

    v8::Local<v8::Value> api_exception_obj = v8::Utils::ToLocal(exception);
    ReportMessageNoExceptions(isolate, loc, message, api_exception_obj);
  } else {
    ReportMessageNoExceptions(isolate, loc, message, v8::Local<v8::Value>());
  }
}

void MessageHandler::ReportMessageNoExceptions(
    Isolate* isolate, const MessageLocation* loc, Handle<Object> message,
    v8::Local<v8::Value> api_exception_obj) {
  v8::Local<v8::Message> api_message_obj = v8::Utils::MessageToLocal(message);
  int error_level = api_message_obj->ErrorLevel();

  Handle<TemplateList> global_listeners =
      isolate->factory()->message_listeners();
  int global_length = global_listeners->length();
  if (global_length == 0) {
    DefaultMessageReport(isolate, loc, message);
    if (isolate->has_scheduled_exception()) {
      isolate->clear_scheduled_exception();
    }
  } else {
    for (int i = 0; i < global_length; i++) {
      HandleScope scope(isolate);
      if (global_listeners->get(i).IsUndefined(isolate)) continue;
      FixedArray listener = FixedArray::cast(global_listeners->get(i));
      Foreign callback_obj = Foreign::cast(listener.get(0));
      int32_t message_levels =
          static_cast<int32_t>(Smi::ToInt(listener.get(2)));
      if (!(message_levels & error_level)) {
        continue;
      }
      v8::MessageCallback callback =
          FUNCTION_CAST<v8::MessageCallback>(callback_obj.foreign_address());
      Handle<Object> callback_data(listener.get(1), isolate);
      {
        RCS_SCOPE(isolate, RuntimeCallCounterId::kMessageListenerCallback);
        // Do not allow exceptions to propagate.
        v8::TryCatch try_catch(reinterpret_cast<v8::Isolate*>(isolate));
        callback(api_message_obj, callback_data->IsUndefined(isolate)
                                      ? api_exception_obj
                                      : v8::Utils::ToLocal(callback_data));
      }
      if (isolate->has_scheduled_exception()) {
        isolate->clear_scheduled_exception();
      }
    }
  }
}

Handle<String> MessageHandler::GetMessage(Isolate* isolate,
                                          Handle<Object> data) {
  Handle<JSMessageObject> message = Handle<JSMessageObject>::cast(data);
  Handle<Object> arg = Handle<Object>(message->argument(), isolate);
  return MessageFormatter::Format(isolate, message->type(), arg);
}

std::unique_ptr<char[]> MessageHandler::GetLocalizedMessage(
    Isolate* isolate, Handle<Object> data) {
  HandleScope scope(isolate);
  return GetMessage(isolate, data)->ToCString(DISALLOW_NULLS);
}

namespace {

// Convert the raw frames as written by Isolate::CaptureSimpleStackTrace into
// a JSArray of JSCallSite objects.
MaybeHandle<JSArray> GetStackFrames(Isolate* isolate,
                                    Handle<FixedArray> frames) {
  int frame_count = frames->length();
  Handle<JSFunction> constructor = isolate->callsite_function();
  Handle<FixedArray> sites = isolate->factory()->NewFixedArray(frame_count);
  for (int i = 0; i < frame_count; ++i) {
    Handle<StackFrameInfo> frame(StackFrameInfo::cast(frames->get(i)), isolate);
    Handle<JSObject> site;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, site,
        JSObject::New(constructor, constructor, Handle<AllocationSite>::null()),
        JSArray);
    RETURN_ON_EXCEPTION(
        isolate,
        JSObject::SetOwnPropertyIgnoreAttributes(
            site, isolate->factory()->call_site_frame_info_symbol(), frame,
            DONT_ENUM),
        JSArray);
    sites->set(i, *site);
  }

  return isolate->factory()->NewJSArrayWithElements(sites);
}

MaybeHandle<Object> AppendErrorString(Isolate* isolate, Handle<Object> error,
                                      IncrementalStringBuilder* builder) {
  MaybeHandle<String> err_str =
      ErrorUtils::ToString(isolate, Handle<Object>::cast(error));
  if (err_str.is_null()) {
    // Error.toString threw. Try to return a string representation of the thrown
    // exception instead.

    DCHECK(isolate->has_pending_exception());
    Handle<Object> pending_exception =
        handle(isolate->pending_exception(), isolate);
    isolate->clear_pending_exception();
    isolate->set_external_caught_exception(false);

    err_str = ErrorUtils::ToString(isolate, pending_exception);
    if (err_str.is_null()) {
      // Formatting the thrown exception threw again, give up.
      DCHECK(isolate->has_pending_exception());
      isolate->clear_pending_exception();
      isolate->set_external_caught_exception(false);
      builder->AppendCString("<error>");
    } else {
      // Formatted thrown exception successfully, append it.
      builder->AppendCString("<error: ");
      builder->AppendString(err_str.ToHandleChecked());
      builder->AppendCharacter('>');
    }
  } else {
    builder->AppendString(err_str.ToHandleChecked());
  }

  return error;
}

class V8_NODISCARD PrepareStackTraceScope {
 public:
  explicit PrepareStackTraceScope(Isolate* isolate) : isolate_(isolate) {
    DCHECK(!isolate_->formatting_stack_trace());
    isolate_->set_formatting_stack_trace(true);
  }

  ~PrepareStackTraceScope() { isolate_->set_formatting_stack_trace(false); }

  PrepareStackTraceScope(const PrepareStackTraceScope&) = delete;
  PrepareStackTraceScope& operator=(const PrepareStackTraceScope&) = delete;

 private:
  Isolate* isolate_;
};

}  // namespace

// static
MaybeHandle<Object> ErrorUtils::FormatStackTrace(Isolate* isolate,
                                                 Handle<JSObject> error,
                                                 Handle<Object> raw_stack) {
  DCHECK(raw_stack->IsFixedArray());
  Handle<FixedArray> elems = Handle<FixedArray>::cast(raw_stack);

  const bool in_recursion = isolate->formatting_stack_trace();
  Handle<Context> error_context;
  if (!in_recursion && error->GetCreationContext().ToHandle(&error_context)) {
    DCHECK(error_context->IsNativeContext());

    if (isolate->HasPrepareStackTraceCallback()) {
      PrepareStackTraceScope scope(isolate);

      Handle<JSArray> sites;
      ASSIGN_RETURN_ON_EXCEPTION(isolate, sites, GetStackFrames(isolate, elems),
                                 Object);

      Handle<Object> result;
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, result,
          isolate->RunPrepareStackTraceCallback(error_context, error, sites),
          Object);
      return result;
    } else {
      Handle<JSFunction> global_error =
          handle(error_context->error_function(), isolate);

      // If there's a user-specified "prepareStackTrace" function, call it on
      // the frames and use its result.

      Handle<Object> prepare_stack_trace;
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, prepare_stack_trace,
          JSFunction::GetProperty(isolate, global_error, "prepareStackTrace"),
          Object);

      if (prepare_stack_trace->IsJSFunction()) {
        PrepareStackTraceScope scope(isolate);

        isolate->CountUsage(v8::Isolate::kErrorPrepareStackTrace);

        Handle<JSArray> sites;
        ASSIGN_RETURN_ON_EXCEPTION(isolate, sites,
                                   GetStackFrames(isolate, elems), Object);

        const int argc = 2;
        ScopedVector<Handle<Object>> argv(argc);
        argv[0] = error;
        argv[1] = sites;

        Handle<Object> result;

        ASSIGN_RETURN_ON_EXCEPTION(
            isolate, result,
            Execution::Call(isolate, prepare_stack_trace, global_error, argc,
                            argv.begin()),
            Object);

        return result;
      }
    }
  }

  // Otherwise, run our internal formatting logic.

  IncrementalStringBuilder builder(isolate);

  RETURN_ON_EXCEPTION(isolate, AppendErrorString(isolate, error, &builder),
                      Object);

  for (int i = 0; i < elems->length(); ++i) {
    builder.AppendCString("\n    at ");

    Handle<StackFrameInfo> frame(StackFrameInfo::cast(elems->get(i)), isolate);
    SerializeStackFrameInfo(isolate, frame, &builder);

    if (isolate->has_pending_exception()) {
      // CallSite.toString threw. Parts of the current frame might have been
      // stringified already regardless. Still, try to append a string
      // representation of the thrown exception.

      Handle<Object> pending_exception =
          handle(isolate->pending_exception(), isolate);
      isolate->clear_pending_exception();
      isolate->set_external_caught_exception(false);

      MaybeHandle<String> exception_string =
          ErrorUtils::ToString(isolate, pending_exception);
      if (exception_string.is_null()) {
        // Formatting the thrown exception threw again, give up.

        builder.AppendCString("<error>");
      } else {
        // Formatted thrown exception successfully, append it.
        builder.AppendCString("<error: ");
        builder.AppendString(exception_string.ToHandleChecked());
        builder.AppendCString("<error>");
      }
    }
  }

  return builder.Finish();
}

Handle<String> MessageFormatter::Format(Isolate* isolate, MessageTemplate index,
                                        Handle<Object> arg0,
                                        Handle<Object> arg1,
                                        Handle<Object> arg2) {
  Factory* factory = isolate->factory();
  Handle<String> arg0_string = factory->empty_string();
  if (!arg0.is_null()) {
    arg0_string = Object::NoSideEffectsToString(isolate, arg0);
  }
  Handle<String> arg1_string = factory->empty_string();
  if (!arg1.is_null()) {
    arg1_string = Object::NoSideEffectsToString(isolate, arg1);
  }
  Handle<String> arg2_string = factory->empty_string();
  if (!arg2.is_null()) {
    arg2_string = Object::NoSideEffectsToString(isolate, arg2);
  }
  MaybeHandle<String> maybe_result_string = MessageFormatter::Format(
      isolate, index, arg0_string, arg1_string, arg2_string);
  Handle<String> result_string;
  if (!maybe_result_string.ToHandle(&result_string)) {
    DCHECK(isolate->has_pending_exception());
    isolate->clear_pending_exception();
    return factory->InternalizeString(StaticCharVector("<error>"));
  }
  // A string that has been obtained from JS code in this way is
  // likely to be a complicated ConsString of some sort.  We flatten it
  // here to improve the efficiency of converting it to a C string and
  // other operations that are likely to take place (see GetLocalizedMessage
  // for example).
  return String::Flatten(isolate, result_string);
}

const char* MessageFormatter::TemplateString(MessageTemplate index) {
  switch (index) {
#define CASE(NAME, STRING)       \
  case MessageTemplate::k##NAME: \
    return STRING;
    MESSAGE_TEMPLATES(CASE)
#undef CASE
    case MessageTemplate::kMessageCount:
    default:
      return nullptr;
  }
}

MaybeHandle<String> MessageFormatter::Format(Isolate* isolate,
                                             MessageTemplate index,
                                             Handle<String> arg0,
                                             Handle<String> arg1,
                                             Handle<String> arg2) {
  const char* template_string = TemplateString(index);
  if (template_string == nullptr) {
    isolate->ThrowIllegalOperation();
    return MaybeHandle<String>();
  }

  IncrementalStringBuilder builder(isolate);

  unsigned int i = 0;
  Handle<String> args[] = {arg0, arg1, arg2};
  for (const char* c = template_string; *c != '\0'; c++) {
    if (*c == '%') {
      // %% results in verbatim %.
      if (*(c + 1) == '%') {
        c++;
        builder.AppendCharacter('%');
      } else {
        DCHECK(i < arraysize(args));
        Handle<String> arg = args[i++];
        builder.AppendString(arg);
      }
    } else {
      builder.AppendCharacter(*c);
    }
  }

  return builder.Finish();
}

MaybeHandle<JSObject> ErrorUtils::Construct(Isolate* isolate,
                                            Handle<JSFunction> target,
                                            Handle<Object> new_target,
                                            Handle<Object> message,
                                            Handle<Object> options) {
  FrameSkipMode mode = SKIP_FIRST;
  Handle<Object> caller;

  // When we're passed a JSFunction as new target, we can skip frames until that
  // specific function is seen instead of unconditionally skipping the first
  // frame.
  if (new_target->IsJSFunction()) {
    mode = SKIP_UNTIL_SEEN;
    caller = new_target;
  }

  return ErrorUtils::Construct(isolate, target, new_target, message, options,
                               mode, caller,
                               ErrorUtils::StackTraceCollection::kDetailed);
}

MaybeHandle<JSObject> ErrorUtils::Construct(
    Isolate* isolate, Handle<JSFunction> target, Handle<Object> new_target,
    Handle<Object> message, Handle<Object> options, FrameSkipMode mode,
    Handle<Object> caller, StackTraceCollection stack_trace_collection) {
  if (FLAG_correctness_fuzzer_suppressions) {
    // Abort range errors in correctness fuzzing, as their causes differ
    // accross correctness-fuzzing scenarios.
    if (target.is_identical_to(isolate->range_error_function())) {
      FATAL("Aborting on range error");
    }
    // Ignore error messages in correctness fuzzing, because the spec leaves
    // room for undefined behavior.
    message = isolate->factory()->InternalizeUtf8String(
        "Message suppressed for fuzzers (--correctness-fuzzer-suppressions)");
  }

  // 1. If NewTarget is undefined, let newTarget be the active function object,
  // else let newTarget be NewTarget.
  Handle<JSReceiver> new_target_recv =
      new_target->IsJSReceiver() ? Handle<JSReceiver>::cast(new_target)
                                 : Handle<JSReceiver>::cast(target);

  // 2. Let O be ? OrdinaryCreateFromConstructor(newTarget, "%ErrorPrototype%",
  //    « [[ErrorData]] »).
  Handle<JSObject> err;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, err,
      JSObject::New(target, new_target_recv, Handle<AllocationSite>::null()),
      JSObject);

  // 3. If message is not undefined, then
  //  a. Let msg be ? ToString(message).
  //  b. Let msgDesc be the PropertyDescriptor{[[Value]]: msg, [[Writable]]:
  //     true, [[Enumerable]]: false, [[Configurable]]: true}.
  //  c. Perform ! DefinePropertyOrThrow(O, "message", msgDesc).
  // 4. Return O.
  if (!message->IsUndefined(isolate)) {
    Handle<String> msg_string;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, msg_string,
                               Object::ToString(isolate, message), JSObject);
    RETURN_ON_EXCEPTION(
        isolate,
        JSObject::SetOwnPropertyIgnoreAttributes(
            err, isolate->factory()->message_string(), msg_string, DONT_ENUM),
        JSObject);
  }

  if (FLAG_harmony_error_cause && !options->IsUndefined(isolate)) {
    // If Type(options) is Object and ? HasProperty(options, "cause") then
    //   a. Let cause be ? Get(options, "cause").
    //   b. Perform ! CreateNonEnumerableDataPropertyOrThrow(O, "cause", cause).
    Handle<Name> cause_string = isolate->factory()->cause_string();
    if (options->IsJSReceiver()) {
      Handle<JSReceiver> js_options = Handle<JSReceiver>::cast(options);
      Maybe<bool> has_cause = JSObject::HasProperty(js_options, cause_string);
      if (has_cause.IsNothing()) {
        DCHECK((isolate)->has_pending_exception());
        return MaybeHandle<JSObject>();
      }
      if (has_cause.ToChecked()) {
        Handle<Object> cause;
        ASSIGN_RETURN_ON_EXCEPTION(
            isolate, cause,
            JSObject::GetProperty(isolate, js_options, cause_string), JSObject);
        RETURN_ON_EXCEPTION(isolate,
                            JSObject::SetOwnPropertyIgnoreAttributes(
                                err, cause_string, cause, DONT_ENUM),
                            JSObject);
      }
    }
  }

  switch (stack_trace_collection) {
    case StackTraceCollection::kDetailed:
      RETURN_ON_EXCEPTION(
          isolate, isolate->CaptureAndSetDetailedStackTrace(err), JSObject);
      V8_FALLTHROUGH;
    case StackTraceCollection::kSimple:
      RETURN_ON_EXCEPTION(
          isolate, isolate->CaptureAndSetSimpleStackTrace(err, mode, caller),
          JSObject);
      break;
    case StackTraceCollection::kNone:
      break;
  }
  return err;
}

namespace {

MaybeHandle<String> GetStringPropertyOrDefault(Isolate* isolate,
                                               Handle<JSReceiver> recv,
                                               Handle<String> key,
                                               Handle<String> default_str) {
  Handle<Object> obj;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, obj,
                             JSObject::GetProperty(isolate, recv, key), String);

  Handle<String> str;
  if (obj->IsUndefined(isolate)) {
    str = default_str;
  } else {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, str, Object::ToString(isolate, obj),
                               String);
  }

  return str;
}

}  // namespace

// ES6 section 19.5.3.4 Error.prototype.toString ( )
MaybeHandle<String> ErrorUtils::ToString(Isolate* isolate,
                                         Handle<Object> receiver) {
  // 1. Let O be the this value.
  // 2. If Type(O) is not Object, throw a TypeError exception.
  if (!receiver->IsJSReceiver()) {
    return isolate->Throw<String>(isolate->factory()->NewTypeError(
        MessageTemplate::kIncompatibleMethodReceiver,
        isolate->factory()->NewStringFromAsciiChecked(
            "Error.prototype.toString"),
        receiver));
  }
  Handle<JSReceiver> recv = Handle<JSReceiver>::cast(receiver);

  // 3. Let name be ? Get(O, "name").
  // 4. If name is undefined, let name be "Error"; otherwise let name be
  // ? ToString(name).
  Handle<String> name_key = isolate->factory()->name_string();
  Handle<String> name_default = isolate->factory()->Error_string();
  Handle<String> name;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, name,
      GetStringPropertyOrDefault(isolate, recv, name_key, name_default),
      String);

  // 5. Let msg be ? Get(O, "message").
  // 6. If msg is undefined, let msg be the empty String; otherwise let msg be
  // ? ToString(msg).
  Handle<String> msg_key = isolate->factory()->message_string();
  Handle<String> msg_default = isolate->factory()->empty_string();
  Handle<String> msg;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, msg,
      GetStringPropertyOrDefault(isolate, recv, msg_key, msg_default), String);

  // 7. If name is the empty String, return msg.
  // 8. If msg is the empty String, return name.
  if (name->length() == 0) return msg;
  if (msg->length() == 0) return name;

  // 9. Return the result of concatenating name, the code unit 0x003A (COLON),
  // the code unit 0x0020 (SPACE), and msg.
  IncrementalStringBuilder builder(isolate);
  builder.AppendString(name);
  builder.AppendCString(": ");
  builder.AppendString(msg);

  Handle<String> result;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, result, builder.Finish(), String);
  return result;
}

namespace {

Handle<String> DoFormatMessage(Isolate* isolate, MessageTemplate index,
                               Handle<Object> arg0, Handle<Object> arg1,
                               Handle<Object> arg2) {
  Handle<String> arg0_str = Object::NoSideEffectsToString(isolate, arg0);
  Handle<String> arg1_str = Object::NoSideEffectsToString(isolate, arg1);
  Handle<String> arg2_str = Object::NoSideEffectsToString(isolate, arg2);

  isolate->native_context()->IncrementErrorsThrown();

  Handle<String> msg;
  if (!MessageFormatter::Format(isolate, index, arg0_str, arg1_str, arg2_str)
           .ToHandle(&msg)) {
    DCHECK(isolate->has_pending_exception());
    isolate->clear_pending_exception();
    isolate->set_external_caught_exception(false);
    return isolate->factory()->NewStringFromAsciiChecked("<error>");
  }

  return msg;
}

}  // namespace

// static
Handle<JSObject> ErrorUtils::MakeGenericError(
    Isolate* isolate, Handle<JSFunction> constructor, MessageTemplate index,
    Handle<Object> arg0, Handle<Object> arg1, Handle<Object> arg2,
    FrameSkipMode mode) {
  if (FLAG_clear_exceptions_on_js_entry) {
    // This function used to be implemented in JavaScript, and JSEntry
    // clears any pending exceptions - so whenever we'd call this from C++,
    // pending exceptions would be cleared. Preserve this behavior.
    isolate->clear_pending_exception();
  }
  Handle<String> msg = DoFormatMessage(isolate, index, arg0, arg1, arg2);
  Handle<Object> options = isolate->factory()->undefined_value();

  DCHECK(mode != SKIP_UNTIL_SEEN);

  Handle<Object> no_caller;
  // The call below can't fail because constructor is a builtin.
  DCHECK(constructor->shared().HasBuiltinId());
  return ErrorUtils::Construct(isolate, constructor, constructor, msg, options,
                               mode, no_caller, StackTraceCollection::kDetailed)
      .ToHandleChecked();
}

namespace {

bool ComputeLocation(Isolate* isolate, MessageLocation* target) {
  JavaScriptFrameIterator it(isolate);
  if (!it.done()) {
    // Compute the location from the function and the relocation info of the
    // baseline code. For optimized code this will use the deoptimization
    // information to get canonical location information.
    std::vector<FrameSummary> frames;
    it.frame()->Summarize(&frames);
    auto& summary = frames.back().AsJavaScript();
    Handle<SharedFunctionInfo> shared(summary.function()->shared(), isolate);
    Handle<Object> script(shared->script(), isolate);
    SharedFunctionInfo::EnsureSourcePositionsAvailable(isolate, shared);
    int pos = summary.abstract_code()->SourcePosition(summary.code_offset());
    if (script->IsScript() &&
        !(Handle<Script>::cast(script)->source().IsUndefined(isolate))) {
      Handle<Script> casted_script = Handle<Script>::cast(script);
      *target = MessageLocation(casted_script, pos, pos + 1, shared);
      return true;
    }
  }
  return false;
}

Handle<String> BuildDefaultCallSite(Isolate* isolate, Handle<Object> object) {
  IncrementalStringBuilder builder(isolate);

  builder.AppendString(Object::TypeOf(isolate, object));
  if (object->IsString()) {
    builder.AppendCString(" \"");
    Handle<String> string = Handle<String>::cast(object);
    // This threshold must be sufficiently far below String::kMaxLength that
    // the {builder}'s result can never exceed that limit.
    constexpr int kMaxPrintedStringLength = 100;
    if (string->length() <= kMaxPrintedStringLength) {
      builder.AppendString(string);
    } else {
      string = isolate->factory()->NewProperSubString(string, 0,
                                                      kMaxPrintedStringLength);
      builder.AppendString(string);
      builder.AppendCString("<...>");
    }
    builder.AppendCString("\"");
  } else if (object->IsNull(isolate)) {
    builder.AppendCString(" ");
    builder.AppendString(isolate->factory()->null_string());
  } else if (object->IsTrue(isolate)) {
    builder.AppendCString(" ");
    builder.AppendString(isolate->factory()->true_string());
  } else if (object->IsFalse(isolate)) {
    builder.AppendCString(" ");
    builder.AppendString(isolate->factory()->false_string());
  } else if (object->IsNumber()) {
    builder.AppendCString(" ");
    builder.AppendString(isolate->factory()->NumberToString(object));
  }

  return builder.Finish().ToHandleChecked();
}

Handle<String> RenderCallSite(Isolate* isolate, Handle<Object> object,
                              MessageLocation* location,
                              CallPrinter::ErrorHint* hint) {
  if (ComputeLocation(isolate, location)) {
    UnoptimizedCompileFlags flags = UnoptimizedCompileFlags::ForFunctionCompile(
        isolate, *location->shared());
    UnoptimizedCompileState compile_state(isolate);
    ParseInfo info(isolate, flags, &compile_state);
    if (parsing::ParseAny(&info, location->shared(), isolate,
                          parsing::ReportStatisticsMode::kNo)) {
      info.ast_value_factory()->Internalize(isolate);
      CallPrinter printer(isolate, location->shared()->IsUserJavaScript());
      Handle<String> str = printer.Print(info.literal(), location->start_pos());
      *hint = printer.GetErrorHint();
      if (str->length() > 0) return str;
    }
  }
  return BuildDefaultCallSite(isolate, object);
}

MessageTemplate UpdateErrorTemplate(CallPrinter::ErrorHint hint,
                                    MessageTemplate default_id) {
  switch (hint) {
    case CallPrinter::ErrorHint::kNormalIterator:
      return MessageTemplate::kNotIterable;

    case CallPrinter::ErrorHint::kCallAndNormalIterator:
      return MessageTemplate::kNotCallableOrIterable;

    case CallPrinter::ErrorHint::kAsyncIterator:
      return MessageTemplate::kNotAsyncIterable;

    case CallPrinter::ErrorHint::kCallAndAsyncIterator:
      return MessageTemplate::kNotCallableOrAsyncIterable;

    case CallPrinter::ErrorHint::kNone:
      return default_id;
  }
}

}  // namespace

Handle<JSObject> ErrorUtils::NewIteratorError(Isolate* isolate,
                                              Handle<Object> source) {
  MessageLocation location;
  CallPrinter::ErrorHint hint = CallPrinter::kNone;
  Handle<String> callsite = RenderCallSite(isolate, source, &location, &hint);
  MessageTemplate id = MessageTemplate::kNotIterableNoSymbolLoad;

  if (hint == CallPrinter::kNone) {
    Handle<Symbol> iterator_symbol = isolate->factory()->iterator_symbol();
    return isolate->factory()->NewTypeError(id, callsite, iterator_symbol);
  }

  id = UpdateErrorTemplate(hint, id);
  return isolate->factory()->NewTypeError(id, callsite);
}

Object ErrorUtils::ThrowSpreadArgError(Isolate* isolate, MessageTemplate id,
                                       Handle<Object> object) {
  MessageLocation location;
  Handle<String> callsite;
  if (ComputeLocation(isolate, &location)) {
    UnoptimizedCompileFlags flags = UnoptimizedCompileFlags::ForFunctionCompile(
        isolate, *location.shared());
    UnoptimizedCompileState compile_state(isolate);
    ParseInfo info(isolate, flags, &compile_state);
    if (parsing::ParseAny(&info, location.shared(), isolate,
                          parsing::ReportStatisticsMode::kNo)) {
      info.ast_value_factory()->Internalize(isolate);
      CallPrinter printer(isolate, location.shared()->IsUserJavaScript(),
                          CallPrinter::SpreadErrorInArgsHint::kErrorInArgs);
      Handle<String> str = printer.Print(info.literal(), location.start_pos());
      callsite =
          str->length() > 0 ? str : BuildDefaultCallSite(isolate, object);

      if (printer.spread_arg() != nullptr) {
        // Change the message location to point at the property name.
        int pos = printer.spread_arg()->position();
        location =
            MessageLocation(location.script(), pos, pos + 1, location.shared());
      }
    } else {
      callsite = BuildDefaultCallSite(isolate, object);
    }
  }

  isolate->ThrowAt(isolate->factory()->NewTypeError(id, callsite, object),
                   &location);
  return ReadOnlyRoots(isolate).exception();
}

Handle<JSObject> ErrorUtils::NewCalledNonCallableError(Isolate* isolate,
                                                       Handle<Object> source) {
  MessageLocation location;
  CallPrinter::ErrorHint hint = CallPrinter::kNone;
  Handle<String> callsite = RenderCallSite(isolate, source, &location, &hint);
  MessageTemplate id = MessageTemplate::kCalledNonCallable;
  id = UpdateErrorTemplate(hint, id);
  return isolate->factory()->NewTypeError(id, callsite);
}

Handle<JSObject> ErrorUtils::NewConstructedNonConstructable(
    Isolate* isolate, Handle<Object> source) {
  MessageLocation location;
  CallPrinter::ErrorHint hint = CallPrinter::kNone;
  Handle<String> callsite = RenderCallSite(isolate, source, &location, &hint);
  MessageTemplate id = MessageTemplate::kNotConstructor;
  return isolate->factory()->NewTypeError(id, callsite);
}

Object ErrorUtils::ThrowLoadFromNullOrUndefined(Isolate* isolate,
                                                Handle<Object> object,
                                                MaybeHandle<Object> key) {
  DCHECK(object->IsNullOrUndefined());

  MaybeHandle<String> maybe_property_name;

  // Try to extract the property name from the given key, if any.
  Handle<Object> key_handle;
  if (key.ToHandle(&key_handle)) {
    if (key_handle->IsString()) {
      maybe_property_name = Handle<String>::cast(key_handle);
    }
  }

  Handle<String> callsite;

  // Inline the RenderCallSite logic here so that we can additonally access the
  // destructuring property.
  bool location_computed = false;
  bool is_destructuring = false;
  MessageLocation location;
  if (ComputeLocation(isolate, &location)) {
    location_computed = true;

    UnoptimizedCompileFlags flags = UnoptimizedCompileFlags::ForFunctionCompile(
        isolate, *location.shared());
    UnoptimizedCompileState compile_state(isolate);
    ParseInfo info(isolate, flags, &compile_state);
    if (parsing::ParseAny(&info, location.shared(), isolate,
                          parsing::ReportStatisticsMode::kNo)) {
      info.ast_value_factory()->Internalize(isolate);
      CallPrinter printer(isolate, location.shared()->IsUserJavaScript());
      Handle<String> str = printer.Print(info.literal(), location.start_pos());

      int pos = -1;
      is_destructuring = printer.destructuring_assignment() != nullptr;

      if (is_destructuring) {
        // If we don't have one yet, try to extract the property name from the
        // destructuring property in the AST.
        ObjectLiteralProperty* destructuring_prop =
            printer.destructuring_prop();
        if (maybe_property_name.is_null() && destructuring_prop != nullptr &&
            destructuring_prop->key()->IsPropertyName()) {
          maybe_property_name = destructuring_prop->key()
                                    ->AsLiteral()
                                    ->AsRawPropertyName()
                                    ->string();
          // Change the message location to point at the property name.
          pos = destructuring_prop->key()->position();
        }
        if (maybe_property_name.is_null()) {
          // Change the message location to point at the destructured value.
          pos = printer.destructuring_assignment()->value()->position();
        }

        // If we updated the pos to a valid pos, rewrite the location.
        if (pos != -1) {
          location = MessageLocation(location.script(), pos, pos + 1,
                                     location.shared());
        }
      }

      if (str->length() > 0) callsite = str;
    }
  }

  if (callsite.is_null()) {
    callsite = BuildDefaultCallSite(isolate, object);
  }

  Handle<JSObject> error;
  Handle<String> property_name;
  if (is_destructuring) {
    if (maybe_property_name.ToHandle(&property_name)) {
      error = isolate->factory()->NewTypeError(
          MessageTemplate::kNonCoercibleWithProperty, property_name, callsite,
          object);
    } else {
      error = isolate->factory()->NewTypeError(MessageTemplate::kNonCoercible,
                                               callsite, object);
    }
  } else {
    Handle<Object> key_handle;
    if (!key.ToHandle(&key_handle)) {
      key_handle = ReadOnlyRoots(isolate).undefined_value_handle();
    }
    if (*key_handle == ReadOnlyRoots(isolate).iterator_symbol()) {
      error = NewIteratorError(isolate, object);
    } else {
      error = isolate->factory()->NewTypeError(
          MessageTemplate::kNonObjectPropertyLoad, key_handle, object);
    }
  }

  if (location_computed) {
    isolate->ThrowAt(error, &location);
  } else {
    isolate->Throw(*error);
  }
  return ReadOnlyRoots(isolate).exception();
}

}  // namespace internal
}  // namespace v8
