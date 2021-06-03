// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "src/api/api.h"
#include "src/base/platform/condition-variable.h"
#include "src/base/platform/mutex.h"
#include "src/base/platform/semaphore.h"
#include "src/codegen/assembler-inl.h"
#include "src/codegen/assembler.h"
#include "src/codegen/macro-assembler-inl.h"
#include "src/codegen/macro-assembler.h"
#include "src/common/globals.h"
#include "src/handles/handles-inl.h"
#include "src/handles/handles.h"
#include "src/handles/local-handles-inl.h"
#include "src/handles/persistent-handles.h"
#include "src/heap/concurrent-allocator-inl.h"
#include "src/heap/heap.h"
#include "src/heap/local-heap-inl.h"
#include "src/heap/parked-scope.h"
#include "src/heap/safepoint.h"
#include "src/objects/heap-number.h"
#include "src/objects/heap-object.h"
#include "test/cctest/cctest.h"
#include "test/cctest/heap/heap-utils.h"

namespace v8 {
namespace internal {

namespace {
void CreateFixedArray(Heap* heap, Address start, int size) {
  HeapObject object = HeapObject::FromAddress(start);
  object.set_map_after_allocation(ReadOnlyRoots(heap).fixed_array_map(),
                                  SKIP_WRITE_BARRIER);
  FixedArray array = FixedArray::cast(object);
  int length = (size - FixedArray::kHeaderSize) / kTaggedSize;
  array.set_length(length);
  MemsetTagged(array.data_start(), ReadOnlyRoots(heap).undefined_value(),
               length);
}

const int kNumIterations = 2000;
const int kSmallObjectSize = 10 * kTaggedSize;
const int kMediumObjectSize = 8 * KB;

void AllocateSomeObjects(LocalHeap* local_heap) {
  for (int i = 0; i < kNumIterations; i++) {
    Address address = local_heap->AllocateRawOrFail(
        kSmallObjectSize, AllocationType::kOld, AllocationOrigin::kRuntime,
        AllocationAlignment::kWordAligned);
    CreateFixedArray(local_heap->heap(), address, kSmallObjectSize);
    address = local_heap->AllocateRawOrFail(
        kMediumObjectSize, AllocationType::kOld, AllocationOrigin::kRuntime,
        AllocationAlignment::kWordAligned);
    CreateFixedArray(local_heap->heap(), address, kMediumObjectSize);
    if (i % 10 == 0) {
      local_heap->Safepoint();
    }
  }
}
}  // namespace

class ConcurrentAllocationThread final : public v8::base::Thread {
 public:
  explicit ConcurrentAllocationThread(Heap* heap,
                                      std::atomic<int>* pending = nullptr)
      : v8::base::Thread(base::Thread::Options("ThreadWithLocalHeap")),
        heap_(heap),
        pending_(pending) {}

  void Run() override {
    LocalHeap local_heap(heap_, ThreadKind::kBackground);
    UnparkedScope unparked_scope(&local_heap);
    AllocateSomeObjects(&local_heap);
    if (pending_) pending_->fetch_sub(1);
  }

