// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "src/api/api-inl.h"
#include "src/base/utils/random-number-generator.h"
#include "src/builtins/builtins-promise-gen.h"
#include "src/builtins/builtins-promise.h"
#include "src/builtins/builtins-string-gen.h"
#include "src/codegen/code-factory.h"
#include "src/codegen/code-stub-assembler.h"
#include "src/codegen/interface-descriptors-inl.h"
#include "src/compiler/node.h"
#include "src/debug/debug.h"
#include "src/execution/isolate.h"
#include "src/heap/heap-inl.h"
#include "src/numbers/hash-seed-inl.h"
#include "src/objects/hash-table-inl.h"
#include "src/objects/heap-number-inl.h"
#include "src/objects/js-array-buffer-inl.h"
#include "src/objects/js-array-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/ordered-hash-table-inl.h"
#include "src/objects/promise-inl.h"
#include "src/objects/smi.h"
#include "src/objects/struct-inl.h"
#include "src/objects/transitions-inl.h"
#include "src/strings/char-predicates.h"
#include "test/cctest/cctest-utils.h"
#include "test/cctest/compiler/code-assembler-tester.h"
#include "test/cctest/compiler/function-tester.h"

namespace v8 {
namespace internal {
namespace compiler {

namespace {

using Label = CodeAssemblerLabel;
template <class T>
using TVariable = TypedCodeAssemblerVariable<T>;
using PromiseResolvingFunctions = TorqueStructPromiseResolvingFunctions;

intptr_t sum10(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4,
               intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8,
               intptr_t a9) {
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

static int sum3(int a0, int a1, int a2) { return a0 + a1 + a2; }

}  // namespace

TEST(CallCFunction) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());

  {
    const TNode<ExternalReference> fun_constant = m.ExternalConstant(
        ExternalReference::Create(reinterpret_cast<Address>(sum10)));

    MachineType type_intptr = MachineType::IntPtr();

    TNode<IntPtrT> const result = m.UncheckedCast<IntPtrT>(
        m.CallCFunction(fun_constant, type_intptr,
                        std::make_pair(type_intptr, m.IntPtrConstant(0)),
                        std::make_pair(type_intptr, m.IntPtrConstant(1)),
                        std::make_pair(type_intptr, m.IntPtrConstant(2)),
                        std::make_pair(type_intptr, m.IntPtrConstant(3)),
                        std::make_pair(type_intptr, m.IntPtrConstant(4)),
                        std::make_pair(type_intptr, m.IntPtrConstant(5)),
                        std::make_pair(type_intptr, m.IntPtrConstant(6)),
                        std::make_pair(type_intptr, m.IntPtrConstant(7)),
                        std::make_pair(type_intptr, m.IntPtrConstant(8)),
                        std::make_pair(type_intptr, m.IntPtrConstant(9))));
    m.Return(m.SmiTag(result));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> result = ft.Call().ToHandleChecked();
  CHECK_EQ(45, Handle<Smi>::cast(result)->value());
}

TEST(CallCFunctionWithCallerSavedRegisters) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());

  {
    const TNode<ExternalReference> fun_constant = m.ExternalConstant(
        ExternalReference::Create(reinterpret_cast<Address>(sum3)));

    MachineType type_intptr = MachineType::IntPtr();

    TNode<IntPtrT> const result =
        m.UncheckedCast<IntPtrT>(m.CallCFunctionWithCallerSavedRegisters(
            fun_constant, type_intptr, SaveFPRegsMode::kSave,
            std::make_pair(type_intptr, m.IntPtrConstant(0)),
            std::make_pair(type_intptr, m.IntPtrConstant(1)),
            std::make_pair(type_intptr, m.IntPtrConstant(2))));
    m.Return(m.SmiTag(result));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> result = ft.Call().ToHandleChecked();
  CHECK_EQ(3, Handle<Smi>::cast(result)->value());
}

TEST(NumberToString) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* factory = isolate->factory();

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  {
    auto input = m.Parameter<Number>(1);

    Label bailout(&m);
    m.Return(m.NumberToString(input, &bailout));

    m.BIND(&bailout);
    m.Return(m.UndefinedConstant());
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  // clang-format off
  double inputs[] = {
     1, 2, 42, 153, -1, -100, 0, 51095154, -1241950,
     std::nan("-1"), std::nan("1"), std::nan("2"),
    -std::numeric_limits<double>::infinity(),
     std::numeric_limits<double>::infinity(),
    -0.0, -0.001, -0.5, -0.999, -1.0,
     0.0,  0.001,  0.5,  0.999,  1.0,
    -2147483647.9, -2147483648.0, -2147483648.5, -2147483648.9,  // SmiMin.
     2147483646.9,  2147483647.0,  2147483647.5,  2147483647.9,  // SmiMax.
    -4294967295.9, -4294967296.0, -4294967296.5, -4294967297.0,  // - 2^32.
     4294967295.9,  4294967296.0,  4294967296.5,  4294967297.0,  //   2^32.
  };
  // clang-format on

  const int kFullCacheSize = isolate->heap()->MaxNumberToStringCacheSize();
  const int test_count = arraysize(inputs);
  for (int i = 0; i < test_count; i++) {
    int cache_length_before_addition = factory->number_string_cache()->length();
    Handle<Object> input = factory->NewNumber(inputs[i]);
    Handle<String> expected = factory->NumberToString(input);

    Handle<Object> result = ft.Call(input).ToHandleChecked();
    if (result->IsUndefined(isolate)) {
      // Query may fail if cache was resized, in which case the entry is not
      // added to the cache.
      CHECK_LT(cache_length_before_addition, kFullCacheSize);
      CHECK_EQ(factory->number_string_cache()->length(), kFullCacheSize);
      expected = factory->NumberToString(input);
      result = ft.Call(input).ToHandleChecked();
    }
    CHECK(!result->IsUndefined(isolate));
    CHECK_EQ(*expected, *result);
  }
}

namespace {

void CheckToUint32Result(uint32_t expected, Handle<Object> result) {
  const int64_t result_int64 = NumberToInt64(*result);
  const uint32_t result_uint32 = NumberToUint32(*result);

  CHECK_EQ(static_cast<int64_t>(result_uint32), result_int64);
  CHECK_EQ(expected, result_uint32);

  // Ensure that the result is normalized to a Smi, i.e. a HeapNumber is only
  // returned if the result is not within Smi range.
  const bool expected_fits_into_intptr =
      static_cast<int64_t>(expected) <=
      static_cast<int64_t>(std::numeric_limits<intptr_t>::max());
  if (expected_fits_into_intptr &&
      Smi::IsValid(static_cast<intptr_t>(expected))) {
    CHECK(result->IsSmi());
  } else {
    CHECK(result->IsHeapNumber());
  }
}

}  // namespace

TEST(ToUint32) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* factory = isolate->factory();

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  const int kContextOffset = 3;
  auto context = m.Parameter<Context>(kNumParams + kContextOffset);
  auto input = m.Parameter<Object>(1);
  m.Return(m.ToUint32(context, input));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  // clang-format off
  double inputs[] = {
     std::nan("-1"), std::nan("1"), std::nan("2"),
    -std::numeric_limits<double>::infinity(),
     std::numeric_limits<double>::infinity(),
    -0.0, -0.001, -0.5, -0.999, -1.0,
     0.0,  0.001,  0.5,  0.999,  1.0,
    -2147483647.9, -2147483648.0, -2147483648.5, -2147483648.9,  // SmiMin.
     2147483646.9,  2147483647.0,  2147483647.5,  2147483647.9,  // SmiMax.
    -4294967295.9, -4294967296.0, -4294967296.5, -4294967297.0,  // - 2^32.
     4294967295.9,  4294967296.0,  4294967296.5,  4294967297.0,  //   2^32.
  };

  uint32_t expectations[] = {
     0, 0, 0,
     0,
     0,
     0, 0, 0, 0, 4294967295,
     0, 0, 0, 0, 1,
     2147483649, 2147483648, 2147483648, 2147483648,
     2147483646, 2147483647, 2147483647, 2147483647,
     1, 0, 0, 4294967295,
     4294967295, 0, 0, 1,
  };
  // clang-format on

  STATIC_ASSERT(arraysize(inputs) == arraysize(expectations));

  const int test_count = arraysize(inputs);
  for (int i = 0; i < test_count; i++) {
    Handle<Object> input_obj = factory->NewNumber(inputs[i]);
    Handle<HeapNumber> input_num;

    // Check with Smi input.
    if (input_obj->IsSmi()) {
      Handle<Smi> input_smi = Handle<Smi>::cast(input_obj);
      Handle<Object> result = ft.Call(input_smi).ToHandleChecked();
      CheckToUint32Result(expectations[i], result);
      input_num = factory->NewHeapNumber(inputs[i]);
    } else {
      input_num = Handle<HeapNumber>::cast(input_obj);
    }

    // Check with HeapNumber input.
    {
      CHECK(input_num->IsHeapNumber());
      Handle<Object> result = ft.Call(input_num).ToHandleChecked();
      CheckToUint32Result(expectations[i], result);
    }
  }

  // A couple of final cases for ToNumber conversions.
  CheckToUint32Result(0, ft.Call(factory->undefined_value()).ToHandleChecked());
  CheckToUint32Result(0, ft.Call(factory->null_value()).ToHandleChecked());
  CheckToUint32Result(0, ft.Call(factory->false_value()).ToHandleChecked());
  CheckToUint32Result(1, ft.Call(factory->true_value()).ToHandleChecked());
  CheckToUint32Result(
      42,
      ft.Call(factory->NewStringFromAsciiChecked("0x2A")).ToHandleChecked());

  ft.CheckThrows(factory->match_symbol());
}

namespace {
void IsValidPositiveSmiCase(Isolate* isolate, intptr_t value) {
  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);

  CodeStubAssembler m(asm_tester.state());
  m.Return(
      m.SelectBooleanConstant(m.IsValidPositiveSmi(m.IntPtrConstant(value))));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  MaybeHandle<Object> maybe_handle = ft.Call();

  bool expected = i::PlatformSmiTagging::IsValidSmi(value) && (value >= 0);
  if (expected) {
    CHECK(maybe_handle.ToHandleChecked()->IsTrue(isolate));
  } else {
    CHECK(maybe_handle.ToHandleChecked()->IsFalse(isolate));
  }
}
}  // namespace

TEST(IsValidPositiveSmi) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  IsValidPositiveSmiCase(isolate, -1);
  IsValidPositiveSmiCase(isolate, 0);
  IsValidPositiveSmiCase(isolate, 1);

  IsValidPositiveSmiCase(isolate, 0x3FFFFFFFU);
  IsValidPositiveSmiCase(isolate, 0xC0000000U);
  IsValidPositiveSmiCase(isolate, 0x40000000U);
  IsValidPositiveSmiCase(isolate, 0xBFFFFFFFU);

  using int32_limits = std::numeric_limits<int32_t>;
  IsValidPositiveSmiCase(isolate, int32_limits::max());
  IsValidPositiveSmiCase(isolate, int32_limits::min());
#if V8_TARGET_ARCH_64_BIT
  IsValidPositiveSmiCase(isolate,
                         static_cast<intptr_t>(int32_limits::max()) + 1);
  IsValidPositiveSmiCase(isolate,
                         static_cast<intptr_t>(int32_limits::min()) - 1);
#endif
}

TEST(ConvertToRelativeIndex) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  enum Result { kFound, kNotFound };
  {
    auto index = m.Parameter<Number>(1);
    auto length_number = m.Parameter<Number>(2);
    auto expected_relative_index = m.Parameter<Number>(3);

    TNode<UintPtrT> length = m.ChangeUintPtrNumberToUintPtr(length_number);
    TNode<UintPtrT> expected =
        m.ChangeUintPtrNumberToUintPtr(expected_relative_index);

    TNode<UintPtrT> result = m.ConvertToRelativeIndex(index, length);

    m.Return(m.SelectBooleanConstant(m.WordEqual(result, expected)));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  const double kMaxSmi = static_cast<double>(kSmiMaxValue);
  const double kMaxInt32 =
      static_cast<double>(std::numeric_limits<int32_t>::max());
  const double kMaxUInt32 =
      static_cast<double>(std::numeric_limits<uint32_t>::max());
  const double kMaxUIntPtr =
      static_cast<double>(std::numeric_limits<uintptr_t>::max());

  struct {
    double index;
    double length;
    double expected_result;
  } test_cases[] = {
      // Simple Smi-range cases.
      {0, 0, 0},
      {0, 42, 0},
      {5, 42, 5},
      {100, 42, 42},
      {-10, 153, 153 - 10},
      {-200, 153, 0},
      // Beyond Smi-range index cases.
      {0, kMaxSmi, 0},
      {-153, kMaxSmi, kMaxSmi - 153},
      {kMaxSmi + 153, kMaxSmi, kMaxSmi},
      {kMaxSmi * 33, kMaxSmi, kMaxSmi},
      {-kMaxSmi, kMaxSmi, 0},
      {-kMaxSmi - 1, kMaxSmi, 0},
      {-kMaxSmi - 153, kMaxSmi, 0},
      {-kMaxSmi * 33, kMaxSmi, 0},
      {-std::numeric_limits<double>::infinity(), 153, 0},
      {std::numeric_limits<double>::infinity(), 424242, 424242},
      // Beyond Smi-range length cases.
      {kMaxSmi + 2, kMaxSmi + 1, kMaxSmi + 1},
      {-kMaxSmi + 2, kMaxSmi + 1, 3},
      {kMaxInt32 + 1, kMaxInt32, kMaxInt32},
      {-kMaxInt32 + 1, kMaxInt32, 1},
      {kMaxUInt32 + 1, kMaxUInt32, kMaxUInt32},
      {-42, kMaxUInt32, kMaxUInt32 - 42},
      {-kMaxUInt32 - 1, kMaxUInt32, 0},
      {-kMaxUInt32, kMaxUInt32, 0},
      {-kMaxUInt32 + 1, kMaxUInt32, 1},
      {-kMaxUInt32 + 5, kMaxUInt32, 5},
      {-kMaxUInt32 + 5, kMaxUInt32 + 1, 6},
      {-kMaxSmi * 33, kMaxSmi * 153, kMaxSmi * (153 - 33)},
      {0, kMaxSafeInteger, 0},
      {kMaxSmi, kMaxSafeInteger, kMaxSmi},
      {kMaxSmi * 153, kMaxSafeInteger, kMaxSmi * 153},
      {-10, kMaxSafeInteger, kMaxSafeInteger - 10},
      {-kMaxSafeInteger, kMaxSafeInteger, 0},
      {-kMaxSafeInteger + 1, kMaxSafeInteger, 1},
      {-kMaxSafeInteger + 42, kMaxSafeInteger, 42},
      {kMaxSafeInteger - 153, kMaxSafeInteger, kMaxSafeInteger - 153},
      {kMaxSafeInteger - 1, kMaxSafeInteger, kMaxSafeInteger - 1},
      {kMaxSafeInteger, kMaxSafeInteger, kMaxSafeInteger},
      {kMaxSafeInteger + 1, kMaxSafeInteger, kMaxSafeInteger},
      {kMaxSafeInteger + 42, kMaxSafeInteger, kMaxSafeInteger},
      {kMaxSafeInteger * 11, kMaxSafeInteger, kMaxSafeInteger},
  };

  Factory* factory = isolate->factory();
  for (size_t i = 0; i < arraysize(test_cases); i++) {
    if (test_cases[i].length > kMaxUIntPtr) {
      // Test cases where length does not fit into uintptr are not valid, so
      // skip them instead of ifdef'ing the test cases above.
      continue;
    }
    Handle<Object> index = factory->NewNumber(test_cases[i].index);
    Handle<Object> length = factory->NewNumber(test_cases[i].length);
    Handle<Object> expected = factory->NewNumber(test_cases[i].expected_result);

    ft.CheckTrue(index, length, expected);
  }
}

TEST(FixedArrayAccessSmiIndex) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate);
  CodeStubAssembler m(asm_tester.state());
  Handle<FixedArray> array = isolate->factory()->NewFixedArray(5);
  array->set(4, Smi::FromInt(733));
  m.Return(m.LoadFixedArrayElement(m.HeapConstant(array),
                                   m.SmiTag(m.IntPtrConstant(4)), 0));
  FunctionTester ft(asm_tester.GenerateCode());
  MaybeHandle<Object> result = ft.Call();
  CHECK_EQ(733, Handle<Smi>::cast(result.ToHandleChecked())->value());
}

TEST(LoadHeapNumberValue) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate);
  CodeStubAssembler m(asm_tester.state());
  Handle<HeapNumber> number = isolate->factory()->NewHeapNumber(1234);
  m.Return(m.SmiFromInt32(m.Signed(
      m.ChangeFloat64ToUint32(m.LoadHeapNumberValue(m.HeapConstant(number))))));
  FunctionTester ft(asm_tester.GenerateCode());
  MaybeHandle<Object> result = ft.Call();
  CHECK_EQ(1234, Handle<Smi>::cast(result.ToHandleChecked())->value());
}

TEST(LoadInstanceType) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate);
  CodeStubAssembler m(asm_tester.state());
  Handle<HeapObject> undefined = isolate->factory()->undefined_value();
  m.Return(m.SmiFromInt32(m.LoadInstanceType(m.HeapConstant(undefined))));
  FunctionTester ft(asm_tester.GenerateCode());
  MaybeHandle<Object> result = ft.Call();
  CHECK_EQ(InstanceType::ODDBALL_TYPE,
           Handle<Smi>::cast(result.ToHandleChecked())->value());
}

TEST(DecodeWordFromWord32) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate);
  CodeStubAssembler m(asm_tester.state());

  using TestBitField = base::BitField<unsigned, 3, 3>;
  m.Return(m.SmiTag(
      m.Signed(m.DecodeWordFromWord32<TestBitField>(m.Int32Constant(0x2F)))));
  FunctionTester ft(asm_tester.GenerateCode());
  MaybeHandle<Object> result = ft.Call();
  // value  = 00101111
  // mask   = 00111000
  // result = 101
  CHECK_EQ(5, Handle<Smi>::cast(result.ToHandleChecked())->value());
}

TEST(JSFunction) {
  const int kNumParams = 2;  // left, right.
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());
  m.Return(m.SmiFromInt32(m.Int32Add(m.SmiToInt32(m.Parameter<Smi>(1)),
                                     m.SmiToInt32(m.Parameter<Smi>(2)))));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  MaybeHandle<Object> result = ft.Call(handle(Smi::FromInt(23), isolate),
                                       handle(Smi::FromInt(34), isolate));
  CHECK_EQ(57, Handle<Smi>::cast(result.ToHandleChecked())->value());
}

TEST(ComputeIntegerHash) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  m.Return(m.SmiFromInt32(m.UncheckedCast<Int32T>(
      m.ComputeSeededHash(m.SmiUntag(m.Parameter<Smi>(1))))));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  base::RandomNumberGenerator rand_gen(FLAG_random_seed);

  for (int i = 0; i < 1024; i++) {
    int k = rand_gen.NextInt(Smi::kMaxValue);

    Handle<Smi> key(Smi::FromInt(k), isolate);
    Handle<Object> result = ft.Call(key).ToHandleChecked();

    uint32_t hash = ComputeSeededHash(k, HashSeed(isolate));
    Smi expected = Smi::FromInt(hash);
    CHECK_EQ(expected, Smi::cast(*result));
  }
}

