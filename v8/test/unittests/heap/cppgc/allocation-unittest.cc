// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/cppgc/allocation.h"

#include "include/cppgc/visitor.h"
#include "src/heap/cppgc/heap-object-header.h"
#include "test/unittests/heap/cppgc/tests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cppgc {
namespace internal {

namespace {

class CppgcAllocationTest : public testing::TestWithHeap {};

struct GCed final : GarbageCollected<GCed> {
  void Trace(cppgc::Visitor*) const {}
};

class HeapAllocatedArray final : public GarbageCollected<HeapAllocatedArray> {
 public:
  HeapAllocatedArray() {
    for (int i = 0; i < kArraySize; ++i) {
      array_[i] = i % 128;
    }
  }

  int8_t at(size_t i) { return array_[i]; }
  void Trace(Visitor* visitor) const {}

 private:
  static const int kArraySize = 1000;
  int8_t array_[kArraySize];
};

}  // namespace

TEST_F(CppgcAllocationTest, MakeGarbageCollectedPreservesPayload) {
  // Allocate an object in the heap.
  HeapAllocatedArray* array =
      MakeGarbageCollected<HeapAllocatedArray>(GetAllocationHandle());

  // Sanity check of the contents in the heap.
  EXPECT_EQ(0, array->at(0));
  EXPECT_EQ(42, array->at(42));
  EXPECT_EQ(0, array->at(128));
  EXPECT_EQ(999 % 128, array->at(999));
}

TEST_F(CppgcAllocationTest, ReuseMemoryFromFreelist) {
  // Allocate 3 objects so that the address we look for below is not at the
  // start of the page.
  MakeGarbageCollected<GCed>(GetAllocationHandle());
  MakeGarbageCollected<GCed>(GetAllocationHandle());
  GCed* p1 = MakeGarbageCollected<GCed>(GetAllocationHandle());
  // GC reclaims all objects. LABs are reset during the GC.
  PreciseGC();
  // Now the freed memory in the first GC should be reused. Allocating 3
  // objects again should suffice but allocating 5 to give the test some slack.
  bool reused_memory_found = false;
  for (int i = 0; i < 5; i++) {
    GCed* p2 = MakeGarbageCollected<GCed>(GetAllocationHandle());
    if (p1 == p2) {
      reused_memory_found = true;
      break;
    }
  }
  EXPECT_TRUE(reused_memory_found);
}

namespace {
class CallbackInCtor final : public GarbageCollected<CallbackInCtor> {
 public:
  template <typename Callback>
  explicit CallbackInCtor(Callback callback) {
    callback();
  }

  void Trace(Visitor*) const {}
};
}  // namespace

TEST_F(CppgcAllocationTest,
       ConservativeGCDuringAllocationDoesNotReclaimObject) {
  CallbackInCtor* obj = MakeGarbageCollected<CallbackInCtor>(
      GetAllocationHandle(), [this]() { ConservativeGC(); });
  EXPECT_FALSE(HeapObjectHeader::FromObject(obj).IsFree());
}

namespace {
class LargeObject : public GarbageCollected<LargeObject> {
 public:
  static constexpr size_t kDataSize = kLargeObjectSizeThreshold + 1;
  static size_t destructor_calls;

  explicit LargeObject(bool check) {
    if (!check) return;
    for (size_t i = 0; i < LargeObject::kDataSize; ++i) {
      EXPECT_EQ(0, data[i]);
    }
  }
  ~LargeObject() { ++destructor_calls; }
  void Trace(Visitor*) const {}

  char data[kDataSize];
};
size_t LargeObject::destructor_calls = 0u;
}  // namespace

TEST_F(CppgcAllocationTest, LargePagesAreZeroedOut) {
  static constexpr size_t kNumObjects = 1u;
  LargeObject::destructor_calls = 0u;
  std::vector<void*> pages;
  for (size_t i = 0; i < kNumObjects; ++i) {
    auto* obj = MakeGarbageCollected<LargeObject>(GetAllocationHandle(), false);
    pages.push_back(obj);
    memset(obj->data, 0xff, LargeObject::kDataSize);
  }
  PreciseGC();
  EXPECT_EQ(kNumObjects, LargeObject::destructor_calls);
  bool reused_page = false;
  for (size_t i = 0; i < kNumObjects; ++i) {
    auto* obj = MakeGarbageCollected<LargeObject>(GetAllocationHandle(), true);
    if (std::find(pages.begin(), pages.end(), obj) != pages.end())
      reused_page = true;
  }
  EXPECT_TRUE(reused_page);
}

}  // namespace internal
}  // namespace cppgc
