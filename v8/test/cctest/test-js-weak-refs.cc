// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/execution/microtask-queue.h"
#include "src/handles/handles-inl.h"
#include "src/heap/factory-inl.h"
#include "src/heap/heap-inl.h"
#include "src/objects/js-objects.h"
#include "src/objects/js-weak-refs-inl.h"
#include "test/cctest/cctest.h"
#include "test/cctest/heap/heap-utils.h"

namespace v8 {
namespace internal {

namespace {

Handle<JSFinalizationRegistry> ConstructJSFinalizationRegistry(
    Isolate* isolate) {
  Factory* factory = isolate->factory();
  Handle<String> finalization_registry_name =
      factory->NewStringFromStaticChars("FinalizationRegistry");
  Handle<Object> global =
      handle(isolate->native_context()->global_object(), isolate);
  Handle<JSFunction> finalization_registry_fun = Handle<JSFunction>::cast(
      Object::GetProperty(isolate, global, finalization_registry_name)
          .ToHandleChecked());
  auto finalization_registry = Handle<JSFinalizationRegistry>::cast(
      JSObject::New(finalization_registry_fun, finalization_registry_fun,
                    Handle<AllocationSite>::null())
          .ToHandleChecked());
#ifdef VERIFY_HEAP
  finalization_registry->JSFinalizationRegistryVerify(isolate);
#endif  // VERIFY_HEAP
  return finalization_registry;
}

Handle<JSWeakRef> ConstructJSWeakRef(Handle<JSReceiver> target,
                                     Isolate* isolate) {
  Factory* factory = isolate->factory();
  Handle<String> weak_ref_name = factory->WeakRef_string();
  Handle<Object> global =
      handle(isolate->native_context()->global_object(), isolate);
  Handle<JSFunction> weak_ref_fun = Handle<JSFunction>::cast(
      Object::GetProperty(isolate, global, weak_ref_name).ToHandleChecked());
  auto weak_ref = Handle<JSWeakRef>::cast(
      JSObject::New(weak_ref_fun, weak_ref_fun, Handle<AllocationSite>::null())
          .ToHandleChecked());
  weak_ref->set_target(*target);
#ifdef VERIFY_HEAP
  weak_ref->JSWeakRefVerify(isolate);
#endif  // VERIFY_HEAP
  return weak_ref;
}

Handle<JSObject> CreateKey(const char* key_prop_value, Isolate* isolate) {
  Factory* factory = isolate->factory();
  Handle<String> key_string = factory->NewStringFromStaticChars("key_string");
  Handle<JSObject> key =
      isolate->factory()->NewJSObject(isolate->object_function());
  JSObject::AddProperty(isolate, key, key_string,
                        factory->NewStringFromAsciiChecked(key_prop_value),
                        NONE);
  return key;
}

Handle<WeakCell> FinalizationRegistryRegister(
    Handle<JSFinalizationRegistry> finalization_registry,
    Handle<JSObject> target, Handle<Object> held_value,
    Handle<Object> unregister_token, Isolate* isolate) {
  Factory* factory = isolate->factory();
  Handle<JSFunction> regfunc = Handle<JSFunction>::cast(
      Object::GetProperty(isolate, finalization_registry,
                          factory->NewStringFromStaticChars("register"))
          .ToHandleChecked());
  Handle<Object> args[] = {target, held_value, unregister_token};
  Execution::Call(isolate, regfunc, finalization_registry, arraysize(args),
                  args)
      .ToHandleChecked();
  CHECK(finalization_registry->active_cells().IsWeakCell());
  Handle<WeakCell> weak_cell =
      handle(WeakCell::cast(finalization_registry->active_cells()), isolate);
#ifdef VERIFY_HEAP
  weak_cell->WeakCellVerify(isolate);
#endif  // VERIFY_HEAP
  return weak_cell;
}

Handle<WeakCell> FinalizationRegistryRegister(
    Handle<JSFinalizationRegistry> finalization_registry,
    Handle<JSObject> target, Isolate* isolate) {
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);
  return FinalizationRegistryRegister(finalization_registry, target, undefined,
                                      undefined, isolate);
}

void NullifyWeakCell(Handle<WeakCell> weak_cell, Isolate* isolate) {
  auto empty_func = [](HeapObject object, ObjectSlot slot, Object target) {};
  weak_cell->Nullify(isolate, empty_func);
#ifdef VERIFY_HEAP
  weak_cell->WeakCellVerify(isolate);
#endif  // VERIFY_HEAP
}

Object PopClearedCellHoldings(
    Handle<JSFinalizationRegistry> finalization_registry, Isolate* isolate) {
  // PopClearedCell is implemented in Torque. Reproduce that implementation here
  // for testing.
  Handle<WeakCell> weak_cell =
      handle(WeakCell::cast(finalization_registry->cleared_cells()), isolate);
  DCHECK(weak_cell->prev().IsUndefined(isolate));
  finalization_registry->set_cleared_cells(weak_cell->next());
  weak_cell->set_next(ReadOnlyRoots(isolate).undefined_value());

  if (finalization_registry->cleared_cells().IsWeakCell()) {
    WeakCell cleared_cells_head =
        WeakCell::cast(finalization_registry->cleared_cells());
    DCHECK_EQ(cleared_cells_head.prev(), *weak_cell);
    cleared_cells_head.set_prev(ReadOnlyRoots(isolate).undefined_value());
  } else {
    DCHECK(finalization_registry->cleared_cells().IsUndefined(isolate));
  }

  if (!weak_cell->unregister_token().IsUndefined(isolate)) {
    JSFinalizationRegistry::RemoveCellFromUnregisterTokenMap(
        isolate, finalization_registry->ptr(), weak_cell->ptr());
  }

  return weak_cell->holdings();
}

// Usage: VerifyWeakCellChain(isolate, list_head, n, cell1, cell2, ..., celln);
// verifies that list_head == cell1 and cell1, cell2, ..., celln. form a list.
void VerifyWeakCellChain(Isolate* isolate, Object list_head, int n_args, ...) {
  CHECK_GE(n_args, 0);

  va_list args;
  va_start(args, n_args);

  if (n_args == 0) {
    // Verify empty list
    CHECK(list_head.IsUndefined(isolate));
  } else {
    WeakCell current = WeakCell::cast(Object(va_arg(args, Address)));
    CHECK_EQ(current, list_head);
    CHECK(current.prev().IsUndefined(isolate));

    for (int i = 1; i < n_args; i++) {
      WeakCell next = WeakCell::cast(Object(va_arg(args, Address)));
      CHECK_EQ(current.next(), next);
      CHECK_EQ(next.prev(), current);
      current = next;
    }
    CHECK(current.next().IsUndefined(isolate));
  }
  va_end(args);
}

// Like VerifyWeakCellChain but verifies the chain created with key_list_prev
// and key_list_next instead of prev and next.
void VerifyWeakCellKeyChain(Isolate* isolate, SimpleNumberDictionary key_map,
                            Object unregister_token, int n_args, ...) {
  CHECK_GE(n_args, 0);

  va_list args;
  va_start(args, n_args);

  Object hash = unregister_token.GetHash();
  InternalIndex entry = InternalIndex::NotFound();
  if (!hash.IsUndefined(isolate)) {
    uint32_t key = Smi::ToInt(hash);
    entry = key_map.FindEntry(isolate, key);
  }
  if (n_args == 0) {
    // Verify empty list
    CHECK(entry.is_not_found());
  } else {
    CHECK(entry.is_found());
    WeakCell current = WeakCell::cast(Object(va_arg(args, Address)));
    Object list_head = key_map.ValueAt(entry);
    CHECK_EQ(current, list_head);
    CHECK(current.key_list_prev().IsUndefined(isolate));

    for (int i = 1; i < n_args; i++) {
      WeakCell next = WeakCell::cast(Object(va_arg(args, Address)));
      CHECK_EQ(current.key_list_next(), next);
      CHECK_EQ(next.key_list_prev(), current);
      current = next;
    }
    CHECK(current.key_list_next().IsUndefined(isolate));
  }
  va_end(args);
}

Handle<JSWeakRef> MakeWeakRefAndKeepDuringJob(Isolate* isolate) {
  HandleScope inner_scope(isolate);

  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());
  Handle<JSWeakRef> inner_weak_ref = ConstructJSWeakRef(js_object, isolate);
  isolate->heap()->KeepDuringJob(js_object);

