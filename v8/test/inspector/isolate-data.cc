// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test/inspector/isolate-data.h"

#include "src/inspector/test-interface.h"
#include "src/utils/vector.h"
#include "test/inspector/task-runner.h"
#include "test/inspector/utils.h"

namespace v8 {
namespace internal {

namespace {

const int kIsolateDataIndex = 2;
const int kContextGroupIdIndex = 3;

void Print(v8::Isolate* isolate, const v8_inspector::StringView& string) {
  v8::Local<v8::String> v8_string = ToV8String(isolate, string);
  v8::String::Utf8Value utf8_string(isolate, v8_string);
  fwrite(*utf8_string, sizeof(**utf8_string), utf8_string.length(), stdout);
}

class Inspectable : public v8_inspector::V8InspectorSession::Inspectable {
 public:
  Inspectable(v8::Isolate* isolate, v8::Local<v8::Value> object)
      : object_(isolate, object) {}
  ~Inspectable() override = default;
  v8::Local<v8::Value> get(v8::Local<v8::Context> context) override {
    return object_.Get(context->GetIsolate());
  }

 private:
  v8::Global<v8::Value> object_;
};

}  //  namespace

IsolateData::IsolateData(TaskRunner* task_runner,
                         IsolateData::SetupGlobalTasks setup_global_tasks,
                         v8::StartupData* startup_data,
                         WithInspector with_inspector)
    : task_runner_(task_runner),
      setup_global_tasks_(std::move(setup_global_tasks)) {
  v8::Isolate::CreateParams params;
  array_buffer_allocator_.reset(
      v8::ArrayBuffer::Allocator::NewDefaultAllocator());
  params.array_buffer_allocator = array_buffer_allocator_.get();
  params.snapshot_blob = startup_data;
  params.only_terminate_in_safe_scope = true;
  isolate_.reset(v8::Isolate::New(params));
  isolate_->SetMicrotasksPolicy(v8::MicrotasksPolicy::kScoped);
  if (with_inspector) {
    isolate_->AddMessageListener(&IsolateData::MessageHandler);
    isolate_->SetPromiseRejectCallback(&IsolateData::PromiseRejectHandler);
    inspector_ = v8_inspector::V8Inspector::create(isolate_.get(), this);
  }
  v8::HandleScope handle_scope(isolate_.get());
  not_inspectable_private_.Reset(
      isolate_.get(),
      v8::Private::ForApi(
          isolate_.get(),
          v8::String::NewFromUtf8Literal(isolate_.get(), "notInspectable")));
}

IsolateData* IsolateData::FromContext(v8::Local<v8::Context> context) {
  return static_cast<IsolateData*>(
      context->GetAlignedPointerFromEmbedderData(kIsolateDataIndex));
}

int IsolateData::CreateContextGroup() {
  int context_group_id = ++last_context_group_id_;
  if (!CreateContext(context_group_id, v8_inspector::StringView())) {
    DCHECK(isolate_->IsExecutionTerminating());
    return -1;
  }
  return context_group_id;
}

bool IsolateData::CreateContext(int context_group_id,
                                v8_inspector::StringView name) {
  v8::HandleScope handle_scope(isolate_.get());
  v8::Local<v8::ObjectTemplate> global_template =
      v8::ObjectTemplate::New(isolate_.get());
  for (auto it = setup_global_tasks_.begin(); it != setup_global_tasks_.end();
       ++it) {
    (*it)->Run(isolate_.get(), global_template);
  }
  v8::Local<v8::Context> context =
      v8::Context::New(isolate_.get(), nullptr, global_template);
  if (context.IsEmpty()) return false;
  context->SetAlignedPointerInEmbedderData(kIsolateDataIndex, this);
  // Should be 2-byte aligned.
  context->SetAlignedPointerInEmbedderData(
      kContextGroupIdIndex, reinterpret_cast<void*>(context_group_id * 2));
  contexts_[context_group_id].emplace_back(isolate_.get(), context);
  if (inspector_) FireContextCreated(context, context_group_id, name);
  return true;
}

v8::Local<v8::Context> IsolateData::GetDefaultContext(int context_group_id) {
  return contexts_[context_group_id].begin()->Get(isolate_.get());
}

void IsolateData::ResetContextGroup(int context_group_id) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->resetContextGroup(context_group_id);
}

int IsolateData::GetContextGroupId(v8::Local<v8::Context> context) {
  return static_cast<int>(
      reinterpret_cast<intptr_t>(
          context->GetAlignedPointerFromEmbedderData(kContextGroupIdIndex)) /
      2);
}

void IsolateData::RegisterModule(v8::Local<v8::Context> context,
                                 std::vector<uint16_t> name,
                                 v8::ScriptCompiler::Source* source) {
  v8::Local<v8::Module> module;
  if (!v8::ScriptCompiler::CompileModule(isolate(), source).ToLocal(&module))
    return;
  if (!module->InstantiateModule(context, &IsolateData::ModuleResolveCallback)
           .FromMaybe(false)) {
    return;
  }
  v8::Local<v8::Value> result;
  if (!module->Evaluate(context).ToLocal(&result)) return;
  modules_[name] = v8::Global<v8::Module>(isolate_.get(), module);
}

// static
v8::MaybeLocal<v8::Module> IsolateData::ModuleResolveCallback(
    v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
    v8::Local<v8::FixedArray> import_assertions,
    v8::Local<v8::Module> referrer) {
  // TODO(v8:11189) Consider JSON modules support in the InspectorClient
  IsolateData* data = IsolateData::FromContext(context);
  std::string str = *v8::String::Utf8Value(data->isolate(), specifier);
  v8::MaybeLocal<v8::Module> maybe_module =
      data->modules_[ToVector(data->isolate(), specifier)].Get(data->isolate());
  if (maybe_module.IsEmpty()) {
    data->isolate()->ThrowError(v8::String::Concat(
        data->isolate(),
        ToV8String(data->isolate(), "Failed to resolve module: "), specifier));
  }
  return maybe_module;
}

int IsolateData::ConnectSession(int context_group_id,
                                const v8_inspector::StringView& state,
                                v8_inspector::V8Inspector::Channel* channel) {
  v8::SealHandleScope seal_handle_scope(isolate());
  int session_id = ++last_session_id_;
  sessions_[session_id] = inspector_->connect(context_group_id, channel, state);
  context_group_by_session_[sessions_[session_id].get()] = context_group_id;
  return session_id;
}

std::vector<uint8_t> IsolateData::DisconnectSession(int session_id) {
  v8::SealHandleScope seal_handle_scope(isolate());
  auto it = sessions_.find(session_id);
  CHECK(it != sessions_.end());
  context_group_by_session_.erase(it->second.get());
  std::vector<uint8_t> result = it->second->state();
  sessions_.erase(it);
  return result;
}

void IsolateData::SendMessage(int session_id,
                              const v8_inspector::StringView& message) {
  v8::SealHandleScope seal_handle_scope(isolate());
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) it->second->dispatchProtocolMessage(message);
}

