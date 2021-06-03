// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --expose-gc --noincremental-marking

let cleanup_called = false;
let cleanup = function(holdings) {
  assertFalse(cleanup_called);
  let holdings_list = [];
  holdings_list.push(holdings);
  assertEquals(1, holdings_list.length);
  assertEquals("holdings", holdings_list[0]);
  cleanup_called = true;
}

let fg = new FinalizationRegistry(cleanup);
let weak_ref;
(function() {
  let o = {};
  weak_ref = new WeakRef(o);
  fg.register(o, "holdings");
})();

// Since the WeakRef was created during this turn, it is not cleared by GC. The
// pointer inside the FinalizationRegistry is not cleared either, since the WeakRef
// keeps the target object alive.
gc();
(function() {
  assertNotEquals(undefined, weak_ref.deref());
})();

// Trigger gc in next task
setTimeout(() => {
  gc();

  // Check that cleanup callback was called in a follow up task
  setTimeout(() => {
    assertTrue(cleanup_called);
    assertEquals(undefined, weak_ref.deref());
  }, 0);
}, 0);