TEST(ToString) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());
  m.Return(m.ToStringImpl(m.Parameter<Context>(kNumParams + 3),
                          m.Parameter<Object>(1)));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> test_cases = isolate->factory()->NewFixedArray(5);
  Handle<FixedArray> smi_test = isolate->factory()->NewFixedArray(2);
  smi_test->set(0, Smi::FromInt(42));
  Handle<String> str(isolate->factory()->InternalizeUtf8String("42"));
  smi_test->set(1, *str);
  test_cases->set(0, *smi_test);

  Handle<FixedArray> number_test = isolate->factory()->NewFixedArray(2);
  Handle<HeapNumber> num(isolate->factory()->NewHeapNumber(3.14));
  number_test->set(0, *num);
  str = isolate->factory()->InternalizeUtf8String("3.14");
  number_test->set(1, *str);
  test_cases->set(1, *number_test);

  Handle<FixedArray> string_test = isolate->factory()->NewFixedArray(2);
  str = isolate->factory()->InternalizeUtf8String("test");
  string_test->set(0, *str);
  string_test->set(1, *str);
  test_cases->set(2, *string_test);

  Handle<FixedArray> oddball_test = isolate->factory()->NewFixedArray(2);
  oddball_test->set(0, ReadOnlyRoots(isolate).undefined_value());
  str = isolate->factory()->InternalizeUtf8String("undefined");
  oddball_test->set(1, *str);
  test_cases->set(3, *oddball_test);

  Handle<FixedArray> tostring_test = isolate->factory()->NewFixedArray(2);
  Handle<FixedArray> js_array_storage = isolate->factory()->NewFixedArray(2);
  js_array_storage->set(0, Smi::FromInt(1));
  js_array_storage->set(1, Smi::FromInt(2));
  Handle<JSArray> js_array = isolate->factory()->NewJSArray(2);
  JSArray::SetContent(js_array, js_array_storage);
  tostring_test->set(0, *js_array);
  str = isolate->factory()->InternalizeUtf8String("1,2");
  tostring_test->set(1, *str);
  test_cases->set(4, *tostring_test);

  for (int i = 0; i < 5; ++i) {
    Handle<FixedArray> test =
        handle(FixedArray::cast(test_cases->get(i)), isolate);
    Handle<Object> obj = handle(test->get(0), isolate);
    Handle<String> expected = handle(String::cast(test->get(1)), isolate);
    Handle<Object> result = ft.Call(obj).ToHandleChecked();
    CHECK(result->IsString());
    CHECK(String::Equals(isolate, Handle<String>::cast(result), expected));
  }
}

TEST(TryToName) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  enum Result { kKeyIsIndex, kKeyIsUnique, kBailout };
  {
    auto key = m.Parameter<Object>(1);
    auto expected_result = m.UncheckedParameter<MaybeObject>(2);
    auto expected_arg = m.Parameter<Object>(3);

    Label passed(&m), failed(&m);
    Label if_keyisindex(&m), if_keyisunique(&m), if_bailout(&m);
    {
      TYPED_VARIABLE_DEF(IntPtrT, var_index, &m);
      TYPED_VARIABLE_DEF(Name, var_unique, &m);
      TYPED_VARIABLE_DEF(IntPtrT, var_expected, &m);

      m.TryToName(key, &if_keyisindex, &var_index, &if_keyisunique, &var_unique,
                  &if_bailout);

      m.BIND(&if_keyisindex);
      m.GotoIfNot(m.TaggedEqual(expected_result,
                                m.SmiConstant(Smi::FromInt(kKeyIsIndex))),
                  &failed);

      Label if_expectedissmi(&m), if_expectedisheapnumber(&m), check_result(&m);
      m.Branch(m.TaggedIsSmi(expected_arg), &if_expectedissmi,
               &if_expectedisheapnumber);

      m.BIND(&if_expectedissmi);
      var_expected = m.SmiUntag(m.CAST(expected_arg));
      m.Goto(&check_result);

      m.BIND(&if_expectedisheapnumber);
      CSA_ASSERT(&m, m.IsHeapNumber(m.CAST(expected_arg)));
      TNode<Float64T> value = m.LoadHeapNumberValue(m.CAST(expected_arg));
      // We know this to be safe as all expected values are in intptr
      // range.
      var_expected = m.UncheckedCast<IntPtrT>(m.ChangeFloat64ToUintPtr(value));
      m.Goto(&check_result);

      m.BIND(&check_result);
      m.Branch(m.IntPtrEqual(var_expected.value(), var_index.value()), &passed,
               &failed);

      m.BIND(&if_keyisunique);
      m.GotoIfNot(m.TaggedEqual(expected_result,
                                m.SmiConstant(Smi::FromInt(kKeyIsUnique))),
                  &failed);
      m.Branch(m.TaggedEqual(expected_arg, var_unique.value()), &passed,
               &failed);
    }

    m.BIND(&if_bailout);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kBailout))),
        &passed, &failed);

    m.BIND(&passed);
    m.Return(m.BooleanConstant(true));

    m.BIND(&failed);
    m.Return(m.BooleanConstant(false));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> expect_index(Smi::FromInt(kKeyIsIndex), isolate);
  Handle<Object> expect_unique(Smi::FromInt(kKeyIsUnique), isolate);
  Handle<Object> expect_bailout(Smi::FromInt(kBailout), isolate);

  {
    // TryToName(<zero smi>) => if_keyisindex: smi value.
    Handle<Object> key(Smi::zero(), isolate);
    ft.CheckTrue(key, expect_index, key);
  }

  {
    // TryToName(<positive smi>) => if_keyisindex: smi value.
    Handle<Object> key(Smi::FromInt(153), isolate);
    ft.CheckTrue(key, expect_index, key);
  }

  {
    // TryToName(<negative smi>) => if_keyisindex: smi value.
    // A subsequent bounds check needs to take care of this case.
    Handle<Object> key(Smi::FromInt(-1), isolate);
    ft.CheckTrue(key, expect_index, key);
  }

  {
    // TryToName(<heap number with int value>) => if_keyisindex: number.
    Handle<Object> key(isolate->factory()->NewHeapNumber(153));
    Handle<Object> index(Smi::FromInt(153), isolate);
    ft.CheckTrue(key, expect_index, index);
  }

  {
    // TryToName(<true>) => if_keyisunique: "true".
    Handle<Object> key = isolate->factory()->true_value();
    Handle<Object> unique = isolate->factory()->InternalizeUtf8String("true");
    ft.CheckTrue(key, expect_unique, unique);
  }

  {
    // TryToName(<false>) => if_keyisunique: "false".
    Handle<Object> key = isolate->factory()->false_value();
    Handle<Object> unique = isolate->factory()->InternalizeUtf8String("false");
    ft.CheckTrue(key, expect_unique, unique);
  }

  {
    // TryToName(<null>) => if_keyisunique: "null".
    Handle<Object> key = isolate->factory()->null_value();
    Handle<Object> unique = isolate->factory()->InternalizeUtf8String("null");
    ft.CheckTrue(key, expect_unique, unique);
  }

  {
    // TryToName(<undefined>) => if_keyisunique: "undefined".
    Handle<Object> key = isolate->factory()->undefined_value();
    Handle<Object> unique =
        isolate->factory()->InternalizeUtf8String("undefined");
    ft.CheckTrue(key, expect_unique, unique);
  }

  {
    // TryToName(<symbol>) => if_keyisunique: <symbol>.
    Handle<Object> key = isolate->factory()->NewSymbol();
    ft.CheckTrue(key, expect_unique, key);
  }

  {
    // TryToName(<internalized string>) => if_keyisunique: <internalized string>
    Handle<Object> key = isolate->factory()->InternalizeUtf8String("test");
    ft.CheckTrue(key, expect_unique, key);
  }

  {
    // TryToName(<internalized number string>) => if_keyisindex: number.
    Handle<Object> key = isolate->factory()->InternalizeUtf8String("153");
    Handle<Object> index(Smi::FromInt(153), isolate);
    ft.CheckTrue(key, expect_index, index);
  }

  {
    // TryToName(<internalized uncacheable number string greater than
    // array index but less than MAX_SAFE_INTEGER>) => 32-bit platforms
    // take the if_keyisunique path, 64-bit platforms bail out because they
    // let the runtime handle the string-to-size_t parsing.
    Handle<Object> key =
        isolate->factory()->InternalizeUtf8String("4294967296");
#if V8_TARGET_ARCH_64_BIT
    ft.CheckTrue(key, expect_bailout);
#else
    ft.CheckTrue(key, expect_unique, key);
#endif
  }

  {
    // TryToName(<internalized uncacheable number string greater than
    // INT_MAX but less than array index>) => bailout.
    Handle<Object> key =
        isolate->factory()->InternalizeUtf8String("4294967294");
    ft.CheckTrue(key, expect_bailout);
  }

  {
    // TryToName(<internalized uncacheable number string less than
    // INT_MAX>) => bailout
    Handle<Object> key =
        isolate->factory()->InternalizeUtf8String("2147483647");
    ft.CheckTrue(key, expect_bailout);
  }

  {
    // TryToName(<non-internalized number string>) => if_keyisindex: number.
    Handle<String> key = isolate->factory()->NewStringFromAsciiChecked("153");
    uint32_t dummy;
    CHECK(key->AsArrayIndex(&dummy));
    CHECK(key->HasHashCode());
    CHECK(!key->IsInternalizedString());
    Handle<Object> index(Smi::FromInt(153), isolate);
    ft.CheckTrue(key, expect_index, index);
  }

  {
    // TryToName(<number string without cached index>) => is_keyisindex: number.
    Handle<String> key = isolate->factory()->NewStringFromAsciiChecked("153");
    CHECK(!key->HasHashCode());
    ft.CheckTrue(key, expect_bailout);
  }

  {
    // TryToName(<non-internalized string>) => bailout.
    Handle<Object> key = isolate->factory()->NewStringFromAsciiChecked("test");
    ft.CheckTrue(key, expect_bailout);
  }

  if (FLAG_thin_strings) {
    // TryToName(<thin string>) => internalized version.
    Handle<String> s = isolate->factory()->NewStringFromAsciiChecked("foo");
    Handle<String> internalized = isolate->factory()->InternalizeString(s);
    ft.CheckTrue(s, expect_unique, internalized);
  }

  if (FLAG_thin_strings) {
    // TryToName(<thin two-byte string>) => internalized version.
    uc16 array1[] = {2001, 2002, 2003};
    Handle<String> s = isolate->factory()
                           ->NewStringFromTwoByte(ArrayVector(array1))
                           .ToHandleChecked();
    Handle<String> internalized = isolate->factory()->InternalizeString(s);
    ft.CheckTrue(s, expect_unique, internalized);
  }
}

namespace {

template <typename Dictionary>
void TestEntryToIndex() {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());
  {
    TNode<IntPtrT> entry = m.SmiUntag(m.Parameter<Smi>(1));
    TNode<IntPtrT> result = m.EntryToIndex<Dictionary>(entry);
    m.Return(m.SmiTag(result));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  // Test a wide range of entries but staying linear in the first 100 entries.
  for (int entry = 0; entry < Dictionary::kMaxCapacity;
       entry = entry * 1.01 + 1) {
    Handle<Object> result =
        ft.Call(handle(Smi::FromInt(entry), isolate)).ToHandleChecked();
    CHECK_EQ(Dictionary::EntryToIndex(InternalIndex(entry)),
             Smi::ToInt(*result));
  }
}

TEST(NameDictionaryEntryToIndex) { TestEntryToIndex<NameDictionary>(); }
TEST(GlobalDictionaryEntryToIndex) { TestEntryToIndex<GlobalDictionary>(); }

}  // namespace

namespace {

template <typename Dictionary>
void TestNameDictionaryLookup() {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 4;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  enum Result { kFound, kNotFound };
  {
    auto dictionary = m.Parameter<Dictionary>(1);
    auto unique_name = m.Parameter<Name>(2);
    auto expected_result = m.Parameter<Smi>(3);
    auto expected_arg = m.Parameter<Object>(4);

    Label passed(&m), failed(&m);
    Label if_found(&m), if_not_found(&m);
    TVariable<IntPtrT> var_name_index(&m);

    m.NameDictionaryLookup<Dictionary>(dictionary, unique_name, &if_found,
                                       &var_name_index, &if_not_found);
    m.BIND(&if_found);
    m.GotoIfNot(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kFound))),
        &failed);
    m.Branch(
        m.WordEqual(m.SmiUntag(m.CAST(expected_arg)), var_name_index.value()),
        &passed, &failed);

    m.BIND(&if_not_found);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kNotFound))),
        &passed, &failed);

    m.BIND(&passed);
    m.Return(m.BooleanConstant(true));

    m.BIND(&failed);
    m.Return(m.BooleanConstant(false));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> expect_found(Smi::FromInt(kFound), isolate);
  Handle<Object> expect_not_found(Smi::FromInt(kNotFound), isolate);

  Handle<Dictionary> dictionary = Dictionary::New(isolate, 40);
  PropertyDetails fake_details = PropertyDetails::Empty();

  Factory* factory = isolate->factory();
  Handle<Name> keys[] = {
      factory->InternalizeUtf8String("0"),
      factory->InternalizeUtf8String("42"),
      factory->InternalizeUtf8String("-153"),
      factory->InternalizeUtf8String("0.0"),
      factory->InternalizeUtf8String("4.2"),
      factory->InternalizeUtf8String(""),
      factory->InternalizeUtf8String("name"),
      factory->NewSymbol(),
      factory->NewPrivateSymbol(),
  };

  for (size_t i = 0; i < arraysize(keys); i++) {
    Handle<Object> value =
        factory->NewPropertyCell(keys[i], fake_details, keys[i]);
    dictionary =
        Dictionary::Add(isolate, dictionary, keys[i], value, fake_details);
  }

  for (size_t i = 0; i < arraysize(keys); i++) {
    InternalIndex entry = dictionary->FindEntry(isolate, keys[i]);
    int name_index =
        Dictionary::EntryToIndex(entry) + Dictionary::kEntryKeyIndex;
    CHECK(entry.is_found());

    Handle<Object> expected_name_index(Smi::FromInt(name_index), isolate);
    ft.CheckTrue(dictionary, keys[i], expect_found, expected_name_index);
  }

  Handle<Name> non_existing_keys[] = {
      factory->InternalizeUtf8String("1"),
      factory->InternalizeUtf8String("-42"),
      factory->InternalizeUtf8String("153"),
      factory->InternalizeUtf8String("-1.0"),
      factory->InternalizeUtf8String("1.3"),
      factory->InternalizeUtf8String("a"),
      factory->InternalizeUtf8String("boom"),
      factory->NewSymbol(),
      factory->NewPrivateSymbol(),
  };

  for (size_t i = 0; i < arraysize(non_existing_keys); i++) {
    InternalIndex entry = dictionary->FindEntry(isolate, non_existing_keys[i]);
    CHECK(entry.is_not_found());

    ft.CheckTrue(dictionary, non_existing_keys[i], expect_not_found);
  }
}

}  // namespace

TEST(NameDictionaryLookup) { TestNameDictionaryLookup<NameDictionary>(); }

TEST(GlobalDictionaryLookup) { TestNameDictionaryLookup<GlobalDictionary>(); }

TEST(NumberDictionaryLookup) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 4;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  enum Result { kFound, kNotFound };
  {
    auto dictionary = m.Parameter<NumberDictionary>(1);
    TNode<IntPtrT> key = m.SmiUntag(m.Parameter<Smi>(2));
    auto expected_result = m.Parameter<Smi>(3);
    auto expected_arg = m.Parameter<Object>(4);

    Label passed(&m), failed(&m);
    Label if_found(&m), if_not_found(&m);
    TVariable<IntPtrT> var_entry(&m);

    m.NumberDictionaryLookup(dictionary, key, &if_found, &var_entry,
                             &if_not_found);
    m.BIND(&if_found);
    m.GotoIfNot(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kFound))),
        &failed);
    m.Branch(m.WordEqual(m.SmiUntag(m.CAST(expected_arg)), var_entry.value()),
             &passed, &failed);

    m.BIND(&if_not_found);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kNotFound))),
        &passed, &failed);

    m.BIND(&passed);
    m.Return(m.BooleanConstant(true));

    m.BIND(&failed);
    m.Return(m.BooleanConstant(false));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> expect_found(Smi::FromInt(kFound), isolate);
  Handle<Object> expect_not_found(Smi::FromInt(kNotFound), isolate);

  const int kKeysCount = 1000;
  Handle<NumberDictionary> dictionary =
      NumberDictionary::New(isolate, kKeysCount);
  uint32_t keys[kKeysCount];

  Handle<Object> fake_value(Smi::FromInt(42), isolate);
  PropertyDetails fake_details = PropertyDetails::Empty();

  base::RandomNumberGenerator rand_gen(FLAG_random_seed);

  for (int i = 0; i < kKeysCount; i++) {
    int random_key = rand_gen.NextInt(Smi::kMaxValue);
    keys[i] = static_cast<uint32_t>(random_key);
    if (dictionary->FindEntry(isolate, keys[i]).is_found()) continue;

    dictionary = NumberDictionary::Add(isolate, dictionary, keys[i], fake_value,
                                       fake_details);
  }

  // Now try querying existing keys.
  for (int i = 0; i < kKeysCount; i++) {
    InternalIndex entry = dictionary->FindEntry(isolate, keys[i]);
    CHECK(entry.is_found());

    Handle<Object> key(Smi::FromInt(keys[i]), isolate);
    Handle<Object> expected_entry(Smi::FromInt(entry.as_int()), isolate);
    ft.CheckTrue(dictionary, key, expect_found, expected_entry);
  }

  // Now try querying random keys which do not exist in the dictionary.
  for (int i = 0; i < kKeysCount;) {
    int random_key = rand_gen.NextInt(Smi::kMaxValue);
    InternalIndex entry = dictionary->FindEntry(isolate, random_key);
    if (entry.is_found()) continue;
    i++;

    Handle<Object> key(Smi::FromInt(random_key), isolate);
    ft.CheckTrue(dictionary, key, expect_not_found);
  }
}

