// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/heap/cppgc/heap-growing.h"

#include "include/cppgc/platform.h"
#include "src/heap/cppgc/heap.h"
#include "src/heap/cppgc/stats-collector.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cppgc {
namespace internal {

namespace {

class FakeGarbageCollector : public GarbageCollector {
 public:
  explicit FakeGarbageCollector(StatsCollector* stats_collector)
      : stats_collector_(stats_collector) {}

  void SetLiveBytes(size_t live_bytes) { live_bytes_ = live_bytes; }

  void CollectGarbage(GarbageCollector::Config config) override {
    stats_collector_->NotifyMarkingStarted(
        GarbageCollector::Config::CollectionType::kMajor,
        GarbageCollector::Config::IsForcedGC::kNotForced);
    stats_collector_->NotifyMarkingCompleted(live_bytes_);
    stats_collector_->NotifySweepingCompleted();
    callcount_++;
  }

  void StartIncrementalGarbageCollection(
      GarbageCollector::Config config) override {
    UNREACHABLE();
  }

  size_t epoch() const override { return callcount_; }

 private:
  StatsCollector* stats_collector_;
  size_t live_bytes_ = 0;
  size_t callcount_ = 0;
};

class MockGarbageCollector : public GarbageCollector {
 public:
  MOCK_METHOD(void, CollectGarbage, (GarbageCollector::Config), (override));
  MOCK_METHOD(void, StartIncrementalGarbageCollection,
              (GarbageCollector::Config), (override));
  MOCK_METHOD(size_t, epoch, (), (const, override));
};

void FakeAllocate(StatsCollector* stats_collector, size_t bytes) {
  stats_collector->NotifyAllocation(bytes);
  stats_collector->NotifySafePointForConservativeCollection();
}

}  // namespace

TEST(HeapGrowingTest, ConservativeGCInvoked) {
  StatsCollector stats_collector(nullptr /* metric_recorder */,
                                 nullptr /* platform */);
  MockGarbageCollector gc;
  cppgc::Heap::ResourceConstraints constraints;
  // Force GC at the first update.
  constraints.initial_heap_size_bytes = 1;
  HeapGrowing growing(&gc, &stats_collector, constraints,
                      cppgc::Heap::MarkingType::kIncrementalAndConcurrent,
                      cppgc::Heap::SweepingType::kIncrementalAndConcurrent);
  EXPECT_CALL(
      gc, CollectGarbage(::testing::Field(
              &GarbageCollector::Config::stack_state,
              GarbageCollector::Config::StackState::kMayContainHeapPointers)));
  FakeAllocate(&stats_collector, 100 * kMB);
}

TEST(HeapGrowingTest, InitialHeapSize) {
  StatsCollector stats_collector(nullptr /* metric_recorder */,
                                 nullptr /* platform */);
  MockGarbageCollector gc;
  cppgc::Heap::ResourceConstraints constraints;
  // Use larger size to avoid running into small heap optimizations.
  constexpr size_t kObjectSize = 10 * HeapGrowing::kMinLimitIncrease;
  constraints.initial_heap_size_bytes = kObjectSize;
  HeapGrowing growing(&gc, &stats_collector, constraints,
                      cppgc::Heap::MarkingType::kIncrementalAndConcurrent,
                      cppgc::Heap::SweepingType::kIncrementalAndConcurrent);
  FakeAllocate(&stats_collector, kObjectSize - 1);
  EXPECT_CALL(
      gc, CollectGarbage(::testing::Field(
              &GarbageCollector::Config::stack_state,
              GarbageCollector::Config::StackState::kMayContainHeapPointers)));
  FakeAllocate(&stats_collector, kObjectSize);
}

TEST(HeapGrowingTest, ConstantGrowingFactor) {
  // Use larger size to avoid running into small heap optimizations.
  constexpr size_t kObjectSize = 10 * HeapGrowing::kMinLimitIncrease;
  StatsCollector stats_collector(nullptr /* metric_recorder */,
                                 nullptr /* platform */);
  FakeGarbageCollector gc(&stats_collector);
  cppgc::Heap::ResourceConstraints constraints;
  // Force GC at the first update.
  constraints.initial_heap_size_bytes = HeapGrowing::kMinLimitIncrease;
  HeapGrowing growing(&gc, &stats_collector, constraints,
                      cppgc::Heap::MarkingType::kIncrementalAndConcurrent,
                      cppgc::Heap::SweepingType::kIncrementalAndConcurrent);
  EXPECT_EQ(0u, gc.epoch());
  gc.SetLiveBytes(kObjectSize);
  FakeAllocate(&stats_collector, kObjectSize + 1);
  EXPECT_EQ(1u, gc.epoch());
  EXPECT_EQ(1.5 * kObjectSize, growing.limit_for_atomic_gc());
}

TEST(HeapGrowingTest, SmallHeapGrowing) {
  // Larger constant to avoid running into special handling for smaller heaps.
  constexpr size_t kLargeAllocation = 100 * kMB;
  StatsCollector stats_collector(nullptr /* metric_recorder */,
                                 nullptr /* platform */);
  FakeGarbageCollector gc(&stats_collector);
  cppgc::Heap::ResourceConstraints constraints;
  // Force GC at the first update.
  constraints.initial_heap_size_bytes = 1;
  HeapGrowing growing(&gc, &stats_collector, constraints,
                      cppgc::Heap::MarkingType::kIncrementalAndConcurrent,
                      cppgc::Heap::SweepingType::kIncrementalAndConcurrent);
  EXPECT_EQ(0u, gc.epoch());
  gc.SetLiveBytes(1);
  FakeAllocate(&stats_collector, kLargeAllocation);
  EXPECT_EQ(1u, gc.epoch());
  EXPECT_EQ(1 + HeapGrowing::kMinLimitIncrease, growing.limit_for_atomic_gc());
}

TEST(HeapGrowingTest, IncrementalGCStarted) {
  StatsCollector stats_collector(nullptr /* metric_recorder */,
                                 nullptr /* platform */);
  MockGarbageCollector gc;
  cppgc::Heap::ResourceConstraints constraints;
  HeapGrowing growing(&gc, &stats_collector, constraints,
                      cppgc::Heap::MarkingType::kIncrementalAndConcurrent,
                      cppgc::Heap::SweepingType::kIncrementalAndConcurrent);
  EXPECT_CALL(
      gc, CollectGarbage(::testing::Field(
              &GarbageCollector::Config::stack_state,
              GarbageCollector::Config::StackState::kMayContainHeapPointers)))
      .Times(0);
  EXPECT_CALL(gc, StartIncrementalGarbageCollection(::testing::_));
  // Allocate 1 byte less the limit for atomic gc to trigger incremental gc.
  FakeAllocate(&stats_collector, growing.limit_for_atomic_gc() - 1);
}

TEST(HeapGrowingTest, IncrementalGCFinalized) {
  StatsCollector stats_collector(nullptr /* metric_recorder */,
                                 nullptr /* platform */);
  MockGarbageCollector gc;
  cppgc::Heap::ResourceConstraints constraints;
  HeapGrowing growing(&gc, &stats_collector, constraints,
                      cppgc::Heap::MarkingType::kIncrementalAndConcurrent,
                      cppgc::Heap::SweepingType::kIncrementalAndConcurrent);
  EXPECT_CALL(
      gc, CollectGarbage(::testing::Field(
              &GarbageCollector::Config::stack_state,
              GarbageCollector::Config::StackState::kMayContainHeapPointers)))
      .Times(0);
  EXPECT_CALL(gc, StartIncrementalGarbageCollection(::testing::_));
  // Allocate 1 byte less the limit for atomic gc to trigger incremental gc.
  size_t bytes_for_incremental_gc = growing.limit_for_atomic_gc() - 1;
  FakeAllocate(&stats_collector, bytes_for_incremental_gc);
  ::testing::Mock::VerifyAndClearExpectations(&gc);
  EXPECT_CALL(
      gc, CollectGarbage(::testing::Field(
              &GarbageCollector::Config::stack_state,
              GarbageCollector::Config::StackState::kMayContainHeapPointers)));
  EXPECT_CALL(gc, StartIncrementalGarbageCollection(::testing::_)).Times(0);
  // Allocate the rest needed to trigger atomic gc ().
  FakeAllocate(&stats_collector, StatsCollector::kAllocationThresholdBytes);
}

}  // namespace internal
}  // namespace cppgc
