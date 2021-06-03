// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/cppgc/ephemeron-pair.h"

#include "include/cppgc/allocation.h"
#include "include/cppgc/garbage-collected.h"
#include "include/cppgc/persistent.h"
#include "src/heap/cppgc/heap-object-header.h"
#include "src/heap/cppgc/marking-visitor.h"
#include "src/heap/cppgc/stats-collector.h"
#include "test/unittests/heap/cppgc/tests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cppgc {
namespace internal {

namespace {
class GCed : public GarbageCollected<GCed> {
 public:
  void Trace(cppgc::Visitor*) const {}
};

class EphemeronHolder : public GarbageCollected<EphemeronHolder> {
 public:
  EphemeronHolder(GCed* key, GCed* value) : ephemeron_pair_(key, value) {}
  void Trace(cppgc::Visitor* visitor) const { visitor->Trace(ephemeron_pair_); }

  const EphemeronPair<GCed, GCed>& ephemeron_pair() const {
    return ephemeron_pair_;
  }

 private:
  EphemeronPair<GCed, GCed> ephemeron_pair_;
};

class EphemeronHolderTraceEphemeron
    : public GarbageCollected<EphemeronHolderTraceEphemeron> {
 public:
  EphemeronHolderTraceEphemeron(GCed* key, GCed* value)
      : ephemeron_pair_(key, value) {}
  void Trace(cppgc::Visitor* visitor) const {
    visitor->TraceEphemeron(ephemeron_pair_.key, &ephemeron_pair_.value);
  }

 private:
  EphemeronPair<GCed, GCed> ephemeron_pair_;
};

class EphemeronPairTest : public testing::TestWithHeap {
  using MarkingConfig = Marker::MarkingConfig;

  static constexpr Marker::MarkingConfig IncrementalPreciseMarkingConfig = {
      MarkingConfig::CollectionType::kMajor,
      MarkingConfig::StackState::kNoHeapPointers,
      MarkingConfig::MarkingType::kIncremental};

 public:
  void FinishSteps() {
    while (!SingleStep()) {
    }
  }

  void FinishMarking() {
    marker_->FinishMarking(MarkingConfig::StackState::kNoHeapPointers);
    // Pretend do finish sweeping as StatsCollector verifies that Notify*
    // methods are called in the right order.
    Heap::From(GetHeap())->stats_collector()->NotifySweepingCompleted();
  }

  void InitializeMarker(HeapBase& heap, cppgc::Platform* platform) {
    marker_ = MarkerFactory::CreateAndStartMarking<Marker>(
        heap, platform, IncrementalPreciseMarkingConfig);
  }

  Marker* marker() const { return marker_.get(); }

 private:
  bool SingleStep() {
    return marker_->IncrementalMarkingStepForTesting(
        MarkingConfig::StackState::kNoHeapPointers);
  }