TEST(TransitionLookup) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 4;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.

  enum Result { kFound, kNotFound };

  class TempAssembler : public CodeStubAssembler {
   public:
    explicit TempAssembler(compiler::CodeAssemblerState* state)
        : CodeStubAssembler(state) {}

    void Generate() {
      auto transitions = Parameter<TransitionArray>(1);
      auto name = Parameter<Name>(2);
      auto expected_result = Parameter<Smi>(3);
      auto expected_arg = Parameter<Object>(4);

      Label passed(this), failed(this);
      Label if_found(this), if_not_found(this);
      TVARIABLE(IntPtrT, var_transition_index);

      TransitionLookup(name, transitions, &if_found, &var_transition_index,
                       &if_not_found);

      BIND(&if_found);
      GotoIfNot(TaggedEqual(expected_result, SmiConstant(kFound)), &failed);
      Branch(TaggedEqual(expected_arg, SmiTag(var_transition_index.value())),
             &passed, &failed);

      BIND(&if_not_found);
      Branch(TaggedEqual(expected_result, SmiConstant(kNotFound)), &passed,
             &failed);

      BIND(&passed);
      Return(BooleanConstant(true));

      BIND(&failed);
      Return(BooleanConstant(false));
    }
  };
  TempAssembler(asm_tester.state()).Generate();

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> expect_found(Smi::FromInt(kFound), isolate);
  Handle<Object> expect_not_found(Smi::FromInt(kNotFound), isolate);

  const int ATTRS_COUNT = (READ_ONLY | DONT_ENUM | DONT_DELETE) + 1;
  STATIC_ASSERT(ATTRS_COUNT == 8);

  const int kKeysCount = 300;
  Handle<Map> root_map = Map::Create(isolate, 0);
  Handle<Name> keys[kKeysCount];

  base::RandomNumberGenerator rand_gen(FLAG_random_seed);

  Factory* factory = isolate->factory();
  Handle<FieldType> any = FieldType::Any(isolate);

  for (int i = 0; i < kKeysCount; i++) {
    Handle<Name> name;
    if (i % 30 == 0) {
      name = factory->NewPrivateSymbol();
    } else if (i % 10 == 0) {
      name = factory->NewSymbol();
    } else {
      int random_key = rand_gen.NextInt(Smi::kMaxValue);
      name = CcTest::MakeName("p", random_key);
    }
    keys[i] = name;

    bool is_private = name->IsPrivate();
    PropertyAttributes base_attributes = is_private ? DONT_ENUM : NONE;

    // Ensure that all the combinations of cases are covered:
    // 1) there is a "base" attributes transition
    // 2) there are other non-base attributes transitions
    if ((i & 1) == 0) {
      CHECK(!Map::CopyWithField(isolate, root_map, name, any, base_attributes,
                                PropertyConstness::kMutable,
                                Representation::Tagged(), INSERT_TRANSITION)
                 .is_null());
    }

    if ((i & 2) == 0) {
      for (int j = 0; j < ATTRS_COUNT; j++) {
        PropertyAttributes attributes = static_cast<PropertyAttributes>(j);
        if (attributes == base_attributes) continue;
        // Don't add private symbols with enumerable attributes.
        if (is_private && ((attributes & DONT_ENUM) == 0)) continue;
        CHECK(!Map::CopyWithField(isolate, root_map, name, any, attributes,
                                  PropertyConstness::kMutable,
                                  Representation::Tagged(), INSERT_TRANSITION)
                   .is_null());
      }
    }
  }

  CHECK(root_map->raw_transitions()
            ->GetHeapObjectAssumeStrong()
            .IsTransitionArray());
  Handle<TransitionArray> transitions(
      TransitionArray::cast(
          root_map->raw_transitions()->GetHeapObjectAssumeStrong()),
      isolate);
  DCHECK(transitions->IsSortedNoDuplicates());

  // Ensure we didn't overflow transition array and therefore all the
  // combinations of cases are covered.
  CHECK(TransitionsAccessor(isolate, root_map).CanHaveMoreTransitions());

  // Now try querying keys.
  bool positive_lookup_tested = false;
  bool negative_lookup_tested = false;
  for (int i = 0; i < kKeysCount; i++) {
    Handle<Name> name = keys[i];

    int transition_number = transitions->SearchNameForTesting(*name);

    if (transition_number != TransitionArray::kNotFound) {
      Handle<Smi> expected_value(
          Smi::FromInt(TransitionArray::ToKeyIndex(transition_number)),
          isolate);
      ft.CheckTrue(transitions, name, expect_found, expected_value);
      positive_lookup_tested = true;
    } else {
      ft.CheckTrue(transitions, name, expect_not_found);
      negative_lookup_tested = true;
    }
  }
  CHECK(positive_lookup_tested);
  CHECK(negative_lookup_tested);
}

namespace {

void AddProperties(Handle<JSObject> object, Handle<Name> names[],
                   size_t count) {
  Isolate* isolate = object->GetIsolate();
  for (size_t i = 0; i < count; i++) {
    Handle<Object> value(Smi::FromInt(static_cast<int>(42 + i)), isolate);
    JSObject::AddProperty(isolate, object, names[i], value, NONE);
  }
}

Handle<AccessorPair> CreateAccessorPair(FunctionTester* ft,
                                        const char* getter_body,
                                        const char* setter_body) {
  Handle<AccessorPair> pair = ft->isolate->factory()->NewAccessorPair();
  if (getter_body) {
    pair->set_getter(*ft->NewFunction(getter_body));
  }
  if (setter_body) {
    pair->set_setter(*ft->NewFunction(setter_body));
  }
  return pair;
}

void AddProperties(Handle<JSObject> object, Handle<Name> names[],
                   size_t names_count, Handle<Object> values[],
                   size_t values_count, int seed = 0) {
  Isolate* isolate = object->GetIsolate();
  for (size_t i = 0; i < names_count; i++) {
    Handle<Object> value = values[(seed + i) % values_count];
    if (value->IsAccessorPair()) {
      Handle<AccessorPair> pair = Handle<AccessorPair>::cast(value);
      Handle<Object> getter(pair->getter(), isolate);
      Handle<Object> setter(pair->setter(), isolate);
      JSObject::DefineAccessor(object, names[i], getter, setter, NONE).Check();
    } else {
      JSObject::AddProperty(isolate, object, names[i], value, NONE);
    }
  }
}

}  // namespace

TEST(TryHasOwnProperty) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  enum Result { kFound, kNotFound, kBailout };
  {
    auto object = m.Parameter<HeapObject>(1);
    auto unique_name = m.Parameter<Name>(2);
    TNode<MaybeObject> expected_result = m.UncheckedParameter<MaybeObject>(3);

    Label passed(&m), failed(&m);
    Label if_found(&m), if_not_found(&m), if_bailout(&m);

    TNode<Map> map = m.LoadMap(object);
    TNode<Uint16T> instance_type = m.LoadMapInstanceType(map);

    m.TryHasOwnProperty(object, map, instance_type, unique_name, &if_found,
                        &if_not_found, &if_bailout);

    m.BIND(&if_found);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kFound))),
        &passed, &failed);

    m.BIND(&if_not_found);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kNotFound))),
        &passed, &failed);

    m.BIND(&if_bailout);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kBailout))),
        &passed, &failed);

    m.BIND(&passed);
    m.Return(m.BooleanConstant(true));

    m.BIND(&failed);
    m.Return(m.BooleanConstant(false));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> expect_found(Smi::FromInt(kFound), isolate);
  Handle<Object> expect_not_found(Smi::FromInt(kNotFound), isolate);
  Handle<Object> expect_bailout(Smi::FromInt(kBailout), isolate);

  Factory* factory = isolate->factory();

  Handle<Name> deleted_property_name =
      factory->InternalizeUtf8String("deleted");

  Handle<Name> names[] = {
      factory->InternalizeUtf8String("a"),
      factory->InternalizeUtf8String("bb"),
      factory->InternalizeUtf8String("ccc"),
      factory->InternalizeUtf8String("dddd"),
      factory->InternalizeUtf8String("eeeee"),
      factory->InternalizeUtf8String(""),
      factory->InternalizeUtf8String("name"),
      factory->NewSymbol(),
      factory->NewPrivateSymbol(),
  };

  std::vector<Handle<JSObject>> objects;

  {
    // Fast object, no inobject properties.
    int inobject_properties = 0;
    Handle<Map> map = Map::Create(isolate, inobject_properties);
    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
    AddProperties(object, names, arraysize(names));
    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK_EQ(inobject_properties, object->map().GetInObjectProperties());
    CHECK(!object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Fast object, all inobject properties.
    int inobject_properties = arraysize(names) * 2;
    Handle<Map> map = Map::Create(isolate, inobject_properties);
    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
    AddProperties(object, names, arraysize(names));
    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK_EQ(inobject_properties, object->map().GetInObjectProperties());
    CHECK(!object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Fast object, half inobject properties.
    int inobject_properties = arraysize(names) / 2;
    Handle<Map> map = Map::Create(isolate, inobject_properties);
    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
    AddProperties(object, names, arraysize(names));
    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK_EQ(inobject_properties, object->map().GetInObjectProperties());
    CHECK(!object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Dictionary mode object.
    Handle<JSFunction> function =
        factory->NewFunctionForTesting(factory->empty_string());
    Handle<JSObject> object = factory->NewJSObject(function);
    AddProperties(object, names, arraysize(names));
    JSObject::NormalizeProperties(isolate, object, CLEAR_INOBJECT_PROPERTIES, 0,
                                  "test");

    JSObject::AddProperty(isolate, object, deleted_property_name, object, NONE);
    CHECK(JSObject::DeleteProperty(object, deleted_property_name,
                                   LanguageMode::kSloppy)
              .FromJust());

    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK(object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Global object.
    Handle<JSFunction> function =
        factory->NewFunctionForTesting(factory->empty_string());
    JSFunction::EnsureHasInitialMap(function);
    function->initial_map().set_instance_type(JS_GLOBAL_OBJECT_TYPE);
    function->initial_map().set_is_prototype_map(true);
    function->initial_map().set_is_dictionary_map(true);
    function->initial_map().set_may_have_interesting_symbols(true);
    Handle<JSObject> object = factory->NewJSGlobalObject(function);
    AddProperties(object, names, arraysize(names));

    JSObject::AddProperty(isolate, object, deleted_property_name, object, NONE);
    CHECK(JSObject::DeleteProperty(object, deleted_property_name,
                                   LanguageMode::kSloppy)
              .FromJust());

    CHECK_EQ(JS_GLOBAL_OBJECT_TYPE, object->map().instance_type());
    CHECK(object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    for (Handle<JSObject> object : objects) {
      for (size_t name_index = 0; name_index < arraysize(names); name_index++) {
        Handle<Name> name = names[name_index];
        CHECK(JSReceiver::HasProperty(object, name).FromJust());
        ft.CheckTrue(object, name, expect_found);
      }
    }
  }

  {
    Handle<Name> non_existing_names[] = {
        factory->NewSymbol(),
        factory->InternalizeUtf8String("ne_a"),
        factory->InternalizeUtf8String("ne_bb"),
        factory->NewPrivateSymbol(),
        factory->InternalizeUtf8String("ne_ccc"),
        factory->InternalizeUtf8String("ne_dddd"),
        deleted_property_name,
    };
    for (Handle<JSObject> object : objects) {
      for (size_t key_index = 0; key_index < arraysize(non_existing_names);
           key_index++) {
        Handle<Name> name = non_existing_names[key_index];
        CHECK(!JSReceiver::HasProperty(object, name).FromJust());
        ft.CheckTrue(object, name, expect_not_found);
      }
    }
  }

  {
    Handle<JSFunction> function =
        factory->NewFunctionForTesting(factory->empty_string());
    Handle<JSProxy> object = factory->NewJSProxy(function, objects[0]);
    CHECK_EQ(JS_PROXY_TYPE, object->map().instance_type());
    ft.CheckTrue(object, names[0], expect_bailout);
  }

  {
    Handle<JSObject> object = isolate->global_proxy();
    CHECK_EQ(JS_GLOBAL_PROXY_TYPE, object->map().instance_type());
    ft.CheckTrue(object, names[0], expect_bailout);
  }
}

TEST(TryGetOwnProperty) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* factory = isolate->factory();

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  Handle<Symbol> not_found_symbol = factory->NewSymbol();
  Handle<Symbol> bailout_symbol = factory->NewSymbol();
  {
    auto object = m.Parameter<JSReceiver>(1);
    auto unique_name = m.Parameter<Name>(2);
    auto context = m.Parameter<Context>(kNumParams + 3);

    TVariable<Object> var_value(&m);
    Label if_found(&m), if_not_found(&m), if_bailout(&m);

    TNode<Map> map = m.LoadMap(object);
    TNode<Uint16T> instance_type = m.LoadMapInstanceType(map);

    m.TryGetOwnProperty(context, object, object, map, instance_type,
                        unique_name, &if_found, &var_value, &if_not_found,
                        &if_bailout);

    m.BIND(&if_found);
    m.Return(m.UncheckedCast<Object>(var_value.value()));

    m.BIND(&if_not_found);
    m.Return(m.HeapConstant(not_found_symbol));

    m.BIND(&if_bailout);
    m.Return(m.HeapConstant(bailout_symbol));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Name> deleted_property_name =
      factory->InternalizeUtf8String("deleted");

  Handle<Name> names[] = {
      factory->InternalizeUtf8String("bb"),
      factory->NewSymbol(),
      factory->InternalizeUtf8String("a"),
      factory->InternalizeUtf8String("ccc"),
      factory->InternalizeUtf8String("esajefe"),
      factory->NewPrivateSymbol(),
      factory->InternalizeUtf8String("eeeee"),
      factory->InternalizeUtf8String("p1"),
      factory->InternalizeUtf8String("acshw23e"),
      factory->InternalizeUtf8String(""),
      factory->InternalizeUtf8String("dddd"),
      factory->NewPrivateSymbol(),
      factory->InternalizeUtf8String("name"),
      factory->InternalizeUtf8String("p2"),
      factory->InternalizeUtf8String("p3"),
      factory->InternalizeUtf8String("p4"),
      factory->NewPrivateSymbol(),
  };
  Handle<Object> values[] = {
      factory->NewFunctionForTesting(factory->empty_string()),
      factory->NewSymbol(),
      factory->InternalizeUtf8String("a"),
      CreateAccessorPair(&ft, "() => 188;", "() => 199;"),
      factory->NewFunctionForTesting(factory->InternalizeUtf8String("bb")),
      factory->InternalizeUtf8String("ccc"),
      CreateAccessorPair(&ft, "() => 88;", nullptr),
      handle(Smi::FromInt(1), isolate),
      factory->InternalizeUtf8String(""),
      CreateAccessorPair(&ft, nullptr, "() => 99;"),
      factory->NewHeapNumber(4.2),
      handle(Smi::FromInt(153), isolate),
      factory->NewJSObject(
          factory->NewFunctionForTesting(factory->empty_string())),
      factory->NewPrivateSymbol(),
  };
  STATIC_ASSERT(arraysize(values) < arraysize(names));

  base::RandomNumberGenerator rand_gen(FLAG_random_seed);

  std::vector<Handle<JSObject>> objects;

  {
    // Fast object, no inobject properties.
    int inobject_properties = 0;
    Handle<Map> map = Map::Create(isolate, inobject_properties);
    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
    AddProperties(object, names, arraysize(names), values, arraysize(values),
                  rand_gen.NextInt());
    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK_EQ(inobject_properties, object->map().GetInObjectProperties());
    CHECK(!object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Fast object, all inobject properties.
    int inobject_properties = arraysize(names) * 2;
    Handle<Map> map = Map::Create(isolate, inobject_properties);
    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
    AddProperties(object, names, arraysize(names), values, arraysize(values),
                  rand_gen.NextInt());
    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK_EQ(inobject_properties, object->map().GetInObjectProperties());
    CHECK(!object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Fast object, half inobject properties.
    int inobject_properties = arraysize(names) / 2;
    Handle<Map> map = Map::Create(isolate, inobject_properties);
    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
    AddProperties(object, names, arraysize(names), values, arraysize(values),
                  rand_gen.NextInt());
    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK_EQ(inobject_properties, object->map().GetInObjectProperties());
    CHECK(!object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Dictionary mode object.
    Handle<JSFunction> function =
        factory->NewFunctionForTesting(factory->empty_string());
    Handle<JSObject> object = factory->NewJSObject(function);
    AddProperties(object, names, arraysize(names), values, arraysize(values),
                  rand_gen.NextInt());
    JSObject::NormalizeProperties(isolate, object, CLEAR_INOBJECT_PROPERTIES, 0,
                                  "test");

    JSObject::AddProperty(isolate, object, deleted_property_name, object, NONE);
    CHECK(JSObject::DeleteProperty(object, deleted_property_name,
                                   LanguageMode::kSloppy)
              .FromJust());

    CHECK_EQ(JS_OBJECT_TYPE, object->map().instance_type());
    CHECK(object->map().is_dictionary_map());
    objects.push_back(object);
  }

  {
    // Global object.
    Handle<JSGlobalObject> object = isolate->global_object();
    AddProperties(object, names, arraysize(names), values, arraysize(values),
                  rand_gen.NextInt());

    JSObject::AddProperty(isolate, object, deleted_property_name, object, NONE);
    CHECK(JSObject::DeleteProperty(object, deleted_property_name,
                                   LanguageMode::kSloppy)
              .FromJust());

    CHECK_EQ(JS_GLOBAL_OBJECT_TYPE, object->map().instance_type());
    CHECK(object->map().is_dictionary_map());
    objects.push_back(object);
  }

  // TODO(ishell): test proxy and interceptors when they are supported.

  {
    for (Handle<JSObject> object : objects) {
      for (size_t name_index = 0; name_index < arraysize(names); name_index++) {
        Handle<Name> name = names[name_index];
        Handle<Object> expected_value =
            JSReceiver::GetProperty(isolate, object, name).ToHandleChecked();
        Handle<Object> value = ft.Call(object, name).ToHandleChecked();
        CHECK(expected_value->SameValue(*value));
      }
    }
  }

  {
    Handle<Name> non_existing_names[] = {
        factory->NewSymbol(),
        factory->InternalizeUtf8String("ne_a"),
        factory->InternalizeUtf8String("ne_bb"),
        factory->NewPrivateSymbol(),
        factory->InternalizeUtf8String("ne_ccc"),
        factory->InternalizeUtf8String("ne_dddd"),
        deleted_property_name,
    };
    for (Handle<JSObject> object : objects) {
      for (size_t key_index = 0; key_index < arraysize(non_existing_names);
           key_index++) {
        Handle<Name> name = non_existing_names[key_index];
        Handle<Object> expected_value =
            JSReceiver::GetProperty(isolate, object, name).ToHandleChecked();
        CHECK(expected_value->IsUndefined(isolate));
        Handle<Object> value = ft.Call(object, name).ToHandleChecked();
        CHECK_EQ(*not_found_symbol, *value);
      }
    }
  }

  {
    Handle<JSFunction> function =
        factory->NewFunctionForTesting(factory->empty_string());
    Handle<JSProxy> object = factory->NewJSProxy(function, objects[0]);
    CHECK_EQ(JS_PROXY_TYPE, object->map().instance_type());
    Handle<Object> value = ft.Call(object, names[0]).ToHandleChecked();
    // Proxies are not supported yet.
    CHECK_EQ(*bailout_symbol, *value);
  }

  {
    Handle<JSObject> object = isolate->global_proxy();
    CHECK_EQ(JS_GLOBAL_PROXY_TYPE, object->map().instance_type());
    // Global proxies are not supported yet.
    Handle<Object> value = ft.Call(object, names[0]).ToHandleChecked();
    CHECK_EQ(*bailout_symbol, *value);
  }
}

namespace {

void AddElement(Handle<JSObject> object, uint32_t index, Handle<Object> value,
                PropertyAttributes attributes = NONE) {
  JSObject::AddDataElement(object, index, value, attributes);
}

}  // namespace

TEST(TryLookupElement) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  enum Result { kFound, kAbsent, kNotFound, kBailout };
  {
    auto object = m.Parameter<HeapObject>(1);
    TNode<IntPtrT> index = m.SmiUntag(m.Parameter<Smi>(2));
    TNode<MaybeObject> expected_result = m.UncheckedParameter<MaybeObject>(3);

    Label passed(&m), failed(&m);
    Label if_found(&m), if_not_found(&m), if_bailout(&m), if_absent(&m);

    TNode<Map> map = m.LoadMap(object);
    TNode<Uint16T> instance_type = m.LoadMapInstanceType(map);

    m.TryLookupElement(object, map, instance_type, index, &if_found, &if_absent,
                       &if_not_found, &if_bailout);

    m.BIND(&if_found);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kFound))),
        &passed, &failed);

    m.BIND(&if_absent);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kAbsent))),
        &passed, &failed);

    m.BIND(&if_not_found);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kNotFound))),
        &passed, &failed);

    m.BIND(&if_bailout);
    m.Branch(
        m.TaggedEqual(expected_result, m.SmiConstant(Smi::FromInt(kBailout))),
        &passed, &failed);

    m.BIND(&passed);
    m.Return(m.BooleanConstant(true));

    m.BIND(&failed);
    m.Return(m.BooleanConstant(false));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Factory* factory = isolate->factory();
  Handle<Object> smi0(Smi::zero(), isolate);
  Handle<Object> smi1(Smi::FromInt(1), isolate);
  Handle<Object> smi7(Smi::FromInt(7), isolate);
  Handle<Object> smi13(Smi::FromInt(13), isolate);
  Handle<Object> smi42(Smi::FromInt(42), isolate);

  Handle<Object> expect_found(Smi::FromInt(kFound), isolate);
  Handle<Object> expect_absent(Smi::FromInt(kAbsent), isolate);
  Handle<Object> expect_not_found(Smi::FromInt(kNotFound), isolate);
  Handle<Object> expect_bailout(Smi::FromInt(kBailout), isolate);

