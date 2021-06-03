// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/v8.h"
#include "src/flags/flags.h"
#include "src/trap-handler/trap-handler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

int main(int argc, char** argv) {
  // Don't catch SEH exceptions and continue as the following tests might hang
  // in an broken environment on windows.
  testing::GTEST_FLAG(catch_exceptions) = false;
  testing::InitGoogleMock(&argc, argv);
  v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
  v8::V8::InitializeExternalStartupData(argv[0]);
  if (V8_TRAP_HANDLER_SUPPORTED && i::FLAG_wasm_trap_handler) {
    constexpr bool use_default_trap_handler = true;
    if (!v8::V8::EnableWebAssemblyTrapHandler(use_default_trap_handler)) {
      FATAL("Could not register trap handler");
    }
  }

  return RUN_ALL_TESTS();
}
