// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/heap/cppgc/marking-verifier.h"

#include "include/cppgc/allocation.h"
#include "include/cppgc/member.h"
#include "include/cppgc/persistent.h"
#include "include/cppgc/prefinalizer.h"
#include "src/heap/cppgc/heap-object-header.h"
#include "src/heap/cppgc/heap.h"
#include "test/unittests/heap/cppgc/tests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cppgc {
namespace internal {

namespace {

class MarkingVerifierTest : public testing::TestWithHeap {
 public:
  using StackState = Heap::Config::StackState;

  V8_NOINLINE void VerifyMarking(HeapBase& heap, StackState stack_state,
                                 size_t expected_marked_bytes) {
    Heap::From(GetHeap())->object_allocator().ResetLinearAllocationBuffers();
    MarkingVerifier verifier(heap);
    verifier.Run(stack_state, v8::base::Stack::GetCurrentStackPosition(),
                 expected_marked_bytes);
  }
};

class GCed : public GarbageCollected<GCed> {
 public:
  void SetChild(GCed* child) { child_ = child; }
  void SetWeakChild(GCed* child) { weak_child_ = child; }
  GCed* child() const { return child_.Get(); }
  GCed* weak_child() const { return weak_child_.Get(); }
  void Trace(cppgc::Visitor* visitor) const {
    visitor->Trace(child_);
    visitor->Trace(weak_child_);
  }