  Heap* heap_;
  std::atomic<int>* pending_;
};

UNINITIALIZED_TEST(ConcurrentAllocationInOldSpace) {
  FLAG_max_old_space_size = 32;
  FLAG_stress_concurrent_allocation = false;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  std::vector<std::unique_ptr<ConcurrentAllocationThread>> threads;

  const int kThreads = 4;

  std::atomic<int> pending(kThreads);

  for (int i = 0; i < kThreads; i++) {
    auto thread = std::make_unique<ConcurrentAllocationThread>(
        i_isolate->heap(), &pending);
    CHECK(thread->Start());
    threads.push_back(std::move(thread));
  }

  while (pending > 0) {
    v8::platform::PumpMessageLoop(i::V8::GetCurrentPlatform(), isolate);
  }

  for (auto& thread : threads) {
    thread->Join();
  }

  isolate->Dispose();
}

UNINITIALIZED_TEST(ConcurrentAllocationInOldSpaceFromMainThread) {
  FLAG_max_old_space_size = 4;
  FLAG_stress_concurrent_allocation = false;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  AllocateSomeObjects(i_isolate->main_thread_local_heap());

  isolate->Dispose();
}

UNINITIALIZED_TEST(ConcurrentAllocationWhileMainThreadIsParked) {
  FLAG_max_old_space_size = 4;
  FLAG_stress_concurrent_allocation = false;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  std::vector<std::unique_ptr<ConcurrentAllocationThread>> threads;
  const int kThreads = 4;

  {
    ParkedScope scope(i_isolate->main_thread_local_isolate());

    for (int i = 0; i < kThreads; i++) {
      auto thread =
          std::make_unique<ConcurrentAllocationThread>(i_isolate->heap());
      CHECK(thread->Start());
      threads.push_back(std::move(thread));
    }

    for (auto& thread : threads) {
      thread->Join();
    }
  }

  isolate->Dispose();
}

UNINITIALIZED_TEST(ConcurrentAllocationWhileMainThreadParksAndUnparks) {
  FLAG_max_old_space_size = 4;
  FLAG_stress_concurrent_allocation = false;
  FLAG_incremental_marking = false;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  std::vector<std::unique_ptr<ConcurrentAllocationThread>> threads;
  const int kThreads = 4;

  for (int i = 0; i < kThreads; i++) {
    auto thread =
        std::make_unique<ConcurrentAllocationThread>(i_isolate->heap());
    CHECK(thread->Start());
    threads.push_back(std::move(thread));
  }

  for (int i = 0; i < 300'000; i++) {
    ParkedScope scope(i_isolate->main_thread_local_isolate());
  }

  {
    ParkedScope scope(i_isolate->main_thread_local_isolate());

    for (auto& thread : threads) {
      thread->Join();
    }
  }

  isolate->Dispose();
}

UNINITIALIZED_TEST(ConcurrentAllocationWhileMainThreadRunsWithSafepoints) {
  FLAG_max_old_space_size = 4;
  FLAG_stress_concurrent_allocation = false;
  FLAG_incremental_marking = false;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  std::vector<std::unique_ptr<ConcurrentAllocationThread>> threads;
  const int kThreads = 4;

  for (int i = 0; i < kThreads; i++) {
    auto thread =
        std::make_unique<ConcurrentAllocationThread>(i_isolate->heap());
    CHECK(thread->Start());
    threads.push_back(std::move(thread));
  }

  // Some of the following Safepoint() invocations are supposed to perform a GC.
  for (int i = 0; i < 1'000'000; i++) {
    i_isolate->main_thread_local_heap()->Safepoint();
  }

  {
    ParkedScope scope(i_isolate->main_thread_local_isolate());

    for (auto& thread : threads) {
      thread->Join();
    }
  }

  i_isolate->main_thread_local_heap()->Safepoint();
  isolate->Dispose();
}

class LargeObjectConcurrentAllocationThread final : public v8::base::Thread {
 public:
  explicit LargeObjectConcurrentAllocationThread(Heap* heap,
                                                 std::atomic<int>* pending)
      : v8::base::Thread(base::Thread::Options("ThreadWithLocalHeap")),
        heap_(heap),
        pending_(pending) {}

  void Run() override {
    LocalHeap local_heap(heap_, ThreadKind::kBackground);
    UnparkedScope unparked_scope(&local_heap);
    const size_t kLargeObjectSize = kMaxRegularHeapObjectSize * 2;

    for (int i = 0; i < kNumIterations; i++) {
      AllocationResult result = local_heap.AllocateRaw(
          kLargeObjectSize, AllocationType::kOld, AllocationOrigin::kRuntime,
          AllocationAlignment::kWordAligned);
      if (result.IsRetry()) {
        local_heap.TryPerformCollection();
      } else {
        Address address = result.ToAddress();
        CreateFixedArray(heap_, address, kLargeObjectSize);
      }
      local_heap.Safepoint();
    }

    pending_->fetch_sub(1);
  }

  Heap* heap_;
  std::atomic<int>* pending_;
};

UNINITIALIZED_TEST(ConcurrentAllocationInLargeSpace) {
  FLAG_max_old_space_size = 32;
  FLAG_stress_concurrent_allocation = false;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);

  std::vector<std::unique_ptr<LargeObjectConcurrentAllocationThread>> threads;

  const int kThreads = 4;

  std::atomic<int> pending(kThreads);

  for (int i = 0; i < kThreads; i++) {
    auto thread = std::make_unique<LargeObjectConcurrentAllocationThread>(
        i_isolate->heap(), &pending);
    CHECK(thread->Start());
    threads.push_back(std::move(thread));
  }