#define CHECK_FOUND(object, index)                         \
  CHECK(JSReceiver::HasElement(object, index).FromJust()); \
  ft.CheckTrue(object, smi##index, expect_found);

#define CHECK_NOT_FOUND(object, index)                      \
  CHECK(!JSReceiver::HasElement(object, index).FromJust()); \
  ft.CheckTrue(object, smi##index, expect_not_found);

#define CHECK_ABSENT(object, index)                  \
  {                                                  \
    Handle<Smi> smi(Smi::FromInt(index), isolate);   \
    LookupIterator::Key key(isolate, smi);           \
    LookupIterator it(isolate, object, key);         \
    CHECK(!JSReceiver::HasProperty(&it).FromJust()); \
    ft.CheckTrue(object, smi, expect_absent);        \
  }

  {
    Handle<JSArray> object = factory->NewJSArray(0, PACKED_SMI_ELEMENTS);
    AddElement(object, 0, smi0);
    AddElement(object, 1, smi0);
    CHECK_EQ(PACKED_SMI_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_FOUND(object, 1);
    CHECK_NOT_FOUND(object, 7);
    CHECK_NOT_FOUND(object, 13);
    CHECK_NOT_FOUND(object, 42);
  }

  {
    Handle<JSArray> object = factory->NewJSArray(0, HOLEY_SMI_ELEMENTS);
    AddElement(object, 0, smi0);
    AddElement(object, 13, smi0);
    CHECK_EQ(HOLEY_SMI_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_NOT_FOUND(object, 1);
    CHECK_NOT_FOUND(object, 7);
    CHECK_FOUND(object, 13);
    CHECK_NOT_FOUND(object, 42);
  }

  {
    Handle<JSArray> object = factory->NewJSArray(0, PACKED_ELEMENTS);
    AddElement(object, 0, smi0);
    AddElement(object, 1, smi0);
    CHECK_EQ(PACKED_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_FOUND(object, 1);
    CHECK_NOT_FOUND(object, 7);
    CHECK_NOT_FOUND(object, 13);
    CHECK_NOT_FOUND(object, 42);
  }

  {
    Handle<JSArray> object = factory->NewJSArray(0, HOLEY_ELEMENTS);
    AddElement(object, 0, smi0);
    AddElement(object, 13, smi0);
    CHECK_EQ(HOLEY_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_NOT_FOUND(object, 1);
    CHECK_NOT_FOUND(object, 7);
    CHECK_FOUND(object, 13);
    CHECK_NOT_FOUND(object, 42);
  }

  {
    v8::Local<v8::ArrayBuffer> buffer =
        v8::ArrayBuffer::New(reinterpret_cast<v8::Isolate*>(isolate), 8);
    Handle<JSTypedArray> object = factory->NewJSTypedArray(
        kExternalInt32Array, v8::Utils::OpenHandle(*buffer), 0, 2);

    CHECK_EQ(INT32_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_FOUND(object, 1);
    CHECK_ABSENT(object, -10);
    CHECK_ABSENT(object, 13);
    CHECK_ABSENT(object, 42);

    {
      std::shared_ptr<v8::BackingStore> backing_store =
          buffer->GetBackingStore();
      buffer->Detach();
    }
    CHECK_ABSENT(object, 0);
    CHECK_ABSENT(object, 1);
    CHECK_ABSENT(object, -10);
    CHECK_ABSENT(object, 13);
    CHECK_ABSENT(object, 42);
  }

  {
    Handle<JSFunction> constructor = isolate->string_function();
    Handle<JSObject> object = factory->NewJSObject(constructor);
    Handle<String> str = factory->InternalizeUtf8String("ab");
    Handle<JSPrimitiveWrapper>::cast(object)->set_value(*str);
    AddElement(object, 13, smi0);
    CHECK_EQ(FAST_STRING_WRAPPER_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_FOUND(object, 1);
    CHECK_NOT_FOUND(object, 7);
    CHECK_FOUND(object, 13);
    CHECK_NOT_FOUND(object, 42);
  }

  {
    Handle<JSFunction> constructor = isolate->string_function();
    Handle<JSObject> object = factory->NewJSObject(constructor);
    Handle<String> str = factory->InternalizeUtf8String("ab");
    Handle<JSPrimitiveWrapper>::cast(object)->set_value(*str);
    AddElement(object, 13, smi0);
    JSObject::NormalizeElements(object);
    CHECK_EQ(SLOW_STRING_WRAPPER_ELEMENTS, object->map().elements_kind());

    CHECK_FOUND(object, 0);
    CHECK_FOUND(object, 1);
    CHECK_NOT_FOUND(object, 7);
    CHECK_FOUND(object, 13);
    CHECK_NOT_FOUND(object, 42);
  }

  // TODO(ishell): uncomment once NO_ELEMENTS kind is supported.
  //  {
  //    Handle<Map> map = Map::Create(isolate, 0);
  //    map->set_elements_kind(NO_ELEMENTS);
  //    Handle<JSObject> object = factory->NewJSObjectFromMap(map);
  //    CHECK_EQ(NO_ELEMENTS, object->map()->elements_kind());
  //
  //    CHECK_NOT_FOUND(object, 0);
  //    CHECK_NOT_FOUND(object, 1);
  //    CHECK_NOT_FOUND(object, 7);
  //    CHECK_NOT_FOUND(object, 13);
  //    CHECK_NOT_FOUND(object, 42);
  //  }

#undef CHECK_FOUND
#undef CHECK_NOT_FOUND
#undef CHECK_ABSENT

  {
    Handle<JSArray> handler = factory->NewJSArray(0);
    Handle<JSFunction> function =
        factory->NewFunctionForTesting(factory->empty_string());
    Handle<JSProxy> object = factory->NewJSProxy(function, handler);
    CHECK_EQ(JS_PROXY_TYPE, object->map().instance_type());
    ft.CheckTrue(object, smi0, expect_bailout);
  }

  {
    Handle<JSObject> object = isolate->global_object();
    CHECK_EQ(JS_GLOBAL_OBJECT_TYPE, object->map().instance_type());
    ft.CheckTrue(object, smi0, expect_bailout);
  }

  {
    Handle<JSObject> object = isolate->global_proxy();
    CHECK_EQ(JS_GLOBAL_PROXY_TYPE, object->map().instance_type());
    ft.CheckTrue(object, smi0, expect_bailout);
  }
}

TEST(AllocateJSObjectFromMap) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* factory = isolate->factory();

  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  {
    auto map = m.Parameter<Map>(1);
    auto properties = m.Parameter<HeapObject>(2);
    auto elements = m.Parameter<FixedArray>(3);

    TNode<JSObject> result =
        m.AllocateJSObjectFromMap(map, properties, elements);

    CodeStubAssembler::Label done(&m);
    m.GotoIfNot(m.IsJSArrayMap(map), &done);

    // JS array verification requires the length field to be set.
    m.StoreObjectFieldNoWriteBarrier(result, JSArray::kLengthOffset,
                                     m.SmiConstant(0));
    m.Goto(&done);

    m.Bind(&done);
    m.Return(result);
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Map> maps[] = {
      handle(isolate->object_function()->initial_map(), isolate),
      handle(isolate->array_function()->initial_map(), isolate),
  };

  {
    Handle<FixedArray> empty_fixed_array = factory->empty_fixed_array();
    Handle<PropertyArray> empty_property_array =
        factory->empty_property_array();
    for (size_t i = 0; i < arraysize(maps); i++) {
      Handle<Map> map = maps[i];
      Handle<JSObject> result = Handle<JSObject>::cast(
          ft.Call(map, empty_fixed_array, empty_fixed_array).ToHandleChecked());
      CHECK_EQ(result->map(), *map);
      CHECK_EQ(result->property_array(), *empty_property_array);
      CHECK_EQ(result->elements(), *empty_fixed_array);
      CHECK(result->HasFastProperties());
#ifdef VERIFY_HEAP
      isolate->heap()->Verify();
#endif
    }
  }

  {
    // TODO(cbruni): handle in-object properties
    Handle<JSObject> object = Handle<JSObject>::cast(
        v8::Utils::OpenHandle(*CompileRun("var object = {a:1,b:2, 1:1, 2:2}; "
                                          "object")));
    JSObject::NormalizeProperties(isolate, object, KEEP_INOBJECT_PROPERTIES, 0,
                                  "Normalize");
    Handle<HeapObject> properties =
        V8_ENABLE_SWISS_NAME_DICTIONARY_BOOL
            ? Handle<HeapObject>(object->property_dictionary_swiss(), isolate)
            : handle(object->property_dictionary(), isolate);
    Handle<JSObject> result = Handle<JSObject>::cast(
        ft.Call(handle(object->map(), isolate), properties,
                handle(object->elements(), isolate))
            .ToHandleChecked());
    CHECK_EQ(result->map(), object->map());
    if (V8_ENABLE_SWISS_NAME_DICTIONARY_BOOL) {
      CHECK_EQ(result->property_dictionary_swiss(),
               object->property_dictionary_swiss());
    } else {
      CHECK_EQ(result->property_dictionary(), object->property_dictionary());
    }
    CHECK(!result->HasFastProperties());
#ifdef VERIFY_HEAP
    isolate->heap()->Verify();
#endif
  }
}

namespace {

template <typename Dictionary>
using CSAAllocator =
    std::function<TNode<Dictionary>(CodeStubAssembler&, TNode<IntPtrT>)> const&;

template <typename Dictionary>
using Allocator = std::function<Handle<Dictionary>(Isolate*, int)> const&;

// Tests that allocation code emitted by {csa_alloc} yields ordered hash tables
// identical to those produced by {alloc}.
template <typename Dictionary>
void TestDictionaryAllocation(CSAAllocator<Dictionary> csa_alloc,
                              Allocator<Dictionary> alloc, int max_capacity) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  {
    auto capacity = m.Parameter<Smi>(1);
    TNode<Dictionary> result = csa_alloc(m, m.SmiUntag(capacity));
    m.Return(result);
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  {
    for (int i = 0; i < max_capacity; i = i * 1.1 + 1) {
      Handle<HeapObject> result = Handle<HeapObject>::cast(
          ft.Call(handle(Smi::FromInt(i), isolate)).ToHandleChecked());
      Handle<Dictionary> dict = alloc(isolate, i);
      // Both dictionaries should be memory equal.
      int size = dict->Size();
      CHECK_EQ(0, memcmp(reinterpret_cast<void*>(dict->address()),
                         reinterpret_cast<void*>(result->address()), size));
    }
  }
}

}  // namespace

TEST(AllocateNameDictionary) {
  auto csa_alloc = [](CodeStubAssembler& m, TNode<IntPtrT> cap) {
    return m.AllocateNameDictionary(cap);
  };
  auto alloc = [](Isolate* isolate, int capacity) {
    return NameDictionary::New(isolate, capacity);
  };
  TestDictionaryAllocation<NameDictionary>(csa_alloc, alloc, 256);
}

TEST(AllocateOrderedNameDictionary) {
  auto csa_alloc = [](CodeStubAssembler& m, TNode<IntPtrT> cap) {
    return m.AllocateOrderedNameDictionary(cap);
  };
  auto alloc = [](Isolate* isolate, int capacity) {
    return OrderedNameDictionary::Allocate(isolate, capacity).ToHandleChecked();
  };
  TestDictionaryAllocation<OrderedNameDictionary>(csa_alloc, alloc, 256);
}

TEST(AllocateOrderedHashSet) {
  // ignoring capacitites, as the API cannot take them
  auto csa_alloc = [](CodeStubAssembler& m, TNode<IntPtrT> cap) {
    return m.AllocateOrderedHashSet();
  };
  auto alloc = [](Isolate* isolate, int capacity) {
    return OrderedHashSet::Allocate(isolate, OrderedHashSet::kInitialCapacity)
        .ToHandleChecked();
  };
  TestDictionaryAllocation<OrderedHashSet>(csa_alloc, alloc, 1);
}

TEST(AllocateOrderedHashMap) {
  // ignoring capacities, as the API cannot take them
  auto csa_alloc = [](CodeStubAssembler& m, TNode<IntPtrT> cap) {
    return m.AllocateOrderedHashMap();
  };
  auto alloc = [](Isolate* isolate, int capacity) {
    return OrderedHashMap::Allocate(isolate, OrderedHashMap::kInitialCapacity)
        .ToHandleChecked();
  };
  TestDictionaryAllocation<OrderedHashMap>(csa_alloc, alloc, 1);
}

TEST(PopAndReturnFromJSBuiltinWithStackParameters) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumStackParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumStackParams);
  {
    CodeStubAssembler m(asm_tester.state());
    m.PopAndReturn(m.SmiUntag(m.Parameter<Smi>(0)),
                   m.SmiConstant(Smi::FromInt(1234)));
  }

  // Attempt to generate code must trigger CHECK failure in RawMachineAssebler.
  // PopAndReturn is not allowed in builtins with JS linkage and declared stack
  // parameters.
  asm_tester.GenerateCode();
}

TEST(PopAndReturnFromTFCBuiltinWithStackParameters) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  // Setup CSA for creating TFC-style builtin with stack arguments.
  // For the testing purposes we need any interface descriptor that has at
  // least one argument passed on stack.
  using Descriptor = FlatMapIntoArrayDescriptor;
  Descriptor descriptor;
  CHECK_LT(0, Descriptor::GetStackParameterCount());

  CodeAssemblerTester asm_tester(isolate, Descriptor());
  {
    CodeStubAssembler m(asm_tester.state());
    m.PopAndReturn(m.SmiUntag(m.Parameter<Smi>(0)),
                   m.SmiConstant(Smi::FromInt(1234)));
  }

  // Attempt to generate code must trigger CHECK failure in RawMachineAssebler.
  // PopAndReturn is not allowed in builtins with JS linkage and declared stack
  // parameters.
  asm_tester.GenerateCode();
}

namespace {

TNode<Object> MakeConstantNode(CodeStubAssembler& m, Handle<Object> value) {
  if (value->IsSmi()) {
    return m.SmiConstant(Smi::ToInt(*value));
  }
  return m.HeapConstant(Handle<HeapObject>::cast(value));
}

// Buids a CSA function that calls |target| function with given arguments
// |number_of_iterations| times and checks that the stack pointer values before
// the calls and after the calls are the same.
// Then this new function is called multiple times.
template <typename... Args>
void CallFunctionWithStackPointerChecks(Isolate* isolate,
                                        Handle<Object> expected_result,
                                        Handle<Object> target,
                                        Handle<Object> receiver, Args... args) {
  // Setup CSA for creating TFJ-style builtin.
  using Descriptor = JSTrampolineDescriptor;
  CodeAssemblerTester asm_tester(isolate, Descriptor());

  {
    CodeStubAssembler m(asm_tester.state());

    TNode<Context> context = m.Parameter<Context>(Descriptor::kContext);

#ifdef V8_CC_GNU
    // GetStackPointer is available only when V8_CC_GNU is defined.
    const TNode<ExternalReference> get_stack_ptr = m.ExternalConstant(
        ExternalReference::Create(reinterpret_cast<Address>(GetStackPointer)));

    // CSA doesn't have instructions for reading current stack pointer value,
    // so we use a C function that returns address of its local variable.
    // This is a good-enough approximation for the stack pointer.
    MachineType type_intptr = MachineType::IntPtr();
    TNode<WordT> stack_pointer0 =
        m.UncheckedCast<WordT>(m.CallCFunction(get_stack_ptr, type_intptr));
#endif

    // CSA::CallCFunction() aligns stack pointer before the call, so off-by one
    // errors will not be detected. In order to handle this we do the calls in a
    // loop in order to exaggerate the effect of potentially broken stack
    // pointer so that the GetStackPointer function will be able to notice it.
    m.BuildFastLoop<IntPtrT>(
        m.IntPtrConstant(0), m.IntPtrConstant(153),
        [&](TNode<IntPtrT> index) {
          TNode<Object> result = m.Call(context, MakeConstantNode(m, target),
                                        MakeConstantNode(m, receiver),
                                        MakeConstantNode(m, args)...);
          CSA_CHECK(
              &m, m.TaggedEqual(result, MakeConstantNode(m, expected_result)));
        },
        1, CodeStubAssembler::IndexAdvanceMode::kPost);

#ifdef V8_CC_GNU
    TNode<WordT> stack_pointer1 =
        m.UncheckedCast<WordT>(m.CallCFunction(get_stack_ptr, type_intptr));
    CSA_CHECK(&m, m.WordEqual(stack_pointer0, stack_pointer1));
#endif
    m.Return(m.SmiConstant(42));
  }
  FunctionTester ft(asm_tester.GenerateCode(), 1);  // Include receiver.

  Handle<Object> result;
  for (int test_count = 0; test_count < 100; ++test_count) {
    result = ft.Call().ToHandleChecked();
    CHECK_EQ(Smi::FromInt(42), *result);
  }
}

}  // namespace

TEST(PopAndReturnConstant) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  // Setup CSA for creating TFJ-style builtin.
  using Descriptor = JSTrampolineDescriptor;
  CodeAssemblerTester asm_tester(isolate, Descriptor());

  const int kNumParams = 4;  // Not including receiver
  {
    CodeStubAssembler m(asm_tester.state());
    TNode<Int32T> argc =
        m.UncheckedParameter<Int32T>(Descriptor::kActualArgumentsCount);
    CSA_CHECK(&m, m.Word32Equal(argc, m.Int32Constant(kNumParams)));

    m.PopAndReturn(m.IntPtrConstant(kNumParams + 1),  // Include receiver.
                   m.SmiConstant(1234));
  }

  FunctionTester ft(asm_tester.GenerateCode(), 0);
  ft.function->shared().DontAdaptArguments();

  // Now call this function multiple time also checking that the stack pointer
  // didn't change after the calls.
  Handle<Object> receiver = isolate->factory()->undefined_value();
  Handle<Smi> expected_result(Smi::FromInt(1234), isolate);
  CallFunctionWithStackPointerChecks(isolate, expected_result, ft.function,
                                     receiver,
                                     // Pass kNumParams arguments.
                                     Handle<Smi>(Smi::FromInt(1), isolate),
                                     Handle<Smi>(Smi::FromInt(2), isolate),
                                     Handle<Smi>(Smi::FromInt(3), isolate),
                                     Handle<Smi>(Smi::FromInt(4), isolate));
}

TEST(PopAndReturnVariable) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  // Setup CSA for creating TFJ-style builtin.
  using Descriptor = JSTrampolineDescriptor;
  CodeAssemblerTester asm_tester(isolate, Descriptor());

  const int kNumParams = 4;  // Not including receiver
  {
    CodeStubAssembler m(asm_tester.state());
    TNode<Int32T> argc =
        m.UncheckedParameter<Int32T>(Descriptor::kActualArgumentsCount);
    CSA_CHECK(&m, m.Word32Equal(argc, m.Int32Constant(kNumParams)));

    TNode<Int32T> argc_with_receiver = m.Int32Add(argc, m.Int32Constant(1));
    m.PopAndReturn(m.ChangeInt32ToIntPtr(argc_with_receiver),
                   m.SmiConstant(1234));
  }

  FunctionTester ft(asm_tester.GenerateCode(), 0);
  ft.function->shared().DontAdaptArguments();

  // Now call this function multiple time also checking that the stack pointer
  // didn't change after the calls.
  Handle<Object> receiver = isolate->factory()->undefined_value();
  Handle<Smi> expected_result(Smi::FromInt(1234), isolate);
  CallFunctionWithStackPointerChecks(isolate, expected_result, ft.function,
                                     receiver,
                                     // Pass kNumParams arguments.
                                     Handle<Smi>(Smi::FromInt(1), isolate),
                                     Handle<Smi>(Smi::FromInt(2), isolate),
                                     Handle<Smi>(Smi::FromInt(3), isolate),
                                     Handle<Smi>(Smi::FromInt(4), isolate));
}

TEST(OneToTwoByteStringCopy) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  StringBuiltinsAssembler m(asm_tester.state());

  m.CopyStringCharacters<String>(m.Parameter<String>(1), m.Parameter<String>(2),
                                 m.IntPtrConstant(0), m.IntPtrConstant(0),
                                 m.IntPtrConstant(5), String::ONE_BYTE_ENCODING,
                                 String::TWO_BYTE_ENCODING);
  m.Return(m.SmiConstant(Smi::FromInt(0)));

  Handle<String> string1 = isolate->factory()->InternalizeUtf8String("abcde");
  uc16 array[] = {1000, 1001, 1002, 1003, 1004};
  Handle<String> string2 = isolate->factory()
                               ->NewStringFromTwoByte(ArrayVector(array))
                               .ToHandleChecked();
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  ft.Call(string1, string2);
  DisallowGarbageCollection no_gc;
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[0],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[0]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[1],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[1]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[2],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[2]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[3],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[3]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[4],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[4]);
}

