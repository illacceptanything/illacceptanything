/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "src/inspector/v8-runtime-agent-impl.h"

#include <inttypes.h>

#include "../../third_party/inspector_protocol/crdtp/json.h"
#include "src/debug/debug-interface.h"
#include "src/inspector/injected-script.h"
#include "src/inspector/inspected-context.h"
#include "src/inspector/protocol/Protocol.h"
#include "src/inspector/remote-object-id.h"
#include "src/inspector/v8-console-message.h"
#include "src/inspector/v8-debugger-agent-impl.h"
#include "src/inspector/v8-debugger.h"
#include "src/inspector/v8-inspector-impl.h"
#include "src/inspector/v8-inspector-session-impl.h"
#include "src/inspector/v8-stack-trace-impl.h"
#include "src/inspector/v8-value-utils.h"
#include "src/tracing/trace-event.h"

#include "include/v8-inspector.h"

namespace v8_inspector {

namespace V8RuntimeAgentImplState {
static const char customObjectFormatterEnabled[] =
    "customObjectFormatterEnabled";
static const char runtimeEnabled[] = "runtimeEnabled";
static const char bindings[] = "bindings";
static const char globalBindingsKey[] = "";
}  // namespace V8RuntimeAgentImplState

using protocol::Runtime::RemoteObject;

namespace {

template <typename ProtocolCallback>
class EvaluateCallbackWrapper : public EvaluateCallback {
 public:
  static std::unique_ptr<EvaluateCallback> wrap(
      std::unique_ptr<ProtocolCallback> callback) {
    return std::unique_ptr<EvaluateCallback>(
        new EvaluateCallbackWrapper(std::move(callback)));
  }
  void sendSuccess(std::unique_ptr<protocol::Runtime::RemoteObject> result,
                   protocol::Maybe<protocol::Runtime::ExceptionDetails>
                       exceptionDetails) override {
    return m_callback->sendSuccess(std::move(result),
                                   std::move(exceptionDetails));
  }
  void sendFailure(const protocol::DispatchResponse& response) override {
    return m_callback->sendFailure(response);
  }

 private:
  explicit EvaluateCallbackWrapper(std::unique_ptr<ProtocolCallback> callback)
      : m_callback(std::move(callback)) {}