  return inner_scope.CloseAndEscape(inner_weak_ref);
}

}  // namespace

TEST(TestRegister) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  // Register a weak reference and verify internal data structures.
  Handle<WeakCell> weak_cell1 =
      FinalizationRegistryRegister(finalization_registry, js_object, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 1,
                      *weak_cell1);
  CHECK(weak_cell1->key_list_prev().IsUndefined(isolate));
  CHECK(weak_cell1->key_list_next().IsUndefined(isolate));

  CHECK(finalization_registry->cleared_cells().IsUndefined(isolate));

  // No key was used during registration, key-based map stays uninitialized.
  CHECK(finalization_registry->key_map().IsUndefined(isolate));

  // Register another weak reference and verify internal data structures.
  Handle<WeakCell> weak_cell2 =
      FinalizationRegistryRegister(finalization_registry, js_object, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 2,
                      *weak_cell2, *weak_cell1);
  CHECK(weak_cell2->key_list_prev().IsUndefined(isolate));
  CHECK(weak_cell2->key_list_next().IsUndefined(isolate));

  CHECK(finalization_registry->cleared_cells().IsUndefined(isolate));
  CHECK(finalization_registry->key_map().IsUndefined(isolate));
}

TEST(TestRegisterWithKey) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<JSObject> token1 = CreateKey("token1", isolate);
  Handle<JSObject> token2 = CreateKey("token2", isolate);
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);

  // Register a weak reference with a key and verify internal data structures.
  Handle<WeakCell> weak_cell1 = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);

  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 1, *weak_cell1);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 0);
  }

  // Register another weak reference with a different key and verify internal
  // data structures.
  Handle<WeakCell> weak_cell2 = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);

  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 1, *weak_cell1);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 1, *weak_cell2);
  }

  // Register another weak reference with token1 and verify internal data
  // structures.
  Handle<WeakCell> weak_cell3 = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);

  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell3,
                           *weak_cell1);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 1, *weak_cell2);
  }
}

