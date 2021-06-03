// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_INTERPRETER_BYTECODES_H_
#define V8_INTERPRETER_BYTECODES_H_

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "src/common/globals.h"
#include "src/interpreter/bytecode-operands.h"

// This interface and it's implementation are independent of the
// libv8_base library as they are used by the interpreter and the
// standalone mkpeephole table generator program.

namespace v8 {
namespace internal {
namespace interpreter {

// The list of single-byte Star variants, in the format of BYTECODE_LIST.
#define SHORT_STAR_BYTECODE_LIST(V)                              \
  V(Star15, ImplicitRegisterUse::kReadAccumulatorWriteShortStar) \
  V(Star14, ImplicitRegisterUse::kReadAccumulatorWriteShortStar) \
  V(Star13, ImplicitRegisterUse::kReadAccumulatorWriteShortStar) \
  V(Star12, ImplicitRegisterUse::kReadAccumulatorWriteShortStar) \
  V(Star11, ImplicitRegisterUse::kReadAccumulatorWriteShortStar) \
  V(Star10, ImplicitRegisterUse::kReadAccumulatorWriteShortStar) \
  V(Star9, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star8, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star7, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star6, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star5, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star4, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star3, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star2, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star1, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)  \
  V(Star0, ImplicitRegisterUse::kReadAccumulatorWriteShortStar)

// The list of bytecodes which have unique handlers (no other bytecode is
// executed using identical code).
// Format is V(<bytecode>, <implicit_register_use>, <operands>).
#define BYTECODE_LIST_WITH_UNIQUE_HANDLERS(V)                                  \
  /* Extended width operands */                                                \
  V(Wide, ImplicitRegisterUse::kNone)                                          \
  V(ExtraWide, ImplicitRegisterUse::kNone)                                     \
                                                                               \
  /* Debug Breakpoints - one for each possible size of unscaled bytecodes */   \
  /* and one for each operand widening prefix bytecode                    */   \
  V(DebugBreakWide, ImplicitRegisterUse::kReadWriteAccumulator)                \
  V(DebugBreakExtraWide, ImplicitRegisterUse::kReadWriteAccumulator)           \
  V(DebugBreak0, ImplicitRegisterUse::kReadWriteAccumulator)                   \
  V(DebugBreak1, ImplicitRegisterUse::kReadWriteAccumulator,                   \
    OperandType::kReg)                                                         \
  V(DebugBreak2, ImplicitRegisterUse::kReadWriteAccumulator,                   \
    OperandType::kReg, OperandType::kReg)                                      \
  V(DebugBreak3, ImplicitRegisterUse::kReadWriteAccumulator,                   \
    OperandType::kReg, OperandType::kReg, OperandType::kReg)                   \
  V(DebugBreak4, ImplicitRegisterUse::kReadWriteAccumulator,                   \
    OperandType::kReg, OperandType::kReg, OperandType::kReg,                   \
    OperandType::kReg)                                                         \
  V(DebugBreak5, ImplicitRegisterUse::kReadWriteAccumulator,                   \
    OperandType::kRuntimeId, OperandType::kReg, OperandType::kReg)             \
  V(DebugBreak6, ImplicitRegisterUse::kReadWriteAccumulator,                   \
    OperandType::kRuntimeId, OperandType::kReg, OperandType::kReg,             \
    OperandType::kReg)                                                         \
                                                                               \
  /* Side-effect-free bytecodes -- carefully ordered for efficient checks */   \
  /* - [Loading the accumulator] */                                            \
  V(Ldar, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg)           \
  V(LdaZero, ImplicitRegisterUse::kWriteAccumulator)                           \
  V(LdaSmi, ImplicitRegisterUse::kWriteAccumulator, OperandType::kImm)         \
  V(LdaUndefined, ImplicitRegisterUse::kWriteAccumulator)                      \
  V(LdaNull, ImplicitRegisterUse::kWriteAccumulator)                           \
  V(LdaTheHole, ImplicitRegisterUse::kWriteAccumulator)                        \
  V(LdaTrue, ImplicitRegisterUse::kWriteAccumulator)                           \
  V(LdaFalse, ImplicitRegisterUse::kWriteAccumulator)                          \
  V(LdaConstant, ImplicitRegisterUse::kWriteAccumulator, OperandType::kIdx)    \
  V(LdaContextSlot, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg, \
    OperandType::kIdx, OperandType::kUImm)                                     \
  V(LdaImmutableContextSlot, ImplicitRegisterUse::kWriteAccumulator,           \
    OperandType::kReg, OperandType::kIdx, OperandType::kUImm)                  \
  V(LdaCurrentContextSlot, ImplicitRegisterUse::kWriteAccumulator,             \
    OperandType::kIdx)                                                         \
  V(LdaImmutableCurrentContextSlot, ImplicitRegisterUse::kWriteAccumulator,    \
    OperandType::kIdx)                                                         \
  /* - [Register Loads ] */                                                    \
  V(Star, ImplicitRegisterUse::kReadAccumulator, OperandType::kRegOut)         \
  V(Mov, ImplicitRegisterUse::kNone, OperandType::kReg, OperandType::kRegOut)  \
  V(PushContext, ImplicitRegisterUse::kReadAccumulator, OperandType::kRegOut)  \
  V(PopContext, ImplicitRegisterUse::kNone, OperandType::kReg)                 \
  /* - [Test Operations ] */                                                   \
  V(TestReferenceEqual, ImplicitRegisterUse::kReadWriteAccumulator,            \
    OperandType::kReg)                                                         \
  V(TestUndetectable, ImplicitRegisterUse::kReadWriteAccumulator)              \
  V(TestNull, ImplicitRegisterUse::kReadWriteAccumulator)                      \
  V(TestUndefined, ImplicitRegisterUse::kReadWriteAccumulator)                 \
  V(TestTypeOf, ImplicitRegisterUse::kReadWriteAccumulator,                    \
    OperandType::kFlag8)                                                       \
                                                                               \
  /* Globals */                                                                \
  V(LdaGlobal, ImplicitRegisterUse::kWriteAccumulator, OperandType::kIdx,      \
    OperandType::kIdx)                                                         \
  V(LdaGlobalInsideTypeof, ImplicitRegisterUse::kWriteAccumulator,             \
    OperandType::kIdx, OperandType::kIdx)                                      \
  V(StaGlobal, ImplicitRegisterUse::kReadAccumulator, OperandType::kIdx,       \
    OperandType::kIdx)                                                         \
                                                                               \
  /* Context operations */                                                     \
  V(StaContextSlot, ImplicitRegisterUse::kReadAccumulator, OperandType::kReg,  \
    OperandType::kIdx, OperandType::kUImm)                                     \
  V(StaCurrentContextSlot, ImplicitRegisterUse::kReadAccumulator,              \
    OperandType::kIdx)                                                         \
                                                                               \
  /* Load-Store lookup slots */                                                \
  V(LdaLookupSlot, ImplicitRegisterUse::kWriteAccumulator, OperandType::kIdx)  \
  V(LdaLookupContextSlot, ImplicitRegisterUse::kWriteAccumulator,              \
    OperandType::kIdx, OperandType::kIdx, OperandType::kUImm)                  \
  V(LdaLookupGlobalSlot, ImplicitRegisterUse::kWriteAccumulator,               \
    OperandType::kIdx, OperandType::kIdx, OperandType::kUImm)                  \
  V(LdaLookupSlotInsideTypeof, ImplicitRegisterUse::kWriteAccumulator,         \
    OperandType::kIdx)                                                         \
  V(LdaLookupContextSlotInsideTypeof, ImplicitRegisterUse::kWriteAccumulator,  \
    OperandType::kIdx, OperandType::kIdx, OperandType::kUImm)                  \
  V(LdaLookupGlobalSlotInsideTypeof, ImplicitRegisterUse::kWriteAccumulator,   \
    OperandType::kIdx, OperandType::kIdx, OperandType::kUImm)                  \
  V(StaLookupSlot, ImplicitRegisterUse::kReadWriteAccumulator,                 \
    OperandType::kIdx, OperandType::kFlag8)                                    \
                                                                               \
  /* Property loads (LoadIC) operations */                                     \
  V(LdaNamedProperty, ImplicitRegisterUse::kWriteAccumulator,                  \
    OperandType::kReg, OperandType::kIdx, OperandType::kIdx)                   \
  V(LdaNamedPropertyNoFeedback, ImplicitRegisterUse::kWriteAccumulator,        \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(LdaNamedPropertyFromSuper, ImplicitRegisterUse::kReadWriteAccumulator,     \
    OperandType::kReg, OperandType::kIdx, OperandType::kIdx)                   \
  V(LdaKeyedProperty, ImplicitRegisterUse::kReadWriteAccumulator,              \
    OperandType::kReg, OperandType::kIdx)                                      \
                                                                               \
  /* Operations on module variables */                                         \
  V(LdaModuleVariable, ImplicitRegisterUse::kWriteAccumulator,                 \
    OperandType::kImm, OperandType::kUImm)                                     \
  V(StaModuleVariable, ImplicitRegisterUse::kReadAccumulator,                  \
    OperandType::kImm, OperandType::kUImm)                                     \
                                                                               \
  /* Propery stores (StoreIC) operations */                                    \
  V(StaNamedProperty, ImplicitRegisterUse::kReadWriteAccumulator,              \
    OperandType::kReg, OperandType::kIdx, OperandType::kIdx)                   \
  V(StaNamedPropertyNoFeedback, ImplicitRegisterUse::kReadWriteAccumulator,    \
    OperandType::kReg, OperandType::kIdx, OperandType::kFlag8)                 \
  V(StaNamedOwnProperty, ImplicitRegisterUse::kReadWriteAccumulator,           \
    OperandType::kReg, OperandType::kIdx, OperandType::kIdx)                   \
  V(StaKeyedProperty, ImplicitRegisterUse::kReadWriteAccumulator,              \
    OperandType::kReg, OperandType::kReg, OperandType::kIdx)                   \
  V(StaInArrayLiteral, ImplicitRegisterUse::kReadWriteAccumulator,             \
    OperandType::kReg, OperandType::kReg, OperandType::kIdx)                   \
  V(StaDataPropertyInLiteral, ImplicitRegisterUse::kReadAccumulator,           \
    OperandType::kReg, OperandType::kReg, OperandType::kFlag8,                 \
    OperandType::kIdx)                                                         \
  V(CollectTypeProfile, ImplicitRegisterUse::kReadAccumulator,                 \
    OperandType::kImm)                                                         \
                                                                               \
  /* Binary Operators */                                                       \
  V(Add, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,        \
    OperandType::kIdx)                                                         \
  V(Sub, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,        \
    OperandType::kIdx)                                                         \
  V(Mul, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,        \
    OperandType::kIdx)                                                         \
  V(Div, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,        \
    OperandType::kIdx)                                                         \
  V(Mod, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,        \
    OperandType::kIdx)                                                         \
  V(Exp, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,        \
    OperandType::kIdx)                                                         \
  V(BitwiseOr, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,  \
    OperandType::kIdx)                                                         \
  V(BitwiseXor, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg, \
    OperandType::kIdx)                                                         \
  V(BitwiseAnd, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg, \
    OperandType::kIdx)                                                         \
  V(ShiftLeft, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,  \
    OperandType::kIdx)                                                         \
  V(ShiftRight, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg, \
    OperandType::kIdx)                                                         \
  V(ShiftRightLogical, ImplicitRegisterUse::kReadWriteAccumulator,             \
    OperandType::kReg, OperandType::kIdx)                                      \
                                                                               \
  /* Binary operators with immediate operands */                               \
  V(AddSmi, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kImm,     \
    OperandType::kIdx)                                                         \
  V(SubSmi, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kImm,     \
    OperandType::kIdx)                                                         \
  V(MulSmi, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kImm,     \
    OperandType::kIdx)                                                         \
  V(DivSmi, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kImm,     \
    OperandType::kIdx)                                                         \
  V(ModSmi, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kImm,     \
    OperandType::kIdx)                                                         \
  V(ExpSmi, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kImm,     \
    OperandType::kIdx)                                                         \
  V(BitwiseOrSmi, ImplicitRegisterUse::kReadWriteAccumulator,                  \
    OperandType::kImm, OperandType::kIdx)                                      \
  V(BitwiseXorSmi, ImplicitRegisterUse::kReadWriteAccumulator,                 \
    OperandType::kImm, OperandType::kIdx)                                      \
  V(BitwiseAndSmi, ImplicitRegisterUse::kReadWriteAccumulator,                 \
    OperandType::kImm, OperandType::kIdx)                                      \
  V(ShiftLeftSmi, ImplicitRegisterUse::kReadWriteAccumulator,                  \
    OperandType::kImm, OperandType::kIdx)                                      \
  V(ShiftRightSmi, ImplicitRegisterUse::kReadWriteAccumulator,                 \
    OperandType::kImm, OperandType::kIdx)                                      \
  V(ShiftRightLogicalSmi, ImplicitRegisterUse::kReadWriteAccumulator,          \
    OperandType::kImm, OperandType::kIdx)                                      \
                                                                               \
  /* Unary Operators */                                                        \
  V(Inc, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kIdx)        \
  V(Dec, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kIdx)        \
  V(Negate, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kIdx)     \
  V(BitwiseNot, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kIdx) \
  V(ToBooleanLogicalNot, ImplicitRegisterUse::kReadWriteAccumulator)           \
  V(LogicalNot, ImplicitRegisterUse::kReadWriteAccumulator)                    \
  V(TypeOf, ImplicitRegisterUse::kReadWriteAccumulator)                        \
  V(DeletePropertyStrict, ImplicitRegisterUse::kReadWriteAccumulator,          \
    OperandType::kReg)                                                         \
  V(DeletePropertySloppy, ImplicitRegisterUse::kReadWriteAccumulator,          \
    OperandType::kReg)                                                         \
                                                                               \
  /* GetSuperConstructor operator */                                           \
  V(GetSuperConstructor, ImplicitRegisterUse::kReadAccumulator,                \
    OperandType::kRegOut)                                                      \
                                                                               \
  /* Call operations */                                                        \
  V(CallAnyReceiver, ImplicitRegisterUse::kWriteAccumulator,                   \
    OperandType::kReg, OperandType::kRegList, OperandType::kRegCount,          \
    OperandType::kIdx)                                                         \
  V(CallProperty, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,   \
    OperandType::kRegList, OperandType::kRegCount, OperandType::kIdx)          \
  V(CallProperty0, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,  \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(CallProperty1, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,  \
    OperandType::kReg, OperandType::kReg, OperandType::kIdx)                   \
  V(CallProperty2, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,  \
    OperandType::kReg, OperandType::kReg, OperandType::kReg,                   \
    OperandType::kIdx)                                                         \
  V(CallUndefinedReceiver, ImplicitRegisterUse::kWriteAccumulator,             \
    OperandType::kReg, OperandType::kRegList, OperandType::kRegCount,          \
    OperandType::kIdx)                                                         \
  V(CallUndefinedReceiver0, ImplicitRegisterUse::kWriteAccumulator,            \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(CallUndefinedReceiver1, ImplicitRegisterUse::kWriteAccumulator,            \
    OperandType::kReg, OperandType::kReg, OperandType::kIdx)                   \
  V(CallUndefinedReceiver2, ImplicitRegisterUse::kWriteAccumulator,            \
    OperandType::kReg, OperandType::kReg, OperandType::kReg,                   \
    OperandType::kIdx)                                                         \
  V(CallNoFeedback, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg, \
    OperandType::kRegList, OperandType::kRegCount)                             \
  V(CallWithSpread, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg, \
    OperandType::kRegList, OperandType::kRegCount, OperandType::kIdx)          \
  V(CallRuntime, ImplicitRegisterUse::kWriteAccumulator,                       \
    OperandType::kRuntimeId, OperandType::kRegList, OperandType::kRegCount)    \
  V(CallRuntimeForPair, ImplicitRegisterUse::kNone, OperandType::kRuntimeId,   \
    OperandType::kRegList, OperandType::kRegCount, OperandType::kRegOutPair)   \
  V(CallJSRuntime, ImplicitRegisterUse::kWriteAccumulator,                     \
    OperandType::kNativeContextIndex, OperandType::kRegList,                   \
    OperandType::kRegCount)                                                    \
                                                                               \
  /* Intrinsics */                                                             \
  V(InvokeIntrinsic, ImplicitRegisterUse::kWriteAccumulator,                   \
    OperandType::kIntrinsicId, OperandType::kRegList, OperandType::kRegCount)  \
                                                                               \
  /* Construct operators */                                                    \
  V(Construct, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,  \
    OperandType::kRegList, OperandType::kRegCount, OperandType::kIdx)          \
  V(ConstructWithSpread, ImplicitRegisterUse::kReadWriteAccumulator,           \
    OperandType::kReg, OperandType::kRegList, OperandType::kRegCount,          \
    OperandType::kIdx)                                                         \
                                                                               \
  /* Effectful Test Operators */                                               \
  V(TestEqual, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,  \
    OperandType::kIdx)                                                         \
  V(TestEqualStrict, ImplicitRegisterUse::kReadWriteAccumulator,               \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(TestLessThan, ImplicitRegisterUse::kReadWriteAccumulator,                  \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(TestGreaterThan, ImplicitRegisterUse::kReadWriteAccumulator,               \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(TestLessThanOrEqual, ImplicitRegisterUse::kReadWriteAccumulator,           \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(TestGreaterThanOrEqual, ImplicitRegisterUse::kReadWriteAccumulator,        \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(TestInstanceOf, ImplicitRegisterUse::kReadWriteAccumulator,                \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(TestIn, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kReg,     \
    OperandType::kIdx)                                                         \
                                                                               \
  /* Cast operators */                                                         \
  V(ToName, ImplicitRegisterUse::kReadAccumulator, OperandType::kRegOut)       \
  V(ToNumber, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kIdx)   \
  V(ToNumeric, ImplicitRegisterUse::kReadWriteAccumulator, OperandType::kIdx)  \
  V(ToObject, ImplicitRegisterUse::kReadAccumulator, OperandType::kRegOut)     \
  V(ToString, ImplicitRegisterUse::kReadWriteAccumulator)                      \
                                                                               \
  /* Literals */                                                               \
  V(CreateRegExpLiteral, ImplicitRegisterUse::kWriteAccumulator,               \
    OperandType::kIdx, OperandType::kIdx, OperandType::kFlag8)                 \
  V(CreateArrayLiteral, ImplicitRegisterUse::kWriteAccumulator,                \
    OperandType::kIdx, OperandType::kIdx, OperandType::kFlag8)                 \
  V(CreateArrayFromIterable, ImplicitRegisterUse::kReadWriteAccumulator)       \
  V(CreateEmptyArrayLiteral, ImplicitRegisterUse::kWriteAccumulator,           \
    OperandType::kIdx)                                                         \
  V(CreateObjectLiteral, ImplicitRegisterUse::kWriteAccumulator,               \
    OperandType::kIdx, OperandType::kIdx, OperandType::kFlag8)                 \
  V(CreateEmptyObjectLiteral, ImplicitRegisterUse::kWriteAccumulator)          \
  V(CloneObject, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,    \
    OperandType::kFlag8, OperandType::kIdx)                                    \
                                                                               \
  /* Tagged templates */                                                       \
  V(GetTemplateObject, ImplicitRegisterUse::kWriteAccumulator,                 \
    OperandType::kIdx, OperandType::kIdx)                                      \
                                                                               \
  /* Closure allocation */                                                     \
  V(CreateClosure, ImplicitRegisterUse::kWriteAccumulator, OperandType::kIdx,  \
    OperandType::kIdx, OperandType::kFlag8)                                    \
                                                                               \
  /* Context allocation */                                                     \
  V(CreateBlockContext, ImplicitRegisterUse::kWriteAccumulator,                \
    OperandType::kIdx)                                                         \
  V(CreateCatchContext, ImplicitRegisterUse::kWriteAccumulator,                \
    OperandType::kReg, OperandType::kIdx)                                      \
  V(CreateFunctionContext, ImplicitRegisterUse::kWriteAccumulator,             \
    OperandType::kIdx, OperandType::kUImm)                                     \
  V(CreateEvalContext, ImplicitRegisterUse::kWriteAccumulator,                 \
    OperandType::kIdx, OperandType::kUImm)                                     \
  V(CreateWithContext, ImplicitRegisterUse::kWriteAccumulator,                 \
    OperandType::kReg, OperandType::kIdx)                                      \
                                                                               \
  /* Arguments allocation */                                                   \
  V(CreateMappedArguments, ImplicitRegisterUse::kWriteAccumulator)             \
  V(CreateUnmappedArguments, ImplicitRegisterUse::kWriteAccumulator)           \
  V(CreateRestParameter, ImplicitRegisterUse::kWriteAccumulator)               \
                                                                               \
  /* Control Flow -- carefully ordered for efficient checks */                 \
  /* - [Unconditional jumps] */                                                \
  V(JumpLoop, ImplicitRegisterUse::kNone, OperandType::kUImm,                  \
    OperandType::kImm)                                                         \
  /* - [Forward jumps] */                                                      \
  V(Jump, ImplicitRegisterUse::kNone, OperandType::kUImm)                      \
  /* - [Start constant jumps] */                                               \
  V(JumpConstant, ImplicitRegisterUse::kNone, OperandType::kIdx)               \
  /* - [Conditional jumps] */                                                  \
  /* - [Conditional constant jumps] */                                         \
  V(JumpIfNullConstant, ImplicitRegisterUse::kReadAccumulator,                 \
    OperandType::kIdx)                                                         \
  V(JumpIfNotNullConstant, ImplicitRegisterUse::kReadAccumulator,              \
    OperandType::kIdx)                                                         \
  V(JumpIfUndefinedConstant, ImplicitRegisterUse::kReadAccumulator,            \
    OperandType::kIdx)                                                         \
  V(JumpIfNotUndefinedConstant, ImplicitRegisterUse::kReadAccumulator,         \
    OperandType::kIdx)                                                         \
  V(JumpIfUndefinedOrNullConstant, ImplicitRegisterUse::kReadAccumulator,      \
    OperandType::kIdx)                                                         \
  V(JumpIfTrueConstant, ImplicitRegisterUse::kReadAccumulator,                 \
    OperandType::kIdx)                                                         \
  V(JumpIfFalseConstant, ImplicitRegisterUse::kReadAccumulator,                \
    OperandType::kIdx)                                                         \
  V(JumpIfJSReceiverConstant, ImplicitRegisterUse::kReadAccumulator,           \
    OperandType::kIdx)                                                         \
  /* - [Start ToBoolean jumps] */                                              \
  V(JumpIfToBooleanTrueConstant, ImplicitRegisterUse::kReadAccumulator,        \
    OperandType::kIdx)                                                         \
  V(JumpIfToBooleanFalseConstant, ImplicitRegisterUse::kReadAccumulator,       \
    OperandType::kIdx)                                                         \
  /* - [End constant jumps] */                                                 \
  /* - [Conditional immediate jumps] */                                        \
  V(JumpIfToBooleanTrue, ImplicitRegisterUse::kReadAccumulator,                \
    OperandType::kUImm)                                                        \
  V(JumpIfToBooleanFalse, ImplicitRegisterUse::kReadAccumulator,               \
    OperandType::kUImm)                                                        \
  /* - [End ToBoolean jumps] */                                                \
  V(JumpIfTrue, ImplicitRegisterUse::kReadAccumulator, OperandType::kUImm)     \
  V(JumpIfFalse, ImplicitRegisterUse::kReadAccumulator, OperandType::kUImm)    \
  V(JumpIfNull, ImplicitRegisterUse::kReadAccumulator, OperandType::kUImm)     \
  V(JumpIfNotNull, ImplicitRegisterUse::kReadAccumulator, OperandType::kUImm)  \
  V(JumpIfUndefined, ImplicitRegisterUse::kReadAccumulator,                    \
    OperandType::kUImm)                                                        \
  V(JumpIfNotUndefined, ImplicitRegisterUse::kReadAccumulator,                 \
    OperandType::kUImm)                                                        \
  V(JumpIfUndefinedOrNull, ImplicitRegisterUse::kReadAccumulator,              \
    OperandType::kUImm)                                                        \
  V(JumpIfJSReceiver, ImplicitRegisterUse::kReadAccumulator,                   \
    OperandType::kUImm)                                                        \
                                                                               \
  /* Smi-table lookup for switch statements */                                 \
  V(SwitchOnSmiNoFeedback, ImplicitRegisterUse::kReadAccumulator,              \
    OperandType::kIdx, OperandType::kUImm, OperandType::kImm)                  \
                                                                               \
  /* Complex flow control For..in */                                           \
  V(ForInEnumerate, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg) \
  V(ForInPrepare, ImplicitRegisterUse::kReadAccumulator,                       \
    OperandType::kRegOutTriple, OperandType::kIdx)                             \
  V(ForInContinue, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,  \
    OperandType::kReg)                                                         \
  V(ForInNext, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,      \
    OperandType::kReg, OperandType::kRegPair, OperandType::kIdx)               \
  V(ForInStep, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg)      \
                                                                               \
  /* Update the pending message */                                             \
  V(SetPendingMessage, ImplicitRegisterUse::kReadWriteAccumulator)             \
                                                                               \
  /* Non-local flow control */                                                 \
  V(Throw, ImplicitRegisterUse::kReadAccumulator)                              \
  V(ReThrow, ImplicitRegisterUse::kReadAccumulator)                            \
  V(Return, ImplicitRegisterUse::kReadAccumulator)                             \
  V(ThrowReferenceErrorIfHole, ImplicitRegisterUse::kReadAccumulator,          \
    OperandType::kIdx)                                                         \
  V(ThrowSuperNotCalledIfHole, ImplicitRegisterUse::kReadAccumulator)          \
  V(ThrowSuperAlreadyCalledIfNotHole, ImplicitRegisterUse::kReadAccumulator)   \
  V(ThrowIfNotSuperConstructor, ImplicitRegisterUse::kNone, OperandType::kReg) \
                                                                               \
  /* Generators */                                                             \
  V(SwitchOnGeneratorState, ImplicitRegisterUse::kNone, OperandType::kReg,     \
    OperandType::kIdx, OperandType::kUImm)                                     \
  V(SuspendGenerator, ImplicitRegisterUse::kReadAccumulator,                   \
    OperandType::kReg, OperandType::kRegList, OperandType::kRegCount,          \
    OperandType::kUImm)                                                        \
  V(ResumeGenerator, ImplicitRegisterUse::kWriteAccumulator,                   \
    OperandType::kReg, OperandType::kRegOutList, OperandType::kRegCount)       \
                                                                               \
  /* Iterator protocol operations */                                           \
  V(GetIterator, ImplicitRegisterUse::kWriteAccumulator, OperandType::kReg,    \
    OperandType::kIdx, OperandType::kIdx)                                      \
                                                                               \
  /* Debugger */                                                               \
  V(Debugger, ImplicitRegisterUse::kNone)                                      \
                                                                               \
  /* Block Coverage */                                                         \
  V(IncBlockCounter, ImplicitRegisterUse::kNone, OperandType::kIdx)            \
                                                                               \
  /* Execution Abort (internal error) */                                       \
  V(Abort, ImplicitRegisterUse::kNone, OperandType::kIdx)

// The list of bytecodes which are interpreted by the interpreter.
// Format is V(<bytecode>, <implicit_register_use>, <operands>).
#define BYTECODE_LIST(V)                                             \
  BYTECODE_LIST_WITH_UNIQUE_HANDLERS(V)                              \
                                                                     \
  /* Special-case Star for common register numbers, to save space */ \
  SHORT_STAR_BYTECODE_LIST(V)                                        \
                                                                     \
  /* Illegal bytecode  */                                            \
  V(Illegal, ImplicitRegisterUse::kNone)

// List of debug break bytecodes.
#define DEBUG_BREAK_PLAIN_BYTECODE_LIST(V) \
  V(DebugBreak0)                           \
  V(DebugBreak1)                           \
  V(DebugBreak2)                           \
  V(DebugBreak3)                           \
  V(DebugBreak4)                           \
  V(DebugBreak5)                           \
  V(DebugBreak6)

#define DEBUG_BREAK_PREFIX_BYTECODE_LIST(V) \
  V(DebugBreakWide)                         \
  V(DebugBreakExtraWide)

#define DEBUG_BREAK_BYTECODE_LIST(V) \
  DEBUG_BREAK_PLAIN_BYTECODE_LIST(V) \
  DEBUG_BREAK_PREFIX_BYTECODE_LIST(V)

// Lists of jump bytecodes.

#define JUMP_UNCONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  V(JumpLoop)                                         \
  V(Jump)

#define JUMP_UNCONDITIONAL_CONSTANT_BYTECODE_LIST(V) V(JumpConstant)

#define JUMP_TOBOOLEAN_CONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  V(JumpIfToBooleanTrue)                                      \
  V(JumpIfToBooleanFalse)

#define JUMP_TOBOOLEAN_CONDITIONAL_CONSTANT_BYTECODE_LIST(V) \
  V(JumpIfToBooleanTrueConstant)                             \
  V(JumpIfToBooleanFalseConstant)

#define JUMP_CONDITIONAL_IMMEDIATE_BYTECODE_LIST(V)     \
  JUMP_TOBOOLEAN_CONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  V(JumpIfTrue)                                         \
  V(JumpIfFalse)                                        \
  V(JumpIfNull)                                         \
  V(JumpIfNotNull)                                      \
  V(JumpIfUndefined)                                    \
  V(JumpIfNotUndefined)                                 \
  V(JumpIfUndefinedOrNull)                              \
  V(JumpIfJSReceiver)

#define JUMP_CONDITIONAL_CONSTANT_BYTECODE_LIST(V)     \
  JUMP_TOBOOLEAN_CONDITIONAL_CONSTANT_BYTECODE_LIST(V) \
  V(JumpIfNullConstant)                                \
  V(JumpIfNotNullConstant)                             \
  V(JumpIfUndefinedConstant)                           \
  V(JumpIfNotUndefinedConstant)                        \
  V(JumpIfUndefinedOrNullConstant)                     \
  V(JumpIfTrueConstant)                                \
  V(JumpIfFalseConstant)                               \
  V(JumpIfJSReceiverConstant)

#define JUMP_CONSTANT_BYTECODE_LIST(V)         \
  JUMP_UNCONDITIONAL_CONSTANT_BYTECODE_LIST(V) \
  JUMP_CONDITIONAL_CONSTANT_BYTECODE_LIST(V)

#define JUMP_IMMEDIATE_BYTECODE_LIST(V)         \
  JUMP_UNCONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  JUMP_CONDITIONAL_IMMEDIATE_BYTECODE_LIST(V)

#define JUMP_TO_BOOLEAN_BYTECODE_LIST(V)                \
  JUMP_TOBOOLEAN_CONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  JUMP_TOBOOLEAN_CONDITIONAL_CONSTANT_BYTECODE_LIST(V)

#define JUMP_UNCONDITIONAL_BYTECODE_LIST(V)     \
  JUMP_UNCONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  JUMP_UNCONDITIONAL_CONSTANT_BYTECODE_LIST(V)

#define JUMP_CONDITIONAL_BYTECODE_LIST(V)     \
  JUMP_CONDITIONAL_IMMEDIATE_BYTECODE_LIST(V) \
  JUMP_CONDITIONAL_CONSTANT_BYTECODE_LIST(V)

#define JUMP_FORWARD_BYTECODE_LIST(V) \
  V(Jump)                             \
  V(JumpConstant)                     \
  JUMP_CONDITIONAL_BYTECODE_LIST(V)

#define JUMP_BYTECODE_LIST(V)   \
  JUMP_FORWARD_BYTECODE_LIST(V) \
  V(JumpLoop)

#define RETURN_BYTECODE_LIST(V) \
  V(Return)                     \
  V(SuspendGenerator)

// Enumeration of interpreter bytecodes.
enum class Bytecode : uint8_t {
#define DECLARE_BYTECODE(Name, ...) k##Name,
  BYTECODE_LIST(DECLARE_BYTECODE)
#undef DECLARE_BYTECODE
#define COUNT_BYTECODE(x, ...) +1
  // The COUNT_BYTECODE macro will turn this into kLast = -1 +1 +1... which will
  // evaluate to the same value as the last real bytecode.
  kLast = -1 BYTECODE_LIST(COUNT_BYTECODE),
  kFirstShortStar = kStar15,
  kLastShortStar = kStar0
#undef COUNT_BYTECODE
};

class V8_EXPORT_PRIVATE Bytecodes final : public AllStatic {
 public:
  // The maximum number of operands a bytecode may have.
  static const int kMaxOperands = 5;

  // The total number of bytecodes used.
  static const int kBytecodeCount = static_cast<int>(Bytecode::kLast) + 1;

  static const int kShortStarCount =
      static_cast<int>(Bytecode::kLastShortStar) -
      static_cast<int>(Bytecode::kFirstShortStar) + 1;

  // Returns string representation of |bytecode|.
  static const char* ToString(Bytecode bytecode);

  // Returns string representation of |bytecode| combined with |operand_scale|
  // using the optionally provided |separator|.
  static std::string ToString(Bytecode bytecode, OperandScale operand_scale,
                              const char* separator = ".");

  // Returns byte value of bytecode.
  static uint8_t ToByte(Bytecode bytecode) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    return static_cast<uint8_t>(bytecode);
  }

  // Returns bytecode for |value|.
  static Bytecode FromByte(uint8_t value) {
    Bytecode bytecode = static_cast<Bytecode>(value);
    DCHECK_LE(bytecode, Bytecode::kLast);
    return bytecode;
  }

  // Returns the prefix bytecode representing an operand scale to be
  // applied to a a bytecode.
  static Bytecode OperandScaleToPrefixBytecode(OperandScale operand_scale) {
    switch (operand_scale) {
      case OperandScale::kQuadruple:
        return Bytecode::kExtraWide;
      case OperandScale::kDouble:
        return Bytecode::kWide;
      default:
        UNREACHABLE();
    }
  }

  // Returns true if the operand scale requires a prefix bytecode.
  static bool OperandScaleRequiresPrefixBytecode(OperandScale operand_scale) {
    return operand_scale != OperandScale::kSingle;
  }

  // Returns the scaling applied to scalable operands if bytecode is
  // is a scaling prefix.
  static OperandScale PrefixBytecodeToOperandScale(Bytecode bytecode) {
    switch (bytecode) {
      case Bytecode::kExtraWide:
      case Bytecode::kDebugBreakExtraWide:
        return OperandScale::kQuadruple;
      case Bytecode::kWide:
      case Bytecode::kDebugBreakWide:
        return OperandScale::kDouble;
      default:
        UNREACHABLE();
    }
  }

  // Returns how accumulator is used by |bytecode|.
  static ImplicitRegisterUse GetImplicitRegisterUse(Bytecode bytecode) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    return kImplicitRegisterUse[static_cast<size_t>(bytecode)];
  }

  // Returns true if |bytecode| reads the accumulator.
  static bool ReadsAccumulator(Bytecode bytecode) {
    return BytecodeOperands::ReadsAccumulator(GetImplicitRegisterUse(bytecode));
  }

  // Returns true if |bytecode| writes the accumulator.
  static bool WritesAccumulator(Bytecode bytecode) {
    return BytecodeOperands::WritesAccumulator(
        GetImplicitRegisterUse(bytecode));
  }

  // Returns true if |bytecode| writes to a register not specified by an
  // operand.
  static bool WritesImplicitRegister(Bytecode bytecode) {
    return BytecodeOperands::WritesImplicitRegister(
        GetImplicitRegisterUse(bytecode));
  }

  // Return true if |bytecode| is an accumulator load without effects,
  // e.g. LdaConstant, LdaTrue, Ldar.
  static constexpr bool IsAccumulatorLoadWithoutEffects(Bytecode bytecode) {
    STATIC_ASSERT(Bytecode::kLdar < Bytecode::kLdaImmutableCurrentContextSlot);
    return bytecode >= Bytecode::kLdar &&
           bytecode <= Bytecode::kLdaImmutableCurrentContextSlot;
  }

  // Returns true if |bytecode| is a compare operation without external effects
  // (e.g., Type cooersion).
  static constexpr bool IsCompareWithoutEffects(Bytecode bytecode) {
    STATIC_ASSERT(Bytecode::kTestReferenceEqual < Bytecode::kTestTypeOf);
    return bytecode >= Bytecode::kTestReferenceEqual &&
           bytecode <= Bytecode::kTestTypeOf;
  }

  static constexpr bool IsShortStar(Bytecode bytecode) {
    return bytecode >= Bytecode::kFirstShortStar &&
           bytecode <= Bytecode::kLastShortStar;
  }

  static constexpr bool IsAnyStar(Bytecode bytecode) {
    return bytecode == Bytecode::kStar || IsShortStar(bytecode);
  }

  // Return true if |bytecode| is a register load without effects,
  // e.g. Mov, Star.
  static constexpr bool IsRegisterLoadWithoutEffects(Bytecode bytecode) {
    return IsShortStar(bytecode) ||
           (bytecode >= Bytecode::kStar && bytecode <= Bytecode::kPopContext);
  }

  // Returns true if the bytecode is a conditional jump taking
  // an immediate byte operand (OperandType::kImm).
  static constexpr bool IsConditionalJumpImmediate(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpIfToBooleanTrue &&
           bytecode <= Bytecode::kJumpIfJSReceiver;
  }

  // Returns true if the bytecode is a conditional jump taking
  // a constant pool entry (OperandType::kIdx).
  static constexpr bool IsConditionalJumpConstant(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpIfNullConstant &&
           bytecode <= Bytecode::kJumpIfToBooleanFalseConstant;
  }

  // Returns true if the bytecode is a conditional jump taking
  // any kind of operand.
  static constexpr bool IsConditionalJump(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpIfNullConstant &&
           bytecode <= Bytecode::kJumpIfJSReceiver;
  }

  // Returns true if the bytecode is an unconditional jump.
  static constexpr bool IsUnconditionalJump(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpLoop &&
           bytecode <= Bytecode::kJumpConstant;
  }

  // Returns true if the bytecode is a jump or a conditional jump taking
  // an immediate byte operand (OperandType::kImm).
  static constexpr bool IsJumpImmediate(Bytecode bytecode) {
    return bytecode == Bytecode::kJump || bytecode == Bytecode::kJumpLoop ||
           IsConditionalJumpImmediate(bytecode);
  }

  // Returns true if the bytecode is a jump or conditional jump taking a
  // constant pool entry (OperandType::kIdx).
  static constexpr bool IsJumpConstant(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpConstant &&
           bytecode <= Bytecode::kJumpIfToBooleanFalseConstant;
  }

  // Returns true if the bytecode is a jump that internally coerces the
  // accumulator to a boolean.
  static constexpr bool IsJumpIfToBoolean(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpIfToBooleanTrueConstant &&
           bytecode <= Bytecode::kJumpIfToBooleanFalse;
  }

  // Returns true if the bytecode is a jump or conditional jump taking
  // any kind of operand.
  static constexpr bool IsJump(Bytecode bytecode) {
    return bytecode >= Bytecode::kJumpLoop &&
           bytecode <= Bytecode::kJumpIfJSReceiver;
  }

  // Returns true if the bytecode is a forward jump or conditional jump taking
  // any kind of operand.
  static constexpr bool IsForwardJump(Bytecode bytecode) {
    return bytecode >= Bytecode::kJump &&
           bytecode <= Bytecode::kJumpIfJSReceiver;
  }

  // Return true if |bytecode| is a jump without effects,
  // e.g. any jump excluding those that include type coercion like
  // JumpIfTrueToBoolean, and JumpLoop due to having an implicit StackCheck.
  static constexpr bool IsJumpWithoutEffects(Bytecode bytecode) {
    return IsJump(bytecode) && !IsJumpIfToBoolean(bytecode) &&
           bytecode != Bytecode::kJumpLoop;
  }

  // Returns true if the bytecode is a switch.
  static constexpr bool IsSwitch(Bytecode bytecode) {
    return bytecode == Bytecode::kSwitchOnSmiNoFeedback ||
           bytecode == Bytecode::kSwitchOnGeneratorState;
  }

  // Returns true if |bytecode| has no effects. These bytecodes only manipulate
  // interpreter frame state and will never throw.
  static constexpr bool IsWithoutExternalSideEffects(Bytecode bytecode) {
    return (IsAccumulatorLoadWithoutEffects(bytecode) ||
            IsRegisterLoadWithoutEffects(bytecode) ||
            IsCompareWithoutEffects(bytecode) ||
            IsJumpWithoutEffects(bytecode) || IsSwitch(bytecode));
  }

  // Returns true if the bytecode is Ldar or Star.
  static constexpr bool IsLdarOrStar(Bytecode bytecode) {
    return bytecode == Bytecode::kLdar || IsAnyStar(bytecode);
  }

  // Returns true if the bytecode is a call or a constructor call.
  static constexpr bool IsCallOrConstruct(Bytecode bytecode) {
    return bytecode == Bytecode::kCallAnyReceiver ||
           bytecode == Bytecode::kCallProperty ||
           bytecode == Bytecode::kCallProperty0 ||
           bytecode == Bytecode::kCallProperty1 ||
           bytecode == Bytecode::kCallProperty2 ||
           bytecode == Bytecode::kCallUndefinedReceiver ||
           bytecode == Bytecode::kCallUndefinedReceiver0 ||
           bytecode == Bytecode::kCallUndefinedReceiver1 ||
           bytecode == Bytecode::kCallUndefinedReceiver2 ||
           bytecode == Bytecode::kCallNoFeedback ||
           bytecode == Bytecode::kConstruct ||
           bytecode == Bytecode::kCallWithSpread ||
           bytecode == Bytecode::kConstructWithSpread ||
           bytecode == Bytecode::kCallJSRuntime;
  }

  // Returns true if the bytecode is a call to the runtime.
  static constexpr bool IsCallRuntime(Bytecode bytecode) {
    return bytecode == Bytecode::kCallRuntime ||
           bytecode == Bytecode::kCallRuntimeForPair ||
           bytecode == Bytecode::kInvokeIntrinsic;
  }

  // Returns true if the bytecode is an one-shot bytecode.  One-shot bytecodes
  // don`t collect feedback and are intended for code that runs only once and
  // shouldn`t be optimized.
  static constexpr bool IsOneShotBytecode(Bytecode bytecode) {
    return bytecode == Bytecode::kCallNoFeedback ||
           bytecode == Bytecode::kLdaNamedPropertyNoFeedback ||
           bytecode == Bytecode::kStaNamedPropertyNoFeedback;
  }

  // Returns true if the bytecode is a scaling prefix bytecode.
  static constexpr bool IsPrefixScalingBytecode(Bytecode bytecode) {
    return bytecode == Bytecode::kExtraWide || bytecode == Bytecode::kWide ||
           bytecode == Bytecode::kDebugBreakExtraWide ||
           bytecode == Bytecode::kDebugBreakWide;
  }

  // Returns true if the bytecode returns.
  static constexpr bool Returns(Bytecode bytecode) {
#define OR_BYTECODE(NAME) || bytecode == Bytecode::k##NAME
    return false RETURN_BYTECODE_LIST(OR_BYTECODE);
#undef OR_BYTECODE
  }

  // Returns the number of operands expected by |bytecode|.
  static int NumberOfOperands(Bytecode bytecode) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    return kOperandCount[static_cast<size_t>(bytecode)];
  }

  // Returns the i-th operand of |bytecode|.
  static OperandType GetOperandType(Bytecode bytecode, int i) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    DCHECK_LT(i, NumberOfOperands(bytecode));
    DCHECK_GE(i, 0);
    return GetOperandTypes(bytecode)[i];
  }

  // Returns a pointer to an array of operand types terminated in
  // OperandType::kNone.
  static const OperandType* GetOperandTypes(Bytecode bytecode) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    return kOperandTypes[static_cast<size_t>(bytecode)];
  }

  static bool OperandIsScalableSignedByte(Bytecode bytecode,
                                          int operand_index) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    return kOperandTypeInfos[static_cast<size_t>(bytecode)][operand_index] ==
           OperandTypeInfo::kScalableSignedByte;
  }

  static bool OperandIsScalableUnsignedByte(Bytecode bytecode,
                                            int operand_index) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    return kOperandTypeInfos[static_cast<size_t>(bytecode)][operand_index] ==
           OperandTypeInfo::kScalableUnsignedByte;
  }

  static bool OperandIsScalable(Bytecode bytecode, int operand_index) {
    return OperandIsScalableSignedByte(bytecode, operand_index) ||
           OperandIsScalableUnsignedByte(bytecode, operand_index);
  }

  // Returns true if the bytecode has wider operand forms.
  static bool IsBytecodeWithScalableOperands(Bytecode bytecode);

  // Returns the size of the i-th operand of |bytecode|.
  static OperandSize GetOperandSize(Bytecode bytecode, int i,
                                    OperandScale operand_scale) {
    CHECK_LT(i, NumberOfOperands(bytecode));
    return GetOperandSizes(bytecode, operand_scale)[i];
  }

  // Returns the operand sizes of |bytecode| with scale |operand_scale|.
  static const OperandSize* GetOperandSizes(Bytecode bytecode,
                                            OperandScale operand_scale) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    DCHECK_GE(operand_scale, OperandScale::kSingle);
    DCHECK_LE(operand_scale, OperandScale::kLast);
    STATIC_ASSERT(static_cast<int>(OperandScale::kQuadruple) == 4 &&
                  OperandScale::kLast == OperandScale::kQuadruple);
    int scale_index = static_cast<int>(operand_scale) >> 1;
    return kOperandSizes[scale_index][static_cast<size_t>(bytecode)];
  }

  // Returns the offset of the i-th operand of |bytecode| relative to the start
  // of the bytecode.
  static int GetOperandOffset(Bytecode bytecode, int i,
                              OperandScale operand_scale);

  // Returns the size of the bytecode including its operands for the
  // given |operand_scale|.
  static int Size(Bytecode bytecode, OperandScale operand_scale) {
    DCHECK_LE(bytecode, Bytecode::kLast);
    STATIC_ASSERT(static_cast<int>(OperandScale::kQuadruple) == 4 &&
                  OperandScale::kLast == OperandScale::kQuadruple);
    int scale_index = static_cast<int>(operand_scale) >> 1;
    return kBytecodeSizes[scale_index][static_cast<size_t>(bytecode)];
  }

  // Returns a debug break bytecode to replace |bytecode|.
  static Bytecode GetDebugBreak(Bytecode bytecode);

  // Returns the equivalent jump bytecode without the accumulator coercion.
  static Bytecode GetJumpWithoutToBoolean(Bytecode bytecode);

  // Returns true if there is a call in the most-frequently executed path
  // through the bytecode's handler.
  static bool MakesCallAlongCriticalPath(Bytecode bytecode);

  // Returns the receiver mode of the given call bytecode.
  static ConvertReceiverMode GetReceiverMode(Bytecode bytecode) {
    DCHECK(IsCallOrConstruct(bytecode) ||
           bytecode == Bytecode::kInvokeIntrinsic);
    switch (bytecode) {
      case Bytecode::kCallProperty:
      case Bytecode::kCallProperty0:
      case Bytecode::kCallProperty1:
      case Bytecode::kCallProperty2:
        return ConvertReceiverMode::kNotNullOrUndefined;
      case Bytecode::kCallUndefinedReceiver:
      case Bytecode::kCallUndefinedReceiver0:
      case Bytecode::kCallUndefinedReceiver1:
      case Bytecode::kCallUndefinedReceiver2:
      case Bytecode::kCallJSRuntime:
        return ConvertReceiverMode::kNullOrUndefined;
      case Bytecode::kCallAnyReceiver:
      case Bytecode::kCallNoFeedback:
      case Bytecode::kConstruct:
      case Bytecode::kCallWithSpread:
      case Bytecode::kConstructWithSpread:
      case Bytecode::kInvokeIntrinsic:
        return ConvertReceiverMode::kAny;
      default:
        UNREACHABLE();
    }
  }

  // Returns true if the bytecode is a debug break.
  static bool IsDebugBreak(Bytecode bytecode);

  // Returns true if |operand_type| is any type of register operand.
  static bool IsRegisterOperandType(OperandType operand_type);

  // Returns true if |operand_type| represents a register used as an input.
  static bool IsRegisterInputOperandType(OperandType operand_type);

  // Returns true if |operand_type| represents a register used as an output.
  static bool IsRegisterOutputOperandType(OperandType operand_type);

  // Returns true if |operand_type| represents a register list operand.
  static bool IsRegisterListOperandType(OperandType operand_type);

  // Returns true if the handler for |bytecode| should look ahead and inline a
  // dispatch to a Star bytecode.
  static bool IsStarLookahead(Bytecode bytecode, OperandScale operand_scale);

  // Returns the number of registers represented by a register operand. For
  // instance, a RegPair represents two registers. Should not be called for
  // kRegList which has a variable number of registers based on the following
  // kRegCount operand.
  static int GetNumberOfRegistersRepresentedBy(OperandType operand_type) {
    switch (operand_type) {
      case OperandType::kReg:
      case OperandType::kRegOut:
        return 1;
      case OperandType::kRegPair:
      case OperandType::kRegOutPair:
        return 2;
      case OperandType::kRegOutTriple:
        return 3;
      case OperandType::kRegList:
      case OperandType::kRegOutList:
        UNREACHABLE();
      default:
        return 0;
    }
    UNREACHABLE();
  }

  // Returns the size of |operand_type| for |operand_scale|.
  static OperandSize SizeOfOperand(OperandType operand_type,
                                   OperandScale operand_scale) {
    DCHECK_LE(operand_type, OperandType::kLast);
    DCHECK_GE(operand_scale, OperandScale::kSingle);
    DCHECK_LE(operand_scale, OperandScale::kLast);
    STATIC_ASSERT(static_cast<int>(OperandScale::kQuadruple) == 4 &&
                  OperandScale::kLast == OperandScale::kQuadruple);
    int scale_index = static_cast<int>(operand_scale) >> 1;
    return kOperandKindSizes[scale_index][static_cast<size_t>(operand_type)];
  }

  // Returns true if |operand_type| is a runtime-id operand (kRuntimeId).
  static bool IsRuntimeIdOperandType(OperandType operand_type);

  // Returns true if |operand_type| is unsigned, false if signed.
  static bool IsUnsignedOperandType(OperandType operand_type);

  // Returns true if a handler is generated for a bytecode at a given
  // operand scale. All bytecodes have handlers at OperandScale::kSingle,
  // but only bytecodes with scalable operands have handlers with larger
  // OperandScale values.
  static bool BytecodeHasHandler(Bytecode bytecode, OperandScale operand_scale);

  // Return the operand scale required to hold a signed operand with |value|.
  static OperandScale ScaleForSignedOperand(int32_t value) {
    if (value >= kMinInt8 && value <= kMaxInt8) {
      return OperandScale::kSingle;
    } else if (value >= kMinInt16 && value <= kMaxInt16) {
      return OperandScale::kDouble;
    } else {
      return OperandScale::kQuadruple;
    }
  }

  // Return the operand scale required to hold an unsigned operand with |value|.
  static OperandScale ScaleForUnsignedOperand(uint32_t value) {
    if (value <= kMaxUInt8) {
      return OperandScale::kSingle;
    } else if (value <= kMaxUInt16) {
      return OperandScale::kDouble;
    } else {
      return OperandScale::kQuadruple;
    }
  }

  // Return the operand size required to hold an unsigned operand with |value|.
  static OperandSize SizeForUnsignedOperand(uint32_t value) {
    if (value <= kMaxUInt8) {
      return OperandSize::kByte;
    } else if (value <= kMaxUInt16) {
      return OperandSize::kShort;
    } else {
      return OperandSize::kQuad;
    }
  }

  static Address bytecode_size_table_address() {
    return reinterpret_cast<Address>(
        const_cast<uint8_t*>(&kBytecodeSizes[0][0]));
  }

 private:
  static const OperandType* const kOperandTypes[];
  static const OperandTypeInfo* const kOperandTypeInfos[];
  static const int kOperandCount[];
  static const int kNumberOfRegisterOperands[];
  static const ImplicitRegisterUse kImplicitRegisterUse[];
  static const bool kIsScalable[];
  static const uint8_t kBytecodeSizes[3][kBytecodeCount];
  static const OperandSize* const kOperandSizes[3][kBytecodeCount];
  static OperandSize const
      kOperandKindSizes[3][BytecodeOperands::kOperandTypeCount];
};

V8_EXPORT_PRIVATE std::ostream& operator<<(std::ostream& os,
                                           const Bytecode& bytecode);

}  // namespace interpreter
}  // namespace internal
}  // namespace v8

#endif  // V8_INTERPRETER_BYTECODES_H_