  std::unique_ptr<ProtocolCallback> m_callback;
};

template <typename ProtocolCallback>
bool wrapEvaluateResultAsync(InjectedScript* injectedScript,
                             v8::MaybeLocal<v8::Value> maybeResultValue,
                             const v8::TryCatch& tryCatch,
                             const String16& objectGroup, WrapMode wrapMode,
                             ProtocolCallback* callback) {
  std::unique_ptr<RemoteObject> result;
  Maybe<protocol::Runtime::ExceptionDetails> exceptionDetails;

  Response response = injectedScript->wrapEvaluateResult(
      maybeResultValue, tryCatch, objectGroup, wrapMode, &result,
      &exceptionDetails);
  if (response.IsSuccess()) {
    callback->sendSuccess(std::move(result), std::move(exceptionDetails));
    return true;
  }
  callback->sendFailure(response);
  return false;
}

void innerCallFunctionOn(
    V8InspectorSessionImpl* session, InjectedScript::Scope& scope,
    v8::Local<v8::Value> recv, const String16& expression,
    Maybe<protocol::Array<protocol::Runtime::CallArgument>> optionalArguments,
    bool silent, WrapMode wrapMode, bool userGesture, bool awaitPromise,
    const String16& objectGroup,
    std::unique_ptr<V8RuntimeAgentImpl::CallFunctionOnCallback> callback) {
  V8InspectorImpl* inspector = session->inspector();

  std::unique_ptr<v8::Local<v8::Value>[]> argv = nullptr;
  int argc = 0;
  if (optionalArguments.isJust()) {
    protocol::Array<protocol::Runtime::CallArgument>* arguments =
        optionalArguments.fromJust();
    argc = static_cast<int>(arguments->size());
    argv.reset(new v8::Local<v8::Value>[argc]);
    for (int i = 0; i < argc; ++i) {
      v8::Local<v8::Value> argumentValue;
      Response response = scope.injectedScript()->resolveCallArgument(
          (*arguments)[i].get(), &argumentValue);
      if (!response.IsSuccess()) {
        callback->sendFailure(response);
        return;
      }
      argv[i] = argumentValue;
    }
  }

  if (silent) scope.ignoreExceptionsAndMuteConsole();
  if (userGesture) scope.pretendUserGesture();

  // Temporarily enable allow evals for inspector.
  scope.allowCodeGenerationFromStrings();

  v8::MaybeLocal<v8::Value> maybeFunctionValue;
  v8::Local<v8::Script> functionScript;
  if (inspector
          ->compileScript(scope.context(), "(" + expression + ")", String16())
          .ToLocal(&functionScript)) {
    v8::MicrotasksScope microtasksScope(inspector->isolate(),
                                        v8::MicrotasksScope::kRunMicrotasks);
    maybeFunctionValue = functionScript->Run(scope.context());
  }
  // Re-initialize after running client's code, as it could have destroyed
  // context or session.
  Response response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  if (scope.tryCatch().HasCaught()) {
    wrapEvaluateResultAsync(scope.injectedScript(), maybeFunctionValue,
                            scope.tryCatch(), objectGroup, WrapMode::kNoPreview,
                            callback.get());
    return;
  }

  v8::Local<v8::Value> functionValue;
  if (!maybeFunctionValue.ToLocal(&functionValue) ||
      !functionValue->IsFunction()) {
    callback->sendFailure(Response::ServerError(
        "Given expression does not evaluate to a function"));
    return;
  }

  v8::MaybeLocal<v8::Value> maybeResultValue;
  {
    v8::MicrotasksScope microtasksScope(inspector->isolate(),
                                        v8::MicrotasksScope::kRunMicrotasks);
    maybeResultValue = functionValue.As<v8::Function>()->Call(
        scope.context(), recv, argc, argv.get());
  }
  // Re-initialize after running client's code, as it could have destroyed
  // context or session.
  response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  if (!awaitPromise || scope.tryCatch().HasCaught()) {
    wrapEvaluateResultAsync(scope.injectedScript(), maybeResultValue,
                            scope.tryCatch(), objectGroup, wrapMode,
                            callback.get());
    return;
  }

  scope.injectedScript()->addPromiseCallback(
      session, maybeResultValue, objectGroup, wrapMode, false /* replMode */,
      EvaluateCallbackWrapper<V8RuntimeAgentImpl::CallFunctionOnCallback>::wrap(
          std::move(callback)));
}

Response ensureContext(V8InspectorImpl* inspector, int contextGroupId,
                       Maybe<int> executionContextId,
                       Maybe<String16> uniqueContextId, int* contextId) {
  if (executionContextId.isJust()) {
    if (uniqueContextId.isJust()) {
      return Response::InvalidParams(
          "contextId and uniqueContextId are mutually exclusive");
    }
    *contextId = executionContextId.fromJust();
  } else if (uniqueContextId.isJust()) {
    V8DebuggerId uniqueId(uniqueContextId.fromJust());
    if (!uniqueId.isValid())
      return Response::InvalidParams("invalid uniqueContextId");
    int id = inspector->resolveUniqueContextId(uniqueId);
    if (!id) return Response::InvalidParams("uniqueContextId not found");
    *contextId = id;
  } else {
    v8::HandleScope handles(inspector->isolate());
    v8::Local<v8::Context> defaultContext =
        inspector->client()->ensureDefaultContextInGroup(contextGroupId);
    if (defaultContext.IsEmpty())
      return Response::ServerError("Cannot find default execution context");
    *contextId = InspectedContext::contextId(defaultContext);
  }

  return Response::Success();
}

}  // namespace

V8RuntimeAgentImpl::V8RuntimeAgentImpl(
    V8InspectorSessionImpl* session, protocol::FrontendChannel* FrontendChannel,
    protocol::DictionaryValue* state)
    : m_session(session),
      m_state(state),
      m_frontend(FrontendChannel),
      m_inspector(session->inspector()),
      m_enabled(false) {}

V8RuntimeAgentImpl::~V8RuntimeAgentImpl() = default;

void V8RuntimeAgentImpl::evaluate(
    const String16& expression, Maybe<String16> objectGroup,
    Maybe<bool> includeCommandLineAPI, Maybe<bool> silent,
    Maybe<int> executionContextId, Maybe<bool> returnByValue,
    Maybe<bool> generatePreview, Maybe<bool> userGesture,
    Maybe<bool> maybeAwaitPromise, Maybe<bool> throwOnSideEffect,
    Maybe<double> timeout, Maybe<bool> disableBreaks, Maybe<bool> maybeReplMode,
    Maybe<bool> allowUnsafeEvalBlockedByCSP, Maybe<String16> uniqueContextId,
    std::unique_ptr<EvaluateCallback> callback) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
               "EvaluateScript");
  int contextId = 0;
  Response response = ensureContext(m_inspector, m_session->contextGroupId(),
                                    std::move(executionContextId),
                                    std::move(uniqueContextId), &contextId);
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  InjectedScript::ContextScope scope(m_session, contextId);
  response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  if (silent.fromMaybe(false)) scope.ignoreExceptionsAndMuteConsole();
  if (userGesture.fromMaybe(false)) scope.pretendUserGesture();

  if (includeCommandLineAPI.fromMaybe(false)) scope.installCommandLineAPI();

  const bool replMode = maybeReplMode.fromMaybe(false);

  if (allowUnsafeEvalBlockedByCSP.fromMaybe(true)) {
    // Temporarily enable allow evals for inspector.
    scope.allowCodeGenerationFromStrings();
  }
  v8::MaybeLocal<v8::Value> maybeResultValue;
  {
    V8InspectorImpl::EvaluateScope evaluateScope(scope);
    if (timeout.isJust()) {
      response = evaluateScope.setTimeout(timeout.fromJust() / 1000.0);
      if (!response.IsSuccess()) {
        callback->sendFailure(response);
        return;
      }
    }
    v8::MicrotasksScope microtasksScope(m_inspector->isolate(),
                                        v8::MicrotasksScope::kRunMicrotasks);
    v8::debug::EvaluateGlobalMode mode =
        v8::debug::EvaluateGlobalMode::kDefault;
    if (throwOnSideEffect.fromMaybe(false)) {
      mode = v8::debug::EvaluateGlobalMode::kDisableBreaksAndThrowOnSideEffect;
    } else if (disableBreaks.fromMaybe(false)) {
      mode = v8::debug::EvaluateGlobalMode::kDisableBreaks;
    }
    const v8::Local<v8::String> source =
        toV8String(m_inspector->isolate(), expression);
    maybeResultValue = v8::debug::EvaluateGlobal(m_inspector->isolate(), source,
                                                 mode, replMode);
  }  // Run microtasks before returning result.

  // Re-initialize after running client's code, as it could have destroyed
  // context or session.
  response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  WrapMode mode = generatePreview.fromMaybe(false) ? WrapMode::kWithPreview
                                                   : WrapMode::kNoPreview;
  if (returnByValue.fromMaybe(false)) mode = WrapMode::kForceValue;

  // REPL mode always returns a promise that must be awaited.
  const bool await = replMode || maybeAwaitPromise.fromMaybe(false);
  if (!await || scope.tryCatch().HasCaught()) {
    wrapEvaluateResultAsync(scope.injectedScript(), maybeResultValue,
                            scope.tryCatch(), objectGroup.fromMaybe(""), mode,
                            callback.get());
    return;
  }
  scope.injectedScript()->addPromiseCallback(
      m_session, maybeResultValue, objectGroup.fromMaybe(""), mode, replMode,
      EvaluateCallbackWrapper<EvaluateCallback>::wrap(std::move(callback)));
}