TEST(TestWeakCellNullify1) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<WeakCell> weak_cell1 =
      FinalizationRegistryRegister(finalization_registry, js_object, isolate);
  Handle<WeakCell> weak_cell2 =
      FinalizationRegistryRegister(finalization_registry, js_object, isolate);

  // Nullify the first WeakCell and verify internal data structures.
  NullifyWeakCell(weak_cell1, isolate);
  CHECK_EQ(finalization_registry->active_cells(), *weak_cell2);
  CHECK(weak_cell2->prev().IsUndefined(isolate));
  CHECK(weak_cell2->next().IsUndefined(isolate));
  CHECK_EQ(finalization_registry->cleared_cells(), *weak_cell1);
  CHECK(weak_cell1->prev().IsUndefined(isolate));
  CHECK(weak_cell1->next().IsUndefined(isolate));

  // Nullify the second WeakCell and verify internal data structures.
  NullifyWeakCell(weak_cell2, isolate);
  CHECK(finalization_registry->active_cells().IsUndefined(isolate));
  CHECK_EQ(finalization_registry->cleared_cells(), *weak_cell2);
  CHECK_EQ(weak_cell2->next(), *weak_cell1);
  CHECK(weak_cell2->prev().IsUndefined(isolate));
  CHECK_EQ(weak_cell1->prev(), *weak_cell2);
  CHECK(weak_cell1->next().IsUndefined(isolate));
}

