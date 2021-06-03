// Copyright 2006-2008 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The infrastructure used for (localized) message reporting in V8.
//
// Note: there's a big unresolved issue about ownership of the data
// structures used by this framework.

#ifndef V8_EXECUTION_MESSAGES_H_
#define V8_EXECUTION_MESSAGES_H_

#include <memory>

#include "src/base/optional.h"
#include "src/common/message-template.h"
#include "src/handles/handles.h"

namespace v8 {
namespace internal {
namespace wasm {
class WasmCode;
}  // namespace wasm

// Forward declarations.
class AbstractCode;
class JSMessageObject;
class LookupIterator;
class PrimitiveHeapObject;
class SharedFunctionInfo;
class SourceInfo;
class WasmInstanceObject;

class V8_EXPORT_PRIVATE MessageLocation {
 public:
  // Constructors for when source positions are already known.
  // TODO(delphick): Collapse to a single constructor with a default parameter
  // when we stop using the GCC that requires this separation.
  MessageLocation(Handle<Script> script, int start_pos, int end_pos);
  MessageLocation(Handle<Script> script, int start_pos, int end_pos,
                  Handle<SharedFunctionInfo> shared);
  // Constructor for when source positions were not collected but which can be
  // reconstructed from the SharedFuncitonInfo and bytecode offset.
  MessageLocation(Handle<Script> script, Handle<SharedFunctionInfo> shared,
                  int bytecode_offset);
  MessageLocation();

  Handle<Script> script() const { return script_; }
  int start_pos() const { return start_pos_; }
  int end_pos() const { return end_pos_; }
  int bytecode_offset() const { return bytecode_offset_; }
  Handle<SharedFunctionInfo> shared() const { return shared_; }

 private:
  Handle<Script> script_;
  int start_pos_;
  int end_pos_;
  int bytecode_offset_;
  Handle<SharedFunctionInfo> shared_;
};

// Determines how stack trace collection skips frames.
enum FrameSkipMode {
  // Unconditionally skips the first frame. Used e.g. when the Error constructor
  // is called, in which case the first frame is always a BUILTIN_EXIT frame.
  SKIP_FIRST,
  // Skip all frames until a specified caller function is seen.
  SKIP_UNTIL_SEEN,
  SKIP_NONE,
};

class ErrorUtils : public AllStatic {
 public:
  // |kNone| is useful when you don't need the stack information at all, for
  // example when creating a deserialized error.
  enum class StackTraceCollection { kDetailed, kSimple, kNone };
  static MaybeHandle<JSObject> Construct(Isolate* isolate,
                                         Handle<JSFunction> target,
                                         Handle<Object> new_target,
                                         Handle<Object> message,
                                         Handle<Object> options);
  static MaybeHandle<JSObject> Construct(
      Isolate* isolate, Handle<JSFunction> target, Handle<Object> new_target,
      Handle<Object> message, Handle<Object> options, FrameSkipMode mode,
      Handle<Object> caller, StackTraceCollection stack_trace_collection);

  static MaybeHandle<String> ToString(Isolate* isolate, Handle<Object> recv);

  static Handle<JSObject> MakeGenericError(
      Isolate* isolate, Handle<JSFunction> constructor, MessageTemplate index,
      Handle<Object> arg0, Handle<Object> arg1, Handle<Object> arg2,
      FrameSkipMode mode);

  // Formats a textual stack trace from the given structured stack trace.
  // Note that this can call arbitrary JS code through Error.prepareStackTrace.
  static MaybeHandle<Object> FormatStackTrace(Isolate* isolate,
                                              Handle<JSObject> error,
                                              Handle<Object> stack_trace);

  static Handle<JSObject> NewIteratorError(Isolate* isolate,
                                           Handle<Object> source);
  static Handle<JSObject> NewCalledNonCallableError(Isolate* isolate,
                                                    Handle<Object> source);
  static Handle<JSObject> NewConstructedNonConstructable(Isolate* isolate,
                                                         Handle<Object> source);
  // Returns the Exception sentinel.
  static Object ThrowSpreadArgError(Isolate* isolate, MessageTemplate id,
                                    Handle<Object> object);
  // Returns the Exception sentinel.
  static Object ThrowLoadFromNullOrUndefined(Isolate* isolate,
                                             Handle<Object> object,
                                             MaybeHandle<Object> key);
};

class MessageFormatter {
 public:
  V8_EXPORT_PRIVATE static const char* TemplateString(MessageTemplate index);

  V8_EXPORT_PRIVATE static MaybeHandle<String> Format(Isolate* isolate,
                                                      MessageTemplate index,
                                                      Handle<String> arg0,
                                                      Handle<String> arg1,
                                                      Handle<String> arg2);

  static Handle<String> Format(Isolate* isolate, MessageTemplate index,
                               Handle<Object> arg0,
                               Handle<Object> arg1 = Handle<Object>(),
                               Handle<Object> arg2 = Handle<Object>());
};

// A message handler is a convenience interface for accessing the list
// of message listeners registered in an environment
class MessageHandler {
 public:
  // Returns a message object for the API to use.
  V8_EXPORT_PRIVATE static Handle<JSMessageObject> MakeMessageObject(
      Isolate* isolate, MessageTemplate type, const MessageLocation* location,
      Handle<Object> argument, Handle<FixedArray> stack_frames);

  // Report a formatted message (needs JS allocation).
  V8_EXPORT_PRIVATE static void ReportMessage(Isolate* isolate,
                                              const MessageLocation* loc,
                                              Handle<JSMessageObject> message);

  static void DefaultMessageReport(Isolate* isolate, const MessageLocation* loc,
                                   Handle<Object> message_obj);
  static Handle<String> GetMessage(Isolate* isolate, Handle<Object> data);
  static std::unique_ptr<char[]> GetLocalizedMessage(Isolate* isolate,
                                                     Handle<Object> data);

 private:
  static void ReportMessageNoExceptions(Isolate* isolate,
                                        const MessageLocation* loc,
                                        Handle<Object> message_obj,
                                        v8::Local<v8::Value> api_exception_obj);
};

}  // namespace internal
}  // namespace v8

#endif  // V8_EXECUTION_MESSAGES_H_