void V8RuntimeAgentImpl::awaitPromise(
    const String16& promiseObjectId, Maybe<bool> returnByValue,
    Maybe<bool> generatePreview,
    std::unique_ptr<AwaitPromiseCallback> callback) {
  InjectedScript::ObjectScope scope(m_session, promiseObjectId);
  Response response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }
  if (!scope.object()->IsPromise()) {
    callback->sendFailure(
        Response::ServerError("Could not find promise with given id"));
    return;
  }
  WrapMode mode = generatePreview.fromMaybe(false) ? WrapMode::kWithPreview
                                                   : WrapMode::kNoPreview;
  if (returnByValue.fromMaybe(false)) mode = WrapMode::kForceValue;
  scope.injectedScript()->addPromiseCallback(
      m_session, scope.object(), scope.objectGroupName(), mode,
      false /* replMode */,
      EvaluateCallbackWrapper<AwaitPromiseCallback>::wrap(std::move(callback)));
}

void V8RuntimeAgentImpl::callFunctionOn(
    const String16& expression, Maybe<String16> objectId,
    Maybe<protocol::Array<protocol::Runtime::CallArgument>> optionalArguments,
    Maybe<bool> silent, Maybe<bool> returnByValue, Maybe<bool> generatePreview,
    Maybe<bool> userGesture, Maybe<bool> awaitPromise,
    Maybe<int> executionContextId, Maybe<String16> objectGroup,
    std::unique_ptr<CallFunctionOnCallback> callback) {
  if (objectId.isJust() && executionContextId.isJust()) {
    callback->sendFailure(Response::ServerError(
        "ObjectId must not be specified together with executionContextId"));
    return;
  }
  if (!objectId.isJust() && !executionContextId.isJust()) {
    callback->sendFailure(Response::ServerError(
        "Either ObjectId or executionContextId must be specified"));
    return;
  }
  WrapMode mode = generatePreview.fromMaybe(false) ? WrapMode::kWithPreview
                                                   : WrapMode::kNoPreview;
  if (returnByValue.fromMaybe(false)) mode = WrapMode::kForceValue;
  if (objectId.isJust()) {
    InjectedScript::ObjectScope scope(m_session, objectId.fromJust());
    Response response = scope.initialize();
    if (!response.IsSuccess()) {
      callback->sendFailure(response);
      return;
    }
    innerCallFunctionOn(m_session, scope, scope.object(), expression,
                        std::move(optionalArguments), silent.fromMaybe(false),
                        mode, userGesture.fromMaybe(false),
                        awaitPromise.fromMaybe(false),
                        objectGroup.isJust() ? objectGroup.fromMaybe(String16())
                                             : scope.objectGroupName(),
                        std::move(callback));
  } else {
    int contextId = 0;
    Response response = ensureContext(m_inspector, m_session->contextGroupId(),
                                      std::move(executionContextId.fromJust()),
                                      /* uniqueContextId */ {}, &contextId);
    if (!response.IsSuccess()) {
      callback->sendFailure(response);
      return;
    }
    InjectedScript::ContextScope scope(m_session, contextId);
    response = scope.initialize();
    if (!response.IsSuccess()) {
      callback->sendFailure(response);
      return;
    }
    innerCallFunctionOn(m_session, scope, scope.context()->Global(), expression,
                        std::move(optionalArguments), silent.fromMaybe(false),
                        mode, userGesture.fromMaybe(false),
                        awaitPromise.fromMaybe(false),
                        objectGroup.fromMaybe(""), std::move(callback));
  }
}

