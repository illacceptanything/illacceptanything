// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --experimental-wasm-type-reflection --experimental-wasm-gc

load("test/mjsunit/wasm/wasm-module-builder.js");

(function Test1() {
  var exporting_instance = (function () {
    var builder = new WasmModuleBuilder();

    builder.addFunction("addition", kSig_i_ii)
      .addBody([kExprLocalGet, 0, kExprLocalGet, 1, kExprI32Add])
      .exportFunc();

    return builder.instantiate({});
  })();

  var instance = (function () {
    var builder = new WasmModuleBuilder();

    var sig_index = builder.addType(kSig_i_ii);

    var imported_js_api_function_index =
      builder.addImport("imports", "js_api_mul", sig_index);

    var imported_js_function_index =
      builder.addImport("imports", "js_add", sig_index);

    var imported_wasm_function_index =
      builder.addImport("imports", "wasm_add", sig_index);


    var locally_defined_function =
      builder.addFunction("sub", sig_index)
        .addBody([kExprLocalGet, 0, kExprLocalGet, 1, kExprI32Sub])
        .exportFunc();

    builder.addFunction("main", makeSig(
      [wasmRefType(sig_index), kWasmI32, kWasmI32], [kWasmI32]))
      .addBody([kExprLocalGet, 1, kExprLocalGet, 2, kExprLocalGet, 0,
                kExprCallRef])
      .exportFunc();

    builder.addFunction("test_local", kSig_i_v)
      .addBody([kExprI32Const, 55, kExprI32Const, 42,
                kExprRefFunc, locally_defined_function.index, kExprCallRef])
      .exportFunc();

    builder.addFunction("test_js_import", kSig_i_v)
      .addBody([kExprI32Const, 15, kExprI32Const, 42,
                kExprRefFunc, imported_js_function_index, kExprCallRef])
      .exportFunc();

    builder.addFunction("test_wasm_import", kSig_i_v)
      .addBody([kExprI32Const, 15, kExprI32Const, 42,
                kExprRefFunc, imported_wasm_function_index, kExprCallRef])
      .exportFunc();

    builder.addFunction("test_js_api_import", kSig_i_v)
      .addBody([kExprI32Const, 3, kExprI32Const, 7,
                kExprRefFunc, imported_js_api_function_index,
                kExprCallRef])
      .exportFunc();

    builder.addExport("reexported_js_function", imported_js_function_index);

    // Just to make these functions eligible for call_ref.
    builder.addDeclarativeElementSegment([imported_wasm_function_index,
                                          imported_js_api_function_index]);

    return builder.instantiate({imports: {
      js_add: function(a, b) { return a + b; },
      wasm_add: exporting_instance.exports.addition,
      js_api_mul: new WebAssembly.Function(
          {parameters:['i32', 'i32'], results: ['i32']},
          function(a, b) { return a * b; })
    }});
  })();

  // Check that the modules exist.
  assertTrue(!!exporting_instance);
  assertTrue(!!instance);

  print("--locally defined func--");
  assertEquals(13, instance.exports.test_local());
  print("--locally defined exported func--")
  assertEquals(5, instance.exports.main(instance.exports.sub, 12, 7));

  print("--imported js func--");
  assertEquals(57, instance.exports.test_js_import());
  print("--imported and reexported js func--")
  assertEquals(19, instance.exports.main(
    instance.exports.reexported_js_function, 12, 7));

  print("--imported function from another module--");
  assertEquals(57, instance.exports.test_wasm_import());
  print("--not imported function defined in another module--");
  assertEquals(19, instance.exports.main(
    exporting_instance.exports.addition, 12, 7));

  print("--imported WebAssembly.Function--")
  assertEquals(21, instance.exports.test_js_api_import());
  print("--not imported WebAssembly.Function--")
  assertEquals(-5, instance.exports.main(
    new WebAssembly.Function(
      {parameters:['i32', 'i32'], results: ['i32']},
      function(a, b) { return a - b; }),
    10, 15));
  print("--not imported WebAssembly.Function, arity mismatch--")
  assertEquals(100, instance.exports.main(
    new WebAssembly.Function(
      {parameters:['i32', 'i32'], results: ['i32']},
      function(a) { return a * a; }),
    10, 15));
})();

(function TestFromJSSlowPath() {
  var builder = new WasmModuleBuilder();
  var sig_index = builder.addType(kSig_i_i);

  builder.addFunction("main", makeSig(
      [wasmRefType(sig_index), kWasmI32], [kWasmI32]))
      .addBody([kExprLocalGet, 1, kExprLocalGet, 0, kExprCallRef])
      .exportFunc();

  var instance = builder.instantiate({});

  var fun = new WebAssembly.Function(
      { parameters: ['i32'], results: ['i32'] }, (a) => undefined);
  // {undefined} is converted to 0.
  assertEquals(0, instance.exports.main(fun, 1000));
})();