TEST(TestWeakCellNullify2) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<WeakCell> weak_cell1 =
      FinalizationRegistryRegister(finalization_registry, js_object, isolate);
  Handle<WeakCell> weak_cell2 =
      FinalizationRegistryRegister(finalization_registry, js_object, isolate);

  // Like TestWeakCellNullify1 but nullify the WeakCells in opposite order.
  NullifyWeakCell(weak_cell2, isolate);
  CHECK_EQ(finalization_registry->active_cells(), *weak_cell1);
  CHECK(weak_cell1->prev().IsUndefined(isolate));
  CHECK(weak_cell1->next().IsUndefined(isolate));
  CHECK_EQ(finalization_registry->cleared_cells(), *weak_cell2);
  CHECK(weak_cell2->prev().IsUndefined(isolate));
  CHECK(weak_cell2->next().IsUndefined(isolate));

  NullifyWeakCell(weak_cell1, isolate);
  CHECK(finalization_registry->active_cells().IsUndefined(isolate));
  CHECK_EQ(finalization_registry->cleared_cells(), *weak_cell1);
  CHECK_EQ(weak_cell1->next(), *weak_cell2);
  CHECK(weak_cell1->prev().IsUndefined(isolate));
  CHECK_EQ(weak_cell2->prev(), *weak_cell1);
  CHECK(weak_cell2->next().IsUndefined(isolate));
}

TEST(TestJSFinalizationRegistryPopClearedCellHoldings1) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  Factory* factory = isolate->factory();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);

  Handle<Object> holdings1 = factory->NewStringFromAsciiChecked("holdings1");
  Handle<WeakCell> weak_cell1 = FinalizationRegistryRegister(
      finalization_registry, js_object, holdings1, undefined, isolate);
  Handle<Object> holdings2 = factory->NewStringFromAsciiChecked("holdings2");
  Handle<WeakCell> weak_cell2 = FinalizationRegistryRegister(
      finalization_registry, js_object, holdings2, undefined, isolate);
  Handle<Object> holdings3 = factory->NewStringFromAsciiChecked("holdings3");
  Handle<WeakCell> weak_cell3 = FinalizationRegistryRegister(
      finalization_registry, js_object, holdings3, undefined, isolate);

  NullifyWeakCell(weak_cell2, isolate);
  NullifyWeakCell(weak_cell3, isolate);

  CHECK(finalization_registry->NeedsCleanup());
  Object cleared1 = PopClearedCellHoldings(finalization_registry, isolate);
  CHECK_EQ(cleared1, *holdings3);
  CHECK(weak_cell3->prev().IsUndefined(isolate));
  CHECK(weak_cell3->next().IsUndefined(isolate));

  CHECK(finalization_registry->NeedsCleanup());
  Object cleared2 = PopClearedCellHoldings(finalization_registry, isolate);
  CHECK_EQ(cleared2, *holdings2);
  CHECK(weak_cell2->prev().IsUndefined(isolate));
  CHECK(weak_cell2->next().IsUndefined(isolate));

  CHECK(!finalization_registry->NeedsCleanup());

  NullifyWeakCell(weak_cell1, isolate);

  CHECK(finalization_registry->NeedsCleanup());
  Object cleared3 = PopClearedCellHoldings(finalization_registry, isolate);
  CHECK_EQ(cleared3, *holdings1);
  CHECK(weak_cell1->prev().IsUndefined(isolate));
  CHECK(weak_cell1->next().IsUndefined(isolate));

  CHECK(!finalization_registry->NeedsCleanup());
  CHECK(finalization_registry->active_cells().IsUndefined(isolate));
  CHECK(finalization_registry->cleared_cells().IsUndefined(isolate));
}

TEST(TestJSFinalizationRegistryPopClearedCellHoldings2) {
  // Test that when all WeakCells for a key are popped, the key is removed from
  // the key map.
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  Factory* factory = isolate->factory();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());
  Handle<JSObject> token1 = CreateKey("token1", isolate);

  Handle<Object> holdings1 = factory->NewStringFromAsciiChecked("holdings1");
  Handle<WeakCell> weak_cell1 = FinalizationRegistryRegister(
      finalization_registry, js_object, holdings1, token1, isolate);
  Handle<Object> holdings2 = factory->NewStringFromAsciiChecked("holdings2");
  Handle<WeakCell> weak_cell2 = FinalizationRegistryRegister(
      finalization_registry, js_object, holdings2, token1, isolate);

  NullifyWeakCell(weak_cell1, isolate);
  NullifyWeakCell(weak_cell2, isolate);

  // Nullifying doesn't affect the key chains (just moves WeakCells from
  // active_cells to cleared_cells).
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell2,
                           *weak_cell1);
  }

  Object cleared1 = PopClearedCellHoldings(finalization_registry, isolate);
  CHECK_EQ(cleared1, *holdings2);

  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 1, *weak_cell1);
  }

  Object cleared2 = PopClearedCellHoldings(finalization_registry, isolate);
  CHECK_EQ(cleared2, *holdings1);

  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 0);
  }
}