Response V8RuntimeAgentImpl::getProperties(
    const String16& objectId, Maybe<bool> ownProperties,
    Maybe<bool> accessorPropertiesOnly, Maybe<bool> generatePreview,
    std::unique_ptr<protocol::Array<protocol::Runtime::PropertyDescriptor>>*
        result,
    Maybe<protocol::Array<protocol::Runtime::InternalPropertyDescriptor>>*
        internalProperties,
    Maybe<protocol::Array<protocol::Runtime::PrivatePropertyDescriptor>>*
        privateProperties,
    Maybe<protocol::Runtime::ExceptionDetails>* exceptionDetails) {
  using protocol::Runtime::InternalPropertyDescriptor;
  using protocol::Runtime::PrivatePropertyDescriptor;

  InjectedScript::ObjectScope scope(m_session, objectId);
  Response response = scope.initialize();
  if (!response.IsSuccess()) return response;

  scope.ignoreExceptionsAndMuteConsole();
  v8::MicrotasksScope microtasks_scope(m_inspector->isolate(),
                                       v8::MicrotasksScope::kRunMicrotasks);
  if (!scope.object()->IsObject())
    return Response::ServerError("Value with given id is not an object");

  v8::Local<v8::Object> object = scope.object().As<v8::Object>();
  response = scope.injectedScript()->getProperties(
      object, scope.objectGroupName(), ownProperties.fromMaybe(false),
      accessorPropertiesOnly.fromMaybe(false),
      generatePreview.fromMaybe(false) ? WrapMode::kWithPreview
                                       : WrapMode::kNoPreview,
      result, exceptionDetails);
  if (!response.IsSuccess()) return response;
  if (exceptionDetails->isJust() || accessorPropertiesOnly.fromMaybe(false))
    return Response::Success();
  std::unique_ptr<protocol::Array<InternalPropertyDescriptor>>
      internalPropertiesProtocolArray;
  std::unique_ptr<protocol::Array<PrivatePropertyDescriptor>>
      privatePropertiesProtocolArray;
  response = scope.injectedScript()->getInternalAndPrivateProperties(
      object, scope.objectGroupName(), &internalPropertiesProtocolArray,
      &privatePropertiesProtocolArray);
  if (!response.IsSuccess()) return response;
  if (!internalPropertiesProtocolArray->empty())
    *internalProperties = std::move(internalPropertiesProtocolArray);
  if (!privatePropertiesProtocolArray->empty())
    *privateProperties = std::move(privatePropertiesProtocolArray);
  return Response::Success();
}

Response V8RuntimeAgentImpl::releaseObject(const String16& objectId) {
  InjectedScript::ObjectScope scope(m_session, objectId);
  Response response = scope.initialize();
  if (!response.IsSuccess()) return response;
  scope.injectedScript()->releaseObject(objectId);
  return Response::Success();
}