  while (pending > 0) {
    v8::platform::PumpMessageLoop(i::V8::GetCurrentPlatform(), isolate);
  }

  for (auto& thread : threads) {
    thread->Join();
  }

  isolate->Dispose();
}

const int kWhiteIterations = 1000;

class ConcurrentBlackAllocationThread final : public v8::base::Thread {
 public:
  explicit ConcurrentBlackAllocationThread(
      Heap* heap, std::vector<Address>* objects, base::Semaphore* sema_white,
      base::Semaphore* sema_marking_started)
      : v8::base::Thread(base::Thread::Options("ThreadWithLocalHeap")),
        heap_(heap),
        objects_(objects),
        sema_white_(sema_white),
        sema_marking_started_(sema_marking_started) {}

  void Run() override {
    LocalHeap local_heap(heap_, ThreadKind::kBackground);
    UnparkedScope unparked_scope(&local_heap);

    for (int i = 0; i < kNumIterations; i++) {
      if (i == kWhiteIterations) {
        ParkedScope scope(&local_heap);
        sema_white_->Signal();
        sema_marking_started_->Wait();
      }
      Address address = local_heap.AllocateRawOrFail(
          kSmallObjectSize, AllocationType::kOld, AllocationOrigin::kRuntime,
          AllocationAlignment::kWordAligned);
      objects_->push_back(address);
      CreateFixedArray(heap_, address, kSmallObjectSize);
      address = local_heap.AllocateRawOrFail(
          kMediumObjectSize, AllocationType::kOld, AllocationOrigin::kRuntime,
          AllocationAlignment::kWordAligned);
      objects_->push_back(address);
      CreateFixedArray(heap_, address, kMediumObjectSize);
    }
  }

  Heap* heap_;
  std::vector<Address>* objects_;
  base::Semaphore* sema_white_;
  base::Semaphore* sema_marking_started_;
};

UNINITIALIZED_TEST(ConcurrentBlackAllocation) {
  if (!FLAG_incremental_marking) return;
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);
  Heap* heap = i_isolate->heap();

  std::vector<Address> objects;

  base::Semaphore sema_white(0);
  base::Semaphore sema_marking_started(0);

  auto thread = std::make_unique<ConcurrentBlackAllocationThread>(
      heap, &objects, &sema_white, &sema_marking_started);
  CHECK(thread->Start());

  sema_white.Wait();
  heap->StartIncrementalMarking(i::Heap::kNoGCFlags,
                                i::GarbageCollectionReason::kTesting);
  sema_marking_started.Signal();

  thread->Join();

  const int kObjectsAllocatedPerIteration = 2;

  for (int i = 0; i < kNumIterations * kObjectsAllocatedPerIteration; i++) {
    Address address = objects[i];
    HeapObject object = HeapObject::FromAddress(address);

    if (i < kWhiteIterations * kObjectsAllocatedPerIteration) {
      CHECK(heap->incremental_marking()->marking_state()->IsWhite(object));
    } else {
      CHECK(heap->incremental_marking()->marking_state()->IsBlack(object));
    }
  }

  isolate->Dispose();
}

class ConcurrentWriteBarrierThread final : public v8::base::Thread {
 public:
  explicit ConcurrentWriteBarrierThread(Heap* heap, FixedArray fixed_array,
                                        HeapObject value)
      : v8::base::Thread(base::Thread::Options("ThreadWithLocalHeap")),
        heap_(heap),
        fixed_array_(fixed_array),
        value_(value) {}

  void Run() override {
    LocalHeap local_heap(heap_, ThreadKind::kBackground);
    UnparkedScope unparked_scope(&local_heap);
    fixed_array_.set(0, value_);
  }

  Heap* heap_;
  FixedArray fixed_array_;
  HeapObject value_;
};