TEST(TestUnregisterActiveCells) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<JSObject> token1 = CreateKey("token1", isolate);
  Handle<JSObject> token2 = CreateKey("token2", isolate);
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);

  Handle<WeakCell> weak_cell1a = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);
  Handle<WeakCell> weak_cell1b = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);

  Handle<WeakCell> weak_cell2a = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);
  Handle<WeakCell> weak_cell2b = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 4,
                      *weak_cell2b, *weak_cell2a, *weak_cell1b, *weak_cell1a);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell1b,
                           *weak_cell1a);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 2, *weak_cell2b,
                           *weak_cell2a);
  }

  JSFinalizationRegistry::Unregister(finalization_registry, token1, isolate);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 0);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 2, *weak_cell2b,
                           *weak_cell2a);
  }

  // Both weak_cell1a and weak_cell1b removed from active_cells.
  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 2,
                      *weak_cell2b, *weak_cell2a);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
}

TEST(TestUnregisterActiveAndClearedCells) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<JSObject> token1 = CreateKey("token1", isolate);
  Handle<JSObject> token2 = CreateKey("token2", isolate);
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);

  Handle<WeakCell> weak_cell1a = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);
  Handle<WeakCell> weak_cell1b = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);

  Handle<WeakCell> weak_cell2a = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);
  Handle<WeakCell> weak_cell2b = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);

  NullifyWeakCell(weak_cell2a, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 3,
                      *weak_cell2b, *weak_cell1b, *weak_cell1a);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 1,
                      *weak_cell2a);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell1b,
                           *weak_cell1a);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 2, *weak_cell2b,
                           *weak_cell2a);
  }

  JSFinalizationRegistry::Unregister(finalization_registry, token2, isolate);

  // Both weak_cell2a and weak_cell2b removed.
  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 2,
                      *weak_cell1b, *weak_cell1a);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell1b,
                           *weak_cell1a);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 0);
  }
}

TEST(TestWeakCellUnregisterTwice) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<JSObject> token1 = CreateKey("token1", isolate);
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);

  Handle<WeakCell> weak_cell1 = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 1,
                      *weak_cell1);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 1, *weak_cell1);
  }

  JSFinalizationRegistry::Unregister(finalization_registry, token1, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 0);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 0);
  }

  JSFinalizationRegistry::Unregister(finalization_registry, token1, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 0);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 0);
  }
}

TEST(TestWeakCellUnregisterPopped) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  Factory* factory = isolate->factory();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());
  Handle<JSObject> token1 = CreateKey("token1", isolate);
  Handle<Object> holdings1 = factory->NewStringFromAsciiChecked("holdings1");
  Handle<WeakCell> weak_cell1 = FinalizationRegistryRegister(
      finalization_registry, js_object, holdings1, token1, isolate);

  NullifyWeakCell(weak_cell1, isolate);

  CHECK(finalization_registry->NeedsCleanup());
  Object cleared1 = PopClearedCellHoldings(finalization_registry, isolate);
  CHECK_EQ(cleared1, *holdings1);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 0);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 0);
  }

  JSFinalizationRegistry::Unregister(finalization_registry, token1, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 0);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 0);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 0);
  }
}

TEST(TestWeakCellUnregisterNonexistentKey) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> token1 = CreateKey("token1", isolate);

  JSFinalizationRegistry::Unregister(finalization_registry, token1, isolate);
}

TEST(TestJSWeakRef) {
  CcTest::InitializeVM();
  LocalContext context;

  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSWeakRef> weak_ref;
  {
    HandleScope inner_scope(isolate);

    Handle<JSObject> js_object =
        isolate->factory()->NewJSObject(isolate->object_function());
    // This doesn't add the target into the KeepDuringJob set.
    Handle<JSWeakRef> inner_weak_ref = ConstructJSWeakRef(js_object, isolate);

    CcTest::CollectAllGarbage();
    CHECK(!inner_weak_ref->target().IsUndefined(isolate));

    weak_ref = inner_scope.CloseAndEscape(inner_weak_ref);
  }

  CHECK(!weak_ref->target().IsUndefined(isolate));

  CcTest::CollectAllGarbage();

  CHECK(weak_ref->target().IsUndefined(isolate));
}

