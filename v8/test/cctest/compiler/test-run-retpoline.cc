// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/codegen/assembler-inl.h"
#include "src/codegen/code-stub-assembler.h"
#include "src/codegen/macro-assembler.h"

#include "test/cctest/cctest.h"
#include "test/cctest/compiler/code-assembler-tester.h"
#include "test/cctest/compiler/function-tester.h"

namespace v8 {
namespace internal {
namespace compiler {
namespace test_run_retpoline {

#define __ assembler.

namespace {

// Function that takes a number of pointer-sized integer arguments, calculates a
// weighted sum of them and returns it.
Handle<Code> BuildCallee(Isolate* isolate, CallDescriptor* call_descriptor) {
  CodeAssemblerTester tester(isolate, call_descriptor, "callee");
  CodeStubAssembler assembler(tester.state());
  int param_slots = static_cast<int>(call_descriptor->ParameterSlotCount());
  TNode<IntPtrT> sum = __ IntPtrConstant(0);
  for (int i = 0; i < param_slots; ++i) {
    TNode<IntPtrT> product = __ Signed(__ IntPtrMul(
        __ UncheckedParameter<IntPtrT>(i), __ IntPtrConstant(i + 1)));
    sum = __ IntPtrAdd(sum, product);
  }
  __ Return(sum);
  return tester.GenerateCodeCloseAndEscape();
}

// Function that tail-calls another function with a number of pointer-sized
// integer arguments.
Handle<Code> BuildCaller(Isolate* isolate, CallDescriptor* call_descriptor,
                         CallDescriptor* callee_descriptor, bool tail) {
  CodeAssemblerTester tester(isolate, call_descriptor, "caller");
  CodeStubAssembler assembler(tester.state());
  std::vector<Node*> params;
  // The first parameter is always the callee.
  Handle<Code> callee = BuildCallee(isolate, callee_descriptor);
  // defeat the instruction selector.
  CodeStubAssembler::TVariable<Code> target_var(&assembler);
  CodeStubAssembler::Label t(&assembler), f(&assembler),
      end(&assembler, &target_var);
  __ Branch(__ Int32Constant(0), &t, &f);
  __ BIND(&t);
  target_var = __ HeapConstant(callee);
  __ Goto(&end);
  __ BIND(&f);
  target_var = __ HeapConstant(callee);
  __ Goto(&end);
  __ BIND(&end);
  params.push_back(target_var.value());

  int param_slots = static_cast<int>(callee_descriptor->ParameterSlotCount());
  for (int i = 0; i < param_slots; ++i) {
    params.push_back(__ IntPtrConstant(i));
  }
  DCHECK_EQ(param_slots + 1, params.size());
  if (tail) {
    tester.raw_assembler_for_testing()->TailCallN(
        callee_descriptor, param_slots + 1, params.data());
  } else {
    Node* result = tester.raw_assembler_for_testing()->CallN(
        callee_descriptor, param_slots + 1, params.data());
    __ Return(__ UncheckedCast<IntPtrT>(result));
  }
  return tester.GenerateCodeCloseAndEscape();
}

// Setup function, which calls "caller".
Handle<Code> BuildSetupFunction(Isolate* isolate,
                                CallDescriptor* caller_descriptor,
                                CallDescriptor* callee_descriptor, bool tail) {
  CodeAssemblerTester tester(isolate, 0);
  CodeStubAssembler assembler(tester.state());
  std::vector<Node*> params;
  // The first parameter is always the callee.
  params.push_back(__ HeapConstant(
      BuildCaller(isolate, caller_descriptor, callee_descriptor, tail)));
  // Set up arguments for "Caller".
  int param_slots = static_cast<int>(caller_descriptor->ParameterSlotCount());
  for (int i = 0; i < param_slots; ++i) {
    // Use values that are different from the ones we will pass to this
    // function's callee later.
    params.push_back(__ IntPtrConstant(i + 42));
  }
  DCHECK_EQ(param_slots + 1, params.size());
  TNode<IntPtrT> intptr_result =
      __ UncheckedCast<IntPtrT>(tester.raw_assembler_for_testing()->CallN(
          caller_descriptor, param_slots + 1, params.data()));
  __ Return(__ SmiTag(intptr_result));
  return tester.GenerateCodeCloseAndEscape();
}

CallDescriptor* CreateDescriptorForStackArguments(Zone* zone, int param_slots) {
  LocationSignature::Builder locations(zone, 1,
                                       static_cast<size_t>(param_slots));

  locations.AddReturn(LinkageLocation::ForRegister(kReturnRegister0.code(),
                                                   MachineType::IntPtr()));

  for (int i = 0; i < param_slots; ++i) {
    locations.AddParam(LinkageLocation::ForCallerFrameSlot(
        i - param_slots, MachineType::IntPtr()));
  }

  return zone->New<CallDescriptor>(
      CallDescriptor::kCallCodeObject,  // kind
      MachineType::AnyTagged(),         // target MachineType
      LinkageLocation::ForAnyRegister(
          MachineType::AnyTagged()),  // target location
      locations.Build(),              // location_sig
      param_slots,                    // stack parameter slots
      Operator::kNoProperties,        // properties
      kNoCalleeSaved,                 // callee-saved registers
      kNoCalleeSaved,                 // callee-saved fp
      CallDescriptor::kRetpoline);    // flags
}

// Test a tail call from a caller with n parameters to a callee with m
// parameters. All parameters are pointer-sized.
void TestHelper(int n, int m, bool tail) {
  HandleAndZoneScope scope;
  Isolate* isolate = scope.main_isolate();
  CanonicalHandleScope canonical(isolate);
  Zone* zone = scope.main_zone();
  CallDescriptor* caller_descriptor =
      CreateDescriptorForStackArguments(zone, n);
  CallDescriptor* callee_descriptor =
      CreateDescriptorForStackArguments(zone, m);
  Handle<Code> setup =
      BuildSetupFunction(isolate, caller_descriptor, callee_descriptor, tail);
  FunctionTester ft(setup, 0);
  Handle<Object> result = ft.Call().ToHandleChecked();
  int expected = 0;
  for (int i = 0; i < m; ++i) expected += (i + 1) * i;
  CHECK_EQ(expected, Handle<Smi>::cast(result)->value());
}

}  // namespace

#undef __

TEST(RetpolineOddEven) {
  TestHelper(1, 0, false);
  TestHelper(1, 2, false);
  TestHelper(3, 2, false);
  TestHelper(3, 4, false);
}

TEST(RetpolineOddEvenTail) {
  TestHelper(1, 0, true);
  TestHelper(1, 2, true);
  TestHelper(3, 2, true);
  TestHelper(3, 4, true);
}

TEST(RetpolineOddOdd) {
  TestHelper(1, 1, false);
  TestHelper(1, 3, false);
  TestHelper(3, 1, false);
  TestHelper(3, 3, false);
}

TEST(RetpolineOddOddTail) {
  TestHelper(1, 1, true);
  TestHelper(1, 3, true);
  TestHelper(3, 1, true);
  TestHelper(3, 3, true);
}

TEST(RetpolineEvenEven) {
  TestHelper(0, 0, false);
  TestHelper(0, 2, false);
  TestHelper(2, 0, false);
  TestHelper(2, 2, false);
}

TEST(RetpolineEvenEvenTail) {
  TestHelper(0, 0, true);
  TestHelper(0, 2, true);
  TestHelper(2, 0, true);
  TestHelper(2, 2, true);
}

TEST(RetpolineEvenOdd) {
  TestHelper(0, 1, false);
  TestHelper(0, 3, false);
  TestHelper(2, 1, false);
  TestHelper(2, 3, false);
}

TEST(RetpolineEvenOddTail) {
  TestHelper(0, 1, true);
  TestHelper(0, 3, true);
  TestHelper(2, 1, true);
  TestHelper(2, 3, true);
}

}  // namespace test_run_retpoline
}  // namespace compiler
}  // namespace internal
}  // namespace v8