TEST(OneToOneByteStringCopy) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  StringBuiltinsAssembler m(asm_tester.state());

  m.CopyStringCharacters<String>(m.Parameter<String>(1), m.Parameter<String>(2),
                                 m.IntPtrConstant(0), m.IntPtrConstant(0),
                                 m.IntPtrConstant(5), String::ONE_BYTE_ENCODING,
                                 String::ONE_BYTE_ENCODING);
  m.Return(m.SmiConstant(Smi::FromInt(0)));

  Handle<String> string1 = isolate->factory()->InternalizeUtf8String("abcde");
  uint8_t array[] = {100, 101, 102, 103, 104};
  Handle<String> string2 = isolate->factory()
                               ->NewStringFromOneByte(ArrayVector(array))
                               .ToHandleChecked();
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  ft.Call(string1, string2);
  DisallowGarbageCollection no_gc;
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[0],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[0]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[1],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[1]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[2],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[2]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[3],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[3]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[4],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[4]);
}

TEST(OneToOneByteStringCopyNonZeroStart) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  StringBuiltinsAssembler m(asm_tester.state());

  m.CopyStringCharacters<String>(m.Parameter<String>(1), m.Parameter<String>(2),
                                 m.IntPtrConstant(0), m.IntPtrConstant(3),
                                 m.IntPtrConstant(2), String::ONE_BYTE_ENCODING,
                                 String::ONE_BYTE_ENCODING);
  m.Return(m.SmiConstant(Smi::FromInt(0)));

  Handle<String> string1 = isolate->factory()->InternalizeUtf8String("abcde");
  uint8_t array[] = {100, 101, 102, 103, 104};
  Handle<String> string2 = isolate->factory()
                               ->NewStringFromOneByte(ArrayVector(array))
                               .ToHandleChecked();
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  ft.Call(string1, string2);
  DisallowGarbageCollection no_gc;
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[0],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[3]);
  CHECK_EQ(Handle<SeqOneByteString>::cast(string1)->GetChars(no_gc)[1],
           Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[4]);
  CHECK_EQ(100, Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[0]);
  CHECK_EQ(101, Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[1]);
  CHECK_EQ(102, Handle<SeqOneByteString>::cast(string2)->GetChars(no_gc)[2]);
}

TEST(TwoToTwoByteStringCopy) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  StringBuiltinsAssembler m(asm_tester.state());

  m.CopyStringCharacters<String>(m.Parameter<String>(1), m.Parameter<String>(2),
                                 m.IntPtrConstant(0), m.IntPtrConstant(0),
                                 m.IntPtrConstant(5), String::TWO_BYTE_ENCODING,
                                 String::TWO_BYTE_ENCODING);
  m.Return(m.SmiConstant(Smi::FromInt(0)));

  uc16 array1[] = {2000, 2001, 2002, 2003, 2004};
  Handle<String> string1 = isolate->factory()
                               ->NewStringFromTwoByte(ArrayVector(array1))
                               .ToHandleChecked();
  uc16 array2[] = {1000, 1001, 1002, 1003, 1004};
  Handle<String> string2 = isolate->factory()
                               ->NewStringFromTwoByte(ArrayVector(array2))
                               .ToHandleChecked();
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  ft.Call(string1, string2);
  DisallowGarbageCollection no_gc;
  CHECK_EQ(Handle<SeqTwoByteString>::cast(string1)->GetChars(no_gc)[0],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[0]);
  CHECK_EQ(Handle<SeqTwoByteString>::cast(string1)->GetChars(no_gc)[1],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[1]);
  CHECK_EQ(Handle<SeqTwoByteString>::cast(string1)->GetChars(no_gc)[2],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[2]);
  CHECK_EQ(Handle<SeqTwoByteString>::cast(string1)->GetChars(no_gc)[3],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[3]);
  CHECK_EQ(Handle<SeqTwoByteString>::cast(string1)->GetChars(no_gc)[4],
           Handle<SeqTwoByteString>::cast(string2)->GetChars(no_gc)[4]);
}

TEST(Arguments) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  // Setup CSA for creating TFJ-style builtin.
  using Descriptor = JSTrampolineDescriptor;
  CodeAssemblerTester asm_tester(isolate, Descriptor());

  {
    CodeStubAssembler m(asm_tester.state());
    TNode<Int32T> argc =
        m.UncheckedParameter<Int32T>(Descriptor::kActualArgumentsCount);
    CodeStubArguments arguments(&m, argc);

    CSA_CHECK(&m, m.TaggedEqual(arguments.AtIndex(0), m.SmiConstant(12)));
    CSA_CHECK(&m, m.TaggedEqual(arguments.AtIndex(1), m.SmiConstant(13)));
    CSA_CHECK(&m, m.TaggedEqual(arguments.AtIndex(2), m.SmiConstant(14)));

    arguments.PopAndReturn(arguments.GetReceiver());
  }

  FunctionTester ft(asm_tester.GenerateCode(), 0);
  ft.function->shared().DontAdaptArguments();

  Handle<Object> result;
  result = ft.Call(Handle<Smi>(Smi::FromInt(12), isolate),
                   Handle<Smi>(Smi::FromInt(13), isolate),
                   Handle<Smi>(Smi::FromInt(14), isolate))
               .ToHandleChecked();
  // When calling with undefined object as the receiver, the CallFunction
  // builtin swaps it to the global proxy object.
  CHECK_EQ(*isolate->global_proxy(), *result);

  result = ft.Call(Handle<Smi>(Smi::FromInt(12), isolate),
                   Handle<Smi>(Smi::FromInt(13), isolate),
                   Handle<Smi>(Smi::FromInt(14), isolate),
                   Handle<Smi>(Smi::FromInt(15), isolate))
               .ToHandleChecked();
  CHECK_EQ(*isolate->global_proxy(), *result);

  result = ft.Call(Handle<Smi>(Smi::FromInt(12), isolate),
                   Handle<Smi>(Smi::FromInt(13), isolate),
                   Handle<Smi>(Smi::FromInt(14), isolate),
                   Handle<Smi>(Smi::FromInt(15), isolate),
                   Handle<Smi>(Smi::FromInt(16), isolate),
                   Handle<Smi>(Smi::FromInt(17), isolate),
                   Handle<Smi>(Smi::FromInt(18), isolate),
                   Handle<Smi>(Smi::FromInt(19), isolate))
               .ToHandleChecked();
  CHECK_EQ(*isolate->global_proxy(), *result);
}

TEST(ArgumentsForEach) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  // Setup CSA for creating TFJ-style builtin.
  using Descriptor = JSTrampolineDescriptor;
  CodeAssemblerTester asm_tester(isolate, Descriptor());

  {
    CodeStubAssembler m(asm_tester.state());

    TNode<Int32T> argc =
        m.UncheckedParameter<Int32T>(Descriptor::kActualArgumentsCount);
    CodeStubArguments arguments(&m, argc);

    TVariable<Smi> sum(&m);
    CodeAssemblerVariableList list({&sum}, m.zone());

    sum = m.SmiConstant(0);

    arguments.ForEach(list, [&](TNode<Object> arg) {
      sum = m.SmiAdd(sum.value(), m.CAST(arg));
    });

    arguments.PopAndReturn(sum.value());
  }

  FunctionTester ft(asm_tester.GenerateCode(), 0);
  ft.function->shared().DontAdaptArguments();

  Handle<Object> result;
  result = ft.Call(Handle<Smi>(Smi::FromInt(12), isolate),
                   Handle<Smi>(Smi::FromInt(13), isolate),
                   Handle<Smi>(Smi::FromInt(14), isolate))
               .ToHandleChecked();
  CHECK_EQ(Smi::FromInt(12 + 13 + 14), *result);

  result = ft.Call(Handle<Smi>(Smi::FromInt(12), isolate),
                   Handle<Smi>(Smi::FromInt(13), isolate),
                   Handle<Smi>(Smi::FromInt(14), isolate),
                   Handle<Smi>(Smi::FromInt(15), isolate))
               .ToHandleChecked();
  CHECK_EQ(Smi::FromInt(12 + 13 + 14 + 15), *result);

  result = ft.Call(Handle<Smi>(Smi::FromInt(12), isolate),
                   Handle<Smi>(Smi::FromInt(13), isolate),
                   Handle<Smi>(Smi::FromInt(14), isolate),
                   Handle<Smi>(Smi::FromInt(15), isolate),
                   Handle<Smi>(Smi::FromInt(16), isolate),
                   Handle<Smi>(Smi::FromInt(17), isolate),
                   Handle<Smi>(Smi::FromInt(18), isolate),
                   Handle<Smi>(Smi::FromInt(19), isolate))
               .ToHandleChecked();
  CHECK_EQ(Smi::FromInt(12 + 13 + 14 + 15 + 16 + 17 + 18 + 19), *result);
}

TEST(IsDebugActive) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  Label if_active(&m), if_not_active(&m);

  m.Branch(m.IsDebugActive(), &if_active, &if_not_active);
  m.BIND(&if_active);
  m.Return(m.TrueConstant());
  m.BIND(&if_not_active);
  m.Return(m.FalseConstant());

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  CHECK(!isolate->debug()->is_active());
  Handle<Object> result =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);

  bool* debug_is_active = reinterpret_cast<bool*>(
      ExternalReference::debug_is_active_address(isolate).address());

  // Cheat to enable debug (TODO: do this properly).
  *debug_is_active = true;

  result = ft.Call().ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).true_value(), *result);

  // Reset debug mode.
  *debug_is_active = false;
}

// Ensure that the kShortBuiltinCallsOldSpaceSizeThreshold constant can be used
// for detecting whether the machine has >= 4GB of physical memory by checking
// the max old space size.
TEST(ShortBuiltinCallsThreshold) {
  if (!V8_SHORT_BUILTIN_CALLS_BOOL) return;

  const uint64_t kPhysicalMemoryThreshold = size_t{4} * GB;

  size_t heap_size, old, young;

  // If the physical memory is < kPhysicalMemoryThreshold then the old space
  // size must be below the kShortBuiltinCallsOldSpaceThreshold.
  heap_size = Heap::HeapSizeFromPhysicalMemory(kPhysicalMemoryThreshold - MB);
  i::Heap::GenerationSizesFromHeapSize(heap_size, &young, &old);
  CHECK_LT(old, kShortBuiltinCallsOldSpaceSizeThreshold);

  // If the physical memory is >= kPhysicalMemoryThreshold then the old space
  // size must be below the kShortBuiltinCallsOldSpaceThreshold.
  heap_size = Heap::HeapSizeFromPhysicalMemory(kPhysicalMemoryThreshold);
  i::Heap::GenerationSizesFromHeapSize(heap_size, &young, &old);
  CHECK_GE(old, kShortBuiltinCallsOldSpaceSizeThreshold);

  heap_size = Heap::HeapSizeFromPhysicalMemory(kPhysicalMemoryThreshold + MB);
  i::Heap::GenerationSizesFromHeapSize(heap_size, &young, &old);
  CHECK_GE(old, kShortBuiltinCallsOldSpaceSizeThreshold);
}

TEST(CallBuiltin) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate,
                                 kNumParams + 1);  // Include receiver.
  PromiseBuiltinsAssembler m(asm_tester.state());

  {
    auto receiver = m.Parameter<Object>(1);
    auto name = m.Parameter<Name>(2);
    auto context = m.Parameter<Context>(kNumParams + 3);

    auto value = m.CallBuiltin(Builtins::kGetProperty, context, receiver, name);
    m.Return(value);
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Factory* factory = isolate->factory();
  Handle<Name> name = factory->InternalizeUtf8String("a");
  Handle<Object> value(Smi::FromInt(153), isolate);
  Handle<JSObject> object = factory->NewJSObjectWithNullProto();
  JSObject::AddProperty(isolate, object, name, value, NONE);

  Handle<Object> result = ft.Call(object, name).ToHandleChecked();
  CHECK_EQ(*value, *result);
}

TEST(TailCallBuiltin) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate,
                                 kNumParams + 1);  // Include receiver.
  PromiseBuiltinsAssembler m(asm_tester.state());

  {
    auto receiver = m.Parameter<Object>(1);
    auto name = m.Parameter<Name>(2);
    auto context = m.Parameter<Context>(kNumParams + 3);

    m.TailCallBuiltin(Builtins::kGetProperty, context, receiver, name);
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Factory* factory = isolate->factory();
  Handle<Name> name = factory->InternalizeUtf8String("a");
  Handle<Object> value(Smi::FromInt(153), isolate);
  Handle<JSObject> object = factory->NewJSObjectWithNullProto();
  JSObject::AddProperty(isolate, object, name, value, NONE);

  Handle<Object> result = ft.Call(object, name).ToHandleChecked();
  CHECK_EQ(*value, *result);
}

class AppendJSArrayCodeStubAssembler : public CodeStubAssembler {
 public:
  AppendJSArrayCodeStubAssembler(compiler::CodeAssemblerState* state,
                                 ElementsKind kind)
      : CodeStubAssembler(state), kind_(kind) {}

  void TestAppendJSArrayImpl(Isolate* isolate, CodeAssemblerTester* csa_tester,
                             Handle<Object> o1, Handle<Object> o2,
                             Handle<Object> o3, Handle<Object> o4,
                             int initial_size, int result_size) {
    Handle<JSArray> array = isolate->factory()->NewJSArray(
        kind_, 2, initial_size, INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE);
    Object::SetElement(isolate, array, 0, Handle<Smi>(Smi::FromInt(1), isolate),
                       kDontThrow)
        .Check();
    Object::SetElement(isolate, array, 1, Handle<Smi>(Smi::FromInt(2), isolate),
                       kDontThrow)
        .Check();
    CodeStubArguments args(this, IntPtrConstant(kNumParams));
    TVariable<IntPtrT> arg_index(this);
    Label bailout(this);
    arg_index = IntPtrConstant(0);
    TNode<Smi> length = BuildAppendJSArray(kind_, HeapConstant(array), &args,
                                           &arg_index, &bailout);
    Return(length);

    BIND(&bailout);
    Return(SmiTag(IntPtrAdd(arg_index.value(), IntPtrConstant(2))));

    FunctionTester ft(csa_tester->GenerateCode(), kNumParams);

    Handle<Object> result = ft.Call(o1, o2, o3, o4).ToHandleChecked();

    CHECK_EQ(kind_, array->GetElementsKind());
    CHECK_EQ(result_size, Handle<Smi>::cast(result)->value());
    CHECK_EQ(result_size, Smi::ToInt(array->length()));
    Handle<Object> obj =
        JSObject::GetElement(isolate, array, 2).ToHandleChecked();
    Handle<HeapObject> undefined_value =
        Handle<HeapObject>(ReadOnlyRoots(isolate).undefined_value(), isolate);
    CHECK_EQ(result_size < 3 ? *undefined_value : *o1, *obj);
    obj = JSObject::GetElement(isolate, array, 3).ToHandleChecked();
    CHECK_EQ(result_size < 4 ? *undefined_value : *o2, *obj);
    obj = JSObject::GetElement(isolate, array, 4).ToHandleChecked();
    CHECK_EQ(result_size < 5 ? *undefined_value : *o3, *obj);
    obj = JSObject::GetElement(isolate, array, 5).ToHandleChecked();
    CHECK_EQ(result_size < 6 ? *undefined_value : *o4, *obj);
  }

  static void TestAppendJSArray(Isolate* isolate, ElementsKind kind, Object o1,
                                Object o2, Object o3, Object o4,
                                int initial_size, int result_size) {
    CodeAssemblerTester asm_tester(isolate, kNumParams);
    AppendJSArrayCodeStubAssembler m(asm_tester.state(), kind);
    m.TestAppendJSArrayImpl(
        isolate, &asm_tester, Handle<Object>(o1, isolate),
        Handle<Object>(o2, isolate), Handle<Object>(o3, isolate),
        Handle<Object>(o4, isolate), initial_size, result_size);
  }

 private:
  static const int kNumParams = 4;
  ElementsKind kind_;
};

TEST(BuildAppendJSArrayFastElement) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      Smi::FromInt(5), Smi::FromInt(6), 6, 6);
}

TEST(BuildAppendJSArrayFastElementGrow) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      Smi::FromInt(5), Smi::FromInt(6), 2, 6);
}

TEST(BuildAppendJSArrayFastSmiElement) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_SMI_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      Smi::FromInt(5), Smi::FromInt(6), 6, 6);
}

TEST(BuildAppendJSArrayFastSmiElementGrow) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_SMI_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      Smi::FromInt(5), Smi::FromInt(6), 2, 6);
}

TEST(BuildAppendJSArrayFastSmiElementObject) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_SMI_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      ReadOnlyRoots(isolate).undefined_value(), Smi::FromInt(6), 6, 4);
}

TEST(BuildAppendJSArrayFastSmiElementObjectGrow) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_SMI_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      ReadOnlyRoots(isolate).undefined_value(), Smi::FromInt(6), 2, 4);
}

TEST(BuildAppendJSArrayFastDoubleElements) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_DOUBLE_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      Smi::FromInt(5), Smi::FromInt(6), 6, 6);
}

TEST(BuildAppendJSArrayFastDoubleElementsGrow) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_DOUBLE_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      Smi::FromInt(5), Smi::FromInt(6), 2, 6);
}

TEST(BuildAppendJSArrayFastDoubleElementsObject) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  AppendJSArrayCodeStubAssembler::TestAppendJSArray(
      isolate, PACKED_DOUBLE_ELEMENTS, Smi::FromInt(3), Smi::FromInt(4),
      ReadOnlyRoots(isolate).undefined_value(), Smi::FromInt(6), 6, 4);
}

namespace {

template <typename Stub, typename... Args>
void Recompile(Args... args) {
  Stub stub(args...);
  stub.DeleteStubFromCacheForTesting();
  stub.GetCode();
}

}  // namespace