 private:
  Member<GCed> child_;
  WeakMember<GCed> weak_child_;
};

template <typename T>
V8_NOINLINE T access(volatile const T& t) {
  return t;
}

}  // namespace

// Following tests should not crash.

TEST_F(MarkingVerifierTest, DoesNotDieOnMarkedOnStackReference) {
  GCed* object = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& header = HeapObjectHeader::FromObject(object);
  ASSERT_TRUE(header.TryMarkAtomic());
  VerifyMarking(Heap::From(GetHeap())->AsBase(),
                StackState::kMayContainHeapPointers, header.AllocatedSize());
  access(object);
}

TEST_F(MarkingVerifierTest, DoesNotDieOnMarkedMember) {
  Persistent<GCed> parent = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& parent_header = HeapObjectHeader::FromObject(parent.Get());
  ASSERT_TRUE(parent_header.TryMarkAtomic());
  parent->SetChild(MakeGarbageCollected<GCed>(GetAllocationHandle()));
  auto& child_header = HeapObjectHeader::FromObject(parent->child());
  ASSERT_TRUE(child_header.TryMarkAtomic());
  VerifyMarking(Heap::From(GetHeap())->AsBase(), StackState::kNoHeapPointers,
                parent_header.AllocatedSize() + child_header.AllocatedSize());
}

TEST_F(MarkingVerifierTest, DoesNotDieOnMarkedWeakMember) {
  Persistent<GCed> parent = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& parent_header = HeapObjectHeader::FromObject(parent.Get());
  ASSERT_TRUE(parent_header.TryMarkAtomic());
  parent->SetWeakChild(MakeGarbageCollected<GCed>(GetAllocationHandle()));
  auto& child_header = HeapObjectHeader::FromObject(parent->weak_child());
  ASSERT_TRUE(child_header.TryMarkAtomic());
  VerifyMarking(Heap::From(GetHeap())->AsBase(), StackState::kNoHeapPointers,
                parent_header.AllocatedSize() + child_header.AllocatedSize());
}

namespace {

class GCedWithCallback : public GarbageCollected<GCedWithCallback> {
 public:
  template <typename Callback>
  explicit GCedWithCallback(Callback callback) {
    callback(this);
  }
  void Trace(cppgc::Visitor* visitor) const {}
};

}  // namespace

TEST_F(MarkingVerifierTest, DoesNotDieOnInConstructionOnObject) {
  MakeGarbageCollected<GCedWithCallback>(
      GetAllocationHandle(), [this](GCedWithCallback* obj) {
        auto& header = HeapObjectHeader::FromObject(obj);
        CHECK(header.TryMarkAtomic());
        VerifyMarking(Heap::From(GetHeap())->AsBase(),
                      StackState::kMayContainHeapPointers,
                      header.AllocatedSize());
      });
}

namespace {
class GCedWithCallbackAndChild final
    : public GarbageCollected<GCedWithCallbackAndChild> {
 public:
  template <typename Callback>
  GCedWithCallbackAndChild(GCed* gced, Callback callback) : child_(gced) {
    callback(this);
  }
  void Trace(cppgc::Visitor* visitor) const { visitor->Trace(child_); }

 private:
  Member<GCed> child_;
};

template <typename T>
struct Holder : public GarbageCollected<Holder<T>> {
 public:
  void Trace(cppgc::Visitor* visitor) const { visitor->Trace(object); }
  Member<T> object = nullptr;
};
}  // namespace

TEST_F(MarkingVerifierTest, DoesntDieOnInConstructionObjectWithWriteBarrier) {
  // Regression test: https://crbug.com/v8/10989.
  // GCedWithCallbackAndChild is marked by write barrier and then discarded by
  // FlushNotFullyConstructedObjects because it is already marked.
  Persistent<Holder<GCedWithCallbackAndChild>> persistent =
      MakeGarbageCollected<Holder<GCedWithCallbackAndChild>>(
          GetAllocationHandle());
  GarbageCollector::Config config =
      GarbageCollector::Config::PreciseIncrementalConfig();
  Heap::From(GetHeap())->StartIncrementalGarbageCollection(config);
  MakeGarbageCollected<GCedWithCallbackAndChild>(
      GetAllocationHandle(), MakeGarbageCollected<GCed>(GetAllocationHandle()),
      [&persistent](GCedWithCallbackAndChild* obj) {
        persistent->object = obj;
      });
  GetMarkerRef()->IncrementalMarkingStepForTesting(
      GarbageCollector::Config::StackState::kNoHeapPointers);
  Heap::From(GetHeap())->FinalizeIncrementalGarbageCollectionIfRunning(config);
}

// Death tests.

namespace {

class MarkingVerifierDeathTest : public MarkingVerifierTest {
 protected:
  template <template <typename T> class Reference>
  void TestResurrectingPreFinalizer();
};

}  // namespace

TEST_F(MarkingVerifierDeathTest, DieOnUnmarkedOnStackReference) {
  GCed* object = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& header = HeapObjectHeader::FromObject(object);
  USE(header);
  EXPECT_DEATH_IF_SUPPORTED(VerifyMarking(Heap::From(GetHeap())->AsBase(),
                                          StackState::kMayContainHeapPointers,
                                          header.AllocatedSize()),
                            "");
  access(object);
}

TEST_F(MarkingVerifierDeathTest, DieOnUnmarkedMember) {
  Persistent<GCed> parent = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& parent_header = HeapObjectHeader::FromObject(parent);
  ASSERT_TRUE(parent_header.TryMarkAtomic());
  parent->SetChild(MakeGarbageCollected<GCed>(GetAllocationHandle()));
  EXPECT_DEATH_IF_SUPPORTED(
      VerifyMarking(Heap::From(GetHeap())->AsBase(),
                    StackState::kNoHeapPointers, parent_header.AllocatedSize()),
      "");
}

TEST_F(MarkingVerifierDeathTest, DieOnUnmarkedWeakMember) {
  Persistent<GCed> parent = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& parent_header = HeapObjectHeader::FromObject(parent);
  ASSERT_TRUE(parent_header.TryMarkAtomic());
  parent->SetWeakChild(MakeGarbageCollected<GCed>(GetAllocationHandle()));
  EXPECT_DEATH_IF_SUPPORTED(
      VerifyMarking(Heap::From(GetHeap())->AsBase(),
                    StackState::kNoHeapPointers, parent_header.AllocatedSize()),
      "");
}

#ifdef CPPGC_VERIFY_LIVE_BYTES

TEST_F(MarkingVerifierDeathTest, DieOnUnexpectedLiveByteCount) {
  GCed* object = MakeGarbageCollected<GCed>(GetAllocationHandle());
  auto& header = HeapObjectHeader::FromObject(object);
  ASSERT_TRUE(header.TryMarkAtomic());
  EXPECT_DEATH_IF_SUPPORTED(VerifyMarking(Heap::From(GetHeap())->AsBase(),
                                          StackState::kMayContainHeapPointers,
                                          header.AllocatedSize() - 1),
                            "");
}

#endif  // CPPGC_VERIFY_LIVE_BYTES

namespace {

template <template <typename T> class Reference>
class ResurrectingPreFinalizer
    : public GarbageCollected<ResurrectingPreFinalizer<Reference>> {
  CPPGC_USING_PRE_FINALIZER(ResurrectingPreFinalizer<Reference>, Dispose);

 public:
  class Storage : public GarbageCollected<Storage> {
   public:
    void Trace(Visitor* visitor) const { visitor->Trace(ref); }

    Reference<GCed> ref;
  };

  ResurrectingPreFinalizer(Storage* storage, GCed* object_that_dies)
      : storage_(storage), object_that_dies_(object_that_dies) {}

  void Trace(Visitor* visitor) const {
    visitor->Trace(storage_);
    visitor->Trace(object_that_dies_);
  }

 private:
  void Dispose() { storage_->ref = object_that_dies_; }

  Member<Storage> storage_;
  Member<GCed> object_that_dies_;
};

}  // namespace

template <template <typename T> class Reference>
void MarkingVerifierDeathTest::TestResurrectingPreFinalizer() {
  Persistent<typename ResurrectingPreFinalizer<Reference>::Storage> storage(
      MakeGarbageCollected<
          typename ResurrectingPreFinalizer<Reference>::Storage>(
          GetAllocationHandle()));
  MakeGarbageCollected<ResurrectingPreFinalizer<Reference>>(
      GetAllocationHandle(), storage.Get(),
      MakeGarbageCollected<GCed>(GetAllocationHandle()));
  EXPECT_DEATH_IF_SUPPORTED(PreciseGC(), "");
}

#if DEBUG

TEST_F(MarkingVerifierDeathTest, DiesOnResurrectedMember) {
  TestResurrectingPreFinalizer<Member>();
}

TEST_F(MarkingVerifierDeathTest, DiesOnResurrectedWeakMember) {
  TestResurrectingPreFinalizer<WeakMember>();
}

#endif  // DEBUG

}  // namespace internal
}  // namespace cppgc
