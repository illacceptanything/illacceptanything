// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_IC_CALL_OPTIMIZATION_H_
#define V8_IC_CALL_OPTIMIZATION_H_

#include "src/api/api-arguments.h"
#include "src/objects/objects.h"

namespace v8 {
namespace internal {

// Holds information about possible function call optimizations.
class CallOptimization {
 public:
  template <class IsolateT>
  CallOptimization(IsolateT* isolate, Handle<Object> function);

  Context GetAccessorContext(Map holder_map) const;
  bool IsCrossContextLazyAccessorPair(Context native_context,
                                      Map holder_map) const;

  bool is_constant_call() const { return !constant_function_.is_null(); }
  bool accept_any_receiver() const { return accept_any_receiver_; }
  bool requires_signature_check() const {
    return !expected_receiver_type_.is_null();
  }

  Handle<JSFunction> constant_function() const {
    DCHECK(is_constant_call());
    return constant_function_;
  }

  bool is_simple_api_call() const { return is_simple_api_call_; }

  Handle<FunctionTemplateInfo> expected_receiver_type() const {
    DCHECK(is_simple_api_call());
    return expected_receiver_type_;
  }

  Handle<CallHandlerInfo> api_call_info() const {
    DCHECK(is_simple_api_call());
    return api_call_info_;
  }

  enum HolderLookup { kHolderNotFound, kHolderIsReceiver, kHolderFound };

  template <class IsolateT>
  Handle<JSObject> LookupHolderOfExpectedType(
      IsolateT* isolate, Handle<Map> receiver_map,
      HolderLookup* holder_lookup) const;

  bool IsCompatibleReceiverMap(Handle<JSObject> api_holder,
                               Handle<JSObject> holder, HolderLookup) const;

 private:
  template <class IsolateT>
  void Initialize(IsolateT* isolate, Handle<JSFunction> function);
  template <class IsolateT>
  void Initialize(IsolateT* isolate,
                  Handle<FunctionTemplateInfo> function_template_info);

  // Determines whether the given function can be called using the
  // fast api call builtin.
  template <class IsolateT>
  void AnalyzePossibleApiFunction(IsolateT* isolate,
                                  Handle<JSFunction> function);

  Handle<JSFunction> constant_function_;
  Handle<FunctionTemplateInfo> expected_receiver_type_;
  Handle<CallHandlerInfo> api_call_info_;

  // TODO(gsathya): Change these to be a bitfield and do a single fast check
  // rather than two checks.
  bool is_simple_api_call_ = false;
  bool accept_any_receiver_ = false;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_IC_CALL_OPTIMIZATION_H_