UNINITIALIZED_TEST(ConcurrentWriteBarrier) {
  if (!FLAG_incremental_marking) return;
  if (!FLAG_concurrent_marking) {
    // The test requires concurrent marking barrier.
    return;
  }
  ManualGCScope manual_gc_scope;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);
  Heap* heap = i_isolate->heap();

  FixedArray fixed_array;
  HeapObject value;
  {
    HandleScope handle_scope(i_isolate);
    Handle<FixedArray> fixed_array_handle(
        i_isolate->factory()->NewFixedArray(1));
    Handle<HeapNumber> value_handle(
        i_isolate->factory()->NewHeapNumber<AllocationType::kOld>(1.1));
    fixed_array = *fixed_array_handle;
    value = *value_handle;
  }
  heap->StartIncrementalMarking(i::Heap::kNoGCFlags,
                                i::GarbageCollectionReason::kTesting);
  CHECK(heap->incremental_marking()->marking_state()->IsWhite(value));

  auto thread =
      std::make_unique<ConcurrentWriteBarrierThread>(heap, fixed_array, value);
  CHECK(thread->Start());

  thread->Join();

  CHECK(heap->incremental_marking()->marking_state()->IsBlackOrGrey(value));
  heap::InvokeMarkSweep(i_isolate);

  isolate->Dispose();
}

class ConcurrentRecordRelocSlotThread final : public v8::base::Thread {
 public:
  explicit ConcurrentRecordRelocSlotThread(Heap* heap, Code code,
                                           HeapObject value)
      : v8::base::Thread(base::Thread::Options("ThreadWithLocalHeap")),
        heap_(heap),
        code_(code),
        value_(value) {}

  void Run() override {
    LocalHeap local_heap(heap_, ThreadKind::kBackground);
    UnparkedScope unparked_scope(&local_heap);
    int mode_mask = RelocInfo::EmbeddedObjectModeMask();
    for (RelocIterator it(code_, mode_mask); !it.done(); it.next()) {
      DCHECK(RelocInfo::IsEmbeddedObjectMode(it.rinfo()->rmode()));
      it.rinfo()->set_target_object(heap_, value_);
    }
  }

  Heap* heap_;
  Code code_;
  HeapObject value_;
};

UNINITIALIZED_TEST(ConcurrentRecordRelocSlot) {
  if (!FLAG_incremental_marking) return;
  if (!FLAG_concurrent_marking) {
    // The test requires concurrent marking barrier.
    return;
  }
  FLAG_manual_evacuation_candidates_selection = true;
  ManualGCScope manual_gc_scope;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = CcTest::array_buffer_allocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  Isolate* i_isolate = reinterpret_cast<Isolate*>(isolate);
  Heap* heap = i_isolate->heap();

  Code code;
  HeapObject value;
  {
    HandleScope handle_scope(i_isolate);
    i::byte buffer[i::Assembler::kDefaultBufferSize];
    MacroAssembler masm(i_isolate, v8::internal::CodeObjectRequired::kYes,
                        ExternalAssemblerBuffer(buffer, sizeof(buffer)));
#if V8_TARGET_ARCH_ARM64
    // Arm64 requires stack alignment.
    UseScratchRegisterScope temps(&masm);
    Register tmp = temps.AcquireX();
    masm.Mov(tmp, Operand(ReadOnlyRoots(heap).undefined_value_handle()));
    masm.Push(tmp, padreg);
#else
    masm.Push(ReadOnlyRoots(heap).undefined_value_handle());
#endif
    CodeDesc desc;
    masm.GetCode(i_isolate, &desc);
    Handle<Code> code_handle =
        Factory::CodeBuilder(i_isolate, desc, CodeKind::FOR_TESTING).Build();
    heap::AbandonCurrentlyFreeMemory(heap->old_space());
    Handle<HeapNumber> value_handle(
        i_isolate->factory()->NewHeapNumber<AllocationType::kOld>(1.1));
    heap::ForceEvacuationCandidate(Page::FromHeapObject(*value_handle));
    code = *code_handle;
    value = *value_handle;
  }
  heap->StartIncrementalMarking(i::Heap::kNoGCFlags,
                                i::GarbageCollectionReason::kTesting);
  CHECK(heap->incremental_marking()->marking_state()->IsWhite(value));

  {
    CodeSpaceMemoryModificationScope modification_scope(heap);
    auto thread =
        std::make_unique<ConcurrentRecordRelocSlotThread>(heap, code, value);
    CHECK(thread->Start());

    thread->Join();
  }

  CHECK(heap->incremental_marking()->marking_state()->IsBlackOrGrey(value));
  heap::InvokeMarkSweep(i_isolate);

  isolate->Dispose();
}

}  // namespace internal
}  // namespace v8
