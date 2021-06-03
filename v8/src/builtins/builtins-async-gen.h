// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BUILTINS_BUILTINS_ASYNC_GEN_H_
#define V8_BUILTINS_BUILTINS_ASYNC_GEN_H_

#include "src/builtins/builtins-promise-gen.h"

namespace v8 {
namespace internal {

class AsyncBuiltinsAssembler : public PromiseBuiltinsAssembler {
 public:
  explicit AsyncBuiltinsAssembler(compiler::CodeAssemblerState* state)
      : PromiseBuiltinsAssembler(state) {}

 protected:
  // Perform steps to resume generator after `value` is resolved.
  // `on_reject` is the SharedFunctioninfo instance used to create the reject
  // closure. `on_resolve` is the SharedFunctioninfo instance used to create the
  // resolve closure. Returns the Promise-wrapped `value`.
  TNode<Object> Await(TNode<Context> context,
                      TNode<JSGeneratorObject> generator, TNode<Object> value,
                      TNode<JSPromise> outer_promise,
                      TNode<SharedFunctionInfo> on_resolve_sfi,
                      TNode<SharedFunctionInfo> on_reject_sfi,
                      TNode<Oddball> is_predicted_as_caught);
  TNode<Object> Await(TNode<Context> context,
                      TNode<JSGeneratorObject> generator, TNode<Object> value,
                      TNode<JSPromise> outer_promise,
                      TNode<SharedFunctionInfo> on_resolve_sfi,
                      TNode<SharedFunctionInfo> on_reject_sfi,
                      bool is_predicted_as_caught) {
    return Await(context, generator, value, outer_promise, on_resolve_sfi,
                 on_reject_sfi, BooleanConstant(is_predicted_as_caught));
  }

  // Return a new built-in function object as defined in
  // Async Iterator Value Unwrap Functions
  TNode<JSFunction> CreateUnwrapClosure(TNode<NativeContext> native_context,
                                        TNode<Oddball> done);

 private:
  void InitializeNativeClosure(TNode<Context> context,
                               TNode<NativeContext> native_context,
                               TNode<HeapObject> function,
                               TNode<SharedFunctionInfo> shared_info);
  TNode<Context> AllocateAsyncIteratorValueUnwrapContext(
      TNode<NativeContext> native_context, TNode<Oddball> done);

  TNode<Object> AwaitOld(TNode<Context> context,
                         TNode<JSGeneratorObject> generator,
                         TNode<Object> value, TNode<JSPromise> outer_promise,
                         TNode<SharedFunctionInfo> on_resolve_sfi,
                         TNode<SharedFunctionInfo> on_reject_sfi,
                         TNode<Oddball> is_predicted_as_caught);
  TNode<Object> AwaitOptimized(TNode<Context> context,
                               TNode<JSGeneratorObject> generator,
                               TNode<JSPromise> promise,
                               TNode<JSPromise> outer_promise,
                               TNode<SharedFunctionInfo> on_resolve_sfi,
                               TNode<SharedFunctionInfo> on_reject_sfi,
                               TNode<Oddball> is_predicted_as_caught);

  void InitAwaitPromise(
      Runtime::FunctionId id, TNode<Context> context, TNode<Object> value,
      TNode<Object> promise, TNode<Object> outer_promise,
      TNode<HeapObject> on_reject, TNode<Oddball> is_predicted_as_caught,
      TVariable<HeapObject>* var_throwaway);
};

}  // namespace internal
}  // namespace v8

#endif  // V8_BUILTINS_BUILTINS_ASYNC_GEN_H_