void CustomPromiseHook(v8::PromiseHookType type, v8::Local<v8::Promise> promise,
                       v8::Local<v8::Value> parentPromise) {}

TEST(IsPromiseHookEnabled) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());

  m.Return(
      m.SelectBooleanConstant(
          m.IsIsolatePromiseHookEnabledOrHasAsyncEventDelegate()));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);

  isolate->SetPromiseHook(CustomPromiseHook);
  result = ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).true_value(), *result);

  isolate->SetPromiseHook(nullptr);
  result = ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);
}

TEST(NewJSPromise) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  PromiseBuiltinsAssembler m(asm_tester.state());

  auto context = m.Parameter<Context>(kNumParams + 2);
  const TNode<JSPromise> promise = m.NewJSPromise(context);
  m.Return(promise);

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK(result->IsJSPromise());
}

TEST(NewJSPromise2) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  PromiseBuiltinsAssembler m(asm_tester.state());

  auto context = m.Parameter<Context>(kNumParams + 2);
  const TNode<JSPromise> promise =
      m.NewJSPromise(context, v8::Promise::kRejected, m.SmiConstant(1));
  m.Return(promise);

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK(result->IsJSPromise());
  Handle<JSPromise> js_promise = Handle<JSPromise>::cast(result);
  CHECK_EQ(v8::Promise::kRejected, js_promise->status());
  CHECK_EQ(Smi::FromInt(1), js_promise->result());
  CHECK(!js_promise->has_handler());
}

TEST(IsSymbol) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  auto symbol = m.Parameter<HeapObject>(1);
  m.Return(m.SelectBooleanConstant(m.IsSymbol(symbol)));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result =
      ft.Call(isolate->factory()->NewSymbol()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).true_value(), *result);

  result = ft.Call(isolate->factory()->empty_string()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);
}

TEST(IsPrivateSymbol) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  auto symbol = m.Parameter<HeapObject>(1);
  m.Return(m.SelectBooleanConstant(m.IsPrivateSymbol(symbol)));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result =
      ft.Call(isolate->factory()->NewSymbol()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);

  result = ft.Call(isolate->factory()->empty_string()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);

  result = ft.Call(isolate->factory()->NewPrivateSymbol()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).true_value(), *result);
}

TEST(PromiseHasHandler) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  PromiseBuiltinsAssembler m(asm_tester.state());

  auto context = m.Parameter<Context>(kNumParams + 2);
  const TNode<JSPromise> promise =
      m.NewJSPromise(context, m.UndefinedConstant());
  m.Return(m.SelectBooleanConstant(m.PromiseHasHandler(promise)));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(), *result);
}

TEST(CreatePromiseResolvingFunctionsContext) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  PromiseBuiltinsAssembler m(asm_tester.state());

  const auto context = m.Parameter<Context>(kNumParams + 3);
  const TNode<NativeContext> native_context = m.LoadNativeContext(context);
  const TNode<JSPromise> promise =
      m.NewJSPromise(context, m.UndefinedConstant());
  const TNode<Context> promise_context =
      m.CreatePromiseResolvingFunctionsContext(
          context, promise, m.BooleanConstant(false), native_context);
  m.Return(promise_context);

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result = ft.Call().ToHandleChecked();
  CHECK(result->IsContext());
  Handle<Context> context_js = Handle<Context>::cast(result);
  CHECK_EQ(isolate->root(RootIndex::kEmptyScopeInfo), context_js->scope_info());
  CHECK_EQ(*isolate->native_context(), context_js->native_context());
  CHECK(context_js->get(PromiseBuiltins::kPromiseSlot).IsJSPromise());
  CHECK_EQ(ReadOnlyRoots(isolate).false_value(),
           context_js->get(PromiseBuiltins::kDebugEventSlot));
}

TEST(CreatePromiseResolvingFunctions) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  PromiseBuiltinsAssembler m(asm_tester.state());

  auto context = m.Parameter<Context>(kNumParams + 2);
  const TNode<NativeContext> native_context = m.LoadNativeContext(context);
  const TNode<JSPromise> promise =
      m.NewJSPromise(context, m.UndefinedConstant());
  PromiseResolvingFunctions funcs = m.CreatePromiseResolvingFunctions(
      context, promise, m.BooleanConstant(false), native_context);
  TNode<JSFunction> resolve = funcs.resolve;
  TNode<JSFunction> reject = funcs.reject;
  TNode<IntPtrT> const kSize = m.IntPtrConstant(2);
  TNode<FixedArray> const arr =
      m.Cast(m.AllocateFixedArray(PACKED_ELEMENTS, kSize));
  m.StoreFixedArrayElement(arr, 0, resolve);
  m.StoreFixedArrayElement(arr, 1, reject);
  m.Return(arr);

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result_obj =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK(result_obj->IsFixedArray());
  Handle<FixedArray> result_arr = Handle<FixedArray>::cast(result_obj);
  CHECK(result_arr->get(0).IsJSFunction());
  CHECK(result_arr->get(1).IsJSFunction());
}

TEST(NewElementsCapacity) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate, 2);
  CodeStubAssembler m(asm_tester.state());
  m.Return(m.SmiTag(
      m.CalculateNewElementsCapacity(m.SmiUntag(m.Parameter<Smi>(1)))));

  FunctionTester ft(asm_tester.GenerateCode(), 1);
  Handle<Smi> test_value = Handle<Smi>(Smi::FromInt(1), isolate);
  Handle<Smi> result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
  test_value = Handle<Smi>(Smi::FromInt(1), isolate);
  result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
  test_value = Handle<Smi>(Smi::FromInt(2), isolate);
  result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
  test_value = Handle<Smi>(Smi::FromInt(1025), isolate);
  result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
}

TEST(NewElementsCapacitySmi) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  CodeAssemblerTester asm_tester(isolate, 2);
  CodeStubAssembler m(asm_tester.state());
  m.Return(m.CalculateNewElementsCapacity(m.UncheckedParameter<Smi>(1)));

  FunctionTester ft(asm_tester.GenerateCode(), 1);
  Handle<Smi> test_value = Handle<Smi>(Smi::FromInt(0), isolate);
  Handle<Smi> result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
  test_value = Handle<Smi>(Smi::FromInt(1), isolate);
  result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
  test_value = Handle<Smi>(Smi::FromInt(2), isolate);
  result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
  test_value = Handle<Smi>(Smi::FromInt(1025), isolate);
  result_obj = ft.CallChecked<Smi>(test_value);
  CHECK_EQ(
      result_obj->value(),
      static_cast<int>(JSObject::NewElementsCapacity(test_value->value())));
}

TEST(AllocateFunctionWithMapAndContext) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  PromiseBuiltinsAssembler m(asm_tester.state());

  const auto context = m.Parameter<Context>(kNumParams + 2);
  const TNode<NativeContext> native_context = m.LoadNativeContext(context);
  const TNode<JSPromise> promise =
      m.NewJSPromise(context, m.UndefinedConstant());
  TNode<Context> promise_context = m.CreatePromiseResolvingFunctionsContext(
      context, promise, m.BooleanConstant(false), native_context);
  TNode<Object> resolve_info =
      m.PromiseCapabilityDefaultResolveSharedFunConstant();
  const TNode<Object> map = m.LoadContextElement(
      native_context, Context::STRICT_FUNCTION_WITHOUT_PROTOTYPE_MAP_INDEX);
  const TNode<JSFunction> resolve = m.AllocateFunctionWithMapAndContext(
      m.CAST(map), m.CAST(resolve_info), promise_context);
  m.Return(resolve);

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result_obj =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK(result_obj->IsJSFunction());
  Handle<JSFunction> fun = Handle<JSFunction>::cast(result_obj);
  CHECK_EQ(ReadOnlyRoots(isolate).empty_property_array(),
           fun->property_array());
  CHECK_EQ(ReadOnlyRoots(isolate).empty_fixed_array(), fun->elements());
  CHECK_EQ(isolate->heap()->many_closures_cell(), fun->raw_feedback_cell());
  CHECK(!fun->has_prototype_slot());
  CHECK_EQ(*isolate->factory()->promise_capability_default_resolve_shared_fun(),
           fun->shared());
  CHECK_EQ(isolate->factory()
               ->promise_capability_default_resolve_shared_fun()
               ->GetCode(),
           fun->code());
}

TEST(CreatePromiseGetCapabilitiesExecutorContext) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  PromiseBuiltinsAssembler m(asm_tester.state());

  auto context = m.Parameter<Context>(kNumParams + 2);
  TNode<NativeContext> native_context = m.LoadNativeContext(context);

  TNode<PromiseCapability> capability = m.CreatePromiseCapability(
      m.UndefinedConstant(), m.UndefinedConstant(), m.UndefinedConstant());
  TNode<Context> executor_context =
      m.CreatePromiseCapabilitiesExecutorContext(native_context, capability);
  m.Return(executor_context);

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  Handle<Object> result_obj =
      ft.Call(isolate->factory()->undefined_value()).ToHandleChecked();
  CHECK(result_obj->IsContext());
  Handle<Context> context_js = Handle<Context>::cast(result_obj);
  CHECK_EQ(PromiseBuiltins::kCapabilitiesContextLength, context_js->length());
  CHECK_EQ(isolate->root(RootIndex::kEmptyScopeInfo), context_js->scope_info());
  CHECK_EQ(*isolate->native_context(), context_js->native_context());
  CHECK(
      context_js->get(PromiseBuiltins::kCapabilitySlot).IsPromiseCapability());
}

TEST(NewPromiseCapability) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  {  // Builtin Promise
    const int kNumParams = 0;
    CodeAssemblerTester asm_tester(isolate,
                                   kNumParams + 1);  // Include receiver.
    PromiseBuiltinsAssembler m(asm_tester.state());

    auto context = m.Parameter<Context>(kNumParams + 3);
    const TNode<NativeContext> native_context = m.LoadNativeContext(context);
    const TNode<Object> promise_constructor =
        m.LoadContextElement(native_context, Context::PROMISE_FUNCTION_INDEX);

    const TNode<Oddball> debug_event = m.TrueConstant();
    const TNode<Object> capability =
        m.CallBuiltin(Builtins::kNewPromiseCapability, context,
                      promise_constructor, debug_event);
    m.Return(capability);

    FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

    Handle<Object> result_obj = ft.Call().ToHandleChecked();
    CHECK(result_obj->IsPromiseCapability());
    Handle<PromiseCapability> result =
        Handle<PromiseCapability>::cast(result_obj);

    CHECK(result->promise().IsJSPromise());
    CHECK(result->resolve().IsJSFunction());
    CHECK(result->reject().IsJSFunction());
    CHECK_EQ(
        *isolate->factory()->promise_capability_default_reject_shared_fun(),
        JSFunction::cast(result->reject()).shared());
    CHECK_EQ(
        *isolate->factory()->promise_capability_default_resolve_shared_fun(),
        JSFunction::cast(result->resolve()).shared());

    Handle<JSFunction> callbacks[] = {
        handle(JSFunction::cast(result->resolve()), isolate),
        handle(JSFunction::cast(result->reject()), isolate)};

    for (auto&& callback : callbacks) {
      Handle<Context> context(Context::cast(callback->context()), isolate);
      CHECK_EQ(isolate->root(RootIndex::kEmptyScopeInfo),
               context->scope_info());
      CHECK_EQ(*isolate->native_context(), context->native_context());
      CHECK_EQ(PromiseBuiltins::kPromiseContextLength, context->length());
      CHECK_EQ(context->get(PromiseBuiltins::kPromiseSlot), result->promise());
    }
  }

  {  // Custom Promise
    const int kNumParams = 1;
    CodeAssemblerTester asm_tester(isolate,
                                   kNumParams + 1);  // Include receiver.
    PromiseBuiltinsAssembler m(asm_tester.state());

    auto context = m.Parameter<Context>(kNumParams + 3);

    auto constructor = m.Parameter<Object>(1);
    const TNode<Oddball> debug_event = m.TrueConstant();
    const TNode<Object> capability = m.CallBuiltin(
        Builtins::kNewPromiseCapability, context, constructor, debug_event);
    m.Return(capability);

    FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

    Handle<JSFunction> constructor_fn =
        Handle<JSFunction>::cast(v8::Utils::OpenHandle(*CompileRun(
            "(function FakePromise(executor) {"
            "  var self = this;"
            "  function resolve(value) { self.resolvedValue = value; }"
            "  function reject(reason) { self.rejectedReason = reason; }"
            "  executor(resolve, reject);"
            "})")));

    Handle<Object> result_obj = ft.Call(constructor_fn).ToHandleChecked();
    CHECK(result_obj->IsPromiseCapability());
    Handle<PromiseCapability> result =
        Handle<PromiseCapability>::cast(result_obj);

    CHECK(result->promise().IsJSObject());
    Handle<JSObject> promise(JSObject::cast(result->promise()), isolate);
    CHECK_EQ(constructor_fn->prototype_or_initial_map(kAcquireLoad),
             promise->map());
    CHECK(result->resolve().IsJSFunction());
    CHECK(result->reject().IsJSFunction());

    Handle<String> resolved_str =
        isolate->factory()->NewStringFromAsciiChecked("resolvedStr");
    Handle<String> rejected_str =
        isolate->factory()->NewStringFromAsciiChecked("rejectedStr");

    Handle<Object> argv1[] = {resolved_str};
    Handle<Object> ret =
        Execution::Call(isolate, handle(result->resolve(), isolate),
                        isolate->factory()->undefined_value(), 1, argv1)
            .ToHandleChecked();

    Handle<Object> prop1 =
        JSReceiver::GetProperty(isolate, promise, "resolvedValue")
            .ToHandleChecked();
    CHECK_EQ(*resolved_str, *prop1);

    Handle<Object> argv2[] = {rejected_str};
    ret = Execution::Call(isolate, handle(result->reject(), isolate),
                          isolate->factory()->undefined_value(), 1, argv2)
              .ToHandleChecked();
    Handle<Object> prop2 =
        JSReceiver::GetProperty(isolate, promise, "rejectedReason")
            .ToHandleChecked();
    CHECK_EQ(*rejected_str, *prop2);
  }
}

TEST(DirectMemoryTest8BitWord32Immediate) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());
  int8_t buffer[] = {1, 2, 4, 8, 17, 33, 65, 127};
  const int element_count = 8;
  Label bad(&m);

  TNode<RawPtrT> buffer_node = m.PointerConstant(buffer);
  for (size_t i = 0; i < element_count; ++i) {
    for (size_t j = 0; j < element_count; ++j) {
      TNode<Uint8T> loaded =
          m.LoadBufferData<Uint8T>(buffer_node, static_cast<int>(i));
      TNode<Word32T> masked = m.Word32And(loaded, m.Int32Constant(buffer[j]));
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }
    }
  }

  m.Return(m.SmiConstant(1));

  m.BIND(&bad);
  m.Return(m.SmiConstant(0));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  CHECK_EQ(1, ft.CallChecked<Smi>()->value());
}

TEST(DirectMemoryTest16BitWord32Immediate) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());
  int16_t buffer[] = {156, 2234, 4544, 8444, 1723, 3888, 658, 1278};
  const int element_count = 8;
  Label bad(&m);

  TNode<RawPtrT> buffer_node = m.PointerConstant(buffer);
  for (size_t i = 0; i < element_count; ++i) {
    for (size_t j = 0; j < element_count; ++j) {
      TNode<Uint16T> loaded = m.LoadBufferData<Uint16T>(
          buffer_node, static_cast<int>(i * sizeof(int16_t)));
      TNode<Word32T> masked = m.Word32And(loaded, m.Int32Constant(buffer[j]));
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }
    }
  }

  m.Return(m.SmiConstant(1));

  m.BIND(&bad);
  m.Return(m.SmiConstant(0));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  CHECK_EQ(1, ft.CallChecked<Smi>()->value());
}

TEST(DirectMemoryTest8BitWord32) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());
  int8_t buffer[] = {1, 2, 4, 8, 17, 33, 65, 127, 67, 38};
  const int element_count = 10;
  Label bad(&m);
  TNode<Uint32T> constants[element_count];

  TNode<RawPtrT> buffer_node = m.PointerConstant(buffer);
  for (size_t i = 0; i < element_count; ++i) {
    constants[i] = m.LoadBufferData<Uint8T>(buffer_node, static_cast<int>(i));
  }

  for (size_t i = 0; i < element_count; ++i) {
    for (size_t j = 0; j < element_count; ++j) {
      TNode<Uint8T> loaded =
          m.LoadBufferData<Uint8T>(buffer_node, static_cast<int>(i));
      TNode<Word32T> masked = m.Word32And(loaded, constants[j]);
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }

      masked = m.Word32And(constants[i], constants[j]);
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }
    }
  }

  m.Return(m.SmiConstant(1));

  m.BIND(&bad);
  m.Return(m.SmiConstant(0));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  CHECK_EQ(1, ft.CallChecked<Smi>()->value());
}

TEST(DirectMemoryTest16BitWord32) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 0;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());
  int16_t buffer[] = {1, 2, 4, 8, 12345, 33, 65, 255, 67, 3823};
  const int element_count = 10;
  Label bad(&m);
  TNode<Uint32T> constants[element_count];

  TNode<RawPtrT> buffer_node1 = m.PointerConstant(buffer);
  for (size_t i = 0; i < element_count; ++i) {
    constants[i] = m.LoadBufferData<Uint16T>(
        buffer_node1, static_cast<int>(i * sizeof(int16_t)));
  }
  TNode<RawPtrT> buffer_node2 = m.PointerConstant(buffer);

  for (size_t i = 0; i < element_count; ++i) {
    for (size_t j = 0; j < element_count; ++j) {
      TNode<Uint16T> loaded = m.LoadBufferData<Uint16T>(
          buffer_node1, static_cast<int>(i * sizeof(int16_t)));
      TNode<Word32T> masked = m.Word32And(loaded, constants[j]);
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }

      // Force a memory access relative to a high-number register.
      loaded = m.LoadBufferData<Uint16T>(buffer_node2,
                                         static_cast<int>(i * sizeof(int16_t)));
      masked = m.Word32And(loaded, constants[j]);
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }

      masked = m.Word32And(constants[i], constants[j]);
      if ((buffer[j] & buffer[i]) != 0) {
        m.GotoIf(m.Word32Equal(masked, m.Int32Constant(0)), &bad);
      } else {
        m.GotoIf(m.Word32NotEqual(masked, m.Int32Constant(0)), &bad);
      }
    }
  }

  m.Return(m.SmiConstant(1));

  m.BIND(&bad);
  m.Return(m.SmiConstant(0));

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  CHECK_EQ(1, ft.CallChecked<Smi>()->value());
}

TEST(LoadJSArrayElementsMap) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    auto context = m.Parameter<Context>(kNumParams + 3);
    TNode<NativeContext> native_context = m.LoadNativeContext(context);
    TNode<Int32T> kind = m.SmiToInt32(m.Parameter<Smi>(1));
    m.Return(m.LoadJSArrayElementsMap(kind, native_context));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  for (int kind = 0; kind <= HOLEY_DOUBLE_ELEMENTS; kind++) {
    Handle<Map> csa_result =
        ft.CallChecked<Map>(handle(Smi::FromInt(kind), isolate));
    ElementsKind elements_kind = static_cast<ElementsKind>(kind);
    Handle<Map> result(
        isolate->native_context()->GetInitialJSArrayMap(elements_kind),
        isolate);
    CHECK_EQ(*csa_result, *result);
  }
}

