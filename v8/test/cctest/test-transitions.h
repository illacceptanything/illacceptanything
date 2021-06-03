// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_TEST_CCTEST_TEST_TRANSITIONS_H_
#define V8_TEST_CCTEST_TEST_TRANSITIONS_H_

#include "src/objects/transitions.h"

namespace v8 {
namespace internal {

class TestTransitionsAccessor : public TransitionsAccessor {
 public:
  TestTransitionsAccessor(Isolate* isolate, Map map,
                          DisallowGarbageCollection* no_gc)
      : TransitionsAccessor(isolate, map, no_gc) {}
  TestTransitionsAccessor(Isolate* isolate, Handle<Map> map)
      : TransitionsAccessor(isolate, map) {}

  // Expose internals for tests.
  bool IsUninitializedEncoding() { return encoding() == kUninitialized; }
  bool IsWeakRefEncoding() { return encoding() == kWeakRef; }

  bool IsFullTransitionArrayEncoding() {
    return encoding() == kFullTransitionArray;
  }

  int Capacity() { return TransitionsAccessor::Capacity(); }

  TransitionArray transitions() { return TransitionsAccessor::transitions(); }
};

}  // namespace internal
}  // namespace v8

#endif  // V8_TEST_CCTEST_TEST_TRANSITIONS_H_