Response V8RuntimeAgentImpl::releaseObjectGroup(const String16& objectGroup) {
  m_session->releaseObjectGroup(objectGroup);
  return Response::Success();
}

Response V8RuntimeAgentImpl::runIfWaitingForDebugger() {
  m_inspector->client()->runIfWaitingForDebugger(m_session->contextGroupId());
  return Response::Success();
}

Response V8RuntimeAgentImpl::setCustomObjectFormatterEnabled(bool enabled) {
  m_state->setBoolean(V8RuntimeAgentImplState::customObjectFormatterEnabled,
                      enabled);
  if (!m_enabled) return Response::ServerError("Runtime agent is not enabled");
  m_session->setCustomObjectFormatterEnabled(enabled);
  return Response::Success();
}

Response V8RuntimeAgentImpl::setMaxCallStackSizeToCapture(int size) {
  if (size < 0) {
    return Response::ServerError(
        "maxCallStackSizeToCapture should be non-negative");
  }
  V8StackTraceImpl::maxCallStackSizeToCapture = size;
  return Response::Success();
}

Response V8RuntimeAgentImpl::discardConsoleEntries() {
  V8ConsoleMessageStorage* storage =
      m_inspector->ensureConsoleMessageStorage(m_session->contextGroupId());
  storage->clear();
  return Response::Success();
}

Response V8RuntimeAgentImpl::compileScript(
    const String16& expression, const String16& sourceURL, bool persistScript,
    Maybe<int> executionContextId, Maybe<String16>* scriptId,
    Maybe<protocol::Runtime::ExceptionDetails>* exceptionDetails) {
  if (!m_enabled) return Response::ServerError("Runtime agent is not enabled");

  int contextId = 0;
  Response response = ensureContext(m_inspector, m_session->contextGroupId(),
                                    std::move(executionContextId),
                                    /*uniqueContextId*/ {}, &contextId);
  if (!response.IsSuccess()) return response;
  InjectedScript::ContextScope scope(m_session, contextId);
  response = scope.initialize();
  if (!response.IsSuccess()) return response;

  if (!persistScript) m_inspector->debugger()->muteScriptParsedEvents();
  v8::Local<v8::Script> script;
  bool isOk = m_inspector->compileScript(scope.context(), expression, sourceURL)
                  .ToLocal(&script);
  if (!persistScript) m_inspector->debugger()->unmuteScriptParsedEvents();
  if (!isOk) {
    if (scope.tryCatch().HasCaught()) {
      response = scope.injectedScript()->createExceptionDetails(
          scope.tryCatch(), String16(), exceptionDetails);
      if (!response.IsSuccess()) return response;
      return Response::Success();
    } else {
      return Response::ServerError("Script compilation failed");
    }
  }

  if (!persistScript) return Response::Success();

  String16 scriptValueId =
      String16::fromInteger(script->GetUnboundScript()->GetId());
  std::unique_ptr<v8::Global<v8::Script>> global(
      new v8::Global<v8::Script>(m_inspector->isolate(), script));
  m_compiledScripts[scriptValueId] = std::move(global);
  *scriptId = scriptValueId;
  return Response::Success();
}

void V8RuntimeAgentImpl::runScript(
    const String16& scriptId, Maybe<int> executionContextId,
    Maybe<String16> objectGroup, Maybe<bool> silent,
    Maybe<bool> includeCommandLineAPI, Maybe<bool> returnByValue,
    Maybe<bool> generatePreview, Maybe<bool> awaitPromise,
    std::unique_ptr<RunScriptCallback> callback) {
  if (!m_enabled) {
    callback->sendFailure(
        Response::ServerError("Runtime agent is not enabled"));
    return;
  }

  auto it = m_compiledScripts.find(scriptId);
  if (it == m_compiledScripts.end()) {
    callback->sendFailure(Response::ServerError("No script with given id"));
    return;
  }

  int contextId = 0;
  Response response = ensureContext(m_inspector, m_session->contextGroupId(),
                                    std::move(executionContextId),
                                    /*uniqueContextId*/ {}, &contextId);
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  InjectedScript::ContextScope scope(m_session, contextId);
  response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  if (silent.fromMaybe(false)) scope.ignoreExceptionsAndMuteConsole();

  std::unique_ptr<v8::Global<v8::Script>> scriptWrapper = std::move(it->second);
  m_compiledScripts.erase(it);
  v8::Local<v8::Script> script = scriptWrapper->Get(m_inspector->isolate());
  if (script.IsEmpty()) {
    callback->sendFailure(Response::ServerError("Script execution failed"));
    return;
  }

  if (includeCommandLineAPI.fromMaybe(false)) scope.installCommandLineAPI();

  v8::MaybeLocal<v8::Value> maybeResultValue;
  {
    v8::MicrotasksScope microtasksScope(m_inspector->isolate(),
                                        v8::MicrotasksScope::kRunMicrotasks);
    maybeResultValue = script->Run(scope.context());
  }

  // Re-initialize after running client's code, as it could have destroyed
  // context or session.
  response = scope.initialize();
  if (!response.IsSuccess()) {
    callback->sendFailure(response);
    return;
  }

  WrapMode mode = generatePreview.fromMaybe(false) ? WrapMode::kWithPreview
                                                   : WrapMode::kNoPreview;
  if (returnByValue.fromMaybe(false)) mode = WrapMode::kForceValue;
  if (!awaitPromise.fromMaybe(false) || scope.tryCatch().HasCaught()) {
    wrapEvaluateResultAsync(scope.injectedScript(), maybeResultValue,
                            scope.tryCatch(), objectGroup.fromMaybe(""), mode,
                            callback.get());
    return;
  }
  scope.injectedScript()->addPromiseCallback(
      m_session, maybeResultValue.ToLocalChecked(), objectGroup.fromMaybe(""),
      mode, false /* replMode */,
      EvaluateCallbackWrapper<RunScriptCallback>::wrap(std::move(callback)));
}