TEST(IsWhiteSpaceOrLineTerminator) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.

  {  // Returns true if whitespace, false otherwise.
    CodeStubAssembler m(asm_tester.state());
    Label if_true(&m), if_false(&m);
    m.Branch(m.IsWhiteSpaceOrLineTerminator(
                 m.UncheckedCast<Uint16T>(m.SmiToInt32(m.Parameter<Smi>(1)))),
             &if_true, &if_false);
    m.BIND(&if_true);
    m.Return(m.TrueConstant());
    m.BIND(&if_false);
    m.Return(m.FalseConstant());
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<Object> true_value = ft.true_value();
  Handle<Object> false_value = ft.false_value();

  for (uc16 c = 0; c < 0xFFFF; c++) {
    Handle<Object> expected_value =
        IsWhiteSpaceOrLineTerminator(c) ? true_value : false_value;
    ft.CheckCall(expected_value, handle(Smi::FromInt(c), isolate));
  }
}

TEST(BranchIfNumberRelationalComparison) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* f = isolate->factory();
  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    Label return_true(&m), return_false(&m);
    m.BranchIfNumberRelationalComparison(
        Operation::kGreaterThanOrEqual, m.Parameter<Number>(1),
        m.Parameter<Number>(2), &return_true, &return_false);
    m.BIND(&return_true);
    m.Return(m.BooleanConstant(true));
    m.BIND(&return_false);
    m.Return(m.BooleanConstant(false));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  ft.CheckTrue(f->NewNumber(0), f->NewNumber(0));
  ft.CheckTrue(f->NewNumber(1), f->NewNumber(0));
  ft.CheckTrue(f->NewNumber(1), f->NewNumber(1));
  ft.CheckFalse(f->NewNumber(0), f->NewNumber(1));
  ft.CheckFalse(f->NewNumber(-1), f->NewNumber(0));
  ft.CheckTrue(f->NewNumber(-1), f->NewNumber(-1));

  ft.CheckTrue(f->NewNumber(-1), f->NewNumber(-1.5));
  ft.CheckFalse(f->NewNumber(-1.5), f->NewNumber(-1));
  ft.CheckTrue(f->NewNumber(-1.5), f->NewNumber(-1.5));
}

TEST(IsNumberArrayIndex) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    auto number = m.Parameter<Number>(1);
    m.Return(
        m.SmiFromInt32(m.UncheckedCast<Int32T>(m.IsNumberArrayIndex(number))));
  }

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  double indices[] = {Smi::kMinValue,
                      -11,
                      -1,
                      0,
                      1,
                      2,
                      Smi::kMaxValue,
                      -11.0,
                      -11.1,
                      -2.0,
                      -1.0,
                      -0.0,
                      0.0,
                      0.00001,
                      0.1,
                      1,
                      2,
                      Smi::kMinValue - 1.0,
                      Smi::kMinValue + 1.0,
                      Smi::kMinValue + 1.2,
                      kMaxInt + 1.2,
                      kMaxInt - 10.0,
                      kMaxInt - 1.0,
                      kMaxInt,
                      kMaxInt + 1.0,
                      kMaxInt + 10.0};

  for (size_t i = 0; i < arraysize(indices); i++) {
    Handle<Object> index = isolate->factory()->NewNumber(indices[i]);
    uint32_t array_index;
    CHECK_EQ(index->ToArrayIndex(&array_index),
             (ft.CallChecked<Smi>(index)->value() == 1));
  }
}

TEST(NumberMinMax) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 2;
  CodeAssemblerTester asm_tester_min(isolate,
                                     kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester_min.state());
    m.Return(m.NumberMin(m.Parameter<Number>(1), m.Parameter<Number>(2)));
  }
  FunctionTester ft_min(asm_tester_min.GenerateCode(), kNumParams);

  CodeAssemblerTester asm_tester_max(isolate,
                                     kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester_max.state());
    m.Return(m.NumberMax(m.Parameter<Number>(1), m.Parameter<Number>(2)));
  }
  FunctionTester ft_max(asm_tester_max.GenerateCode(), kNumParams);

  // Test smi values.
  Handle<Smi> smi_1(Smi::FromInt(1), isolate);
  Handle<Smi> smi_2(Smi::FromInt(2), isolate);
  Handle<Smi> smi_5(Smi::FromInt(5), isolate);
  CHECK_EQ(ft_min.CallChecked<Smi>(smi_1, smi_2)->value(), 1);
  CHECK_EQ(ft_min.CallChecked<Smi>(smi_2, smi_1)->value(), 1);
  CHECK_EQ(ft_max.CallChecked<Smi>(smi_1, smi_2)->value(), 2);
  CHECK_EQ(ft_max.CallChecked<Smi>(smi_2, smi_1)->value(), 2);

  // Test double values.
  Handle<Object> double_a = isolate->factory()->NewNumber(2.5);
  Handle<Object> double_b = isolate->factory()->NewNumber(3.5);
  Handle<Object> nan =
      isolate->factory()->NewNumber(std::numeric_limits<double>::quiet_NaN());
  Handle<Object> infinity = isolate->factory()->NewNumber(V8_INFINITY);

  CHECK_EQ(ft_min.CallChecked<HeapNumber>(double_a, double_b)->value(), 2.5);
  CHECK_EQ(ft_min.CallChecked<HeapNumber>(double_b, double_a)->value(), 2.5);
  CHECK_EQ(ft_min.CallChecked<HeapNumber>(infinity, double_a)->value(), 2.5);
  CHECK_EQ(ft_min.CallChecked<HeapNumber>(double_a, infinity)->value(), 2.5);
  CHECK(std::isnan(ft_min.CallChecked<HeapNumber>(nan, double_a)->value()));
  CHECK(std::isnan(ft_min.CallChecked<HeapNumber>(double_a, nan)->value()));

  CHECK_EQ(ft_max.CallChecked<HeapNumber>(double_a, double_b)->value(), 3.5);
  CHECK_EQ(ft_max.CallChecked<HeapNumber>(double_b, double_a)->value(), 3.5);
  CHECK_EQ(ft_max.CallChecked<HeapNumber>(infinity, double_a)->value(),
           V8_INFINITY);
  CHECK_EQ(ft_max.CallChecked<HeapNumber>(double_a, infinity)->value(),
           V8_INFINITY);
  CHECK(std::isnan(ft_max.CallChecked<HeapNumber>(nan, double_a)->value()));
  CHECK(std::isnan(ft_max.CallChecked<HeapNumber>(double_a, nan)->value()));

  // Mixed smi/double values.
  CHECK_EQ(ft_max.CallChecked<HeapNumber>(smi_1, double_b)->value(), 3.5);
  CHECK_EQ(ft_max.CallChecked<HeapNumber>(double_b, smi_1)->value(), 3.5);
  CHECK_EQ(ft_min.CallChecked<HeapNumber>(smi_5, double_b)->value(), 3.5);
  CHECK_EQ(ft_min.CallChecked<HeapNumber>(double_b, smi_5)->value(), 3.5);
}

TEST(NumberAddSub) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 2;
  CodeAssemblerTester asm_tester_add(isolate,
                                     kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester_add.state());
    m.Return(m.NumberAdd(m.Parameter<Number>(1), m.Parameter<Number>(2)));
  }
  FunctionTester ft_add(asm_tester_add.GenerateCode(), kNumParams);

  CodeAssemblerTester asm_tester_sub(isolate,
                                     kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester_sub.state());
    m.Return(m.NumberSub(m.Parameter<Number>(1), m.Parameter<Number>(2)));
  }
  FunctionTester ft_sub(asm_tester_sub.GenerateCode(), kNumParams);

  // Test smi values.
  Handle<Smi> smi_1(Smi::FromInt(1), isolate);
  Handle<Smi> smi_2(Smi::FromInt(2), isolate);
  CHECK_EQ(ft_add.CallChecked<Smi>(smi_1, smi_2)->value(), 3);
  CHECK_EQ(ft_sub.CallChecked<Smi>(smi_2, smi_1)->value(), 1);

  // Test double values.
  Handle<Object> double_a = isolate->factory()->NewNumber(2.5);
  Handle<Object> double_b = isolate->factory()->NewNumber(3.0);
  CHECK_EQ(ft_add.CallChecked<HeapNumber>(double_a, double_b)->value(), 5.5);
  CHECK_EQ(ft_sub.CallChecked<HeapNumber>(double_a, double_b)->value(), -.5);

  // Test overflow.
  Handle<Smi> smi_max(Smi::FromInt(Smi::kMaxValue), isolate);
  Handle<Smi> smi_min(Smi::FromInt(Smi::kMinValue), isolate);
  CHECK_EQ(ft_add.CallChecked<HeapNumber>(smi_max, smi_1)->value(),
           static_cast<double>(Smi::kMaxValue) + 1);
  CHECK_EQ(ft_sub.CallChecked<HeapNumber>(smi_min, smi_1)->value(),
           static_cast<double>(Smi::kMinValue) - 1);

  // Test mixed smi/double values.
  CHECK_EQ(ft_add.CallChecked<HeapNumber>(smi_1, double_a)->value(), 3.5);
  CHECK_EQ(ft_add.CallChecked<HeapNumber>(double_a, smi_1)->value(), 3.5);
  CHECK_EQ(ft_sub.CallChecked<HeapNumber>(smi_1, double_a)->value(), -1.5);
  CHECK_EQ(ft_sub.CallChecked<HeapNumber>(double_a, smi_1)->value(), 1.5);
}

TEST(CloneEmptyFixedArray) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    m.Return(m.CloneFixedArray(m.Parameter<FixedArrayBase>(1)));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->empty_fixed_array());
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(0, result.length());
  CHECK_EQ(*(isolate->factory()->empty_fixed_array()), result);
}

TEST(CloneFixedArray) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    m.Return(m.CloneFixedArray(m.Parameter<FixedArrayBase>(1)));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(5, result.length());
  CHECK(result.get(0).IsTheHole(isolate));
  CHECK_EQ(Smi::cast(result.get(1)).value(), 1234);
  CHECK(result.get(2).IsTheHole(isolate));
  CHECK(result.get(3).IsTheHole(isolate));
  CHECK(result.get(4).IsTheHole(isolate));
}

TEST(CloneFixedArrayCOW) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    m.Return(m.CloneFixedArray(m.Parameter<FixedArrayBase>(1)));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  source->set_map(ReadOnlyRoots(isolate).fixed_cow_array_map());
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(*source, result);
}

TEST(ExtractFixedArrayCOWForceCopy) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    CodeStubAssembler::ExtractFixedArrayFlags flags;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kAllFixedArrays;
    base::Optional<TNode<Smi>> constant(m.SmiConstant(0));
    m.Return(m.ExtractFixedArray(m.Parameter<FixedArrayBase>(1), constant,
                                 base::Optional<TNode<Smi>>(base::nullopt),
                                 base::Optional<TNode<Smi>>(base::nullopt),
                                 flags));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  source->set_map(ReadOnlyRoots(isolate).fixed_cow_array_map());
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_NE(*source, result);
  CHECK_EQ(5, result.length());
  CHECK(result.get(0).IsTheHole(isolate));
  CHECK_EQ(Smi::cast(result.get(1)).value(), 1234);
  CHECK(result.get(2).IsTheHole(isolate));
  CHECK(result.get(3).IsTheHole(isolate));
  CHECK(result.get(4).IsTheHole(isolate));
}

TEST(ExtractFixedArraySimple) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    CodeStubAssembler::ExtractFixedArrayFlags flags;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kAllFixedArrays;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kDontCopyCOW;
    base::Optional<TNode<IntPtrT>> p1_untagged(m.SmiUntag(m.Parameter<Smi>(2)));
    base::Optional<TNode<IntPtrT>> p2_untagged(m.SmiUntag(m.Parameter<Smi>(3)));
    m.Return(m.ExtractFixedArray(
        m.Parameter<FixedArrayBase>(1), p1_untagged, p2_untagged,
        base::Optional<TNode<IntPtrT>>(base::nullopt), flags));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  Handle<Object> result_raw =
      ft.Call(source, Handle<Smi>(Smi::FromInt(1), isolate),
              Handle<Smi>(Smi::FromInt(2), isolate))
          .ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(2, result.length());
  CHECK_EQ(Smi::cast(result.get(0)).value(), 1234);
  CHECK(result.get(1).IsTheHole(isolate));
}

TEST(ExtractFixedArraySimpleSmiConstant) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    CodeStubAssembler::ExtractFixedArrayFlags flags;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kAllFixedArrays;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kDontCopyCOW;
    base::Optional<TNode<Smi>> constant_1(m.SmiConstant(1));
    base::Optional<TNode<Smi>> constant_2(m.SmiConstant(2));
    m.Return(m.ExtractFixedArray(
        m.Parameter<FixedArrayBase>(1), constant_1, constant_2,
        base::Optional<TNode<Smi>>(base::nullopt), flags));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(2, result.length());
  CHECK_EQ(Smi::cast(result.get(0)).value(), 1234);
  CHECK(result.get(1).IsTheHole(isolate));
}

TEST(ExtractFixedArraySimpleIntPtrConstant) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    CodeStubAssembler::ExtractFixedArrayFlags flags;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kAllFixedArrays;
    flags |= CodeStubAssembler::ExtractFixedArrayFlag::kDontCopyCOW;
    base::Optional<TNode<IntPtrT>> constant_1(m.IntPtrConstant(1));
    base::Optional<TNode<IntPtrT>> constant_2(m.IntPtrConstant(2));
    m.Return(m.ExtractFixedArray(
        m.Parameter<FixedArrayBase>(1), constant_1, constant_2,
        base::Optional<TNode<IntPtrT>>(base::nullopt), flags));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(2, result.length());
  CHECK_EQ(Smi::cast(result.get(0)).value(), 1234);
  CHECK(result.get(1).IsTheHole(isolate));
}

TEST(ExtractFixedArraySimpleIntPtrConstantNoDoubles) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    base::Optional<TNode<IntPtrT>> constant_1(m.IntPtrConstant(1));
    base::Optional<TNode<IntPtrT>> constant_2(m.IntPtrConstant(2));
    m.Return(m.ExtractFixedArray(
        m.Parameter<FixedArrayBase>(1), constant_1, constant_2,
        base::Optional<TNode<IntPtrT>>(base::nullopt),
        CodeStubAssembler::ExtractFixedArrayFlag::kFixedArrays));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  Handle<Object> result_raw = ft.Call(source).ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(2, result.length());
  CHECK_EQ(Smi::cast(result.get(0)).value(), 1234);
  CHECK(result.get(1).IsTheHole(isolate));
}

TEST(ExtractFixedArraySimpleIntPtrParameters) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 3;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    base::Optional<TNode<IntPtrT>> p1_untagged(m.SmiUntag(m.Parameter<Smi>(2)));
    base::Optional<TNode<IntPtrT>> p2_untagged(m.SmiUntag(m.Parameter<Smi>(3)));
    m.Return(m.ExtractFixedArray(m.Parameter<FixedArrayBase>(1), p1_untagged,
                                 p2_untagged));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Handle<FixedArray> source(isolate->factory()->NewFixedArrayWithHoles(5));
  source->set(1, Smi::FromInt(1234));
  Handle<Object> result_raw =
      ft.Call(source, Handle<Smi>(Smi::FromInt(1), isolate),
              Handle<Smi>(Smi::FromInt(2), isolate))
          .ToHandleChecked();
  FixedArray result(FixedArray::cast(*result_raw));
  CHECK_EQ(2, result.length());
  CHECK_EQ(Smi::cast(result.get(0)).value(), 1234);
  CHECK(result.get(1).IsTheHole(isolate));

  Handle<FixedDoubleArray> source_double = Handle<FixedDoubleArray>::cast(
      isolate->factory()->NewFixedDoubleArray(5));
  source_double->set(0, 10);
  source_double->set(1, 11);
  source_double->set(2, 12);
  source_double->set(3, 13);
  source_double->set(4, 14);
  Handle<Object> double_result_raw =
      ft.Call(source_double, Handle<Smi>(Smi::FromInt(1), isolate),
              Handle<Smi>(Smi::FromInt(2), isolate))
          .ToHandleChecked();
  FixedDoubleArray double_result = FixedDoubleArray::cast(*double_result_raw);
  CHECK_EQ(2, double_result.length());
  CHECK_EQ(double_result.get_scalar(0), 11);
  CHECK_EQ(double_result.get_scalar(1), 12);
}

TEST(SingleInputPhiElimination) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  {
    CodeStubAssembler m(asm_tester.state());
    TVariable<Smi> temp1(&m);
    TVariable<Smi> temp2(&m);
    Label temp_label(&m, {&temp1, &temp2});
    Label end_label(&m, {&temp1, &temp2});
    temp1 = m.Parameter<Smi>(1);
    temp2 = m.Parameter<Smi>(1);
    m.Branch(m.TaggedEqual(m.UncheckedParameter<Object>(0),
                           m.UncheckedParameter<Object>(1)),
             &end_label, &temp_label);
    m.BIND(&temp_label);
    temp1 = m.Parameter<Smi>(2);
    temp2 = m.Parameter<Smi>(2);
    m.Goto(&end_label);
    m.BIND(&end_label);
    m.Return(m.UncheckedCast<Object>(temp1.value()));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
  // Generating code without an assert is enough to make sure that the
  // single-input phi is properly eliminated.
}

TEST(SmallOrderedHashMapAllocate) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    auto capacity = m.Parameter<Smi>(1);
    m.Return(m.AllocateSmallOrderedHashMap(m.SmiToIntPtr(capacity)));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  Factory* factory = isolate->factory();
  int capacity = SmallOrderedHashMap::kMinCapacity;
  while (capacity <= SmallOrderedHashMap::kMaxCapacity) {
    Handle<SmallOrderedHashMap> expected =
        factory->NewSmallOrderedHashMap(capacity);
    Handle<Object> result_raw =
        ft.Call(Handle<Smi>(Smi::FromInt(capacity), isolate)).ToHandleChecked();
    Handle<SmallOrderedHashMap> actual = Handle<SmallOrderedHashMap>(
        SmallOrderedHashMap::cast(*result_raw), isolate);
    CHECK_EQ(capacity, actual->Capacity());
    CHECK_EQ(0, actual->NumberOfElements());
    CHECK_EQ(0, actual->NumberOfDeletedElements());
    CHECK_EQ(capacity / SmallOrderedHashMap::kLoadFactor,
             actual->NumberOfBuckets());
    CHECK_EQ(0, memcmp(reinterpret_cast<void*>(expected->address()),
                       reinterpret_cast<void*>(actual->address()),
                       SmallOrderedHashMap::SizeFor(capacity)));
#ifdef VERIFY_HEAP
    actual->SmallOrderedHashMapVerify(isolate);
#endif
    capacity = capacity << 1;
  }
#ifdef VERIFY_HEAP
  isolate->heap()->Verify();
#endif
}

TEST(SmallOrderedHashSetAllocate) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(asm_tester.state());
    auto capacity = m.Parameter<Smi>(1);
    m.Return(m.AllocateSmallOrderedHashSet(m.SmiToIntPtr(capacity)));
  }
  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  int capacity = SmallOrderedHashSet::kMinCapacity;
  Factory* factory = isolate->factory();
  while (capacity <= SmallOrderedHashSet::kMaxCapacity) {
    Handle<SmallOrderedHashSet> expected =
        factory->NewSmallOrderedHashSet(capacity);
    Handle<Object> result_raw =
        ft.Call(Handle<Smi>(Smi::FromInt(capacity), isolate)).ToHandleChecked();
    Handle<SmallOrderedHashSet> actual = Handle<SmallOrderedHashSet>(
        SmallOrderedHashSet::cast(*result_raw), isolate);
    CHECK_EQ(capacity, actual->Capacity());
    CHECK_EQ(0, actual->NumberOfElements());
    CHECK_EQ(0, actual->NumberOfDeletedElements());
    CHECK_EQ(capacity / SmallOrderedHashSet::kLoadFactor,
             actual->NumberOfBuckets());
    CHECK_EQ(0, memcmp(reinterpret_cast<void*>(expected->address()),
                       reinterpret_cast<void*>(actual->address()),
                       SmallOrderedHashSet::SizeFor(capacity)));
#ifdef VERIFY_HEAP
    actual->SmallOrderedHashSetVerify(isolate);
#endif
    capacity = capacity << 1;
  }