TEST(TestJSWeakRefIncrementalMarking) {
  if (!FLAG_incremental_marking) {
    return;
  }
  ManualGCScope manual_gc_scope;
  CcTest::InitializeVM();
  LocalContext context;

  Isolate* isolate = CcTest::i_isolate();
  Heap* heap = isolate->heap();
  HandleScope outer_scope(isolate);
  Handle<JSWeakRef> weak_ref;
  {
    HandleScope inner_scope(isolate);

    Handle<JSObject> js_object =
        isolate->factory()->NewJSObject(isolate->object_function());
    // This doesn't add the target into the KeepDuringJob set.
    Handle<JSWeakRef> inner_weak_ref = ConstructJSWeakRef(js_object, isolate);

    heap::SimulateIncrementalMarking(heap, true);
    CcTest::CollectAllGarbage();
    CHECK(!inner_weak_ref->target().IsUndefined(isolate));

    weak_ref = inner_scope.CloseAndEscape(inner_weak_ref);
  }

  CHECK(!weak_ref->target().IsUndefined(isolate));

  heap::SimulateIncrementalMarking(heap, true);
  CcTest::CollectAllGarbage();

  CHECK(weak_ref->target().IsUndefined(isolate));
}

TEST(TestJSWeakRefKeepDuringJob) {
  CcTest::InitializeVM();
  LocalContext context;

  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSWeakRef> weak_ref = MakeWeakRefAndKeepDuringJob(isolate);
  CHECK(!weak_ref->target().IsUndefined(isolate));
  CcTest::CollectAllGarbage();
  CHECK(!weak_ref->target().IsUndefined(isolate));

  // Clears the KeepDuringJob set.
  context->GetIsolate()->ClearKeptObjects();
  CcTest::CollectAllGarbage();
  CHECK(weak_ref->target().IsUndefined(isolate));

  weak_ref = MakeWeakRefAndKeepDuringJob(isolate);
  CHECK(!weak_ref->target().IsUndefined(isolate));
  CcTest::CollectAllGarbage();
  CHECK(!weak_ref->target().IsUndefined(isolate));

  // ClearKeptObjects should be called by PerformMicrotasksCheckpoint.
  CcTest::isolate()->PerformMicrotaskCheckpoint();
  CcTest::CollectAllGarbage();
  CHECK(weak_ref->target().IsUndefined(isolate));

  weak_ref = MakeWeakRefAndKeepDuringJob(isolate);
  CHECK(!weak_ref->target().IsUndefined(isolate));
  CcTest::CollectAllGarbage();
  CHECK(!weak_ref->target().IsUndefined(isolate));

  // ClearKeptObjects should be called by MicrotasksScope::PerformCheckpoint.
  v8::MicrotasksScope::PerformCheckpoint(CcTest::isolate());
  CcTest::CollectAllGarbage();
  CHECK(weak_ref->target().IsUndefined(isolate));
}

TEST(TestJSWeakRefKeepDuringJobIncrementalMarking) {
  if (!FLAG_incremental_marking) {
    return;
  }
  ManualGCScope manual_gc_scope;
  CcTest::InitializeVM();
  LocalContext context;

  Isolate* isolate = CcTest::i_isolate();
  Heap* heap = isolate->heap();
  HandleScope outer_scope(isolate);
  Handle<JSWeakRef> weak_ref = MakeWeakRefAndKeepDuringJob(isolate);

  CHECK(!weak_ref->target().IsUndefined(isolate));

  heap::SimulateIncrementalMarking(heap, true);
  CcTest::CollectAllGarbage();

  CHECK(!weak_ref->target().IsUndefined(isolate));

  // Clears the KeepDuringJob set.
  context->GetIsolate()->ClearKeptObjects();
  heap::SimulateIncrementalMarking(heap, true);
  CcTest::CollectAllGarbage();

  CHECK(weak_ref->target().IsUndefined(isolate));
}