Response V8RuntimeAgentImpl::queryObjects(
    const String16& prototypeObjectId, Maybe<String16> objectGroup,
    std::unique_ptr<protocol::Runtime::RemoteObject>* objects) {
  InjectedScript::ObjectScope scope(m_session, prototypeObjectId);
  Response response = scope.initialize();
  if (!response.IsSuccess()) return response;
  if (!scope.object()->IsObject()) {
    return Response::ServerError("Prototype should be instance of Object");
  }
  v8::Local<v8::Array> resultArray = m_inspector->debugger()->queryObjects(
      scope.context(), scope.object().As<v8::Object>());
  return scope.injectedScript()->wrapObject(
      resultArray, objectGroup.fromMaybe(scope.objectGroupName()),
      WrapMode::kNoPreview, objects);
}

Response V8RuntimeAgentImpl::globalLexicalScopeNames(
    Maybe<int> executionContextId,
    std::unique_ptr<protocol::Array<String16>>* outNames) {
  int contextId = 0;
  Response response = ensureContext(m_inspector, m_session->contextGroupId(),
                                    std::move(executionContextId),
                                    /*uniqueContextId*/ {}, &contextId);
  if (!response.IsSuccess()) return response;

  InjectedScript::ContextScope scope(m_session, contextId);
  response = scope.initialize();
  if (!response.IsSuccess()) return response;

  v8::PersistentValueVector<v8::String> names(m_inspector->isolate());
  v8::debug::GlobalLexicalScopeNames(scope.context(), &names);
  *outNames = std::make_unique<protocol::Array<String16>>();
  for (size_t i = 0; i < names.Size(); ++i) {
    (*outNames)->emplace_back(
        toProtocolString(m_inspector->isolate(), names.Get(i)));
  }
  return Response::Success();
}

Response V8RuntimeAgentImpl::getIsolateId(String16* outIsolateId) {
  char buf[40];
  std::snprintf(buf, sizeof(buf), "%" PRIx64, m_inspector->isolateId());
  *outIsolateId = buf;
  return Response::Success();
}

Response V8RuntimeAgentImpl::getHeapUsage(double* out_usedSize,
                                          double* out_totalSize) {
  v8::HeapStatistics stats;
  m_inspector->isolate()->GetHeapStatistics(&stats);
  *out_usedSize = stats.used_heap_size();
  *out_totalSize = stats.total_heap_size();
  return Response::Success();
}

void V8RuntimeAgentImpl::terminateExecution(
    std::unique_ptr<TerminateExecutionCallback> callback) {
  m_inspector->debugger()->terminateExecution(std::move(callback));
}

namespace {
protocol::DictionaryValue* getOrCreateDictionary(
    protocol::DictionaryValue* dict, const String16& key) {
  if (protocol::DictionaryValue* bindings = dict->getObject(key))
    return bindings;
  dict->setObject(key, protocol::DictionaryValue::create());
  return dict->getObject(key);
}
}  // namespace