void IsolateData::BreakProgram(int context_group_id,
                               const v8_inspector::StringView& reason,
                               const v8_inspector::StringView& details) {
  v8::SealHandleScope seal_handle_scope(isolate());
  for (int session_id : GetSessionIds(context_group_id)) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) it->second->breakProgram(reason, details);
  }
}

void IsolateData::SchedulePauseOnNextStatement(
    int context_group_id, const v8_inspector::StringView& reason,
    const v8_inspector::StringView& details) {
  v8::SealHandleScope seal_handle_scope(isolate());
  for (int session_id : GetSessionIds(context_group_id)) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end())
      it->second->schedulePauseOnNextStatement(reason, details);
  }
}

void IsolateData::CancelPauseOnNextStatement(int context_group_id) {
  v8::SealHandleScope seal_handle_scope(isolate());
  for (int session_id : GetSessionIds(context_group_id)) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) it->second->cancelPauseOnNextStatement();
  }
}

void IsolateData::AsyncTaskScheduled(const v8_inspector::StringView& name,
                                     void* task, bool recurring) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->asyncTaskScheduled(name, task, recurring);
}

void IsolateData::AsyncTaskStarted(void* task) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->asyncTaskStarted(task);
}

void IsolateData::AsyncTaskFinished(void* task) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->asyncTaskFinished(task);
}