TEST(TestRemoveUnregisterToken) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = CcTest::i_isolate();
  HandleScope outer_scope(isolate);
  Handle<JSFinalizationRegistry> finalization_registry =
      ConstructJSFinalizationRegistry(isolate);
  Handle<JSObject> js_object =
      isolate->factory()->NewJSObject(isolate->object_function());

  Handle<JSObject> token1 = CreateKey("token1", isolate);
  Handle<JSObject> token2 = CreateKey("token2", isolate);
  Handle<Object> undefined =
      handle(ReadOnlyRoots(isolate).undefined_value(), isolate);

  Handle<WeakCell> weak_cell1a = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);
  Handle<WeakCell> weak_cell1b = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token1, isolate);

  Handle<WeakCell> weak_cell2a = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);
  Handle<WeakCell> weak_cell2b = FinalizationRegistryRegister(
      finalization_registry, js_object, undefined, token2, isolate);

  NullifyWeakCell(weak_cell2a, isolate);

  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 3,
                      *weak_cell2b, *weak_cell1b, *weak_cell1a);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 1,
                      *weak_cell2a);
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell1b,
                           *weak_cell1a);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 2, *weak_cell2b,
                           *weak_cell2a);
  }

  finalization_registry->RemoveUnregisterToken(
      JSReceiver::cast(*token2), isolate,
      [undefined](WeakCell matched_cell) {
        matched_cell.set_unregister_token(*undefined);
      },
      [](HeapObject, ObjectSlot, Object) {});

  // Both weak_cell2a and weak_cell2b remain on the weak cell chains.
  VerifyWeakCellChain(isolate, finalization_registry->active_cells(), 3,
                      *weak_cell2b, *weak_cell1b, *weak_cell1a);
  VerifyWeakCellChain(isolate, finalization_registry->cleared_cells(), 1,
                      *weak_cell2a);

  // But both weak_cell2a and weak_cell2b are removed from the key chain.
  {
    SimpleNumberDictionary key_map =
        SimpleNumberDictionary::cast(finalization_registry->key_map());
    VerifyWeakCellKeyChain(isolate, key_map, *token1, 2, *weak_cell1b,
                           *weak_cell1a);
    VerifyWeakCellKeyChain(isolate, key_map, *token2, 0);
  }
}

TEST(JSWeakRefScavengedInWorklist) {
  if (!FLAG_incremental_marking || FLAG_single_generation) {
    return;
  }

  ManualGCScope manual_gc_scope;
  CcTest::InitializeVM();
  Isolate* isolate = CcTest::i_isolate();
  Heap* heap = isolate->heap();

  {
    HandleScope outer_scope(isolate);
    Handle<JSWeakRef> weak_ref;

    // Make a WeakRef that points to a target, both of which become unreachable.
    {
      HandleScope inner_scope(isolate);
      Handle<JSObject> js_object =
          isolate->factory()->NewJSObject(isolate->object_function());
      Handle<JSWeakRef> inner_weak_ref = ConstructJSWeakRef(js_object, isolate);
      CHECK(Heap::InYoungGeneration(*js_object));
      CHECK(Heap::InYoungGeneration(*inner_weak_ref));

      weak_ref = inner_scope.CloseAndEscape(inner_weak_ref);
    }

    // Store weak_ref in Global such that it is part of the root set when
    // starting incremental marking.
    v8::Global<Value> global_weak_ref(
        CcTest::isolate(), Utils::ToLocal(Handle<Object>::cast(weak_ref)));

    // Do marking. This puts the WeakRef above into the js_weak_refs worklist
    // since its target isn't marked.
    CHECK(
        heap->mark_compact_collector()->weak_objects()->js_weak_refs.IsEmpty());
    heap::SimulateIncrementalMarking(heap, true);
    CHECK(!heap->mark_compact_collector()
               ->weak_objects()
               ->js_weak_refs.IsEmpty());
  }

  // Now collect both weak_ref and its target. The worklist should be empty.
  CcTest::CollectGarbage(NEW_SPACE);
  CHECK(heap->mark_compact_collector()->weak_objects()->js_weak_refs.IsEmpty());

  // The mark-compactor shouldn't see zapped WeakRefs in the worklist.
  CcTest::CollectAllGarbage();
}