Response V8RuntimeAgentImpl::addBinding(const String16& name,
                                        Maybe<int> executionContextId,
                                        Maybe<String16> executionContextName) {
  if (executionContextId.isJust()) {
    if (executionContextName.isJust()) {
      return Response::InvalidParams(
          "executionContextName is mutually exclusive with executionContextId");
    }
    int contextId = executionContextId.fromJust();
    InspectedContext* context =
        m_inspector->getContext(m_session->contextGroupId(), contextId);
    if (!context) {
      return Response::InvalidParams(
          "Cannot find execution context with given executionContextId");
    }
    addBinding(context, name);
    return Response::Success();
  }

  // If it's a globally exposed binding, i.e. no context name specified, use
  // a special value for the context name.
  String16 contextKey = V8RuntimeAgentImplState::globalBindingsKey;
  if (executionContextName.isJust()) {
    contextKey = executionContextName.fromJust();
    if (contextKey == V8RuntimeAgentImplState::globalBindingsKey) {
      return Response::InvalidParams("Invalid executionContextName");
    }
  }
  // Only persist non context-specific bindings, as contextIds don't make
  // any sense when state is restored in a different process.
  protocol::DictionaryValue* bindings =
      getOrCreateDictionary(m_state, V8RuntimeAgentImplState::bindings);
  protocol::DictionaryValue* contextBindings =
      getOrCreateDictionary(bindings, contextKey);
  contextBindings->setBoolean(name, true);

  m_inspector->forEachContext(
      m_session->contextGroupId(),
      [&name, &executionContextName, this](InspectedContext* context) {
        if (executionContextName.isJust() &&
            executionContextName.fromJust() != context->humanReadableName())
          return;
        addBinding(context, name);
      });
  return Response::Success();
}

void V8RuntimeAgentImpl::bindingCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() != 1 || !info[0]->IsString()) {
    info.GetIsolate()->ThrowError(
        "Invalid arguments: should be exactly one string.");
    return;
  }
  V8InspectorImpl* inspector =
      static_cast<V8InspectorImpl*>(v8::debug::GetInspector(isolate));
  int contextId = InspectedContext::contextId(isolate->GetCurrentContext());
  int contextGroupId = inspector->contextGroupId(contextId);

  String16 name = toProtocolString(isolate, info.Data().As<v8::String>());
  String16 payload = toProtocolString(isolate, info[0].As<v8::String>());

  inspector->forEachSession(
      contextGroupId,
      [&name, &payload, &contextId](V8InspectorSessionImpl* session) {
        session->runtimeAgent()->bindingCalled(name, payload, contextId);
      });
}