v8_inspector::V8StackTraceId IsolateData::StoreCurrentStackTrace(
    const v8_inspector::StringView& description) {
  v8::SealHandleScope seal_handle_scope(isolate());
  return inspector_->storeCurrentStackTrace(description);
}

void IsolateData::ExternalAsyncTaskStarted(
    const v8_inspector::V8StackTraceId& parent) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->externalAsyncTaskStarted(parent);
}

void IsolateData::ExternalAsyncTaskFinished(
    const v8_inspector::V8StackTraceId& parent) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->externalAsyncTaskFinished(parent);
}

void IsolateData::AddInspectedObject(int session_id,
                                     v8::Local<v8::Value> object) {
  v8::SealHandleScope seal_handle_scope(isolate());
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) return;
  std::unique_ptr<Inspectable> inspectable(
      new Inspectable(isolate_.get(), object));
  it->second->addInspectedObject(std::move(inspectable));
}

void IsolateData::SetMaxAsyncTaskStacksForTest(int limit) {
  v8::SealHandleScope seal_handle_scope(isolate());
  v8_inspector::SetMaxAsyncTaskStacksForTest(inspector_.get(), limit);
}

void IsolateData::DumpAsyncTaskStacksStateForTest() {
  v8::SealHandleScope seal_handle_scope(isolate());
  v8_inspector::DumpAsyncTaskStacksStateForTest(inspector_.get());
}

// static
int IsolateData::HandleMessage(v8::Local<v8::Message> message,
                               v8::Local<v8::Value> exception) {
  v8::Isolate* isolate = message->GetIsolate();
  v8::Local<v8::Context> context = isolate->GetEnteredOrMicrotaskContext();
  if (context.IsEmpty()) return 0;
  v8_inspector::V8Inspector* inspector =
      IsolateData::FromContext(context)->inspector_.get();

  v8::Local<v8::StackTrace> stack = message->GetStackTrace();
  int script_id = message->GetScriptOrigin().ScriptId();
  if (!stack.IsEmpty() && stack->GetFrameCount() > 0) {
    int top_script_id = stack->GetFrame(isolate, 0)->GetScriptId();
    if (top_script_id == script_id) script_id = 0;
  }
  int line_number = message->GetLineNumber(context).FromMaybe(0);
  int column_number = 0;
  if (message->GetStartColumn(context).IsJust())
    column_number = message->GetStartColumn(context).FromJust() + 1;

  v8_inspector::StringView detailed_message;
  std::vector<uint16_t> message_text_string = ToVector(isolate, message->Get());
  v8_inspector::StringView message_text(message_text_string.data(),
                                        message_text_string.size());
  std::vector<uint16_t> url_string;
  if (message->GetScriptOrigin().ResourceName()->IsString()) {
    url_string = ToVector(
        isolate, message->GetScriptOrigin().ResourceName().As<v8::String>());
  }
  v8_inspector::StringView url(url_string.data(), url_string.size());

  v8::SealHandleScope seal_handle_scope(isolate);
  return inspector->exceptionThrown(
      context, message_text, exception, detailed_message, url, line_number,
      column_number, inspector->createStackTrace(stack), script_id);
}

// static
void IsolateData::MessageHandler(v8::Local<v8::Message> message,
                                 v8::Local<v8::Value> exception) {
  HandleMessage(message, exception);
}