TEST(JSWeakRefTenuredInWorklist) {
  if (!FLAG_incremental_marking || FLAG_single_generation) {
    return;
  }

  ManualGCScope manual_gc_scope;
  CcTest::InitializeVM();
  Isolate* isolate = CcTest::i_isolate();
  Heap* heap = isolate->heap();

  HandleScope outer_scope(isolate);
  Handle<JSWeakRef> weak_ref;

  // Make a WeakRef that points to a target. The target becomes unreachable.
  {
    HandleScope inner_scope(isolate);
    Handle<JSObject> js_object =
        isolate->factory()->NewJSObject(isolate->object_function());
    Handle<JSWeakRef> inner_weak_ref = ConstructJSWeakRef(js_object, isolate);
    CHECK(Heap::InYoungGeneration(*js_object));
    CHECK(Heap::InYoungGeneration(*inner_weak_ref));

    weak_ref = inner_scope.CloseAndEscape(inner_weak_ref);
  }
  // Store weak_ref such that it is part of the root set when starting
  // incremental marking.
  v8::Global<Value> global_weak_ref(
      CcTest::isolate(), Utils::ToLocal(Handle<Object>::cast(weak_ref)));
  JSWeakRef old_weak_ref_location = *weak_ref;

  // Do marking. This puts the WeakRef above into the js_weak_refs worklist
  // since its target isn't marked.
  CHECK(heap->mark_compact_collector()->weak_objects()->js_weak_refs.IsEmpty());
  heap::SimulateIncrementalMarking(heap, true);
  CHECK(
      !heap->mark_compact_collector()->weak_objects()->js_weak_refs.IsEmpty());

  // Now collect weak_ref's target. We still have a Handle to weak_ref, so it is
  // moved and remains on the worklist.
  CcTest::CollectGarbage(NEW_SPACE);
  JSWeakRef new_weak_ref_location = *weak_ref;
  CHECK_NE(old_weak_ref_location, new_weak_ref_location);
  CHECK(
      !heap->mark_compact_collector()->weak_objects()->js_weak_refs.IsEmpty());

  // The mark-compactor should see the moved WeakRef in the worklist.
  CcTest::CollectAllGarbage();
  CHECK(heap->mark_compact_collector()->weak_objects()->js_weak_refs.IsEmpty());
  CHECK(weak_ref->target().IsUndefined(isolate));
}

TEST(UnregisterTokenHeapVerifier) {
  if (!FLAG_incremental_marking) return;
  ManualGCScope manual_gc_scope;
#ifdef VERIFY_HEAP
  FLAG_verify_heap = true;
#endif

  CcTest::InitializeVM();
  v8::Isolate* isolate = CcTest::isolate();
  Heap* heap = CcTest::heap();
  v8::HandleScope outer_scope(isolate);

  {
    // Make a new FinalizationRegistry and register an object with an unregister
    // token that's unreachable after the IIFE returns.
    v8::HandleScope scope(isolate);
    CompileRun(
        "var token = {}; "
        "var registry = new FinalizationRegistry(function ()  {}); "
        "(function () { "
        "  let o = {}; "
        "  registry.register(o, {}, token); "
        "})();");
  }

  // GC so the WeakCell corresponding to o is moved from the active_cells to
  // cleared_cells.
  CcTest::CollectAllGarbage();
  CcTest::CollectAllGarbage();

  {
    // Override the unregister token to make the original object collectible.
    v8::HandleScope scope(isolate);
    CompileRun("token = 0;");
  }

  heap::SimulateIncrementalMarking(heap, true);

  // Pump message loop to run the finalizer task, then the incremental marking
  // task. The finalizer task will pop the WeakCell from the cleared list. This
  // should make the unregister_token slot undefined. That slot is iterated as a
  // custom weak pointer, so if it is not made undefined, the verifier as part
  // of the incremental marking task will crash.
  EmptyMessageQueues(isolate);
}

}  // namespace internal
}  // namespace v8