  std::unique_ptr<Marker> marker_;
};

// static
constexpr Marker::MarkingConfig
    EphemeronPairTest::IncrementalPreciseMarkingConfig;

}  // namespace

TEST_F(EphemeronPairTest, ValueMarkedWhenKeyIsMarked) {
  GCed* key = MakeGarbageCollected<GCed>(GetAllocationHandle());
  GCed* value = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolder> holder =
      MakeGarbageCollected<EphemeronHolder>(GetAllocationHandle(), key, value);
  HeapObjectHeader::FromObject(key).TryMarkAtomic();
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishMarking();
  EXPECT_TRUE(HeapObjectHeader::FromObject(value).IsMarked());
}

TEST_F(EphemeronPairTest, ValueNotMarkedWhenKeyIsNotMarked) {
  GCed* key = MakeGarbageCollected<GCed>(GetAllocationHandle());
  GCed* value = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolder> holder =
      MakeGarbageCollected<EphemeronHolder>(GetAllocationHandle(), key, value);
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishMarking();
  EXPECT_FALSE(HeapObjectHeader::FromObject(key).IsMarked());
  EXPECT_FALSE(HeapObjectHeader::FromObject(value).IsMarked());
}

TEST_F(EphemeronPairTest, ValueNotMarkedBeforeKey) {
  GCed* key = MakeGarbageCollected<GCed>(GetAllocationHandle());
  GCed* value = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolder> holder =
      MakeGarbageCollected<EphemeronHolder>(GetAllocationHandle(), key, value);
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishSteps();
  EXPECT_FALSE(HeapObjectHeader::FromObject(value).IsMarked());
  HeapObjectHeader::FromObject(key).TryMarkAtomic();
  FinishMarking();
  EXPECT_TRUE(HeapObjectHeader::FromObject(value).IsMarked());
}

TEST_F(EphemeronPairTest, TraceEphemeronDispatch) {
  GCed* key = MakeGarbageCollected<GCed>(GetAllocationHandle());
  GCed* value = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolderTraceEphemeron> holder =
      MakeGarbageCollected<EphemeronHolderTraceEphemeron>(GetAllocationHandle(),
                                                          key, value);
  HeapObjectHeader::FromObject(key).TryMarkAtomic();
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishMarking();
  EXPECT_TRUE(HeapObjectHeader::FromObject(value).IsMarked());
}

TEST_F(EphemeronPairTest, EmptyValue) {
  GCed* key = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolderTraceEphemeron> holder =
      MakeGarbageCollected<EphemeronHolderTraceEphemeron>(GetAllocationHandle(),
                                                          key, nullptr);
  HeapObjectHeader::FromObject(key).TryMarkAtomic();
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishMarking();
}

TEST_F(EphemeronPairTest, EmptyKey) {
  GCed* value = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolderTraceEphemeron> holder =
      MakeGarbageCollected<EphemeronHolderTraceEphemeron>(GetAllocationHandle(),
                                                          nullptr, value);
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishMarking();
  // Key is not alive and value should thus not be held alive.
  EXPECT_FALSE(HeapObjectHeader::FromObject(value).IsMarked());
}

using EphemeronPairGCTest = testing::TestWithHeap;

TEST_F(EphemeronPairGCTest, EphemeronPairValueIsCleared) {
  GCed* value = MakeGarbageCollected<GCed>(GetAllocationHandle());
  Persistent<EphemeronHolder> holder = MakeGarbageCollected<EphemeronHolder>(
      GetAllocationHandle(), nullptr, value);
  PreciseGC();
  EXPECT_EQ(nullptr, holder->ephemeron_pair().value.Get());
}

namespace {

class Mixin : public GarbageCollectedMixin {
 public:
  void Trace(Visitor* v) const override {}
};

class OtherMixin : public GarbageCollectedMixin {
 public:
  void Trace(Visitor* v) const override {}
};

class GCedWithMixin : public GarbageCollected<GCedWithMixin>,
                      public OtherMixin,
                      public Mixin {
 public:
  void Trace(Visitor* v) const override {
    OtherMixin::Trace(v);
    Mixin::Trace(v);
  }
};

class EphemeronHolderWithMixins
    : public GarbageCollected<EphemeronHolderWithMixins> {
 public:
  EphemeronHolderWithMixins(Mixin* key, Mixin* value)
      : ephemeron_pair_(key, value) {}
  void Trace(cppgc::Visitor* visitor) const { visitor->Trace(ephemeron_pair_); }

  const EphemeronPair<Mixin, Mixin>& ephemeron_pair() const {
    return ephemeron_pair_;
  }

 private:
  EphemeronPair<Mixin, Mixin> ephemeron_pair_;
};

}  // namespace

TEST_F(EphemeronPairTest, EphemeronPairWithMixinKey) {
  GCedWithMixin* key =
      MakeGarbageCollected<GCedWithMixin>(GetAllocationHandle());
  GCedWithMixin* value =
      MakeGarbageCollected<GCedWithMixin>(GetAllocationHandle());
  Persistent<EphemeronHolderWithMixins> holder =
      MakeGarbageCollected<EphemeronHolderWithMixins>(GetAllocationHandle(),
                                                      key, value);
  EXPECT_NE(static_cast<void*>(key), holder->ephemeron_pair().key.Get());
  EXPECT_NE(static_cast<void*>(value), holder->ephemeron_pair().value.Get());
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishSteps();
  EXPECT_FALSE(HeapObjectHeader::FromObject(value).IsMarked());
  EXPECT_TRUE(HeapObjectHeader::FromObject(key).TryMarkAtomic());
  FinishMarking();
  EXPECT_TRUE(HeapObjectHeader::FromObject(value).IsMarked());
}

TEST_F(EphemeronPairTest, EphemeronPairWithEmptyMixinValue) {
  GCedWithMixin* key =
      MakeGarbageCollected<GCedWithMixin>(GetAllocationHandle());
  Persistent<EphemeronHolderWithMixins> holder =
      MakeGarbageCollected<EphemeronHolderWithMixins>(GetAllocationHandle(),
                                                      key, nullptr);
  EXPECT_NE(static_cast<void*>(key), holder->ephemeron_pair().key.Get());
  EXPECT_TRUE(HeapObjectHeader::FromObject(key).TryMarkAtomic());
  InitializeMarker(*Heap::From(GetHeap()), GetPlatformHandle().get());
  FinishSteps();
  FinishMarking();
}

}  // namespace internal
}  // namespace cppgc