// static
void IsolateData::PromiseRejectHandler(v8::PromiseRejectMessage data) {
  v8::Isolate* isolate = data.GetPromise()->GetIsolate();
  v8::Local<v8::Context> context = isolate->GetEnteredOrMicrotaskContext();
  if (context.IsEmpty()) return;
  v8::Local<v8::Promise> promise = data.GetPromise();
  v8::Local<v8::Private> id_private = v8::Private::ForApi(
      isolate, v8::String::NewFromUtf8Literal(isolate, "id"));

  if (data.GetEvent() == v8::kPromiseHandlerAddedAfterReject) {
    v8::Local<v8::Value> id;
    if (!promise->GetPrivate(context, id_private).ToLocal(&id)) return;
    if (!id->IsInt32()) return;
    v8_inspector::V8Inspector* inspector =
        IsolateData::FromContext(context)->inspector_.get();
    v8::SealHandleScope seal_handle_scope(isolate);
    const char* reason_str = "Handler added to rejected promise";
    inspector->exceptionRevoked(
        context, id.As<v8::Int32>()->Value(),
        v8_inspector::StringView(reinterpret_cast<const uint8_t*>(reason_str),
                                 strlen(reason_str)));
    return;
  }

  v8::Local<v8::Value> exception = data.GetValue();
  int exception_id = HandleMessage(
      v8::Exception::CreateMessage(isolate, exception), exception);
  if (exception_id) {
    promise
        ->SetPrivate(isolate->GetCurrentContext(), id_private,
                     v8::Int32::New(isolate, exception_id))
        .ToChecked();
  }
}

void IsolateData::FireContextCreated(v8::Local<v8::Context> context,
                                     int context_group_id,
                                     v8_inspector::StringView name) {
  v8_inspector::V8ContextInfo info(context, context_group_id, name);
  info.hasMemoryOnConsole = true;
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->contextCreated(info);
}

void IsolateData::FireContextDestroyed(v8::Local<v8::Context> context) {
  v8::SealHandleScope seal_handle_scope(isolate());
  inspector_->contextDestroyed(context);
}

void IsolateData::FreeContext(v8::Local<v8::Context> context) {
  int context_group_id = GetContextGroupId(context);
  auto it = contexts_.find(context_group_id);
  if (it == contexts_.end()) return;
  contexts_.erase(it);
}

std::vector<int> IsolateData::GetSessionIds(int context_group_id) {
  std::vector<int> result;
  for (auto& it : sessions_) {
    if (context_group_by_session_[it.second.get()] == context_group_id)
      result.push_back(it.first);
  }
  return result;
}

bool IsolateData::formatAccessorsAsProperties(v8::Local<v8::Value> object) {
  v8::Local<v8::Context> context = isolate()->GetCurrentContext();
  v8::Local<v8::Private> shouldFormatAccessorsPrivate = v8::Private::ForApi(
      isolate(),
      v8::String::NewFromUtf8Literal(isolate(), "allowAccessorFormatting"));
  CHECK(object->IsObject());
  return object.As<v8::Object>()
      ->HasPrivate(context, shouldFormatAccessorsPrivate)
      .FromMaybe(false);
}

bool IsolateData::isInspectableHeapObject(v8::Local<v8::Object> object) {
  v8::Local<v8::Context> context = isolate()->GetCurrentContext();
  v8::MicrotasksScope microtasks_scope(
      isolate(), v8::MicrotasksScope::kDoNotRunMicrotasks);
  return !object->HasPrivate(context, not_inspectable_private_.Get(isolate()))
              .FromMaybe(false);
}

v8::Local<v8::Context> IsolateData::ensureDefaultContextInGroup(
    int context_group_id) {
  return GetDefaultContext(context_group_id);
}

void IsolateData::SetCurrentTimeMS(double time) {
  current_time_ = time;
  current_time_set_ = true;
}

double IsolateData::currentTimeMS() {
  if (current_time_set_) return current_time_;
  return V8::GetCurrentPlatform()->CurrentClockTimeMillis();
}

void IsolateData::SetMemoryInfo(v8::Local<v8::Value> memory_info) {
  memory_info_.Reset(isolate_.get(), memory_info);
}

void IsolateData::SetLogConsoleApiMessageCalls(bool log) {
  log_console_api_message_calls_ = log;
}