void V8RuntimeAgentImpl::addBinding(InspectedContext* context,
                                    const String16& name) {
  auto it = m_activeBindings.find(name);
  if (it != m_activeBindings.end() && it->second.count(context->contextId())) {
    return;
  }
  v8::HandleScope handles(m_inspector->isolate());
  v8::Local<v8::Context> localContext = context->context();
  v8::Local<v8::Object> global = localContext->Global();
  v8::Local<v8::String> v8Name = toV8String(m_inspector->isolate(), name);
  v8::Local<v8::Value> functionValue;
  v8::MicrotasksScope microtasks(m_inspector->isolate(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  if (v8::Function::New(localContext, bindingCallback, v8Name)
          .ToLocal(&functionValue)) {
    v8::Maybe<bool> success = global->Set(localContext, v8Name, functionValue);
    USE(success);
    if (it == m_activeBindings.end()) {
      m_activeBindings.emplace(name,
                               std::unordered_set<int>(context->contextId()));
    } else {
      m_activeBindings.at(name).insert(context->contextId());
    }
  }
}

Response V8RuntimeAgentImpl::removeBinding(const String16& name) {
  protocol::DictionaryValue* bindings =
      m_state->getObject(V8RuntimeAgentImplState::bindings);
  if (bindings) bindings->remove(name);
  m_activeBindings.erase(name);
  return Response::Success();
}

void V8RuntimeAgentImpl::bindingCalled(const String16& name,
                                       const String16& payload,
                                       int executionContextId) {
  if (!m_activeBindings.count(name)) return;
  m_frontend.bindingCalled(name, payload, executionContextId);
}

void V8RuntimeAgentImpl::addBindings(InspectedContext* context) {
  const String16 contextName = context->humanReadableName();
  if (!m_enabled) return;
  protocol::DictionaryValue* bindings =
      m_state->getObject(V8RuntimeAgentImplState::bindings);
  if (!bindings) return;
  protocol::DictionaryValue* globalBindings =
      bindings->getObject(V8RuntimeAgentImplState::globalBindingsKey);
  if (globalBindings) {
    for (size_t i = 0; i < globalBindings->size(); ++i)
      addBinding(context, globalBindings->at(i).first);
  }
  protocol::DictionaryValue* contextBindings =
      contextName.isEmpty() ? nullptr : bindings->getObject(contextName);
  if (contextBindings) {
    for (size_t i = 0; i < contextBindings->size(); ++i)
      addBinding(context, contextBindings->at(i).first);
  }
}

void V8RuntimeAgentImpl::restore() {
  if (!m_state->booleanProperty(V8RuntimeAgentImplState::runtimeEnabled, false))
    return;
  m_frontend.executionContextsCleared();
  enable();
  if (m_state->booleanProperty(
          V8RuntimeAgentImplState::customObjectFormatterEnabled, false))
    m_session->setCustomObjectFormatterEnabled(true);

  m_inspector->forEachContext(
      m_session->contextGroupId(),
      [this](InspectedContext* context) { addBindings(context); });
}

Response V8RuntimeAgentImpl::enable() {
  if (m_enabled) return Response::Success();
  m_inspector->client()->beginEnsureAllContextsInGroup(
      m_session->contextGroupId());
  m_enabled = true;
  m_state->setBoolean(V8RuntimeAgentImplState::runtimeEnabled, true);
  m_inspector->enableStackCapturingIfNeeded();
  m_session->reportAllContexts(this);
  V8ConsoleMessageStorage* storage =
      m_inspector->ensureConsoleMessageStorage(m_session->contextGroupId());
  for (const auto& message : storage->messages()) {
    if (!reportMessage(message.get(), false)) break;
  }
  return Response::Success();
}

Response V8RuntimeAgentImpl::disable() {
  if (!m_enabled) return Response::Success();
  m_enabled = false;
  m_state->setBoolean(V8RuntimeAgentImplState::runtimeEnabled, false);
  m_state->remove(V8RuntimeAgentImplState::bindings);
  m_inspector->disableStackCapturingIfNeeded();
  m_session->setCustomObjectFormatterEnabled(false);
  reset();
  m_inspector->client()->endEnsureAllContextsInGroup(
      m_session->contextGroupId());
  if (m_session->debuggerAgent() && !m_session->debuggerAgent()->enabled()) {
    m_session->debuggerAgent()->setAsyncCallStackDepth(0);
  }
  return Response::Success();
}

void V8RuntimeAgentImpl::reset() {
  m_compiledScripts.clear();
  if (m_enabled) {
    int sessionId = m_session->sessionId();
    m_inspector->forEachContext(m_session->contextGroupId(),
                                [&sessionId](InspectedContext* context) {
                                  context->setReported(sessionId, false);
                                });
    m_frontend.executionContextsCleared();
  }
}

void V8RuntimeAgentImpl::reportExecutionContextCreated(
    InspectedContext* context) {
  if (!m_enabled) return;
  context->setReported(m_session->sessionId(), true);
  std::unique_ptr<protocol::Runtime::ExecutionContextDescription> description =
      protocol::Runtime::ExecutionContextDescription::create()
          .setId(context->contextId())
          .setName(context->humanReadableName())
          .setOrigin(context->origin())
          .setUniqueId(context->uniqueId().toString())
          .build();
  const String16& aux = context->auxData();
  if (!aux.isEmpty()) {
    std::vector<uint8_t> cbor;
    v8_crdtp::json::ConvertJSONToCBOR(
        v8_crdtp::span<uint16_t>(aux.characters16(), aux.length()), &cbor);
    description->setAuxData(protocol::DictionaryValue::cast(
        protocol::Value::parseBinary(cbor.data(), cbor.size())));
  }
  m_frontend.executionContextCreated(std::move(description));
}

void V8RuntimeAgentImpl::reportExecutionContextDestroyed(
    InspectedContext* context) {
  if (m_enabled && context->isReported(m_session->sessionId())) {
    context->setReported(m_session->sessionId(), false);
    m_frontend.executionContextDestroyed(context->contextId());
  }
}

void V8RuntimeAgentImpl::inspect(
    std::unique_ptr<protocol::Runtime::RemoteObject> objectToInspect,
    std::unique_ptr<protocol::DictionaryValue> hints) {
  if (m_enabled)
    m_frontend.inspectRequested(std::move(objectToInspect), std::move(hints));
}

void V8RuntimeAgentImpl::messageAdded(V8ConsoleMessage* message) {
  if (m_enabled) reportMessage(message, true);
}

bool V8RuntimeAgentImpl::reportMessage(V8ConsoleMessage* message,
                                       bool generatePreview) {
  message->reportToFrontend(&m_frontend, m_session, generatePreview);
  m_frontend.flush();
  return m_inspector->hasConsoleMessageStorage(m_session->contextGroupId());
}
}  // namespace v8_inspector