#ifdef VERIFY_HEAP
  isolate->heap()->Verify();
#endif
}

TEST(IsDoubleElementsKind) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 2;
  CodeAssemblerTester ft_tester(isolate, kNumParams + 1);  // Include receiver.
  {
    CodeStubAssembler m(ft_tester.state());
    m.Return(m.SmiFromInt32(m.UncheckedCast<Int32T>(
        m.IsDoubleElementsKind(m.SmiToInt32(m.Parameter<Smi>(1))))));
  }
  FunctionTester ft(ft_tester.GenerateCode(), kNumParams);
  CHECK_EQ(
      (*Handle<Smi>::cast(
           ft.Call(Handle<Smi>(Smi::FromInt(PACKED_DOUBLE_ELEMENTS), isolate))
               .ToHandleChecked()))
          .value(),
      1);
  CHECK_EQ(
      (*Handle<Smi>::cast(
           ft.Call(Handle<Smi>(Smi::FromInt(HOLEY_DOUBLE_ELEMENTS), isolate))
               .ToHandleChecked()))
          .value(),
      1);
  CHECK_EQ((*Handle<Smi>::cast(
                ft.Call(Handle<Smi>(Smi::FromInt(HOLEY_ELEMENTS), isolate))
                    .ToHandleChecked()))
               .value(),
           0);
  CHECK_EQ((*Handle<Smi>::cast(
                ft.Call(Handle<Smi>(Smi::FromInt(PACKED_ELEMENTS), isolate))
                    .ToHandleChecked()))
               .value(),
           0);
  CHECK_EQ((*Handle<Smi>::cast(
                ft.Call(Handle<Smi>(Smi::FromInt(PACKED_SMI_ELEMENTS), isolate))
                    .ToHandleChecked()))
               .value(),
           0);
  CHECK_EQ((*Handle<Smi>::cast(
                ft.Call(Handle<Smi>(Smi::FromInt(HOLEY_SMI_ELEMENTS), isolate))
                    .ToHandleChecked()))
               .value(),
           0);
  CHECK_EQ((*Handle<Smi>::cast(
                ft.Call(Handle<Smi>(Smi::FromInt(DICTIONARY_ELEMENTS), isolate))
                    .ToHandleChecked()))
               .value(),
           0);
}

TEST(TestCallBuiltinInlineTrampoline) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  const int kContextOffset = 3;
  auto str = m.Parameter<String>(1);
  auto context = m.Parameter<Context>(kNumParams + kContextOffset);

  TNode<Smi> index = m.SmiConstant(2);

  m.Return(m.CallStub(Builtins::CallableFor(isolate, Builtins::kStringRepeat),
                      context, str, index));
  AssemblerOptions options = AssemblerOptions::Default(isolate);
  options.inline_offheap_trampolines = true;
  options.use_pc_relative_calls_and_jumps = false;
  options.isolate_independent_code = false;
  FunctionTester ft(asm_tester.GenerateCode(options), kNumParams);
  MaybeHandle<Object> result = ft.Call(CcTest::MakeString("abcdef"));
  CHECK(String::Equals(isolate, CcTest::MakeString("abcdefabcdef"),
                       Handle<String>::cast(result.ToHandleChecked())));
}

// TODO(v8:9821): Remove the option to disable inlining off-heap trampolines
// along with this test.
DISABLED_TEST(TestCallBuiltinIndirectLoad) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());

  const int kContextOffset = 2;
  auto str = m.Parameter<String>(0);
  auto context = m.Parameter<Context>(kNumParams + kContextOffset);

  TNode<Smi> index = m.SmiConstant(2);

  m.Return(m.CallStub(Builtins::CallableFor(isolate, Builtins::kStringRepeat),
                      context, str, index));
  AssemblerOptions options = AssemblerOptions::Default(isolate);
  options.inline_offheap_trampolines = false;
  options.use_pc_relative_calls_and_jumps = false;
  options.isolate_independent_code = true;
  FunctionTester ft(asm_tester.GenerateCode(options), kNumParams);
  MaybeHandle<Object> result = ft.Call(CcTest::MakeString("abcdef"));
  CHECK(String::Equals(isolate, CcTest::MakeString("abcdefabcdef"),
                       Handle<String>::cast(result.ToHandleChecked())));
}

TEST(InstructionSchedulingCallerSavedRegisters) {
  // This is a regression test for v8:9775, where TF's instruction scheduler
  // incorrectly moved pure operations in between a ArchSaveCallerRegisters and
  // a ArchRestoreCallerRegisters instruction.
  bool old_turbo_instruction_scheduling = FLAG_turbo_instruction_scheduling;
  FLAG_turbo_instruction_scheduling = true;

  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());

  {
    TNode<IntPtrT> x = m.SmiUntag(m.Parameter<Smi>(1));
    TNode<WordT> y = m.WordOr(m.WordShr(x, 1), m.IntPtrConstant(1));
    TNode<ExternalReference> isolate_ptr =
        m.ExternalConstant(ExternalReference::isolate_address(isolate));
    m.CallCFunctionWithCallerSavedRegisters(
        m.ExternalConstant(
            ExternalReference::smi_lexicographic_compare_function()),
        MachineType::Int32(), SaveFPRegsMode::kSave,
        std::make_pair(MachineType::Pointer(), isolate_ptr),
        std::make_pair(MachineType::TaggedSigned(), m.SmiConstant(0)),
        std::make_pair(MachineType::TaggedSigned(), m.SmiConstant(0)));
    m.Return(m.SmiTag(m.Signed(m.WordOr(x, y))));
  }

  AssemblerOptions options = AssemblerOptions::Default(isolate);
  FunctionTester ft(asm_tester.GenerateCode(options), kNumParams);
  Handle<Object> input = isolate->factory()->NewNumber(8);
  MaybeHandle<Object> result = ft.Call(input);
  CHECK(result.ToHandleChecked()->IsSmi());
  CHECK_EQ(result.ToHandleChecked()->Number(), 13);

  FLAG_turbo_instruction_scheduling = old_turbo_instruction_scheduling;
}

#if V8_ENABLE_WEBASSEMBLY
TEST(WasmInt32ToHeapNumber) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  int32_t test_values[] = {
    // Smi values.
    1,
    0,
    -1,
    kSmiMaxValue,
    kSmiMinValue,
  // Test integers that can't be Smis (only possible if Smis are 31 bits).
#if defined(V8_HOST_ARCH_32_BIT) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
    kSmiMaxValue + 1,
    kSmiMinValue - 1,
#endif
  };

  // FunctionTester can't handle Wasm type arguments, so for each test value,
  // build a function with the arguments baked in, then generate a no-argument
  // function to call.
  const int kNumParams = 1;
  for (size_t i = 0; i < arraysize(test_values); ++i) {
    int32_t test_value = test_values[i];
    CodeAssemblerTester asm_tester(isolate, kNumParams);
    CodeStubAssembler m(asm_tester.state());
    const TNode<Int32T> arg = m.Int32Constant(test_value);
    const TNode<Object> call_result = m.CallBuiltin(
        Builtins::kWasmInt32ToHeapNumber, m.NoContextConstant(), arg);
    m.Return(call_result);

    FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
    Handle<Object> result = ft.Call().ToHandleChecked();
    CHECK(result->IsNumber());
    Handle<Object> expected(isolate->factory()->NewNumber(test_value));
    CHECK(result->StrictEquals(*expected));
  }
}

int32_t NumberToInt32(Handle<Object> number) {
  if (number->IsSmi()) {
    return Smi::ToInt(*number);
  }
  if (number->IsHeapNumber()) {
    double num = HeapNumber::cast(*number).value();
    return DoubleToInt32(num);
  }
  UNREACHABLE();
}

TEST(WasmTaggedNonSmiToInt32) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);

  Handle<Object> test_values[] = {
      // No Smis here; the builtin can't handle them correctly.
      factory->NewNumber(-0.0),
      factory->NewNumber(1.5),
      factory->NewNumber(-1.5),
      factory->NewNumber(2 * static_cast<double>(kSmiMaxValue)),
      factory->NewNumber(2 * static_cast<double>(kSmiMinValue)),
      factory->NewNumber(std::numeric_limits<double>::infinity()),
      factory->NewNumber(-std::numeric_limits<double>::infinity()),
      factory->NewNumber(-std::numeric_limits<double>::quiet_NaN()),
  };

  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());
  auto context = m.Parameter<Context>(kNumParams + 3);
  const auto arg = m.Parameter<Object>(1);
  int32_t result = 0;
  Node* base = m.IntPtrConstant(reinterpret_cast<intptr_t>(&result));
  Node* value = m.CallBuiltin(Builtins::kWasmTaggedNonSmiToInt32, context, arg);
  m.StoreNoWriteBarrier(MachineRepresentation::kWord32, base, value);
  m.Return(m.UndefinedConstant());

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  for (size_t i = 0; i < arraysize(test_values); ++i) {
    Handle<Object> test_value = test_values[i];
    ft.Call(test_value);
    int32_t expected = NumberToInt32(test_value);
    CHECK_EQ(result, expected);
  }
}

TEST(WasmFloat32ToNumber) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  float test_values[] = {
      // Smi values.
      1,
      0,
      -1,
      // Max and min Smis can't be represented as floats.
      // Non-Smi values.
      -0.0,
      1.5,
      std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::infinity(),
  };

  // FunctionTester can't handle Wasm type arguments, so for each test value,
  // build a function with the arguments baked in, then generate a no-argument
  // function to call.
  const int kNumParams = 1;
  for (size_t i = 0; i < arraysize(test_values); ++i) {
    double test_value = test_values[i];
    CodeAssemblerTester asm_tester(isolate, kNumParams);
    CodeStubAssembler m(asm_tester.state());
    const TNode<Float32T> arg = m.Float32Constant(test_value);
    const TNode<Object> call_result = m.CallBuiltin(
        Builtins::kWasmFloat32ToNumber, m.NoContextConstant(), arg);
    m.Return(call_result);

    FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
    Handle<Object> result = ft.Call().ToHandleChecked();
    CHECK(result->IsNumber());
    Handle<Object> expected(isolate->factory()->NewNumber(test_value));
    CHECK(result->StrictEquals(*expected) ||
          (std::isnan(test_value) && std::isnan(result->Number())));
    CHECK_EQ(result->IsSmi(), expected->IsSmi());
  }
}

TEST(WasmFloat64ToNumber) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  double test_values[] = {
      // Smi values.
      1,
      0,
      -1,
      kSmiMaxValue,
      kSmiMinValue,
      // Non-Smi values.
      -0.0,
      1.5,
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
  };

  // FunctionTester can't handle Wasm type arguments, so for each test value,
  // build a function with the arguments baked in, then generate a no-argument
  // function to call.
  const int kNumParams = 1;
  for (size_t i = 0; i < arraysize(test_values); ++i) {
    double test_value = test_values[i];
    CodeAssemblerTester asm_tester(isolate, kNumParams);
    CodeStubAssembler m(asm_tester.state());
    const TNode<Float64T> arg = m.Float64Constant(test_value);
    const TNode<Object> call_result = m.CallBuiltin(
        Builtins::kWasmFloat64ToNumber, m.NoContextConstant(), arg);
    m.Return(call_result);

    FunctionTester ft(asm_tester.GenerateCode(), kNumParams);
    Handle<Object> result = ft.Call().ToHandleChecked();
    CHECK(result->IsNumber());
    Handle<Object> expected(isolate->factory()->NewNumber(test_value));
    CHECK(result->StrictEquals(*expected) ||
          (std::isnan(test_value) && std::isnan(result->Number())));
    CHECK_EQ(result->IsSmi(), expected->IsSmi());
  }
}

double NumberToFloat64(Handle<Object> number) {
  if (number->IsSmi()) {
    return Smi::ToInt(*number);
  }
  if (number->IsHeapNumber()) {
    return HeapNumber::cast(*number).value();
  }
  UNREACHABLE();
}

TEST(WasmTaggedToFloat64) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);

  Handle<Object> test_values[] = {
    // Smi values.
    handle(Smi::FromInt(1), isolate),
    handle(Smi::FromInt(0), isolate),
    handle(Smi::FromInt(-1), isolate),
    handle(Smi::FromInt(kSmiMaxValue), isolate),
    handle(Smi::FromInt(kSmiMinValue), isolate),
    // Test some non-Smis.
    factory->NewNumber(-0.0),
    factory->NewNumber(1.5),
    factory->NewNumber(-1.5),
// Integer Overflows on platforms with 32 bit Smis.
#if defined(V8_HOST_ARCH_32_BIT) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
    factory->NewNumber(2 * kSmiMaxValue),
    factory->NewNumber(2 * kSmiMinValue),
#endif
    factory->NewNumber(std::numeric_limits<double>::infinity()),
    factory->NewNumber(-std::numeric_limits<double>::infinity()),
    factory->NewNumber(-std::numeric_limits<double>::quiet_NaN()),
  };

  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams + 1);  // Include receiver.
  CodeStubAssembler m(asm_tester.state());
  auto context = m.Parameter<Context>(kNumParams + 3);
  const auto arg = m.Parameter<Object>(1);
  double result = 0;
  Node* base = m.IntPtrConstant(reinterpret_cast<intptr_t>(&result));
  Node* value = m.CallBuiltin(Builtins::kWasmTaggedToFloat64, context, arg);
  m.StoreNoWriteBarrier(MachineRepresentation::kFloat64, base, value);
  m.Return(m.UndefinedConstant());

  FunctionTester ft(asm_tester.GenerateCode(), kNumParams);

  for (size_t i = 0; i < arraysize(test_values); ++i) {
    Handle<Object> test_value = test_values[i];
    ft.Call(test_value);
    double expected = NumberToFloat64(test_value);
    if (std::isnan(expected)) {
      CHECK(std::isnan(result));
    } else {
      CHECK_EQ(result, expected);
    }
  }
}
#endif  // V8_ENABLE_WEBASSEMBLY

TEST(SmiUntagLeftShiftOptimization) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 1;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());

  {
    TNode<TaggedIndex> param = m.UncheckedParameter<TaggedIndex>(0);
    TNode<WordT> unoptimized =
        m.IntPtrMul(m.TaggedIndexToIntPtr(param), m.IntPtrConstant(8));
    TNode<WordT> optimized = m.WordShl(
        m.BitcastTaggedToWordForTagAndSmiBits(param), 3 - kSmiTagSize);
    m.StaticAssert(m.WordEqual(unoptimized, optimized));
    m.Return(m.UndefinedConstant());
  }

  AssemblerOptions options = AssemblerOptions::Default(isolate);
  FunctionTester ft(asm_tester.GenerateCode(options), kNumParams);
}

TEST(SmiUntagComparisonOptimization) {
  Isolate* isolate(CcTest::InitIsolateOnce());
  const int kNumParams = 2;
  CodeAssemblerTester asm_tester(isolate, kNumParams);
  CodeStubAssembler m(asm_tester.state());

  {
    TNode<Smi> a = m.UncheckedParameter<Smi>(0);
    TNode<Smi> b = m.UncheckedParameter<Smi>(1);
    TNode<BoolT> unoptimized = m.UintPtrLessThan(m.SmiUntag(a), m.SmiUntag(b));
#ifdef V8_COMPRESS_POINTERS
    TNode<BoolT> optimized = m.Uint32LessThan(
        m.TruncateIntPtrToInt32(m.BitcastTaggedToWordForTagAndSmiBits(a)),
        m.TruncateIntPtrToInt32(m.BitcastTaggedToWordForTagAndSmiBits(b)));
#else
    TNode<BoolT> optimized =
        m.UintPtrLessThan(m.BitcastTaggedToWordForTagAndSmiBits(a),
                          m.BitcastTaggedToWordForTagAndSmiBits(b));
#endif
    m.StaticAssert(m.Word32Equal(unoptimized, optimized));
    m.Return(m.UndefinedConstant());
  }

  AssemblerOptions options = AssemblerOptions::Default(isolate);
  FunctionTester ft(asm_tester.GenerateCode(options), kNumParams);
}

TEST(PopCount) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  CodeAssemblerTester asm_tester(isolate);
  CodeStubAssembler m(asm_tester.state());

  const std::vector<std::pair<uint32_t, int>> test_cases = {
      {0, 0},
      {1, 1},
      {(1 << 31), 1},
      {0b01010101010101010101010101010101, 16},
      {0b10101010101010101010101010101010, 16},
      {0b11100011100000011100011111000111, 17}  // arbitrarily chosen
  };

  for (std::pair<uint32_t, int> test_case : test_cases) {
    uint32_t value32 = test_case.first;
    uint64_t value64 = (static_cast<uint64_t>(value32) << 32) | value32;
    int expected_pop32 = test_case.second;
    int expected_pop64 = 2 * expected_pop32;

    TNode<Int32T> pop32 = m.PopulationCount32(m.Uint32Constant(value32));
    CSA_CHECK(&m, m.Word32Equal(pop32, m.Int32Constant(expected_pop32)));

    if (m.Is64()) {
      // TODO(emrich): enable once 64-bit operations are supported on 32-bit
      // architectures.

      TNode<Int64T> pop64 = m.PopulationCount64(m.Uint64Constant(value64));
      CSA_CHECK(&m, m.Word64Equal(pop64, m.Int64Constant(expected_pop64)));
    }
  }
  m.Return(m.UndefinedConstant());

  FunctionTester ft(asm_tester.GenerateCode());
  ft.Call();
}

TEST(CountTrailingZeros) {
  Isolate* isolate(CcTest::InitIsolateOnce());

  CodeAssemblerTester asm_tester(isolate);
  CodeStubAssembler m(asm_tester.state());

  const std::vector<std::pair<uint32_t, int>> test_cases = {
      {1, 0},
      {2, 1},
      {(0b0101010'0000'0000), 9},
      {(1 << 31), 31},
      {std::numeric_limits<uint32_t>::max(), 0},
  };

  for (std::pair<uint32_t, int> test_case : test_cases) {
    uint32_t value32 = test_case.first;
    uint64_t value64 = static_cast<uint64_t>(value32) << 32;
    int expected_ctz32 = test_case.second;
    int expected_ctz64 = expected_ctz32 + 32;

    TNode<Int32T> pop32 = m.CountTrailingZeros32(m.Uint32Constant(value32));
    CSA_CHECK(&m, m.Word32Equal(pop32, m.Int32Constant(expected_ctz32)));

    if (m.Is64()) {
      // TODO(emrich): enable once 64-bit operations are supported on 32-bit
      // architectures.

      TNode<Int64T> pop64_ext =
          m.CountTrailingZeros64(m.Uint64Constant(value32));
      TNode<Int64T> pop64 = m.CountTrailingZeros64(m.Uint64Constant(value64));

      CSA_CHECK(&m, m.Word64Equal(pop64_ext, m.Int64Constant(expected_ctz32)));
      CSA_CHECK(&m, m.Word64Equal(pop64, m.Int64Constant(expected_ctz64)));
    }
  }
  m.Return(m.UndefinedConstant());

  FunctionTester ft(asm_tester.GenerateCode());
  ft.Call();
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