void IsolateData::SetLogMaxAsyncCallStackDepthChanged(bool log) {
  log_max_async_call_stack_depth_changed_ = log;
}

void IsolateData::SetAdditionalConsoleApi(v8_inspector::StringView api_script) {
  v8::HandleScope handle_scope(isolate());
  additional_console_api_.Reset(isolate(), ToV8String(isolate(), api_script));
}

v8::MaybeLocal<v8::Value> IsolateData::memoryInfo(v8::Isolate* isolate,
                                                  v8::Local<v8::Context>) {
  if (memory_info_.IsEmpty()) return v8::MaybeLocal<v8::Value>();
  return memory_info_.Get(isolate);
}

void IsolateData::runMessageLoopOnPause(int) {
  v8::SealHandleScope seal_handle_scope(isolate());
  task_runner_->RunMessageLoop(true);
}

void IsolateData::quitMessageLoopOnPause() {
  v8::SealHandleScope seal_handle_scope(isolate());
  task_runner_->QuitMessageLoop();
}

void IsolateData::installAdditionalCommandLineAPI(
    v8::Local<v8::Context> context, v8::Local<v8::Object> object) {
  if (additional_console_api_.IsEmpty()) return;
  CHECK(context->GetIsolate() == isolate());
  v8::HandleScope handle_scope(isolate());
  v8::Context::Scope context_scope(context);
  v8::ScriptOrigin origin(isolate(), v8::String::NewFromUtf8Literal(
                                         isolate(), "internal-console-api"));
  v8::ScriptCompiler::Source scriptSource(
      additional_console_api_.Get(isolate()), origin);
  v8::MaybeLocal<v8::Script> script =
      v8::ScriptCompiler::Compile(context, &scriptSource);
  CHECK(!script.ToLocalChecked()->Run(context).IsEmpty());
}

void IsolateData::consoleAPIMessage(int contextGroupId,
                                    v8::Isolate::MessageErrorLevel level,
                                    const v8_inspector::StringView& message,
                                    const v8_inspector::StringView& url,
                                    unsigned lineNumber, unsigned columnNumber,
                                    v8_inspector::V8StackTrace* stack) {
  if (!log_console_api_message_calls_) return;
  Print(isolate_.get(), message);
  fprintf(stdout, " (");
  Print(isolate_.get(), url);
  fprintf(stdout, ":%d:%d)", lineNumber, columnNumber);
  Print(isolate_.get(), stack->toString()->string());
  fprintf(stdout, "\n");
}

void IsolateData::maxAsyncCallStackDepthChanged(int depth) {
  if (!log_max_async_call_stack_depth_changed_) return;
  fprintf(stdout, "maxAsyncCallStackDepthChanged: %d\n", depth);
}

void IsolateData::SetResourceNamePrefix(v8::Local<v8::String> prefix) {
  resource_name_prefix_.Reset(isolate(), prefix);
}

namespace {
class StringBufferImpl : public v8_inspector::StringBuffer {
 public:
  StringBufferImpl(v8::Isolate* isolate, v8::Local<v8::String> string)
      : data_(ToVector(isolate, string)) {}

  v8_inspector::StringView string() const override {
    return v8_inspector::StringView(data_.data(), data_.size());
  }

 private:
  std::vector<uint16_t> data_;
};
}  // anonymous namespace

std::unique_ptr<v8_inspector::StringBuffer> IsolateData::resourceNameToUrl(
    const v8_inspector::StringView& resourceName) {
  if (resource_name_prefix_.IsEmpty()) return nullptr;
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::String> name = ToV8String(isolate(), resourceName);
  v8::Local<v8::String> prefix = resource_name_prefix_.Get(isolate());
  v8::Local<v8::String> url = v8::String::Concat(isolate(), prefix, name);
  return std::make_unique<StringBufferImpl>(isolate(), url);
}

int64_t IsolateData::generateUniqueId() {
  static int64_t last_unique_id = 0L;
  // Keep it not too random for tests.
  return ++last_unique_id;
}

}  // namespace internal
}  // namespace v8
