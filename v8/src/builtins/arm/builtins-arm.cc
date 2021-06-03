// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if V8_TARGET_ARCH_ARM

#include "src/api/api-arguments.h"
#include "src/codegen/code-factory.h"
#include "src/codegen/interface-descriptors-inl.h"
// For interpreter_entry_return_pc_offset. TODO(jkummerow): Drop.
#include "src/codegen/macro-assembler-inl.h"
#include "src/codegen/register-configuration.h"
#include "src/debug/debug.h"
#include "src/deoptimizer/deoptimizer.h"
#include "src/execution/frame-constants.h"
#include "src/execution/frames.h"
#include "src/heap/heap-inl.h"
#include "src/logging/counters.h"
#include "src/objects/cell.h"
#include "src/objects/foreign.h"
#include "src/objects/heap-number.h"
#include "src/objects/js-generator.h"
#include "src/objects/objects-inl.h"
#include "src/objects/smi.h"
#include "src/runtime/runtime.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/wasm-linkage.h"
#include "src/wasm/wasm-objects.h"
#endif  // V8_ENABLE_WEBASSEMBLY

namespace v8 {
namespace internal {

#define __ ACCESS_MASM(masm)

void Builtins::Generate_Adaptor(MacroAssembler* masm, Address address) {
#if defined(__thumb__)
  // Thumb mode builtin.
  DCHECK_EQ(1, reinterpret_cast<uintptr_t>(
                   ExternalReference::Create(address).address()) &
                   1);
#endif
  __ Move(kJavaScriptCallExtraArg1Register, ExternalReference::Create(address));
  __ Jump(BUILTIN_CODE(masm->isolate(), AdaptorWithBuiltinExitFrame),
          RelocInfo::CODE_TARGET);
}

static void GenerateTailCallToReturnedCode(MacroAssembler* masm,
                                           Runtime::FunctionId function_id) {
  // ----------- S t a t e -------------
  //  -- r0 : actual argument count
  //  -- r1 : target function (preserved for callee)
  //  -- r3 : new target (preserved for callee)
  // -----------------------------------
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    // Push a copy of the target function, the new target and the actual
    // argument count.
    // Push function as parameter to the runtime call.
    __ SmiTag(kJavaScriptCallArgCountRegister);
    __ Push(kJavaScriptCallTargetRegister, kJavaScriptCallNewTargetRegister,
            kJavaScriptCallArgCountRegister, kJavaScriptCallTargetRegister);

    __ CallRuntime(function_id, 1);
    __ mov(r2, r0);

    // Restore target function, new target and actual argument count.
    __ Pop(kJavaScriptCallTargetRegister, kJavaScriptCallNewTargetRegister,
           kJavaScriptCallArgCountRegister);
    __ SmiUntag(kJavaScriptCallArgCountRegister);
  }
  static_assert(kJavaScriptCallCodeStartRegister == r2, "ABI mismatch");
  __ JumpCodeObject(r2);
}

namespace {

void Generate_JSBuiltinsConstructStubHelper(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0     : number of arguments
  //  -- r1     : constructor function
  //  -- r3     : new target
  //  -- cp     : context
  //  -- lr     : return address
  //  -- sp[...]: constructor arguments
  // -----------------------------------

  Register scratch = r2;

  Label stack_overflow;

  __ StackOverflowCheck(r0, scratch, &stack_overflow);

  // Enter a construct frame.
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::CONSTRUCT);

    // Preserve the incoming parameters on the stack.
    __ SmiTag(r0);
    __ Push(cp, r0);
    __ SmiUntag(r0);

    // TODO(victorgomes): When the arguments adaptor is completely removed, we
    // should get the formal parameter count and copy the arguments in its
    // correct position (including any undefined), instead of delaying this to
    // InvokeFunction.

    // Set up pointer to last argument (skip receiver).
    __ add(
        r4, fp,
        Operand(StandardFrameConstants::kCallerSPOffset + kSystemPointerSize));
    // Copy arguments and receiver to the expression stack.
    __ PushArray(r4, r0, r5);
    // The receiver for the builtin/api call.
    __ PushRoot(RootIndex::kTheHoleValue);

    // Call the function.
    // r0: number of arguments (untagged)
    // r1: constructor function
    // r3: new target
    __ InvokeFunctionWithNewTarget(r1, r3, r0, InvokeType::kCall);

    // Restore context from the frame.
    __ ldr(cp, MemOperand(fp, ConstructFrameConstants::kContextOffset));
    // Restore smi-tagged arguments count from the frame.
    __ ldr(scratch, MemOperand(fp, ConstructFrameConstants::kLengthOffset));
    // Leave construct frame.
  }

  // Remove caller arguments from the stack and return.
  STATIC_ASSERT(kSmiTagSize == 1 && kSmiTag == 0);
  __ add(sp, sp, Operand(scratch, LSL, kPointerSizeLog2 - kSmiTagSize));
  __ add(sp, sp, Operand(kPointerSize));
  __ Jump(lr);

  __ bind(&stack_overflow);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kThrowStackOverflow);
    __ bkpt(0);  // Unreachable code.
  }
}

}  // namespace

// The construct stub for ES5 constructor functions and ES6 class constructors.
void Builtins::Generate_JSConstructStubGeneric(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  --      r0: number of arguments (untagged)
  //  --      r1: constructor function
  //  --      r3: new target
  //  --      cp: context
  //  --      lr: return address
  //  -- sp[...]: constructor arguments
  // -----------------------------------

  FrameScope scope(masm, StackFrame::MANUAL);
  // Enter a construct frame.
  Label post_instantiation_deopt_entry, not_create_implicit_receiver;
  __ EnterFrame(StackFrame::CONSTRUCT);

  // Preserve the incoming parameters on the stack.
  __ LoadRoot(r4, RootIndex::kTheHoleValue);
  __ SmiTag(r0);
  __ Push(cp, r0, r1, r4, r3);

  // ----------- S t a t e -------------
  //  --        sp[0*kPointerSize]: new target
  //  --        sp[1*kPointerSize]: padding
  //  -- r1 and sp[2*kPointerSize]: constructor function
  //  --        sp[3*kPointerSize]: number of arguments (tagged)
  //  --        sp[4*kPointerSize]: context
  // -----------------------------------

  __ ldr(r4, FieldMemOperand(r1, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(r4, FieldMemOperand(r4, SharedFunctionInfo::kFlagsOffset));
  __ DecodeField<SharedFunctionInfo::FunctionKindBits>(r4);
  __ JumpIfIsInRange(r4, kDefaultDerivedConstructor, kDerivedConstructor,
                     &not_create_implicit_receiver);

  // If not derived class constructor: Allocate the new receiver object.
  __ IncrementCounter(masm->isolate()->counters()->constructed_objects(), 1, r4,
                      r5);
  __ Call(BUILTIN_CODE(masm->isolate(), FastNewObject), RelocInfo::CODE_TARGET);
  __ b(&post_instantiation_deopt_entry);

  // Else: use TheHoleValue as receiver for constructor call
  __ bind(&not_create_implicit_receiver);
  __ LoadRoot(r0, RootIndex::kTheHoleValue);

  // ----------- S t a t e -------------
  //  --                          r0: receiver
  //  -- Slot 3 / sp[0*kPointerSize]: new target
  //  -- Slot 2 / sp[1*kPointerSize]: constructor function
  //  -- Slot 1 / sp[2*kPointerSize]: number of arguments (tagged)
  //  -- Slot 0 / sp[3*kPointerSize]: context
  // -----------------------------------
  // Deoptimizer enters here.
  masm->isolate()->heap()->SetConstructStubCreateDeoptPCOffset(
      masm->pc_offset());
  __ bind(&post_instantiation_deopt_entry);

  // Restore new target.
  __ Pop(r3);

  // Push the allocated receiver to the stack.
  __ Push(r0);
  // We need two copies because we may have to return the original one
  // and the calling conventions dictate that the called function pops the
  // receiver. The second copy is pushed after the arguments, we saved in r6
  // since r0 needs to store the number of arguments before
  // InvokingFunction.
  __ mov(r6, r0);

  // Set up pointer to first argument (skip receiver).
  __ add(r4, fp,
         Operand(StandardFrameConstants::kCallerSPOffset + kSystemPointerSize));

  // Restore constructor function and argument count.
  __ ldr(r1, MemOperand(fp, ConstructFrameConstants::kConstructorOffset));
  __ ldr(r0, MemOperand(fp, ConstructFrameConstants::kLengthOffset));
  __ SmiUntag(r0);

  Label stack_overflow;
  __ StackOverflowCheck(r0, r5, &stack_overflow);

  // TODO(victorgomes): When the arguments adaptor is completely removed, we
  // should get the formal parameter count and copy the arguments in its
  // correct position (including any undefined), instead of delaying this to
  // InvokeFunction.

  // Copy arguments to the expression stack.
  __ PushArray(r4, r0, r5);

  // Push implicit receiver.
  __ Push(r6);

  // Call the function.
  __ InvokeFunctionWithNewTarget(r1, r3, r0, InvokeType::kCall);

  // ----------- S t a t e -------------
  //  --                 r0: constructor result
  //  -- sp[0*kPointerSize]: implicit receiver
  //  -- sp[1*kPointerSize]: padding
  //  -- sp[2*kPointerSize]: constructor function
  //  -- sp[3*kPointerSize]: number of arguments
  //  -- sp[4*kPointerSize]: context
  // -----------------------------------

  // Store offset of return address for deoptimizer.
  masm->isolate()->heap()->SetConstructStubInvokeDeoptPCOffset(
      masm->pc_offset());

  // If the result is an object (in the ECMA sense), we should get rid
  // of the receiver and use the result; see ECMA-262 section 13.2.2-7
  // on page 74.
  Label use_receiver, do_throw, leave_and_return, check_receiver;

  // If the result is undefined, we jump out to using the implicit receiver.
  __ JumpIfNotRoot(r0, RootIndex::kUndefinedValue, &check_receiver);

  // Otherwise we do a smi check and fall through to check if the return value
  // is a valid receiver.

  // Throw away the result of the constructor invocation and use the
  // on-stack receiver as the result.
  __ bind(&use_receiver);
  __ ldr(r0, MemOperand(sp, 0 * kPointerSize));
  __ JumpIfRoot(r0, RootIndex::kTheHoleValue, &do_throw);

  __ bind(&leave_and_return);
  // Restore smi-tagged arguments count from the frame.
  __ ldr(r1, MemOperand(fp, ConstructFrameConstants::kLengthOffset));
  // Leave construct frame.
  __ LeaveFrame(StackFrame::CONSTRUCT);

  // Remove caller arguments from the stack and return.
  STATIC_ASSERT(kSmiTagSize == 1 && kSmiTag == 0);
  __ add(sp, sp, Operand(r1, LSL, kPointerSizeLog2 - kSmiTagSize));
  __ add(sp, sp, Operand(kPointerSize));
  __ Jump(lr);

  __ bind(&check_receiver);
  // If the result is a smi, it is *not* an object in the ECMA sense.
  __ JumpIfSmi(r0, &use_receiver);

  // If the type of the result (stored in its map) is less than
  // FIRST_JS_RECEIVER_TYPE, it is not an object in the ECMA sense.
  STATIC_ASSERT(LAST_JS_RECEIVER_TYPE == LAST_TYPE);
  __ CompareObjectType(r0, r4, r5, FIRST_JS_RECEIVER_TYPE);
  __ b(ge, &leave_and_return);
  __ b(&use_receiver);

  __ bind(&do_throw);
  // Restore the context from the frame.
  __ ldr(cp, MemOperand(fp, ConstructFrameConstants::kContextOffset));
  __ CallRuntime(Runtime::kThrowConstructorReturnedNonObject);
  __ bkpt(0);

  __ bind(&stack_overflow);
  // Restore the context from the frame.
  __ ldr(cp, MemOperand(fp, ConstructFrameConstants::kContextOffset));
  __ CallRuntime(Runtime::kThrowStackOverflow);
  // Unreachable code.
  __ bkpt(0);
}

void Builtins::Generate_JSBuiltinsConstructStub(MacroAssembler* masm) {
  Generate_JSBuiltinsConstructStubHelper(masm);
}

static void GetSharedFunctionInfoBytecodeOrBaseline(MacroAssembler* masm,
                                                    Register sfi_data,
                                                    Register scratch1,
                                                    Label* is_baseline) {
  Label done;

  __ CompareObjectType(sfi_data, scratch1, scratch1, BASELINE_DATA_TYPE);
  __ b(eq, is_baseline);
  __ cmp(scratch1, Operand(INTERPRETER_DATA_TYPE));
  __ b(ne, &done);
  __ ldr(sfi_data,
         FieldMemOperand(sfi_data, InterpreterData::kBytecodeArrayOffset));

  __ bind(&done);
}

// static
void Builtins::Generate_ResumeGeneratorTrampoline(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : the value to pass to the generator
  //  -- r1 : the JSGeneratorObject to resume
  //  -- lr : return address
  // -----------------------------------
  __ AssertGeneratorObject(r1);

  // Store input value into generator object.
  __ str(r0, FieldMemOperand(r1, JSGeneratorObject::kInputOrDebugPosOffset));
  __ RecordWriteField(r1, JSGeneratorObject::kInputOrDebugPosOffset, r0,
                      kLRHasNotBeenSaved, SaveFPRegsMode::kIgnore);

  // Load suspended function and context.
  __ ldr(r4, FieldMemOperand(r1, JSGeneratorObject::kFunctionOffset));
  __ ldr(cp, FieldMemOperand(r4, JSFunction::kContextOffset));

  Label prepare_step_in_if_stepping, prepare_step_in_suspended_generator;
  Label stepping_prepared;
  Register scratch = r5;

  // Flood function if we are stepping.
  ExternalReference debug_hook =
      ExternalReference::debug_hook_on_function_call_address(masm->isolate());
  __ Move(scratch, debug_hook);
  __ ldrsb(scratch, MemOperand(scratch));
  __ cmp(scratch, Operand(0));
  __ b(ne, &prepare_step_in_if_stepping);

  // Flood function if we need to continue stepping in the suspended
  // generator.
  ExternalReference debug_suspended_generator =
      ExternalReference::debug_suspended_generator_address(masm->isolate());
  __ Move(scratch, debug_suspended_generator);
  __ ldr(scratch, MemOperand(scratch));
  __ cmp(scratch, Operand(r1));
  __ b(eq, &prepare_step_in_suspended_generator);
  __ bind(&stepping_prepared);

  // Check the stack for overflow. We are not trying to catch interruptions
  // (i.e. debug break and preemption) here, so check the "real stack limit".
  Label stack_overflow;
  __ LoadStackLimit(scratch, StackLimitKind::kRealStackLimit);
  __ cmp(sp, scratch);
  __ b(lo, &stack_overflow);

  // ----------- S t a t e -------------
  //  -- r1    : the JSGeneratorObject to resume
  //  -- r4    : generator function
  //  -- cp    : generator context
  //  -- lr    : return address
  //  -- sp[0] : generator receiver
  // -----------------------------------

  // Copy the function arguments from the generator object's register file.
  __ ldr(r3, FieldMemOperand(r4, JSFunction::kSharedFunctionInfoOffset));
  __ ldrh(r3,
          FieldMemOperand(r3, SharedFunctionInfo::kFormalParameterCountOffset));
  __ ldr(r2,
         FieldMemOperand(r1, JSGeneratorObject::kParametersAndRegistersOffset));
  {
    Label done_loop, loop;
    __ bind(&loop);
    __ sub(r3, r3, Operand(1), SetCC);
    __ b(lt, &done_loop);
    __ add(scratch, r2, Operand(r3, LSL, kTaggedSizeLog2));
    __ ldr(scratch, FieldMemOperand(scratch, FixedArray::kHeaderSize));
    __ Push(scratch);
    __ b(&loop);
    __ bind(&done_loop);

    // Push receiver.
    __ ldr(scratch, FieldMemOperand(r1, JSGeneratorObject::kReceiverOffset));
    __ Push(scratch);
  }

  // Underlying function needs to have bytecode available.
  if (FLAG_debug_code) {
    Label is_baseline;
    __ ldr(r3, FieldMemOperand(r4, JSFunction::kSharedFunctionInfoOffset));
    __ ldr(r3, FieldMemOperand(r3, SharedFunctionInfo::kFunctionDataOffset));
    GetSharedFunctionInfoBytecodeOrBaseline(masm, r3, r0, &is_baseline);
    __ CompareObjectType(r3, r3, r3, BYTECODE_ARRAY_TYPE);
    __ Assert(eq, AbortReason::kMissingBytecodeArray);
    __ bind(&is_baseline);
  }

  // Resume (Ignition/TurboFan) generator object.
  {
    __ ldr(r0, FieldMemOperand(r4, JSFunction::kSharedFunctionInfoOffset));
    __ ldrh(r0, FieldMemOperand(
                    r0, SharedFunctionInfo::kFormalParameterCountOffset));
    // We abuse new.target both to indicate that this is a resume call and to
    // pass in the generator object.  In ordinary calls, new.target is always
    // undefined because generator functions are non-constructable.
    __ Move(r3, r1);
    __ Move(r1, r4);
    static_assert(kJavaScriptCallCodeStartRegister == r2, "ABI mismatch");
    __ ldr(r2, FieldMemOperand(r1, JSFunction::kCodeOffset));
    __ JumpCodeObject(r2);
  }

  __ bind(&prepare_step_in_if_stepping);
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    __ Push(r1, r4);
    // Push hole as receiver since we do not use it for stepping.
    __ PushRoot(RootIndex::kTheHoleValue);
    __ CallRuntime(Runtime::kDebugOnFunctionCall);
    __ Pop(r1);
    __ ldr(r4, FieldMemOperand(r1, JSGeneratorObject::kFunctionOffset));
  }
  __ b(&stepping_prepared);

  __ bind(&prepare_step_in_suspended_generator);
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    __ Push(r1);
    __ CallRuntime(Runtime::kDebugPrepareStepInSuspendedGenerator);
    __ Pop(r1);
    __ ldr(r4, FieldMemOperand(r1, JSGeneratorObject::kFunctionOffset));
  }
  __ b(&stepping_prepared);

  __ bind(&stack_overflow);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kThrowStackOverflow);
    __ bkpt(0);  // This should be unreachable.
  }
}

void Builtins::Generate_ConstructedNonConstructable(MacroAssembler* masm) {
  FrameScope scope(masm, StackFrame::INTERNAL);
  __ push(r1);
  __ CallRuntime(Runtime::kThrowConstructedNonConstructable);
}

namespace {

// Total size of the stack space pushed by JSEntryVariant.
// JSEntryTrampoline uses this to access on stack arguments passed to
// JSEntryVariant.
constexpr int kPushedStackSpace = kNumCalleeSaved * kPointerSize -
                                  kPointerSize /* FP */ +
                                  kNumDoubleCalleeSaved * kDoubleSize +
                                  5 * kPointerSize /* r5, r6, r7, fp, lr */ +
                                  EntryFrameConstants::kCallerFPOffset;

// Assert that the EntryFrameConstants are in sync with the builtin.
static_assert(kPushedStackSpace == EntryFrameConstants::kDirectCallerSPOffset +
                                       3 * kPointerSize /* r5, r6, r7*/ +
                                       EntryFrameConstants::kCallerFPOffset,
              "Pushed stack space and frame constants do not match. See "
              "frame-constants-arm.h");

// Called with the native C calling convention. The corresponding function
// signature is either:
//
//   using JSEntryFunction = GeneratedCode<Address(
//       Address root_register_value, Address new_target, Address target,
//       Address receiver, intptr_t argc, Address** argv)>;
// or
//   using JSEntryFunction = GeneratedCode<Address(
//       Address root_register_value, MicrotaskQueue* microtask_queue)>;
void Generate_JSEntryVariant(MacroAssembler* masm, StackFrame::Type type,
                             Builtins::Name entry_trampoline) {
  // The register state is either:
  //   r0:                            root_register_value
  //   r1:                            code entry
  //   r2:                            function
  //   r3:                            receiver
  //   [sp + 0 * kSystemPointerSize]: argc
  //   [sp + 1 * kSystemPointerSize]: argv
  // or
  //   r0: root_register_value
  //   r1: microtask_queue
  // Preserve all but r0 and pass them to entry_trampoline.
  Label invoke, handler_entry, exit;
  const RegList kCalleeSavedWithoutFp = kCalleeSaved & ~fp.bit();

  // Update |pushed_stack_space| when we manipulate the stack.
  int pushed_stack_space = EntryFrameConstants::kCallerFPOffset;
  {
    NoRootArrayScope no_root_array(masm);

    // Called from C, so do not pop argc and args on exit (preserve sp)
    // No need to save register-passed args
    // Save callee-saved registers (incl. cp), but without fp
    __ stm(db_w, sp, kCalleeSavedWithoutFp);
    pushed_stack_space +=
        kNumCalleeSaved * kPointerSize - kPointerSize /* FP */;

    // Save callee-saved vfp registers.
    __ vstm(db_w, sp, kFirstCalleeSavedDoubleReg, kLastCalleeSavedDoubleReg);
    pushed_stack_space += kNumDoubleCalleeSaved * kDoubleSize;

    // Set up the reserved register for 0.0.
    __ vmov(kDoubleRegZero, Double(0.0));

    // Initialize the root register.
    // C calling convention. The first argument is passed in r0.
    __ mov(kRootRegister, r0);
  }

  // Push a frame with special values setup to mark it as an entry frame.
  // r0: root_register_value
  __ mov(r7, Operand(StackFrame::TypeToMarker(type)));
  __ mov(r6, Operand(StackFrame::TypeToMarker(type)));
  __ Move(r4, ExternalReference::Create(IsolateAddressId::kCEntryFPAddress,
                                        masm->isolate()));
  __ ldr(r5, MemOperand(r4));

  __ stm(db_w, sp, r5.bit() | r6.bit() | r7.bit() | fp.bit() | lr.bit());
  pushed_stack_space += 5 * kPointerSize /* r5, r6, r7, fp, lr */;

  // Clear c_entry_fp, now we've pushed its previous value to the stack.
  // If the c_entry_fp is not already zero and we don't clear it, the
  // SafeStackFrameIterator will assume we are executing C++ and miss the JS
  // frames on top.
  __ mov(r5, Operand::Zero());
  __ str(r5, MemOperand(r4));

  Register scratch = r6;

  // Set up frame pointer for the frame to be pushed.
  __ add(fp, sp, Operand(-EntryFrameConstants::kCallerFPOffset));

  // If this is the outermost JS call, set js_entry_sp value.
  Label non_outermost_js;
  ExternalReference js_entry_sp = ExternalReference::Create(
      IsolateAddressId::kJSEntrySPAddress, masm->isolate());
  __ Move(r5, js_entry_sp);
  __ ldr(scratch, MemOperand(r5));
  __ cmp(scratch, Operand::Zero());
  __ b(ne, &non_outermost_js);
  __ str(fp, MemOperand(r5));
  __ mov(scratch, Operand(StackFrame::OUTERMOST_JSENTRY_FRAME));
  Label cont;
  __ b(&cont);
  __ bind(&non_outermost_js);
  __ mov(scratch, Operand(StackFrame::INNER_JSENTRY_FRAME));
  __ bind(&cont);
  __ push(scratch);

  // Jump to a faked try block that does the invoke, with a faked catch
  // block that sets the pending exception.
  __ jmp(&invoke);

  // Block literal pool emission whilst taking the position of the handler
  // entry. This avoids making the assumption that literal pools are always
  // emitted after an instruction is emitted, rather than before.
  {
    Assembler::BlockConstPoolScope block_const_pool(masm);
    __ bind(&handler_entry);

    // Store the current pc as the handler offset. It's used later to create the
    // handler table.
    masm->isolate()->builtins()->SetJSEntryHandlerOffset(handler_entry.pos());

    // Caught exception: Store result (exception) in the pending exception
    // field in the JSEnv and return a failure sentinel.  Coming in here the
    // fp will be invalid because the PushStackHandler below sets it to 0 to
    // signal the existence of the JSEntry frame.
    __ Move(scratch,
            ExternalReference::Create(
                IsolateAddressId::kPendingExceptionAddress, masm->isolate()));
  }
  __ str(r0, MemOperand(scratch));
  __ LoadRoot(r0, RootIndex::kException);
  __ b(&exit);

  // Invoke: Link this frame into the handler chain.
  __ bind(&invoke);
  // Must preserve r0-r4, r5-r6 are available.
  __ PushStackHandler();
  // If an exception not caught by another handler occurs, this handler
  // returns control to the code after the bl(&invoke) above, which
  // restores all kCalleeSaved registers (including cp and fp) to their
  // saved values before returning a failure to C.
  //
  // Invoke the function by calling through JS entry trampoline builtin and
  // pop the faked function when we return.
  Handle<Code> trampoline_code =
      masm->isolate()->builtins()->builtin_handle(entry_trampoline);
  DCHECK_EQ(kPushedStackSpace, pushed_stack_space);
  __ Call(trampoline_code, RelocInfo::CODE_TARGET);

  // Unlink this frame from the handler chain.
  __ PopStackHandler();

  __ bind(&exit);  // r0 holds result
  // Check if the current stack frame is marked as the outermost JS frame.
  Label non_outermost_js_2;
  __ pop(r5);
  __ cmp(r5, Operand(StackFrame::OUTERMOST_JSENTRY_FRAME));
  __ b(ne, &non_outermost_js_2);
  __ mov(r6, Operand::Zero());
  __ Move(r5, js_entry_sp);
  __ str(r6, MemOperand(r5));
  __ bind(&non_outermost_js_2);

  // Restore the top frame descriptors from the stack.
  __ pop(r3);
  __ Move(scratch, ExternalReference::Create(IsolateAddressId::kCEntryFPAddress,
                                             masm->isolate()));
  __ str(r3, MemOperand(scratch));

  // Reset the stack to the callee saved registers.
  __ add(sp, sp,
         Operand(-EntryFrameConstants::kCallerFPOffset -
                 kSystemPointerSize /* already popped one */));

  __ ldm(ia_w, sp, fp.bit() | lr.bit());

  // Restore callee-saved vfp registers.
  __ vldm(ia_w, sp, kFirstCalleeSavedDoubleReg, kLastCalleeSavedDoubleReg);

  __ ldm(ia_w, sp, kCalleeSavedWithoutFp);

  __ mov(pc, lr);

  // Emit constant pool.
  __ CheckConstPool(true, false);
}

}  // namespace

void Builtins::Generate_JSEntry(MacroAssembler* masm) {
  Generate_JSEntryVariant(masm, StackFrame::ENTRY,
                          Builtins::kJSEntryTrampoline);
}

void Builtins::Generate_JSConstructEntry(MacroAssembler* masm) {
  Generate_JSEntryVariant(masm, StackFrame::CONSTRUCT_ENTRY,
                          Builtins::kJSConstructEntryTrampoline);
}

void Builtins::Generate_JSRunMicrotasksEntry(MacroAssembler* masm) {
  Generate_JSEntryVariant(masm, StackFrame::ENTRY,
                          Builtins::kRunMicrotasksTrampoline);
}

static void Generate_JSEntryTrampolineHelper(MacroAssembler* masm,
                                             bool is_construct) {
  // Called from Generate_JS_Entry
  // r0:                                                root_register_value
  // r1:                                                new.target
  // r2:                                                function
  // r3:                                                receiver
  // [fp + kPushedStackSpace + 0 * kSystemPointerSize]: argc
  // [fp + kPushedStackSpace + 1 * kSystemPointerSize]: argv
  // r5-r6, r8 and cp may be clobbered

  __ ldr(r0,
         MemOperand(fp, kPushedStackSpace + EntryFrameConstants::kArgcOffset));
  __ ldr(r4,
         MemOperand(fp, kPushedStackSpace + EntryFrameConstants::kArgvOffset));

  // r1: new.target
  // r2: function
  // r3: receiver
  // r0: argc
  // r4: argv

  // Enter an internal frame.
  {
    FrameScope scope(masm, StackFrame::INTERNAL);

    // Setup the context (we need to use the caller context from the isolate).
    ExternalReference context_address = ExternalReference::Create(
        IsolateAddressId::kContextAddress, masm->isolate());
    __ Move(cp, context_address);
    __ ldr(cp, MemOperand(cp));

    // Push the function.
    __ Push(r2);

    // Check if we have enough stack space to push all arguments + receiver.
    // Clobbers r5.
    Label enough_stack_space, stack_overflow;
    __ add(r6, r0, Operand(1));  // Add one for receiver.
    __ StackOverflowCheck(r6, r5, &stack_overflow);
    __ b(&enough_stack_space);
    __ bind(&stack_overflow);
    __ CallRuntime(Runtime::kThrowStackOverflow);
    // Unreachable code.
    __ bkpt(0);

    __ bind(&enough_stack_space);

    // Copy arguments to the stack in a loop.
    // r1: new.target
    // r2: function
    // r3: receiver
    // r0: argc
    // r4: argv, i.e. points to first arg
    Label loop, entry;
    __ add(r6, r4, Operand(r0, LSL, kSystemPointerSizeLog2));
    // r6 points past last arg.
    __ b(&entry);
    __ bind(&loop);
    __ ldr(r5, MemOperand(r6, -kSystemPointerSize,
                          PreIndex));  // read next parameter
    __ ldr(r5, MemOperand(r5));        // dereference handle
    __ push(r5);                       // push parameter
    __ bind(&entry);
    __ cmp(r4, r6);
    __ b(ne, &loop);

    // Push the receiver.
    __ Push(r3);

    // Setup new.target and function.
    __ mov(r3, r1);
    __ mov(r1, r2);
    // r0: argc
    // r1: function
    // r3: new.target

    // Initialize all JavaScript callee-saved registers, since they will be seen
    // by the garbage collector as part of handlers.
    __ LoadRoot(r4, RootIndex::kUndefinedValue);
    __ mov(r2, r4);
    __ mov(r5, r4);
    __ mov(r6, r4);
    __ mov(r8, r4);
    if (kR9Available == 1) {
      __ mov(r9, r4);
    }

    // Invoke the code.
    Handle<Code> builtin = is_construct
                               ? BUILTIN_CODE(masm->isolate(), Construct)
                               : masm->isolate()->builtins()->Call();
    __ Call(builtin, RelocInfo::CODE_TARGET);

    // Exit the JS frame and remove the parameters (except function), and
    // return.
    // Respect ABI stack constraint.
  }
  __ Jump(lr);

  // r0: result
}

void Builtins::Generate_JSEntryTrampoline(MacroAssembler* masm) {
  Generate_JSEntryTrampolineHelper(masm, false);
}

void Builtins::Generate_JSConstructEntryTrampoline(MacroAssembler* masm) {
  Generate_JSEntryTrampolineHelper(masm, true);
}

void Builtins::Generate_RunMicrotasksTrampoline(MacroAssembler* masm) {
  // This expects two C++ function parameters passed by Invoke() in
  // execution.cc.
  //   r0: root_register_value
  //   r1: microtask_queue

  __ mov(RunMicrotasksDescriptor::MicrotaskQueueRegister(), r1);
  __ Jump(BUILTIN_CODE(masm->isolate(), RunMicrotasks), RelocInfo::CODE_TARGET);
}

static void ReplaceClosureCodeWithOptimizedCode(MacroAssembler* masm,
                                                Register optimized_code,
                                                Register closure) {
  // Store code entry in the closure.
  __ str(optimized_code, FieldMemOperand(closure, JSFunction::kCodeOffset));
  __ RecordWriteField(closure, JSFunction::kCodeOffset, optimized_code,
                      kLRHasNotBeenSaved, SaveFPRegsMode::kIgnore,
                      RememberedSetAction::kOmit, SmiCheck::kOmit);
}

static void LeaveInterpreterFrame(MacroAssembler* masm, Register scratch1,
                                  Register scratch2) {
  Register params_size = scratch1;
  // Get the size of the formal parameters + receiver (in bytes).
  __ ldr(params_size,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ ldr(params_size,
         FieldMemOperand(params_size, BytecodeArray::kParameterSizeOffset));

  Register actual_params_size = scratch2;
  // Compute the size of the actual parameters + receiver (in bytes).
  __ ldr(actual_params_size,
         MemOperand(fp, StandardFrameConstants::kArgCOffset));
  __ lsl(actual_params_size, actual_params_size, Operand(kPointerSizeLog2));
  __ add(actual_params_size, actual_params_size, Operand(kSystemPointerSize));

  // If actual is bigger than formal, then we should use it to free up the stack
  // arguments.
  __ cmp(params_size, actual_params_size);
  __ mov(params_size, actual_params_size, LeaveCC, lt);

  // Leave the frame (also dropping the register file).
  __ LeaveFrame(StackFrame::INTERPRETED);

  // Drop receiver + arguments.
  __ add(sp, sp, params_size, LeaveCC);
}

// Tail-call |function_id| if |actual_marker| == |expected_marker|
static void TailCallRuntimeIfMarkerEquals(MacroAssembler* masm,
                                          Register actual_marker,
                                          OptimizationMarker expected_marker,
                                          Runtime::FunctionId function_id) {
  Label no_match;
  __ cmp_raw_immediate(actual_marker, expected_marker);
  __ b(ne, &no_match);
  GenerateTailCallToReturnedCode(masm, function_id);
  __ bind(&no_match);
}

static void TailCallOptimizedCodeSlot(MacroAssembler* masm,
                                      Register optimized_code_entry,
                                      Register scratch) {
  // ----------- S t a t e -------------
  //  -- r0 : actual argument count
  //  -- r3 : new target (preserved for callee if needed, and caller)
  //  -- r1 : target function (preserved for callee if needed, and caller)
  // -----------------------------------
  DCHECK(!AreAliased(r1, r3, optimized_code_entry, scratch));

  Register closure = r1;
  Label heal_optimized_code_slot;

  // If the optimized code is cleared, go to runtime to update the optimization
  // marker field.
  __ LoadWeakValue(optimized_code_entry, optimized_code_entry,
                   &heal_optimized_code_slot);

  // Check if the optimized code is marked for deopt. If it is, call the
  // runtime to clear it.
  __ ldr(scratch,
         FieldMemOperand(optimized_code_entry, Code::kCodeDataContainerOffset));
  __ ldr(scratch,
         FieldMemOperand(scratch, CodeDataContainer::kKindSpecificFlagsOffset));
  __ tst(scratch, Operand(1 << Code::kMarkedForDeoptimizationBit));
  __ b(ne, &heal_optimized_code_slot);

  // Optimized code is good, get it into the closure and link the closure
  // into the optimized functions list, then tail call the optimized code.
  ReplaceClosureCodeWithOptimizedCode(masm, optimized_code_entry, closure);
  static_assert(kJavaScriptCallCodeStartRegister == r2, "ABI mismatch");
  __ LoadCodeObjectEntry(r2, optimized_code_entry);
  __ Jump(r2);

  // Optimized code slot contains deoptimized code or code is cleared and
  // optimized code marker isn't updated. Evict the code, update the marker
  // and re-enter the closure's code.
  __ bind(&heal_optimized_code_slot);
  GenerateTailCallToReturnedCode(masm, Runtime::kHealOptimizedCodeSlot);
}

static void MaybeOptimizeCode(MacroAssembler* masm, Register feedback_vector,
                              Register optimization_marker) {
  // ----------- S t a t e -------------
  //  -- r0 : actual argument count
  //  -- r3 : new target (preserved for callee if needed, and caller)
  //  -- r1 : target function (preserved for callee if needed, and caller)
  //  -- feedback vector (preserved for caller if needed)
  //  -- optimization_marker : a int32 containing a non-zero optimization
  //  marker.
  // -----------------------------------
  DCHECK(!AreAliased(feedback_vector, r1, r3, optimization_marker));

  // TODO(v8:8394): The logging of first execution will break if
  // feedback vectors are not allocated. We need to find a different way of
  // logging these events if required.
  TailCallRuntimeIfMarkerEquals(masm, optimization_marker,
                                OptimizationMarker::kLogFirstExecution,
                                Runtime::kFunctionFirstExecution);
  TailCallRuntimeIfMarkerEquals(masm, optimization_marker,
                                OptimizationMarker::kCompileOptimized,
                                Runtime::kCompileOptimized_NotConcurrent);
  TailCallRuntimeIfMarkerEquals(masm, optimization_marker,
                                OptimizationMarker::kCompileOptimizedConcurrent,
                                Runtime::kCompileOptimized_Concurrent);

  // Marker should be one of LogFirstExecution / CompileOptimized /
  // CompileOptimizedConcurrent. InOptimizationQueue and None shouldn't reach
  // here.
  if (FLAG_debug_code) {
    __ stop();
  }
}

// Advance the current bytecode offset. This simulates what all bytecode
// handlers do upon completion of the underlying operation. Will bail out to a
// label if the bytecode (without prefix) is a return bytecode. Will not advance
// the bytecode offset if the current bytecode is a JumpLoop, instead just
// re-executing the JumpLoop to jump to the correct bytecode.
static void AdvanceBytecodeOffsetOrReturn(MacroAssembler* masm,
                                          Register bytecode_array,
                                          Register bytecode_offset,
                                          Register bytecode, Register scratch1,
                                          Register scratch2, Label* if_return) {
  Register bytecode_size_table = scratch1;

  // The bytecode offset value will be increased by one in wide and extra wide
  // cases. In the case of having a wide or extra wide JumpLoop bytecode, we
  // will restore the original bytecode. In order to simplify the code, we have
  // a backup of it.
  Register original_bytecode_offset = scratch2;
  DCHECK(!AreAliased(bytecode_array, bytecode_offset, bytecode_size_table,
                     bytecode, original_bytecode_offset));

  __ Move(bytecode_size_table,
          ExternalReference::bytecode_size_table_address());
  __ Move(original_bytecode_offset, bytecode_offset);

  // Check if the bytecode is a Wide or ExtraWide prefix bytecode.
  Label process_bytecode;
  STATIC_ASSERT(0 == static_cast<int>(interpreter::Bytecode::kWide));
  STATIC_ASSERT(1 == static_cast<int>(interpreter::Bytecode::kExtraWide));
  STATIC_ASSERT(2 == static_cast<int>(interpreter::Bytecode::kDebugBreakWide));
  STATIC_ASSERT(3 ==
                static_cast<int>(interpreter::Bytecode::kDebugBreakExtraWide));
  __ cmp(bytecode, Operand(0x3));
  __ b(hi, &process_bytecode);
  __ tst(bytecode, Operand(0x1));
  // Load the next bytecode.
  __ add(bytecode_offset, bytecode_offset, Operand(1));
  __ ldrb(bytecode, MemOperand(bytecode_array, bytecode_offset));

  // Update table to the wide scaled table.
  __ add(bytecode_size_table, bytecode_size_table,
         Operand(kByteSize * interpreter::Bytecodes::kBytecodeCount));
  // Conditionally update table to the extra wide scaled table. We are taking
  // advantage of the fact that the extra wide follows the wide one.
  __ add(bytecode_size_table, bytecode_size_table,
         Operand(kByteSize * interpreter::Bytecodes::kBytecodeCount), LeaveCC,
         ne);

  __ bind(&process_bytecode);

  // Bailout to the return label if this is a return bytecode.

  // Create cmp, cmpne, ..., cmpne to check for a return bytecode.
  Condition flag = al;
#define JUMP_IF_EQUAL(NAME)                                                   \
  __ cmp(bytecode, Operand(static_cast<int>(interpreter::Bytecode::k##NAME)), \
         flag);                                                               \
  flag = ne;
  RETURN_BYTECODE_LIST(JUMP_IF_EQUAL)
#undef JUMP_IF_EQUAL

  __ b(if_return, eq);

  // If this is a JumpLoop, re-execute it to perform the jump to the beginning
  // of the loop.
  Label end, not_jump_loop;
  __ cmp(bytecode, Operand(static_cast<int>(interpreter::Bytecode::kJumpLoop)));
  __ b(ne, &not_jump_loop);
  // We need to restore the original bytecode_offset since we might have
  // increased it to skip the wide / extra-wide prefix bytecode.
  __ Move(bytecode_offset, original_bytecode_offset);
  __ b(&end);

  __ bind(&not_jump_loop);
  // Otherwise, load the size of the current bytecode and advance the offset.
  __ ldrb(scratch1, MemOperand(bytecode_size_table, bytecode));
  __ add(bytecode_offset, bytecode_offset, scratch1);

  __ bind(&end);
}

// Read off the optimization state in the feedback vector and check if there
// is optimized code or a optimization marker that needs to be processed.
static void LoadOptimizationStateAndJumpIfNeedsProcessing(
    MacroAssembler* masm, Register optimization_state, Register feedback_vector,
    Label* has_optimized_code_or_marker) {
  __ RecordComment("[ Check optimization state");

  __ ldr(optimization_state,
         FieldMemOperand(feedback_vector, FeedbackVector::kFlagsOffset));
  __ tst(
      optimization_state,
      Operand(FeedbackVector::kHasOptimizedCodeOrCompileOptimizedMarkerMask));
  __ b(ne, has_optimized_code_or_marker);

  __ RecordComment("]");
}

static void MaybeOptimizeCodeOrTailCallOptimizedCodeSlot(
    MacroAssembler* masm, Register optimization_state,
    Register feedback_vector) {
  Label maybe_has_optimized_code;
  // Check if optimized code is available
  __ tst(
      optimization_state,
      Operand(FeedbackVector::kHasCompileOptimizedOrLogFirstExecutionMarker));
  __ b(eq, &maybe_has_optimized_code);

  Register optimization_marker = optimization_state;
  __ DecodeField<FeedbackVector::OptimizationMarkerBits>(optimization_marker);
  MaybeOptimizeCode(masm, feedback_vector, optimization_marker);

  __ bind(&maybe_has_optimized_code);
  Register optimized_code_entry = optimization_state;
  __ ldr(optimization_marker,
         FieldMemOperand(feedback_vector,
                         FeedbackVector::kMaybeOptimizedCodeOffset));
  TailCallOptimizedCodeSlot(masm, optimized_code_entry, r6);
}

// static
void Builtins::Generate_BaselineOutOfLinePrologue(MacroAssembler* masm) {
  UseScratchRegisterScope temps(masm);
  // Need a few extra registers
  temps.Include(r8, r9);

  auto descriptor = Builtins::CallInterfaceDescriptorFor(
      Builtins::kBaselineOutOfLinePrologue);
  Register closure = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kClosure);
  // Load the feedback vector from the closure.
  Register feedback_vector = temps.Acquire();
  __ ldr(feedback_vector,
         FieldMemOperand(closure, JSFunction::kFeedbackCellOffset));
  __ ldr(feedback_vector, FieldMemOperand(feedback_vector, Cell::kValueOffset));
  if (FLAG_debug_code) {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ CompareObjectType(feedback_vector, scratch, scratch,
                         FEEDBACK_VECTOR_TYPE);
    __ Assert(eq, AbortReason::kExpectedFeedbackVector);
  }

  // Check for an optimization marker.
  Label has_optimized_code_or_marker;
  Register optimization_state = no_reg;
  {
    UseScratchRegisterScope temps(masm);
    // optimization_state will be used only in |has_optimized_code_or_marker|
    // and outside it can be reused.
    optimization_state = temps.Acquire();
    LoadOptimizationStateAndJumpIfNeedsProcessing(
        masm, optimization_state, feedback_vector,
        &has_optimized_code_or_marker);
  }

  // Increment invocation count for the function.
  {
    UseScratchRegisterScope temps(masm);
    Register invocation_count = temps.Acquire();
    __ ldr(invocation_count,
           FieldMemOperand(feedback_vector,
                           FeedbackVector::kInvocationCountOffset));
    __ add(invocation_count, invocation_count, Operand(1));
    __ str(invocation_count,
           FieldMemOperand(feedback_vector,
                           FeedbackVector::kInvocationCountOffset));
  }

  __ RecordComment("[ Frame Setup");
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  // Normally the first thing we'd do here is Push(lr, fp), but we already
  // entered the frame in BaselineCompiler::Prologue, as we had to use the
  // value lr before the call to this BaselineOutOfLinePrologue builtin.

  Register callee_context = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kCalleeContext);
  Register callee_js_function = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kClosure);
  __ Push(callee_context, callee_js_function);
  DCHECK_EQ(callee_js_function, kJavaScriptCallTargetRegister);
  DCHECK_EQ(callee_js_function, kJSFunctionRegister);

  Register argc = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kJavaScriptCallArgCount);
  // We'll use the bytecode for both code age/OSR resetting, and pushing onto
  // the frame, so load it into a register.
  Register bytecodeArray = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kInterpreterBytecodeArray);

  // Reset code age and the OSR arming. The OSR field and BytecodeAgeOffset
  // are 8-bit fields next to each other, so we could just optimize by writing
  // a 16-bit. These static asserts guard our assumption is valid.
  STATIC_ASSERT(BytecodeArray::kBytecodeAgeOffset ==
                BytecodeArray::kOsrNestingLevelOffset + kCharSize);
  STATIC_ASSERT(BytecodeArray::kNoAgeBytecodeAge == 0);
  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ mov(scratch, Operand(0));
    __ strh(scratch, FieldMemOperand(bytecodeArray,
                                     BytecodeArray::kOsrNestingLevelOffset));
  }

  __ Push(argc, bytecodeArray);

  // Baseline code frames store the feedback vector where interpreter would
  // store the bytecode offset.
  if (FLAG_debug_code) {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ CompareObjectType(feedback_vector, scratch, scratch,
                         FEEDBACK_VECTOR_TYPE);
    __ Assert(eq, AbortReason::kExpectedFeedbackVector);
  }
  __ Push(feedback_vector);
  __ RecordComment("]");

  __ RecordComment("[ Stack/interrupt check");
  Label call_stack_guard;
  Register frame_size = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kStackFrameSize);
  {
    // Stack check. This folds the checks for both the interrupt stack limit
    // check and the real stack limit into one by just checking for the
    // interrupt limit. The interrupt limit is either equal to the real stack
    // limit or tighter. By ensuring we have space until that limit after
    // building the frame we can quickly precheck both at once.
    UseScratchRegisterScope temps(masm);

    Register sp_minus_frame_size = temps.Acquire();
    __ sub(sp_minus_frame_size, sp, frame_size);
    Register interrupt_limit = temps.Acquire();
    __ LoadStackLimit(interrupt_limit, StackLimitKind::kInterruptStackLimit);
    __ cmp(sp_minus_frame_size, interrupt_limit);
    __ b(&call_stack_guard, lo);
    __ RecordComment("]");
  }

  // Do "fast" return to the caller pc in lr.
  __ Ret();

  __ bind(&has_optimized_code_or_marker);
  {
    UseScratchRegisterScope temps(masm);
    // Ensure the optimization_state is not allocated again.
    temps.Exclude(optimization_state);

    __ RecordComment("[ Optimized marker check");
    // Drop the frame created by the baseline call.
    __ ldm(ia_w, sp, fp.bit() | lr.bit());
    MaybeOptimizeCodeOrTailCallOptimizedCodeSlot(masm, optimization_state,
                                                 feedback_vector);
    __ Trap();
    __ RecordComment("]");
  }

  __ bind(&call_stack_guard);
  {
    FrameScope frame_scope(masm, StackFrame::INTERNAL);
    __ RecordComment("[ Stack/interrupt call");
    // Save incoming new target or generator
    __ Push(kJavaScriptCallNewTargetRegister);
    __ SmiTag(frame_size);
    __ Push(frame_size);
    __ CallRuntime(Runtime::kStackGuardWithGap);
    __ Pop(kJavaScriptCallNewTargetRegister);
    __ RecordComment("]");
  }

  __ Ret();
}

// Generate code for entering a JS function with the interpreter.
// On entry to the function the receiver and arguments have been pushed on the
// stack left to right.
//
// The live registers are:
//   o r0: actual argument count (not including the receiver)
//   o r1: the JS function object being called.
//   o r3: the incoming new target or generator object
//   o cp: our context
//   o fp: the caller's frame pointer
//   o sp: stack pointer
//   o lr: return address
//
// The function builds an interpreter frame.  See InterpreterFrameConstants in
// frames.h for its layout.
void Builtins::Generate_InterpreterEntryTrampoline(MacroAssembler* masm) {
  Register closure = r1;
  Register feedback_vector = r2;

  // Get the bytecode array from the function object and load it into
  // kInterpreterBytecodeArrayRegister.
  __ ldr(r4, FieldMemOperand(closure, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(kInterpreterBytecodeArrayRegister,
         FieldMemOperand(r4, SharedFunctionInfo::kFunctionDataOffset));

  Label is_baseline;
  GetSharedFunctionInfoBytecodeOrBaseline(
      masm, kInterpreterBytecodeArrayRegister, r8, &is_baseline);

  // The bytecode array could have been flushed from the shared function info,
  // if so, call into CompileLazy.
  Label compile_lazy;
  __ CompareObjectType(kInterpreterBytecodeArrayRegister, r4, no_reg,
                       BYTECODE_ARRAY_TYPE);
  __ b(ne, &compile_lazy);

  // Load the feedback vector from the closure.
  __ ldr(feedback_vector,
         FieldMemOperand(closure, JSFunction::kFeedbackCellOffset));
  __ ldr(feedback_vector, FieldMemOperand(feedback_vector, Cell::kValueOffset));

  Label push_stack_frame;
  // Check if feedback vector is valid. If valid, check for optimized code
  // and update invocation count. Otherwise, setup the stack frame.
  __ ldr(r4, FieldMemOperand(feedback_vector, HeapObject::kMapOffset));
  __ ldrh(r4, FieldMemOperand(r4, Map::kInstanceTypeOffset));
  __ cmp(r4, Operand(FEEDBACK_VECTOR_TYPE));
  __ b(ne, &push_stack_frame);

  Register optimization_state = r4;
  Label has_optimized_code_or_marker;
  LoadOptimizationStateAndJumpIfNeedsProcessing(
      masm, optimization_state, feedback_vector, &has_optimized_code_or_marker);

  Label not_optimized;
  __ bind(&not_optimized);

  // Increment invocation count for the function.
  __ ldr(r9, FieldMemOperand(feedback_vector,
                             FeedbackVector::kInvocationCountOffset));
  __ add(r9, r9, Operand(1));
  __ str(r9, FieldMemOperand(feedback_vector,
                             FeedbackVector::kInvocationCountOffset));

  // Open a frame scope to indicate that there is a frame on the stack.  The
  // MANUAL indicates that the scope shouldn't actually generate code to set up
  // the frame (that is done below).
  __ bind(&push_stack_frame);
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ PushStandardFrame(closure);

  // Reset code age and the OSR arming. The OSR field and BytecodeAgeOffset are
  // 8-bit fields next to each other, so we could just optimize by writing a
  // 16-bit. These static asserts guard our assumption is valid.
  STATIC_ASSERT(BytecodeArray::kBytecodeAgeOffset ==
                BytecodeArray::kOsrNestingLevelOffset + kCharSize);
  STATIC_ASSERT(BytecodeArray::kNoAgeBytecodeAge == 0);
  __ mov(r9, Operand(0));
  __ strh(r9, FieldMemOperand(kInterpreterBytecodeArrayRegister,
                              BytecodeArray::kOsrNestingLevelOffset));

  // Load the initial bytecode offset.
  __ mov(kInterpreterBytecodeOffsetRegister,
         Operand(BytecodeArray::kHeaderSize - kHeapObjectTag));

  // Push bytecode array and Smi tagged bytecode array offset.
  __ SmiTag(r4, kInterpreterBytecodeOffsetRegister);
  __ Push(kInterpreterBytecodeArrayRegister, r4);

  // Allocate the local and temporary register file on the stack.
  Label stack_overflow;
  {
    // Load frame size from the BytecodeArray object.
    __ ldr(r4, FieldMemOperand(kInterpreterBytecodeArrayRegister,
                               BytecodeArray::kFrameSizeOffset));

    // Do a stack check to ensure we don't go over the limit.
    __ sub(r9, sp, Operand(r4));
    __ LoadStackLimit(r2, StackLimitKind::kRealStackLimit);
    __ cmp(r9, Operand(r2));
    __ b(lo, &stack_overflow);

    // If ok, push undefined as the initial value for all register file entries.
    Label loop_header;
    Label loop_check;
    __ LoadRoot(kInterpreterAccumulatorRegister, RootIndex::kUndefinedValue);
    __ b(&loop_check, al);
    __ bind(&loop_header);
    // TODO(rmcilroy): Consider doing more than one push per loop iteration.
    __ push(kInterpreterAccumulatorRegister);
    // Continue loop if not done.
    __ bind(&loop_check);
    __ sub(r4, r4, Operand(kPointerSize), SetCC);
    __ b(&loop_header, ge);
  }

  // If the bytecode array has a valid incoming new target or generator object
  // register, initialize it with incoming value which was passed in r3.
  __ ldr(r9, FieldMemOperand(
                 kInterpreterBytecodeArrayRegister,
                 BytecodeArray::kIncomingNewTargetOrGeneratorRegisterOffset));
  __ cmp(r9, Operand::Zero());
  __ str(r3, MemOperand(fp, r9, LSL, kPointerSizeLog2), ne);

  // Perform interrupt stack check.
  // TODO(solanes): Merge with the real stack limit check above.
  Label stack_check_interrupt, after_stack_check_interrupt;
  __ LoadStackLimit(r4, StackLimitKind::kInterruptStackLimit);
  __ cmp(sp, r4);
  __ b(lo, &stack_check_interrupt);
  __ bind(&after_stack_check_interrupt);

  // The accumulator is already loaded with undefined.

  // Load the dispatch table into a register and dispatch to the bytecode
  // handler at the current bytecode offset.
  Label do_dispatch;
  __ bind(&do_dispatch);
  __ Move(
      kInterpreterDispatchTableRegister,
      ExternalReference::interpreter_dispatch_table_address(masm->isolate()));
  __ ldrb(r4, MemOperand(kInterpreterBytecodeArrayRegister,
                         kInterpreterBytecodeOffsetRegister));
  __ ldr(
      kJavaScriptCallCodeStartRegister,
      MemOperand(kInterpreterDispatchTableRegister, r4, LSL, kPointerSizeLog2));
  __ Call(kJavaScriptCallCodeStartRegister);
  masm->isolate()->heap()->SetInterpreterEntryReturnPCOffset(masm->pc_offset());

  // Any returns to the entry trampoline are either due to the return bytecode
  // or the interpreter tail calling a builtin and then a dispatch.

  // Get bytecode array and bytecode offset from the stack frame.
  __ ldr(kInterpreterBytecodeArrayRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ ldr(kInterpreterBytecodeOffsetRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  __ SmiUntag(kInterpreterBytecodeOffsetRegister);

  // Either return, or advance to the next bytecode and dispatch.
  Label do_return;
  __ ldrb(r1, MemOperand(kInterpreterBytecodeArrayRegister,
                         kInterpreterBytecodeOffsetRegister));
  AdvanceBytecodeOffsetOrReturn(masm, kInterpreterBytecodeArrayRegister,
                                kInterpreterBytecodeOffsetRegister, r1, r2, r3,
                                &do_return);
  __ jmp(&do_dispatch);

  __ bind(&do_return);
  // The return value is in r0.
  LeaveInterpreterFrame(masm, r2, r4);
  __ Jump(lr);

  __ bind(&stack_check_interrupt);
  // Modify the bytecode offset in the stack to be kFunctionEntryBytecodeOffset
  // for the call to the StackGuard.
  __ mov(kInterpreterBytecodeOffsetRegister,
         Operand(Smi::FromInt(BytecodeArray::kHeaderSize - kHeapObjectTag +
                              kFunctionEntryBytecodeOffset)));
  __ str(kInterpreterBytecodeOffsetRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  __ CallRuntime(Runtime::kStackGuard);

  // After the call, restore the bytecode array, bytecode offset and accumulator
  // registers again. Also, restore the bytecode offset in the stack to its
  // previous value.
  __ ldr(kInterpreterBytecodeArrayRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ mov(kInterpreterBytecodeOffsetRegister,
         Operand(BytecodeArray::kHeaderSize - kHeapObjectTag));
  __ LoadRoot(kInterpreterAccumulatorRegister, RootIndex::kUndefinedValue);

  __ SmiTag(r4, kInterpreterBytecodeOffsetRegister);
  __ str(r4, MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));

  __ jmp(&after_stack_check_interrupt);

  __ bind(&has_optimized_code_or_marker);
  MaybeOptimizeCodeOrTailCallOptimizedCodeSlot(masm, optimization_state,
                                               feedback_vector);

  __ bind(&is_baseline);
  {
    // Load the feedback vector from the closure.
    __ ldr(feedback_vector,
           FieldMemOperand(closure, JSFunction::kFeedbackCellOffset));
    __ ldr(feedback_vector,
           FieldMemOperand(feedback_vector, Cell::kValueOffset));

    Label install_baseline_code;
    // Check if feedback vector is valid. If not, call prepare for baseline to
    // allocate it.
    __ ldr(r8, FieldMemOperand(feedback_vector, HeapObject::kMapOffset));
    __ ldrh(r8, FieldMemOperand(r8, Map::kInstanceTypeOffset));
    __ cmp(r8, Operand(FEEDBACK_VECTOR_TYPE));
    __ b(ne, &install_baseline_code);

    // Check for an optimization marker.
    LoadOptimizationStateAndJumpIfNeedsProcessing(
        masm, optimization_state, feedback_vector,
        &has_optimized_code_or_marker);

    // Load the baseline code into the closure.
    __ ldr(r2, FieldMemOperand(kInterpreterBytecodeArrayRegister,
                               BaselineData::kBaselineCodeOffset));
    static_assert(kJavaScriptCallCodeStartRegister == r2, "ABI mismatch");
    ReplaceClosureCodeWithOptimizedCode(masm, r2, closure);
    __ JumpCodeObject(r2);

    __ bind(&install_baseline_code);
    GenerateTailCallToReturnedCode(masm, Runtime::kInstallBaselineCode);
  }

  __ bind(&compile_lazy);
  GenerateTailCallToReturnedCode(masm, Runtime::kCompileLazy);

  __ bind(&stack_overflow);
  __ CallRuntime(Runtime::kThrowStackOverflow);
  __ bkpt(0);  // Should not return.
}

static void Generate_InterpreterPushArgs(MacroAssembler* masm,
                                         Register num_args,
                                         Register start_address,
                                         Register scratch) {
  // Find the argument with lowest address.
  __ sub(scratch, num_args, Operand(1));
  __ mov(scratch, Operand(scratch, LSL, kSystemPointerSizeLog2));
  __ sub(start_address, start_address, scratch);
  // Push the arguments.
  __ PushArray(start_address, num_args, scratch,
               TurboAssembler::PushArrayOrder::kReverse);
}

// static
void Builtins::Generate_InterpreterPushArgsThenCallImpl(
    MacroAssembler* masm, ConvertReceiverMode receiver_mode,
    InterpreterPushArgsMode mode) {
  DCHECK(mode != InterpreterPushArgsMode::kArrayFunction);
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r2 : the address of the first argument to be pushed. Subsequent
  //          arguments should be consecutive above this, in the same order as
  //          they are to be pushed onto the stack.
  //  -- r1 : the target to call (can be any Object).
  // -----------------------------------
  Label stack_overflow;

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // The spread argument should not be pushed.
    __ sub(r0, r0, Operand(1));
  }

  __ add(r3, r0, Operand(1));  // Add one for receiver.

  __ StackOverflowCheck(r3, r4, &stack_overflow);

  if (receiver_mode == ConvertReceiverMode::kNullOrUndefined) {
    // Don't copy receiver. Argument count is correct.
    __ mov(r3, r0);
  }

  // Push the arguments. r2 and r4 will be modified.
  Generate_InterpreterPushArgs(masm, r3, r2, r4);

  // Push "undefined" as the receiver arg if we need to.
  if (receiver_mode == ConvertReceiverMode::kNullOrUndefined) {
    __ PushRoot(RootIndex::kUndefinedValue);
  }

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // Pass the spread in the register r2.
    // r2 already points to the penultimate argument, the spread
    // lies in the next interpreter register.
    __ sub(r2, r2, Operand(kSystemPointerSize));
    __ ldr(r2, MemOperand(r2));
  }

  // Call the target.
  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    __ Jump(BUILTIN_CODE(masm->isolate(), CallWithSpread),
            RelocInfo::CODE_TARGET);
  } else {
    __ Jump(masm->isolate()->builtins()->Call(ConvertReceiverMode::kAny),
            RelocInfo::CODE_TARGET);
  }

  __ bind(&stack_overflow);
  {
    __ TailCallRuntime(Runtime::kThrowStackOverflow);
    // Unreachable code.
    __ bkpt(0);
  }
}

// static
void Builtins::Generate_InterpreterPushArgsThenConstructImpl(
    MacroAssembler* masm, InterpreterPushArgsMode mode) {
  // ----------- S t a t e -------------
  // -- r0 : argument count (not including receiver)
  // -- r3 : new target
  // -- r1 : constructor to call
  // -- r2 : allocation site feedback if available, undefined otherwise.
  // -- r4 : address of the first argument
  // -----------------------------------
  Label stack_overflow;

  __ add(r5, r0, Operand(1));  // Add one for receiver.

  __ StackOverflowCheck(r5, r6, &stack_overflow);

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // The spread argument should not be pushed.
    __ sub(r0, r0, Operand(1));
  }

  // Push the arguments. r4 and r5 will be modified.
  Generate_InterpreterPushArgs(masm, r0, r4, r5);

  // Push a slot for the receiver to be constructed.
  __ mov(r5, Operand::Zero());
  __ push(r5);

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // Pass the spread in the register r2.
    // r4 already points to the penultimate argument, the spread
    // lies in the next interpreter register.
    __ sub(r4, r4, Operand(kSystemPointerSize));
    __ ldr(r2, MemOperand(r4));
  } else {
    __ AssertUndefinedOrAllocationSite(r2, r5);
  }

  if (mode == InterpreterPushArgsMode::kArrayFunction) {
    __ AssertFunction(r1);

    // Tail call to the array construct stub (still in the caller
    // context at this point).
    Handle<Code> code = BUILTIN_CODE(masm->isolate(), ArrayConstructorImpl);
    __ Jump(code, RelocInfo::CODE_TARGET);
  } else if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // Call the constructor with r0, r1, and r3 unmodified.
    __ Jump(BUILTIN_CODE(masm->isolate(), ConstructWithSpread),
            RelocInfo::CODE_TARGET);
  } else {
    DCHECK_EQ(InterpreterPushArgsMode::kOther, mode);
    // Call the constructor with r0, r1, and r3 unmodified.
    __ Jump(BUILTIN_CODE(masm->isolate(), Construct), RelocInfo::CODE_TARGET);
  }

  __ bind(&stack_overflow);
  {
    __ TailCallRuntime(Runtime::kThrowStackOverflow);
    // Unreachable code.
    __ bkpt(0);
  }
}

static void Generate_InterpreterEnterBytecode(MacroAssembler* masm) {
  // Set the return address to the correct point in the interpreter entry
  // trampoline.
  Label builtin_trampoline, trampoline_loaded;
  Smi interpreter_entry_return_pc_offset(
      masm->isolate()->heap()->interpreter_entry_return_pc_offset());
  DCHECK_NE(interpreter_entry_return_pc_offset, Smi::zero());

  // If the SFI function_data is an InterpreterData, the function will have a
  // custom copy of the interpreter entry trampoline for profiling. If so,
  // get the custom trampoline, otherwise grab the entry address of the global
  // trampoline.
  __ ldr(r2, MemOperand(fp, StandardFrameConstants::kFunctionOffset));
  __ ldr(r2, FieldMemOperand(r2, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(r2, FieldMemOperand(r2, SharedFunctionInfo::kFunctionDataOffset));
  __ CompareObjectType(r2, kInterpreterDispatchTableRegister,
                       kInterpreterDispatchTableRegister,
                       INTERPRETER_DATA_TYPE);
  __ b(ne, &builtin_trampoline);

  __ ldr(r2,
         FieldMemOperand(r2, InterpreterData::kInterpreterTrampolineOffset));
  __ add(r2, r2, Operand(Code::kHeaderSize - kHeapObjectTag));
  __ b(&trampoline_loaded);

  __ bind(&builtin_trampoline);
  __ Move(r2, ExternalReference::
                  address_of_interpreter_entry_trampoline_instruction_start(
                      masm->isolate()));
  __ ldr(r2, MemOperand(r2));

  __ bind(&trampoline_loaded);
  __ add(lr, r2, Operand(interpreter_entry_return_pc_offset.value()));

  // Initialize the dispatch table register.
  __ Move(
      kInterpreterDispatchTableRegister,
      ExternalReference::interpreter_dispatch_table_address(masm->isolate()));

  // Get the bytecode array pointer from the frame.
  __ ldr(kInterpreterBytecodeArrayRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeArrayFromFp));

  if (FLAG_debug_code) {
    // Check function data field is actually a BytecodeArray object.
    __ SmiTst(kInterpreterBytecodeArrayRegister);
    __ Assert(
        ne, AbortReason::kFunctionDataShouldBeBytecodeArrayOnInterpreterEntry);
    __ CompareObjectType(kInterpreterBytecodeArrayRegister, r1, no_reg,
                         BYTECODE_ARRAY_TYPE);
    __ Assert(
        eq, AbortReason::kFunctionDataShouldBeBytecodeArrayOnInterpreterEntry);
  }

  // Get the target bytecode offset from the frame.
  __ ldr(kInterpreterBytecodeOffsetRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  __ SmiUntag(kInterpreterBytecodeOffsetRegister);

  if (FLAG_debug_code) {
    Label okay;
    __ cmp(kInterpreterBytecodeOffsetRegister,
           Operand(BytecodeArray::kHeaderSize - kHeapObjectTag));
    __ b(ge, &okay);
    __ bkpt(0);
    __ bind(&okay);
  }

  // Dispatch to the target bytecode.
  UseScratchRegisterScope temps(masm);
  Register scratch = temps.Acquire();
  __ ldrb(scratch, MemOperand(kInterpreterBytecodeArrayRegister,
                              kInterpreterBytecodeOffsetRegister));
  __ ldr(kJavaScriptCallCodeStartRegister,
         MemOperand(kInterpreterDispatchTableRegister, scratch, LSL,
                    kPointerSizeLog2));
  __ Jump(kJavaScriptCallCodeStartRegister);
}

void Builtins::Generate_InterpreterEnterAtNextBytecode(MacroAssembler* masm) {
  // Get bytecode array and bytecode offset from the stack frame.
  __ ldr(kInterpreterBytecodeArrayRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ ldr(kInterpreterBytecodeOffsetRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  __ SmiUntag(kInterpreterBytecodeOffsetRegister);

  Label enter_bytecode, function_entry_bytecode;
  __ cmp(kInterpreterBytecodeOffsetRegister,
         Operand(BytecodeArray::kHeaderSize - kHeapObjectTag +
                 kFunctionEntryBytecodeOffset));
  __ b(eq, &function_entry_bytecode);

  // Load the current bytecode.
  __ ldrb(r1, MemOperand(kInterpreterBytecodeArrayRegister,
                         kInterpreterBytecodeOffsetRegister));

  // Advance to the next bytecode.
  Label if_return;
  AdvanceBytecodeOffsetOrReturn(masm, kInterpreterBytecodeArrayRegister,
                                kInterpreterBytecodeOffsetRegister, r1, r2, r3,
                                &if_return);

  __ bind(&enter_bytecode);
  // Convert new bytecode offset to a Smi and save in the stackframe.
  __ SmiTag(r2, kInterpreterBytecodeOffsetRegister);
  __ str(r2, MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));

  Generate_InterpreterEnterBytecode(masm);

  __ bind(&function_entry_bytecode);
  // If the code deoptimizes during the implicit function entry stack interrupt
  // check, it will have a bailout ID of kFunctionEntryBytecodeOffset, which is
  // not a valid bytecode offset. Detect this case and advance to the first
  // actual bytecode.
  __ mov(kInterpreterBytecodeOffsetRegister,
         Operand(BytecodeArray::kHeaderSize - kHeapObjectTag));
  __ b(&enter_bytecode);

  // We should never take the if_return path.
  __ bind(&if_return);
  __ Abort(AbortReason::kInvalidBytecodeAdvance);
}

void Builtins::Generate_InterpreterEnterAtBytecode(MacroAssembler* masm) {
  Generate_InterpreterEnterBytecode(masm);
}

namespace {
void Generate_ContinueToBuiltinHelper(MacroAssembler* masm,
                                      bool java_script_builtin,
                                      bool with_result) {
  const RegisterConfiguration* config(RegisterConfiguration::Default());
  int allocatable_register_count = config->num_allocatable_general_registers();
  UseScratchRegisterScope temps(masm);
  Register scratch = temps.Acquire();  // Temp register is not allocatable.
  if (with_result) {
    if (java_script_builtin) {
      __ mov(scratch, r0);
    } else {
      // Overwrite the hole inserted by the deoptimizer with the return value
      // from the LAZY deopt point.
      __ str(
          r0,
          MemOperand(
              sp, config->num_allocatable_general_registers() * kPointerSize +
                      BuiltinContinuationFrameConstants::kFixedFrameSize));
    }
  }
  for (int i = allocatable_register_count - 1; i >= 0; --i) {
    int code = config->GetAllocatableGeneralCode(i);
    __ Pop(Register::from_code(code));
    if (java_script_builtin && code == kJavaScriptCallArgCountRegister.code()) {
      __ SmiUntag(Register::from_code(code));
    }
  }
  if (java_script_builtin && with_result) {
    // Overwrite the hole inserted by the deoptimizer with the return value from
    // the LAZY deopt point. r0 contains the arguments count, the return value
    // from LAZY is always the last argument.
    __ add(r0, r0, Operand(BuiltinContinuationFrameConstants::kFixedSlotCount));
    __ str(scratch, MemOperand(sp, r0, LSL, kPointerSizeLog2));
    // Recover arguments count.
    __ sub(r0, r0, Operand(BuiltinContinuationFrameConstants::kFixedSlotCount));
  }
  __ ldr(fp, MemOperand(
                 sp, BuiltinContinuationFrameConstants::kFixedFrameSizeFromFp));
  // Load builtin index (stored as a Smi) and use it to get the builtin start
  // address from the builtins table.
  Register builtin = scratch;
  __ Pop(builtin);
  __ add(sp, sp,
         Operand(BuiltinContinuationFrameConstants::kFixedFrameSizeFromFp));
  __ Pop(lr);
  __ LoadEntryFromBuiltinIndex(builtin);
  __ bx(builtin);
}
}  // namespace

void Builtins::Generate_ContinueToCodeStubBuiltin(MacroAssembler* masm) {
  Generate_ContinueToBuiltinHelper(masm, false, false);
}

void Builtins::Generate_ContinueToCodeStubBuiltinWithResult(
    MacroAssembler* masm) {
  Generate_ContinueToBuiltinHelper(masm, false, true);
}

void Builtins::Generate_ContinueToJavaScriptBuiltin(MacroAssembler* masm) {
  Generate_ContinueToBuiltinHelper(masm, true, false);
}

void Builtins::Generate_ContinueToJavaScriptBuiltinWithResult(
    MacroAssembler* masm) {
  Generate_ContinueToBuiltinHelper(masm, true, true);
}

void Builtins::Generate_NotifyDeoptimized(MacroAssembler* masm) {
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kNotifyDeoptimized);
  }

  DCHECK_EQ(kInterpreterAccumulatorRegister.code(), r0.code());
  __ pop(r0);
  __ Ret();
}

void Builtins::Generate_TailCallOptimizedCodeSlot(MacroAssembler* masm) {
  UseScratchRegisterScope temps(masm);
  // Need a few extra registers
  temps.Include(r8, r9);
  Register optimized_code_entry = kJavaScriptCallCodeStartRegister;
  TailCallOptimizedCodeSlot(masm, optimized_code_entry, temps.Acquire());
}

namespace {

void Generate_OSREntry(MacroAssembler* masm, Register entry_address,
                       Operand offset = Operand::Zero()) {
  // Compute the target address = entry_address + offset
  if (offset.IsImmediate() && offset.immediate() == 0) {
    __ mov(lr, entry_address);
  } else {
    __ add(lr, entry_address, offset);
  }

  // "return" to the OSR entry point of the function.
  __ Ret();
}

void OnStackReplacement(MacroAssembler* masm, bool is_interpreter) {
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kCompileForOnStackReplacement);
  }

  // If the code object is null, just return to the caller.
  Label skip;
  __ cmp(r0, Operand(Smi::zero()));
  __ b(ne, &skip);
  __ Ret();

  __ bind(&skip);

  if (is_interpreter) {
    // Drop the handler frame that is be sitting on top of the actual
    // JavaScript frame. This is the case then OSR is triggered from bytecode.
    __ LeaveFrame(StackFrame::STUB);
  }

  // Load deoptimization data from the code object.
  // <deopt_data> = <code>[#deoptimization_data_offset]
  __ ldr(r1, FieldMemOperand(r0, Code::kDeoptimizationDataOffset));

  {
    ConstantPoolUnavailableScope constant_pool_unavailable(masm);
    __ add(r0, r0, Operand(Code::kHeaderSize - kHeapObjectTag));  // Code start

    // Load the OSR entrypoint offset from the deoptimization data.
    // <osr_offset> = <deopt_data>[#header_size + #osr_pc_offset]
    __ ldr(r1, FieldMemOperand(r1, FixedArray::OffsetOfElementAt(
                                       DeoptimizationData::kOsrPcOffsetIndex)));

    Generate_OSREntry(masm, r0, Operand::SmiUntag(r1));
  }
}
}  // namespace

void Builtins::Generate_InterpreterOnStackReplacement(MacroAssembler* masm) {
  return OnStackReplacement(masm, true);
}

void Builtins::Generate_BaselineOnStackReplacement(MacroAssembler* masm) {
  __ ldr(kContextRegister,
         MemOperand(fp, BaselineFrameConstants::kContextOffset));
  return OnStackReplacement(masm, false);
}

// static
void Builtins::Generate_FunctionPrototypeApply(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0    : argc
  //  -- sp[0] : receiver
  //  -- sp[4] : thisArg
  //  -- sp[8] : argArray
  // -----------------------------------

  // 1. Load receiver into r1, argArray into r2 (if present), remove all
  // arguments from the stack (including the receiver), and push thisArg (if
  // present) instead.
  {
    __ LoadRoot(r5, RootIndex::kUndefinedValue);
    __ mov(r2, r5);
    __ ldr(r1, MemOperand(sp, 0));  // receiver
    __ cmp(r0, Operand(1));
    __ ldr(r5, MemOperand(sp, kSystemPointerSize), ge);  // thisArg
    __ cmp(r0, Operand(2), ge);
    __ ldr(r2, MemOperand(sp, 2 * kSystemPointerSize), ge);  // argArray
    __ add(sp, sp, Operand(r0, LSL, kSystemPointerSizeLog2));
    __ str(r5, MemOperand(sp, 0));
  }

  // ----------- S t a t e -------------
  //  -- r2    : argArray
  //  -- r1    : receiver
  //  -- sp[0] : thisArg
  // -----------------------------------

  // 2. We don't need to check explicitly for callable receiver here,
  // since that's the first thing the Call/CallWithArrayLike builtins
  // will do.

  // 3. Tail call with no arguments if argArray is null or undefined.
  Label no_arguments;
  __ JumpIfRoot(r2, RootIndex::kNullValue, &no_arguments);
  __ JumpIfRoot(r2, RootIndex::kUndefinedValue, &no_arguments);

  // 4a. Apply the receiver to the given argArray.
  __ Jump(BUILTIN_CODE(masm->isolate(), CallWithArrayLike),
          RelocInfo::CODE_TARGET);

  // 4b. The argArray is either null or undefined, so we tail call without any
  // arguments to the receiver.
  __ bind(&no_arguments);
  {
    __ mov(r0, Operand(0));
    __ Jump(masm->isolate()->builtins()->Call(), RelocInfo::CODE_TARGET);
  }
}

// static
void Builtins::Generate_FunctionPrototypeCall(MacroAssembler* masm) {
  // 1. Get the callable to call (passed as receiver) from the stack.
  __ Pop(r1);

  // 2. Make sure we have at least one argument.
  // r0: actual number of arguments
  {
    Label done;
    __ cmp(r0, Operand::Zero());
    __ b(ne, &done);
    __ PushRoot(RootIndex::kUndefinedValue);
    __ add(r0, r0, Operand(1));
    __ bind(&done);
  }

  // 3. Adjust the actual number of arguments.
  __ sub(r0, r0, Operand(1));

  // 4. Call the callable.
  __ Jump(masm->isolate()->builtins()->Call(), RelocInfo::CODE_TARGET);
}

void Builtins::Generate_ReflectApply(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0     : argc
  //  -- sp[0]  : receiver
  //  -- sp[4]  : target         (if argc >= 1)
  //  -- sp[8]  : thisArgument   (if argc >= 2)
  //  -- sp[12] : argumentsList  (if argc == 3)
  // -----------------------------------

  // 1. Load target into r1 (if present), argumentsList into r2 (if present),
  // remove all arguments from the stack (including the receiver), and push
  // thisArgument (if present) instead.
  {
    __ LoadRoot(r1, RootIndex::kUndefinedValue);
    __ mov(r5, r1);
    __ mov(r2, r1);
    __ cmp(r0, Operand(1));
    __ ldr(r1, MemOperand(sp, kSystemPointerSize), ge);  // target
    __ cmp(r0, Operand(2), ge);
    __ ldr(r5, MemOperand(sp, 2 * kSystemPointerSize), ge);  // thisArgument
    __ cmp(r0, Operand(3), ge);
    __ ldr(r2, MemOperand(sp, 3 * kSystemPointerSize), ge);  // argumentsList
    __ add(sp, sp, Operand(r0, LSL, kSystemPointerSizeLog2));
    __ str(r5, MemOperand(sp, 0));
  }

  // ----------- S t a t e -------------
  //  -- r2    : argumentsList
  //  -- r1    : target
  //  -- sp[0] : thisArgument
  // -----------------------------------

  // 2. We don't need to check explicitly for callable target here,
  // since that's the first thing the Call/CallWithArrayLike builtins
  // will do.

  // 3. Apply the target to the given argumentsList.
  __ Jump(BUILTIN_CODE(masm->isolate(), CallWithArrayLike),
          RelocInfo::CODE_TARGET);
}

void Builtins::Generate_ReflectConstruct(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0     : argc
  //  -- sp[0]  : receiver
  //  -- sp[4]  : target
  //  -- sp[8]  : argumentsList
  //  -- sp[12] : new.target (optional)
  // -----------------------------------

  // 1. Load target into r1 (if present), argumentsList into r2 (if present),
  // new.target into r3 (if present, otherwise use target), remove all
  // arguments from the stack (including the receiver), and push thisArgument
  // (if present) instead.
  {
    __ LoadRoot(r1, RootIndex::kUndefinedValue);
    __ mov(r2, r1);
    __ mov(r4, r1);
    __ cmp(r0, Operand(1));
    __ ldr(r1, MemOperand(sp, kSystemPointerSize), ge);  // target
    __ mov(r3, r1);  // new.target defaults to target
    __ cmp(r0, Operand(2), ge);
    __ ldr(r2, MemOperand(sp, 2 * kSystemPointerSize), ge);  // argumentsList
    __ cmp(r0, Operand(3), ge);
    __ ldr(r3, MemOperand(sp, 3 * kSystemPointerSize), ge);  // new.target
    __ add(sp, sp, Operand(r0, LSL, kSystemPointerSizeLog2));
    __ str(r4, MemOperand(sp, 0));  // set undefined to the receiver
  }

  // ----------- S t a t e -------------
  //  -- r2    : argumentsList
  //  -- r3    : new.target
  //  -- r1    : target
  //  -- sp[0] : receiver (undefined)
  // -----------------------------------

  // 2. We don't need to check explicitly for constructor target here,
  // since that's the first thing the Construct/ConstructWithArrayLike
  // builtins will do.

  // 3. We don't need to check explicitly for constructor new.target here,
  // since that's the second thing the Construct/ConstructWithArrayLike
  // builtins will do.

  // 4. Construct the target with the given new.target and argumentsList.
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructWithArrayLike),
          RelocInfo::CODE_TARGET);
}

// static
// TODO(v8:11615): Observe Code::kMaxArguments in CallOrConstructVarargs
void Builtins::Generate_CallOrConstructVarargs(MacroAssembler* masm,
                                               Handle<Code> code) {
  // ----------- S t a t e -------------
  //  -- r1 : target
  //  -- r0 : number of parameters on the stack (not including the receiver)
  //  -- r2 : arguments list (a FixedArray)
  //  -- r4 : len (number of elements to push from args)
  //  -- r3 : new.target (for [[Construct]])
  // -----------------------------------
  Register scratch = r8;

  if (FLAG_debug_code) {
    // Allow r2 to be a FixedArray, or a FixedDoubleArray if r4 == 0.
    Label ok, fail;
    __ AssertNotSmi(r2);
    __ ldr(scratch, FieldMemOperand(r2, HeapObject::kMapOffset));
    __ ldrh(r6, FieldMemOperand(scratch, Map::kInstanceTypeOffset));
    __ cmp(r6, Operand(FIXED_ARRAY_TYPE));
    __ b(eq, &ok);
    __ cmp(r6, Operand(FIXED_DOUBLE_ARRAY_TYPE));
    __ b(ne, &fail);
    __ cmp(r4, Operand(0));
    __ b(eq, &ok);
    // Fall through.
    __ bind(&fail);
    __ Abort(AbortReason::kOperandIsNotAFixedArray);

    __ bind(&ok);
  }

  Label stack_overflow;
  __ StackOverflowCheck(r4, scratch, &stack_overflow);

  // Move the arguments already in the stack,
  // including the receiver and the return address.
  {
    Label copy, check;
    Register num = r5, src = r6, dest = r9;  // r7 and r8 are context and root.
    __ mov(src, sp);
    // Update stack pointer.
    __ lsl(scratch, r4, Operand(kSystemPointerSizeLog2));
    __ AllocateStackSpace(scratch);
    __ mov(dest, sp);
    __ mov(num, r0);
    __ b(&check);
    __ bind(&copy);
    __ ldr(scratch, MemOperand(src, kSystemPointerSize, PostIndex));
    __ str(scratch, MemOperand(dest, kSystemPointerSize, PostIndex));
    __ sub(num, num, Operand(1), SetCC);
    __ bind(&check);
    __ b(ge, &copy);
  }

  // Copy arguments onto the stack (thisArgument is already on the stack).
  {
    __ mov(r6, Operand(0));
    __ LoadRoot(r5, RootIndex::kTheHoleValue);
    Label done, loop;
    __ bind(&loop);
    __ cmp(r6, r4);
    __ b(eq, &done);
    __ add(scratch, r2, Operand(r6, LSL, kTaggedSizeLog2));
    __ ldr(scratch, FieldMemOperand(scratch, FixedArray::kHeaderSize));
    __ cmp(scratch, r5);
    // Turn the hole into undefined as we go.
    __ LoadRoot(scratch, RootIndex::kUndefinedValue, eq);
    __ str(scratch, MemOperand(r9, kSystemPointerSize, PostIndex));
    __ add(r6, r6, Operand(1));
    __ b(&loop);
    __ bind(&done);
    __ add(r0, r0, r6);
  }

  // Tail-call to the actual Call or Construct builtin.
  __ Jump(code, RelocInfo::CODE_TARGET);

  __ bind(&stack_overflow);
  __ TailCallRuntime(Runtime::kThrowStackOverflow);
}

// static
void Builtins::Generate_CallOrConstructForwardVarargs(MacroAssembler* masm,
                                                      CallOrConstructMode mode,
                                                      Handle<Code> code) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r3 : the new.target (for [[Construct]] calls)
  //  -- r1 : the target to call (can be any Object)
  //  -- r2 : start index (to support rest parameters)
  // -----------------------------------

  Register scratch = r6;

  // Check if new.target has a [[Construct]] internal method.
  if (mode == CallOrConstructMode::kConstruct) {
    Label new_target_constructor, new_target_not_constructor;
    __ JumpIfSmi(r3, &new_target_not_constructor);
    __ ldr(scratch, FieldMemOperand(r3, HeapObject::kMapOffset));
    __ ldrb(scratch, FieldMemOperand(scratch, Map::kBitFieldOffset));
    __ tst(scratch, Operand(Map::Bits1::IsConstructorBit::kMask));
    __ b(ne, &new_target_constructor);
    __ bind(&new_target_not_constructor);
    {
      FrameScope scope(masm, StackFrame::MANUAL);
      __ EnterFrame(StackFrame::INTERNAL);
      __ Push(r3);
      __ CallRuntime(Runtime::kThrowNotConstructor);
    }
    __ bind(&new_target_constructor);
  }

  Label stack_done, stack_overflow;
  __ ldr(r5, MemOperand(fp, StandardFrameConstants::kArgCOffset));
  __ sub(r5, r5, r2, SetCC);
  __ b(le, &stack_done);
  {
    // ----------- S t a t e -------------
    //  -- r0 : the number of arguments already in the stack (not including the
    //  receiver)
    //  -- r1 : the target to call (can be any Object)
    //  -- r2 : start index (to support rest parameters)
    //  -- r3 : the new.target (for [[Construct]] calls)
    //  -- fp : point to the caller stack frame
    //  -- r5 : number of arguments to copy, i.e. arguments count - start index
    // -----------------------------------

    // Check for stack overflow.
    __ StackOverflowCheck(r5, scratch, &stack_overflow);

    // Forward the arguments from the caller frame.
    // Point to the first argument to copy (skipping the receiver).
    __ add(r4, fp,
           Operand(CommonFrameConstants::kFixedFrameSizeAboveFp +
                   kSystemPointerSize));
    __ add(r4, r4, Operand(r2, LSL, kSystemPointerSizeLog2));

    // Move the arguments already in the stack,
    // including the receiver and the return address.
    {
      Label copy, check;
      Register num = r8, src = r9,
               dest = r2;  // r7 and r10 are context and root.
      __ mov(src, sp);
      // Update stack pointer.
      __ lsl(scratch, r5, Operand(kSystemPointerSizeLog2));
      __ AllocateStackSpace(scratch);
      __ mov(dest, sp);
      __ mov(num, r0);
      __ b(&check);
      __ bind(&copy);
      __ ldr(scratch, MemOperand(src, kSystemPointerSize, PostIndex));
      __ str(scratch, MemOperand(dest, kSystemPointerSize, PostIndex));
      __ sub(num, num, Operand(1), SetCC);
      __ bind(&check);
      __ b(ge, &copy);
    }
    // Copy arguments from the caller frame.
    // TODO(victorgomes): Consider using forward order as potentially more cache
    // friendly.
    {
      Label loop;
      __ add(r0, r0, r5);
      __ bind(&loop);
      {
        __ sub(r5, r5, Operand(1), SetCC);
        __ ldr(scratch, MemOperand(r4, r5, LSL, kSystemPointerSizeLog2));
        __ str(scratch, MemOperand(r2, r5, LSL, kSystemPointerSizeLog2));
        __ b(ne, &loop);
      }
    }
  }
  __ b(&stack_done);
  __ bind(&stack_overflow);
  __ TailCallRuntime(Runtime::kThrowStackOverflow);
  __ bind(&stack_done);

  // Tail-call to the {code} handler.
  __ Jump(code, RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_CallFunction(MacroAssembler* masm,
                                     ConvertReceiverMode mode) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the function to call (checked to be a JSFunction)
  // -----------------------------------
  __ AssertFunction(r1);

  // See ES6 section 9.2.1 [[Call]] ( thisArgument, argumentsList)
  // Check that the function is not a "classConstructor".
  Label class_constructor;
  __ ldr(r2, FieldMemOperand(r1, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(r3, FieldMemOperand(r2, SharedFunctionInfo::kFlagsOffset));
  __ tst(r3, Operand(SharedFunctionInfo::IsClassConstructorBit::kMask));
  __ b(ne, &class_constructor);

  // Enter the context of the function; ToObject has to run in the function
  // context, and we also need to take the global proxy from the function
  // context in case of conversion.
  __ ldr(cp, FieldMemOperand(r1, JSFunction::kContextOffset));
  // We need to convert the receiver for non-native sloppy mode functions.
  Label done_convert;
  __ ldr(r3, FieldMemOperand(r2, SharedFunctionInfo::kFlagsOffset));
  __ tst(r3, Operand(SharedFunctionInfo::IsNativeBit::kMask |
                     SharedFunctionInfo::IsStrictBit::kMask));
  __ b(ne, &done_convert);
  {
    // ----------- S t a t e -------------
    //  -- r0 : the number of arguments (not including the receiver)
    //  -- r1 : the function to call (checked to be a JSFunction)
    //  -- r2 : the shared function info.
    //  -- cp : the function context.
    // -----------------------------------

    if (mode == ConvertReceiverMode::kNullOrUndefined) {
      // Patch receiver to global proxy.
      __ LoadGlobalProxy(r3);
    } else {
      Label convert_to_object, convert_receiver;
      __ ldr(r3, __ ReceiverOperand(r0));
      __ JumpIfSmi(r3, &convert_to_object);
      STATIC_ASSERT(LAST_JS_RECEIVER_TYPE == LAST_TYPE);
      __ CompareObjectType(r3, r4, r4, FIRST_JS_RECEIVER_TYPE);
      __ b(hs, &done_convert);
      if (mode != ConvertReceiverMode::kNotNullOrUndefined) {
        Label convert_global_proxy;
        __ JumpIfRoot(r3, RootIndex::kUndefinedValue, &convert_global_proxy);
        __ JumpIfNotRoot(r3, RootIndex::kNullValue, &convert_to_object);
        __ bind(&convert_global_proxy);
        {
          // Patch receiver to global proxy.
          __ LoadGlobalProxy(r3);
        }
        __ b(&convert_receiver);
      }
      __ bind(&convert_to_object);
      {
        // Convert receiver using ToObject.
        // TODO(bmeurer): Inline the allocation here to avoid building the frame
        // in the fast case? (fall back to AllocateInNewSpace?)
        FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
        __ SmiTag(r0);
        __ Push(r0, r1);
        __ mov(r0, r3);
        __ Push(cp);
        __ Call(BUILTIN_CODE(masm->isolate(), ToObject),
                RelocInfo::CODE_TARGET);
        __ Pop(cp);
        __ mov(r3, r0);
        __ Pop(r0, r1);
        __ SmiUntag(r0);
      }
      __ ldr(r2, FieldMemOperand(r1, JSFunction::kSharedFunctionInfoOffset));
      __ bind(&convert_receiver);
    }
    __ str(r3, __ ReceiverOperand(r0));
  }
  __ bind(&done_convert);

  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the function to call (checked to be a JSFunction)
  //  -- r2 : the shared function info.
  //  -- cp : the function context.
  // -----------------------------------

  __ ldrh(r2,
          FieldMemOperand(r2, SharedFunctionInfo::kFormalParameterCountOffset));
  __ InvokeFunctionCode(r1, no_reg, r2, r0, InvokeType::kJump);

  // The function is a "classConstructor", need to raise an exception.
  __ bind(&class_constructor);
  {
    FrameScope frame(masm, StackFrame::INTERNAL);
    __ push(r1);
    __ CallRuntime(Runtime::kThrowConstructorNonCallableError);
  }
}

namespace {

void Generate_PushBoundArguments(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : target (checked to be a JSBoundFunction)
  //  -- r3 : new.target (only in case of [[Construct]])
  // -----------------------------------

  // Load [[BoundArguments]] into r2 and length of that into r4.
  Label no_bound_arguments;
  __ ldr(r2, FieldMemOperand(r1, JSBoundFunction::kBoundArgumentsOffset));
  __ ldr(r4, FieldMemOperand(r2, FixedArray::kLengthOffset));
  __ SmiUntag(r4);
  __ cmp(r4, Operand(0));
  __ b(eq, &no_bound_arguments);
  {
    // ----------- S t a t e -------------
    //  -- r0 : the number of arguments (not including the receiver)
    //  -- r1 : target (checked to be a JSBoundFunction)
    //  -- r2 : the [[BoundArguments]] (implemented as FixedArray)
    //  -- r3 : new.target (only in case of [[Construct]])
    //  -- r4 : the number of [[BoundArguments]]
    // -----------------------------------

    Register scratch = r6;

    {
      // Check the stack for overflow. We are not trying to catch interruptions
      // (i.e. debug break and preemption) here, so check the "real stack
      // limit".
      Label done;
      __ mov(scratch, Operand(r4, LSL, kSystemPointerSizeLog2));
      {
        UseScratchRegisterScope temps(masm);
        Register remaining_stack_size = temps.Acquire();
        DCHECK(!AreAliased(r0, r1, r2, r3, r4, scratch, remaining_stack_size));

        // Compute the space we have left. The stack might already be overflowed
        // here which will cause remaining_stack_size to become negative.
        __ LoadStackLimit(remaining_stack_size,
                          StackLimitKind::kRealStackLimit);
        __ sub(remaining_stack_size, sp, remaining_stack_size);

        // Check if the arguments will overflow the stack.
        __ cmp(remaining_stack_size, scratch);
      }
      __ b(gt, &done);
      {
        FrameScope scope(masm, StackFrame::MANUAL);
        __ EnterFrame(StackFrame::INTERNAL);
        __ CallRuntime(Runtime::kThrowStackOverflow);
      }
      __ bind(&done);
    }

    // Pop receiver.
    __ Pop(r5);

    // Push [[BoundArguments]].
    {
      Label loop;
      __ add(r0, r0, r4);  // Adjust effective number of arguments.
      __ add(r2, r2, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
      __ bind(&loop);
      __ sub(r4, r4, Operand(1), SetCC);
      __ ldr(scratch, MemOperand(r2, r4, LSL, kTaggedSizeLog2));
      __ Push(scratch);
      __ b(gt, &loop);
    }

    // Push receiver.
    __ Push(r5);
  }
  __ bind(&no_bound_arguments);
}

}  // namespace

// static
void Builtins::Generate_CallBoundFunctionImpl(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the function to call (checked to be a JSBoundFunction)
  // -----------------------------------
  __ AssertBoundFunction(r1);

  // Patch the receiver to [[BoundThis]].
  __ ldr(r3, FieldMemOperand(r1, JSBoundFunction::kBoundThisOffset));
  __ str(r3, __ ReceiverOperand(r0));

  // Push the [[BoundArguments]] onto the stack.
  Generate_PushBoundArguments(masm);

  // Call the [[BoundTargetFunction]] via the Call builtin.
  __ ldr(r1, FieldMemOperand(r1, JSBoundFunction::kBoundTargetFunctionOffset));
  __ Jump(BUILTIN_CODE(masm->isolate(), Call_ReceiverIsAny),
          RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_Call(MacroAssembler* masm, ConvertReceiverMode mode) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the target to call (can be any Object).
  // -----------------------------------

  Label non_callable, non_smi;
  __ JumpIfSmi(r1, &non_callable);
  __ bind(&non_smi);
  __ LoadMap(r4, r1);
  __ CompareInstanceTypeRange(r4, r5, FIRST_JS_FUNCTION_TYPE,
                              LAST_JS_FUNCTION_TYPE);
  __ Jump(masm->isolate()->builtins()->CallFunction(mode),
          RelocInfo::CODE_TARGET, ls);
  __ cmp(r5, Operand(JS_BOUND_FUNCTION_TYPE));
  __ Jump(BUILTIN_CODE(masm->isolate(), CallBoundFunction),
          RelocInfo::CODE_TARGET, eq);

  // Check if target has a [[Call]] internal method.
  __ ldrb(r4, FieldMemOperand(r4, Map::kBitFieldOffset));
  __ tst(r4, Operand(Map::Bits1::IsCallableBit::kMask));
  __ b(eq, &non_callable);

  // Check if target is a proxy and call CallProxy external builtin
  __ cmp(r5, Operand(JS_PROXY_TYPE));
  __ Jump(BUILTIN_CODE(masm->isolate(), CallProxy), RelocInfo::CODE_TARGET, eq);

  // 2. Call to something else, which might have a [[Call]] internal method (if
  // not we raise an exception).
  // Overwrite the original receiver the (original) target.
  __ str(r1, __ ReceiverOperand(r0));
  // Let the "call_as_function_delegate" take care of the rest.
  __ LoadNativeContextSlot(r1, Context::CALL_AS_FUNCTION_DELEGATE_INDEX);
  __ Jump(masm->isolate()->builtins()->CallFunction(
              ConvertReceiverMode::kNotNullOrUndefined),
          RelocInfo::CODE_TARGET);

  // 3. Call to something that is not callable.
  __ bind(&non_callable);
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    __ Push(r1);
    __ CallRuntime(Runtime::kThrowCalledNonCallable);
  }
}

// static
void Builtins::Generate_ConstructFunction(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the constructor to call (checked to be a JSFunction)
  //  -- r3 : the new target (checked to be a constructor)
  // -----------------------------------
  __ AssertConstructor(r1);
  __ AssertFunction(r1);

  // Calling convention for function specific ConstructStubs require
  // r2 to contain either an AllocationSite or undefined.
  __ LoadRoot(r2, RootIndex::kUndefinedValue);

  Label call_generic_stub;

  // Jump to JSBuiltinsConstructStub or JSConstructStubGeneric.
  __ ldr(r4, FieldMemOperand(r1, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(r4, FieldMemOperand(r4, SharedFunctionInfo::kFlagsOffset));
  __ tst(r4, Operand(SharedFunctionInfo::ConstructAsBuiltinBit::kMask));
  __ b(eq, &call_generic_stub);

  __ Jump(BUILTIN_CODE(masm->isolate(), JSBuiltinsConstructStub),
          RelocInfo::CODE_TARGET);

  __ bind(&call_generic_stub);
  __ Jump(BUILTIN_CODE(masm->isolate(), JSConstructStubGeneric),
          RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_ConstructBoundFunction(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the function to call (checked to be a JSBoundFunction)
  //  -- r3 : the new target (checked to be a constructor)
  // -----------------------------------
  __ AssertConstructor(r1);
  __ AssertBoundFunction(r1);

  // Push the [[BoundArguments]] onto the stack.
  Generate_PushBoundArguments(masm);

  // Patch new.target to [[BoundTargetFunction]] if new.target equals target.
  __ cmp(r1, r3);
  __ ldr(r3, FieldMemOperand(r1, JSBoundFunction::kBoundTargetFunctionOffset),
         eq);

  // Construct the [[BoundTargetFunction]] via the Construct builtin.
  __ ldr(r1, FieldMemOperand(r1, JSBoundFunction::kBoundTargetFunctionOffset));
  __ Jump(BUILTIN_CODE(masm->isolate(), Construct), RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_Construct(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : the number of arguments (not including the receiver)
  //  -- r1 : the constructor to call (can be any Object)
  //  -- r3 : the new target (either the same as the constructor or
  //          the JSFunction on which new was invoked initially)
  // -----------------------------------

  // Check if target is a Smi.
  Label non_constructor, non_proxy;
  __ JumpIfSmi(r1, &non_constructor);

  // Check if target has a [[Construct]] internal method.
  __ ldr(r4, FieldMemOperand(r1, HeapObject::kMapOffset));
  __ ldrb(r2, FieldMemOperand(r4, Map::kBitFieldOffset));
  __ tst(r2, Operand(Map::Bits1::IsConstructorBit::kMask));
  __ b(eq, &non_constructor);

  // Dispatch based on instance type.
  __ CompareInstanceTypeRange(r4, r5, FIRST_JS_FUNCTION_TYPE,
                              LAST_JS_FUNCTION_TYPE);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructFunction),
          RelocInfo::CODE_TARGET, ls);

  // Only dispatch to bound functions after checking whether they are
  // constructors.
  __ cmp(r5, Operand(JS_BOUND_FUNCTION_TYPE));
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructBoundFunction),
          RelocInfo::CODE_TARGET, eq);

  // Only dispatch to proxies after checking whether they are constructors.
  __ cmp(r5, Operand(JS_PROXY_TYPE));
  __ b(ne, &non_proxy);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructProxy),
          RelocInfo::CODE_TARGET);

  // Called Construct on an exotic Object with a [[Construct]] internal method.
  __ bind(&non_proxy);
  {
    // Overwrite the original receiver with the (original) target.
    __ str(r1, __ ReceiverOperand(r0));
    // Let the "call_as_constructor_delegate" take care of the rest.
    __ LoadNativeContextSlot(r1, Context::CALL_AS_CONSTRUCTOR_DELEGATE_INDEX);
    __ Jump(masm->isolate()->builtins()->CallFunction(),
            RelocInfo::CODE_TARGET);
  }

  // Called Construct on an Object that doesn't have a [[Construct]] internal
  // method.
  __ bind(&non_constructor);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructedNonConstructable),
          RelocInfo::CODE_TARGET);
}

#if V8_ENABLE_WEBASSEMBLY
void Builtins::Generate_WasmCompileLazy(MacroAssembler* masm) {
  // The function index was put in a register by the jump table trampoline.
  // Convert to Smi for the runtime call.
  __ SmiTag(kWasmCompileLazyFuncIndexRegister,
            kWasmCompileLazyFuncIndexRegister);
  {
    HardAbortScope hard_abort(masm);  // Avoid calls to Abort.
    FrameAndConstantPoolScope scope(masm, StackFrame::WASM_COMPILE_LAZY);

    // Save all parameter registers (see wasm-linkage.h). They might be
    // overwritten in the runtime call below. We don't have any callee-saved
    // registers in wasm, so no need to store anything else.
    RegList gp_regs = 0;
    for (Register gp_param_reg : wasm::kGpParamRegisters) {
      gp_regs |= gp_param_reg.bit();
    }
    DwVfpRegister lowest_fp_reg = std::begin(wasm::kFpParamRegisters)[0];
    DwVfpRegister highest_fp_reg = std::end(wasm::kFpParamRegisters)[-1];
    for (DwVfpRegister fp_param_reg : wasm::kFpParamRegisters) {
      CHECK(fp_param_reg.code() >= lowest_fp_reg.code() &&
            fp_param_reg.code() <= highest_fp_reg.code());
    }

    CHECK_EQ(NumRegs(gp_regs), arraysize(wasm::kGpParamRegisters));
    CHECK_EQ(highest_fp_reg.code() - lowest_fp_reg.code() + 1,
             arraysize(wasm::kFpParamRegisters));
    CHECK_EQ(NumRegs(gp_regs),
             WasmCompileLazyFrameConstants::kNumberOfSavedGpParamRegs);
    CHECK_EQ(highest_fp_reg.code() - lowest_fp_reg.code() + 1,
             WasmCompileLazyFrameConstants::kNumberOfSavedFpParamRegs);

    __ stm(db_w, sp, gp_regs);
    __ vstm(db_w, sp, lowest_fp_reg, highest_fp_reg);

    // Pass instance and function index as explicit arguments to the runtime
    // function.
    __ push(kWasmInstanceRegister);
    __ push(kWasmCompileLazyFuncIndexRegister);
    // Initialize the JavaScript context with 0. CEntry will use it to
    // set the current context on the isolate.
    __ Move(cp, Smi::zero());
    __ CallRuntime(Runtime::kWasmCompileLazy, 2);
    // The entrypoint address is the return value.
    __ mov(r8, kReturnRegister0);

    // Restore registers.
    __ vldm(ia_w, sp, lowest_fp_reg, highest_fp_reg);
    __ ldm(ia_w, sp, gp_regs);
  }
  // Finally, jump to the entrypoint.
  __ Jump(r8);
}

void Builtins::Generate_WasmDebugBreak(MacroAssembler* masm) {
  HardAbortScope hard_abort(masm);  // Avoid calls to Abort.
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::WASM_DEBUG_BREAK);

    STATIC_ASSERT(DwVfpRegister::kNumRegisters == 32);
    constexpr uint32_t last =
        31 - base::bits::CountLeadingZeros32(
                 WasmDebugBreakFrameConstants::kPushedFpRegs);
    constexpr uint32_t first = base::bits::CountTrailingZeros32(
        WasmDebugBreakFrameConstants::kPushedFpRegs);
    static_assert(
        base::bits::CountPopulation(
            WasmDebugBreakFrameConstants::kPushedFpRegs) == last - first + 1,
        "All registers in the range from first to last have to be set");

    // Save all parameter registers. They might hold live values, we restore
    // them after the runtime call.
    constexpr DwVfpRegister lowest_fp_reg = DwVfpRegister::from_code(first);
    constexpr DwVfpRegister highest_fp_reg = DwVfpRegister::from_code(last);

    // Store gp parameter registers.
    __ stm(db_w, sp, WasmDebugBreakFrameConstants::kPushedGpRegs);
    // Store fp parameter registers.
    __ vstm(db_w, sp, lowest_fp_reg, highest_fp_reg);

    // Initialize the JavaScript context with 0. CEntry will use it to
    // set the current context on the isolate.
    __ Move(cp, Smi::zero());
    __ CallRuntime(Runtime::kWasmDebugBreak, 0);

    // Restore registers.
    __ vldm(ia_w, sp, lowest_fp_reg, highest_fp_reg);
    __ ldm(ia_w, sp, WasmDebugBreakFrameConstants::kPushedGpRegs);
  }
  __ Ret();
}

void Builtins::Generate_GenericJSToWasmWrapper(MacroAssembler* masm) {
  // TODO(v8:10701): Implement for this platform.
  __ Trap();
}

void Builtins::Generate_WasmOnStackReplace(MacroAssembler* masm) {
  // Only needed on x64.
  __ Trap();
}
#endif  // V8_ENABLE_WEBASSEMBLY

void Builtins::Generate_CEntry(MacroAssembler* masm, int result_size,
                               SaveFPRegsMode save_doubles, ArgvMode argv_mode,
                               bool builtin_exit_frame) {
  // Called from JavaScript; parameters are on stack as if calling JS function.
  // r0: number of arguments including receiver
  // r1: pointer to builtin function
  // fp: frame pointer  (restored after C call)
  // sp: stack pointer  (restored as callee's sp after C call)
  // cp: current context  (C callee-saved)
  //
  // If argv_mode == ArgvMode::kRegister:
  // r2: pointer to the first argument

  __ mov(r5, Operand(r1));

  if (argv_mode == ArgvMode::kRegister) {
    // Move argv into the correct register.
    __ mov(r1, Operand(r2));
  } else {
    // Compute the argv pointer in a callee-saved register.
    __ add(r1, sp, Operand(r0, LSL, kPointerSizeLog2));
    __ sub(r1, r1, Operand(kPointerSize));
  }

  // Enter the exit frame that transitions from JavaScript to C++.
  FrameScope scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(
      save_doubles == SaveFPRegsMode::kSave, 0,
      builtin_exit_frame ? StackFrame::BUILTIN_EXIT : StackFrame::EXIT);

  // Store a copy of argc in callee-saved registers for later.
  __ mov(r4, Operand(r0));

// r0, r4: number of arguments including receiver  (C callee-saved)
// r1: pointer to the first argument (C callee-saved)
// r5: pointer to builtin function  (C callee-saved)

#if V8_HOST_ARCH_ARM
  int frame_alignment = MacroAssembler::ActivationFrameAlignment();
  int frame_alignment_mask = frame_alignment - 1;
  if (FLAG_debug_code) {
    if (frame_alignment > kPointerSize) {
      Label alignment_as_expected;
      DCHECK(base::bits::IsPowerOfTwo(frame_alignment));
      __ tst(sp, Operand(frame_alignment_mask));
      __ b(eq, &alignment_as_expected);
      // Don't use Check here, as it will call Runtime_Abort re-entering here.
      __ stop();
      __ bind(&alignment_as_expected);
    }
  }
#endif

  // Call C built-in.
  // r0 = argc, r1 = argv, r2 = isolate
  __ Move(r2, ExternalReference::isolate_address(masm->isolate()));
  __ StoreReturnAddressAndCall(r5);

  // Result returned in r0 or r1:r0 - do not destroy these registers!

  // Check result for exception sentinel.
  Label exception_returned;
  __ CompareRoot(r0, RootIndex::kException);
  __ b(eq, &exception_returned);

  // Check that there is no pending exception, otherwise we
  // should have returned the exception sentinel.
  if (FLAG_debug_code) {
    Label okay;
    ExternalReference pending_exception_address = ExternalReference::Create(
        IsolateAddressId::kPendingExceptionAddress, masm->isolate());
    __ Move(r3, pending_exception_address);
    __ ldr(r3, MemOperand(r3));
    __ CompareRoot(r3, RootIndex::kTheHoleValue);
    // Cannot use check here as it attempts to generate call into runtime.
    __ b(eq, &okay);
    __ stop();
    __ bind(&okay);
  }

  // Exit C frame and return.
  // r0:r1: result
  // sp: stack pointer
  // fp: frame pointer
  Register argc = argv_mode == ArgvMode::kRegister
                      // We don't want to pop arguments so set argc to no_reg.
                      ? no_reg
                      // Callee-saved register r4 still holds argc.
                      : r4;
  __ LeaveExitFrame(save_doubles == SaveFPRegsMode::kSave, argc);
  __ mov(pc, lr);

  // Handling of exception.
  __ bind(&exception_returned);

  ExternalReference pending_handler_context_address = ExternalReference::Create(
      IsolateAddressId::kPendingHandlerContextAddress, masm->isolate());
  ExternalReference pending_handler_entrypoint_address =
      ExternalReference::Create(
          IsolateAddressId::kPendingHandlerEntrypointAddress, masm->isolate());
  ExternalReference pending_handler_fp_address = ExternalReference::Create(
      IsolateAddressId::kPendingHandlerFPAddress, masm->isolate());
  ExternalReference pending_handler_sp_address = ExternalReference::Create(
      IsolateAddressId::kPendingHandlerSPAddress, masm->isolate());

  // Ask the runtime for help to determine the handler. This will set r0 to
  // contain the current pending exception, don't clobber it.
  ExternalReference find_handler =
      ExternalReference::Create(Runtime::kUnwindAndFindExceptionHandler);
  {
    FrameScope scope(masm, StackFrame::MANUAL);
    __ PrepareCallCFunction(3, 0);
    __ mov(r0, Operand(0));
    __ mov(r1, Operand(0));
    __ Move(r2, ExternalReference::isolate_address(masm->isolate()));
    __ CallCFunction(find_handler, 3);
  }

  // Retrieve the handler context, SP and FP.
  __ Move(cp, pending_handler_context_address);
  __ ldr(cp, MemOperand(cp));
  __ Move(sp, pending_handler_sp_address);
  __ ldr(sp, MemOperand(sp));
  __ Move(fp, pending_handler_fp_address);
  __ ldr(fp, MemOperand(fp));

  // If the handler is a JS frame, restore the context to the frame. Note that
  // the context will be set to (cp == 0) for non-JS frames.
  __ cmp(cp, Operand(0));
  __ str(cp, MemOperand(fp, StandardFrameConstants::kContextOffset), ne);

  // Reset the masking register. This is done independent of the underlying
  // feature flag {FLAG_untrusted_code_mitigations} to make the snapshot work
  // with both configurations. It is safe to always do this, because the
  // underlying register is caller-saved and can be arbitrarily clobbered.
  __ ResetSpeculationPoisonRegister();

  // Clear c_entry_fp, like we do in `LeaveExitFrame`.
  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ Move(scratch, ExternalReference::Create(
                         IsolateAddressId::kCEntryFPAddress, masm->isolate()));
    __ mov(r1, Operand::Zero());
    __ str(r1, MemOperand(scratch));
  }

  // Compute the handler entry address and jump to it.
  ConstantPoolUnavailableScope constant_pool_unavailable(masm);
  __ Move(r1, pending_handler_entrypoint_address);
  __ ldr(r1, MemOperand(r1));
  __ Jump(r1);
}

void Builtins::Generate_DoubleToI(MacroAssembler* masm) {
  Label negate, done;

  HardAbortScope hard_abort(masm);  // Avoid calls to Abort.
  UseScratchRegisterScope temps(masm);
  Register result_reg = r7;
  Register double_low = GetRegisterThatIsNotOneOf(result_reg);
  Register double_high = GetRegisterThatIsNotOneOf(result_reg, double_low);
  LowDwVfpRegister double_scratch = temps.AcquireLowD();

  // Save the old values from these temporary registers on the stack.
  __ Push(result_reg, double_high, double_low);

  // Account for saved regs.
  const int kArgumentOffset = 3 * kPointerSize;

  MemOperand input_operand(sp, kArgumentOffset);
  MemOperand result_operand = input_operand;

  // Load double input.
  __ vldr(double_scratch, input_operand);
  __ vmov(double_low, double_high, double_scratch);
  // Try to convert with a FPU convert instruction. This handles all
  // non-saturating cases.
  __ TryInlineTruncateDoubleToI(result_reg, double_scratch, &done);

  Register scratch = temps.Acquire();
  __ Ubfx(scratch, double_high, HeapNumber::kExponentShift,
          HeapNumber::kExponentBits);
  // Load scratch with exponent - 1. This is faster than loading
  // with exponent because Bias + 1 = 1024 which is an *ARM* immediate value.
  STATIC_ASSERT(HeapNumber::kExponentBias + 1 == 1024);
  __ sub(scratch, scratch, Operand(HeapNumber::kExponentBias + 1));
  // If exponent is greater than or equal to 84, the 32 less significant
  // bits are 0s (2^84 = 1, 52 significant bits, 32 uncoded bits),
  // the result is 0.
  // Compare exponent with 84 (compare exponent - 1 with 83). If the exponent is
  // greater than this, the conversion is out of range, so return zero.
  __ cmp(scratch, Operand(83));
  __ mov(result_reg, Operand::Zero(), LeaveCC, ge);
  __ b(ge, &done);

  // If we reach this code, 30 <= exponent <= 83.
  // `TryInlineTruncateDoubleToI` above will have truncated any double with an
  // exponent lower than 30.
  if (FLAG_debug_code) {
    // Scratch is exponent - 1.
    __ cmp(scratch, Operand(30 - 1));
    __ Check(ge, AbortReason::kUnexpectedValue);
  }

  // We don't have to handle cases where 0 <= exponent <= 20 for which we would
  // need to shift right the high part of the mantissa.
  // Scratch contains exponent - 1.
  // Load scratch with 52 - exponent (load with 51 - (exponent - 1)).
  __ rsb(scratch, scratch, Operand(51), SetCC);

  // 52 <= exponent <= 83, shift only double_low.
  // On entry, scratch contains: 52 - exponent.
  __ rsb(scratch, scratch, Operand::Zero(), LeaveCC, ls);
  __ mov(result_reg, Operand(double_low, LSL, scratch), LeaveCC, ls);
  __ b(ls, &negate);

  // 21 <= exponent <= 51, shift double_low and double_high
  // to generate the result.
  __ mov(double_low, Operand(double_low, LSR, scratch));
  // Scratch contains: 52 - exponent.
  // We needs: exponent - 20.
  // So we use: 32 - scratch = 32 - 52 + exponent = exponent - 20.
  __ rsb(scratch, scratch, Operand(32));
  __ Ubfx(result_reg, double_high, 0, HeapNumber::kMantissaBitsInTopWord);
  // Set the implicit 1 before the mantissa part in double_high.
  __ orr(result_reg, result_reg,
         Operand(1 << HeapNumber::kMantissaBitsInTopWord));
  __ orr(result_reg, double_low, Operand(result_reg, LSL, scratch));

  __ bind(&negate);
  // If input was positive, double_high ASR 31 equals 0 and
  // double_high LSR 31 equals zero.
  // New result = (result eor 0) + 0 = result.
  // If the input was negative, we have to negate the result.
  // Input_high ASR 31 equals 0xFFFFFFFF and double_high LSR 31 equals 1.
  // New result = (result eor 0xFFFFFFFF) + 1 = 0 - result.
  __ eor(result_reg, result_reg, Operand(double_high, ASR, 31));
  __ add(result_reg, result_reg, Operand(double_high, LSR, 31));

  __ bind(&done);
  __ str(result_reg, result_operand);

  // Restore registers corrupted in this routine and return.
  __ Pop(result_reg, double_high, double_low);
  __ Ret();
}

namespace {

int AddressOffset(ExternalReference ref0, ExternalReference ref1) {
  return ref0.address() - ref1.address();
}

// Calls an API function.  Allocates HandleScope, extracts returned value
// from handle and propagates exceptions.  Restores context.  stack_space
// - space to be unwound on exit (includes the call JS arguments space and
// the additional space allocated for the fast call).
void CallApiFunctionAndReturn(MacroAssembler* masm, Register function_address,
                              ExternalReference thunk_ref, int stack_space,
                              MemOperand* stack_space_operand,
                              MemOperand return_value_operand) {
  Isolate* isolate = masm->isolate();
  ExternalReference next_address =
      ExternalReference::handle_scope_next_address(isolate);
  const int kNextOffset = 0;
  const int kLimitOffset = AddressOffset(
      ExternalReference::handle_scope_limit_address(isolate), next_address);
  const int kLevelOffset = AddressOffset(
      ExternalReference::handle_scope_level_address(isolate), next_address);

  DCHECK(function_address == r1 || function_address == r2);

  Label profiler_enabled, end_profiler_check;
  __ Move(r9, ExternalReference::is_profiling_address(isolate));
  __ ldrb(r9, MemOperand(r9, 0));
  __ cmp(r9, Operand(0));
  __ b(ne, &profiler_enabled);
  __ Move(r9, ExternalReference::address_of_runtime_stats_flag());
  __ ldr(r9, MemOperand(r9, 0));
  __ cmp(r9, Operand(0));
  __ b(ne, &profiler_enabled);
  {
    // Call the api function directly.
    __ Move(r3, function_address);
    __ b(&end_profiler_check);
  }
  __ bind(&profiler_enabled);
  {
    // Additional parameter is the address of the actual callback.
    __ Move(r3, thunk_ref);
  }
  __ bind(&end_profiler_check);

  // Allocate HandleScope in callee-save registers.
  __ Move(r9, next_address);
  __ ldr(r4, MemOperand(r9, kNextOffset));
  __ ldr(r5, MemOperand(r9, kLimitOffset));
  __ ldr(r6, MemOperand(r9, kLevelOffset));
  __ add(r6, r6, Operand(1));
  __ str(r6, MemOperand(r9, kLevelOffset));

  __ StoreReturnAddressAndCall(r3);

  Label promote_scheduled_exception;
  Label delete_allocated_handles;
  Label leave_exit_frame;
  Label return_value_loaded;

  // load value from ReturnValue
  __ ldr(r0, return_value_operand);
  __ bind(&return_value_loaded);
  // No more valid handles (the result handle was the last one). Restore
  // previous handle scope.
  __ str(r4, MemOperand(r9, kNextOffset));
  if (FLAG_debug_code) {
    __ ldr(r1, MemOperand(r9, kLevelOffset));
    __ cmp(r1, r6);
    __ Check(eq, AbortReason::kUnexpectedLevelAfterReturnFromApiCall);
  }
  __ sub(r6, r6, Operand(1));
  __ str(r6, MemOperand(r9, kLevelOffset));
  __ ldr(r6, MemOperand(r9, kLimitOffset));
  __ cmp(r5, r6);
  __ b(ne, &delete_allocated_handles);

  // Leave the API exit frame.
  __ bind(&leave_exit_frame);
  // LeaveExitFrame expects unwind space to be in a register.
  if (stack_space_operand == nullptr) {
    DCHECK_NE(stack_space, 0);
    __ mov(r4, Operand(stack_space));
  } else {
    DCHECK_EQ(stack_space, 0);
    __ ldr(r4, *stack_space_operand);
  }
  __ LeaveExitFrame(false, r4, stack_space_operand != nullptr);

  // Check if the function scheduled an exception.
  __ LoadRoot(r4, RootIndex::kTheHoleValue);
  __ Move(r6, ExternalReference::scheduled_exception_address(isolate));
  __ ldr(r5, MemOperand(r6));
  __ cmp(r4, r5);
  __ b(ne, &promote_scheduled_exception);

  __ mov(pc, lr);

  // Re-throw by promoting a scheduled exception.
  __ bind(&promote_scheduled_exception);
  __ TailCallRuntime(Runtime::kPromoteScheduledException);

  // HandleScope limit has changed. Delete allocated extensions.
  __ bind(&delete_allocated_handles);
  __ str(r5, MemOperand(r9, kLimitOffset));
  __ mov(r4, r0);
  __ PrepareCallCFunction(1);
  __ Move(r0, ExternalReference::isolate_address(isolate));
  __ CallCFunction(ExternalReference::delete_handle_scope_extensions(), 1);
  __ mov(r0, r4);
  __ jmp(&leave_exit_frame);
}

}  // namespace

void Builtins::Generate_CallApiCallback(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- cp                  : context
  //  -- r1                  : api function address
  //  -- r2                  : arguments count (not including the receiver)
  //  -- r3                  : call data
  //  -- r0                  : holder
  //  -- sp[0]               : receiver
  //  -- sp[8]               : first argument
  //  -- ...
  //  -- sp[(argc) * 8]      : last argument
  // -----------------------------------

  Register api_function_address = r1;
  Register argc = r2;
  Register call_data = r3;
  Register holder = r0;
  Register scratch = r4;

  DCHECK(!AreAliased(api_function_address, argc, call_data, holder, scratch));

  using FCA = FunctionCallbackArguments;

  STATIC_ASSERT(FCA::kArgsLength == 6);
  STATIC_ASSERT(FCA::kNewTargetIndex == 5);
  STATIC_ASSERT(FCA::kDataIndex == 4);
  STATIC_ASSERT(FCA::kReturnValueOffset == 3);
  STATIC_ASSERT(FCA::kReturnValueDefaultValueIndex == 2);
  STATIC_ASSERT(FCA::kIsolateIndex == 1);
  STATIC_ASSERT(FCA::kHolderIndex == 0);

  // Set up FunctionCallbackInfo's implicit_args on the stack as follows:
  //
  // Target state:
  //   sp[0 * kPointerSize]: kHolder
  //   sp[1 * kPointerSize]: kIsolate
  //   sp[2 * kPointerSize]: undefined (kReturnValueDefaultValue)
  //   sp[3 * kPointerSize]: undefined (kReturnValue)
  //   sp[4 * kPointerSize]: kData
  //   sp[5 * kPointerSize]: undefined (kNewTarget)

  // Reserve space on the stack.
  __ AllocateStackSpace(FCA::kArgsLength * kPointerSize);

  // kHolder.
  __ str(holder, MemOperand(sp, 0 * kPointerSize));

  // kIsolate.
  __ Move(scratch, ExternalReference::isolate_address(masm->isolate()));
  __ str(scratch, MemOperand(sp, 1 * kPointerSize));

  // kReturnValueDefaultValue and kReturnValue.
  __ LoadRoot(scratch, RootIndex::kUndefinedValue);
  __ str(scratch, MemOperand(sp, 2 * kPointerSize));
  __ str(scratch, MemOperand(sp, 3 * kPointerSize));

  // kData.
  __ str(call_data, MemOperand(sp, 4 * kPointerSize));

  // kNewTarget.
  __ str(scratch, MemOperand(sp, 5 * kPointerSize));

  // Keep a pointer to kHolder (= implicit_args) in a scratch register.
  // We use it below to set up the FunctionCallbackInfo object.
  __ mov(scratch, sp);

  // Allocate the v8::Arguments structure in the arguments' space since
  // it's not controlled by GC.
  static constexpr int kApiStackSpace = 4;
  static constexpr bool kDontSaveDoubles = false;
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(kDontSaveDoubles, kApiStackSpace);

  // FunctionCallbackInfo::implicit_args_ (points at kHolder as set up above).
  // Arguments are after the return address (pushed by EnterExitFrame()).
  __ str(scratch, MemOperand(sp, 1 * kPointerSize));

  // FunctionCallbackInfo::values_ (points at the first varargs argument passed
  // on the stack).
  __ add(scratch, scratch, Operand((FCA::kArgsLength + 1) * kPointerSize));
  __ str(scratch, MemOperand(sp, 2 * kPointerSize));

  // FunctionCallbackInfo::length_.
  __ str(argc, MemOperand(sp, 3 * kPointerSize));

  // We also store the number of bytes to drop from the stack after returning
  // from the API function here.
  __ mov(scratch,
         Operand((FCA::kArgsLength + 1 /* receiver */) * kPointerSize));
  __ add(scratch, scratch, Operand(argc, LSL, kPointerSizeLog2));
  __ str(scratch, MemOperand(sp, 4 * kPointerSize));

  // v8::InvocationCallback's argument.
  __ add(r0, sp, Operand(1 * kPointerSize));

  ExternalReference thunk_ref = ExternalReference::invoke_function_callback();

  // There are two stack slots above the arguments we constructed on the stack.
  // TODO(jgruber): Document what these arguments are.
  static constexpr int kStackSlotsAboveFCA = 2;
  MemOperand return_value_operand(
      fp, (kStackSlotsAboveFCA + FCA::kReturnValueOffset) * kPointerSize);

  static constexpr int kUseStackSpaceOperand = 0;
  MemOperand stack_space_operand(sp, 4 * kPointerSize);

  AllowExternalCallThatCantCauseGC scope(masm);
  CallApiFunctionAndReturn(masm, api_function_address, thunk_ref,
                           kUseStackSpaceOperand, &stack_space_operand,
                           return_value_operand);
}

void Builtins::Generate_CallApiGetter(MacroAssembler* masm) {
  // Build v8::PropertyCallbackInfo::args_ array on the stack and push property
  // name below the exit frame to make GC aware of them.
  STATIC_ASSERT(PropertyCallbackArguments::kShouldThrowOnErrorIndex == 0);
  STATIC_ASSERT(PropertyCallbackArguments::kHolderIndex == 1);
  STATIC_ASSERT(PropertyCallbackArguments::kIsolateIndex == 2);
  STATIC_ASSERT(PropertyCallbackArguments::kReturnValueDefaultValueIndex == 3);
  STATIC_ASSERT(PropertyCallbackArguments::kReturnValueOffset == 4);
  STATIC_ASSERT(PropertyCallbackArguments::kDataIndex == 5);
  STATIC_ASSERT(PropertyCallbackArguments::kThisIndex == 6);
  STATIC_ASSERT(PropertyCallbackArguments::kArgsLength == 7);

  Register receiver = ApiGetterDescriptor::ReceiverRegister();
  Register holder = ApiGetterDescriptor::HolderRegister();
  Register callback = ApiGetterDescriptor::CallbackRegister();
  Register scratch = r4;
  DCHECK(!AreAliased(receiver, holder, callback, scratch));

  Register api_function_address = r2;

  __ push(receiver);
  // Push data from AccessorInfo.
  __ ldr(scratch, FieldMemOperand(callback, AccessorInfo::kDataOffset));
  __ push(scratch);
  __ LoadRoot(scratch, RootIndex::kUndefinedValue);
  __ Push(scratch, scratch);
  __ Move(scratch, ExternalReference::isolate_address(masm->isolate()));
  __ Push(scratch, holder);
  __ Push(Smi::zero());  // should_throw_on_error -> false
  __ ldr(scratch, FieldMemOperand(callback, AccessorInfo::kNameOffset));
  __ push(scratch);
  // v8::PropertyCallbackInfo::args_ array and name handle.
  const int kStackUnwindSpace = PropertyCallbackArguments::kArgsLength + 1;

  // Load address of v8::PropertyAccessorInfo::args_ array and name handle.
  __ mov(r0, sp);                             // r0 = Handle<Name>
  __ add(r1, r0, Operand(1 * kPointerSize));  // r1 = v8::PCI::args_

  const int kApiStackSpace = 1;
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(false, kApiStackSpace);

  // Create v8::PropertyCallbackInfo object on the stack and initialize
  // it's args_ field.
  __ str(r1, MemOperand(sp, 1 * kPointerSize));
  __ add(r1, sp, Operand(1 * kPointerSize));  // r1 = v8::PropertyCallbackInfo&

  ExternalReference thunk_ref =
      ExternalReference::invoke_accessor_getter_callback();

  __ ldr(scratch, FieldMemOperand(callback, AccessorInfo::kJsGetterOffset));
  __ ldr(api_function_address,
         FieldMemOperand(scratch, Foreign::kForeignAddressOffset));

  // +3 is to skip prolog, return address and name handle.
  MemOperand return_value_operand(
      fp, (PropertyCallbackArguments::kReturnValueOffset + 3) * kPointerSize);
  MemOperand* const kUseStackSpaceConstant = nullptr;
  CallApiFunctionAndReturn(masm, api_function_address, thunk_ref,
                           kStackUnwindSpace, kUseStackSpaceConstant,
                           return_value_operand);
}

void Builtins::Generate_DirectCEntry(MacroAssembler* masm) {
  // The sole purpose of DirectCEntry is for movable callers (e.g. any general
  // purpose Code object) to be able to call into C functions that may trigger
  // GC and thus move the caller.
  //
  // DirectCEntry places the return address on the stack (updated by the GC),
  // making the call GC safe. The irregexp backend relies on this.

  __ str(lr, MemOperand(sp, 0));  // Store the return address.
  __ blx(ip);                     // Call the C++ function.
  __ ldr(pc, MemOperand(sp, 0));  // Return to calling code.
}

void Builtins::Generate_MemCopyUint8Uint8(MacroAssembler* masm) {
  Register dest = r0;
  Register src = r1;
  Register chars = r2;
  Register temp1 = r3;
  Label less_4;

  {
    UseScratchRegisterScope temps(masm);
    Register temp2 = temps.Acquire();
    Label loop;

    __ bic(temp2, chars, Operand(0x3), SetCC);
    __ b(&less_4, eq);
    __ add(temp2, dest, temp2);

    __ bind(&loop);
    __ ldr(temp1, MemOperand(src, 4, PostIndex));
    __ str(temp1, MemOperand(dest, 4, PostIndex));
    __ cmp(dest, temp2);
    __ b(&loop, ne);
  }

  __ bind(&less_4);
  __ mov(chars, Operand(chars, LSL, 31), SetCC);
  // bit0 => Z (ne), bit1 => C (cs)
  __ ldrh(temp1, MemOperand(src, 2, PostIndex), cs);
  __ strh(temp1, MemOperand(dest, 2, PostIndex), cs);
  __ ldrb(temp1, MemOperand(src), ne);
  __ strb(temp1, MemOperand(dest), ne);
  __ Ret();
}

namespace {

// This code tries to be close to ia32 code so that any changes can be
// easily ported.
void Generate_DeoptimizationEntry(MacroAssembler* masm,
                                  DeoptimizeKind deopt_kind) {
  Isolate* isolate = masm->isolate();

  // Note: This is an overapproximation; we always reserve space for 32 double
  // registers, even though the actual CPU may only support 16. In the latter
  // case, SaveFPRegs and RestoreFPRegs still use 32 stack slots, but only fill
  // 16.
  static constexpr int kDoubleRegsSize =
      kDoubleSize * DwVfpRegister::kNumRegisters;

  // Save all allocatable VFP registers before messing with them.
  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ SaveFPRegs(sp, scratch);
  }

  // Save all general purpose registers before messing with them.
  static constexpr int kNumberOfRegisters = Register::kNumRegisters;
  STATIC_ASSERT(kNumberOfRegisters == 16);

  // Everything but pc, lr and ip which will be saved but not restored.
  RegList restored_regs = kJSCallerSaved | kCalleeSaved | ip.bit();

  // Push all 16 registers (needed to populate FrameDescription::registers_).
  // TODO(v8:1588): Note that using pc with stm is deprecated, so we should
  // perhaps handle this a bit differently.
  __ stm(db_w, sp, restored_regs | sp.bit() | lr.bit() | pc.bit());

  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ Move(scratch, ExternalReference::Create(
                         IsolateAddressId::kCEntryFPAddress, isolate));
    __ str(fp, MemOperand(scratch));
  }

  static constexpr int kSavedRegistersAreaSize =
      (kNumberOfRegisters * kPointerSize) + kDoubleRegsSize;

  __ mov(r2, Operand(Deoptimizer::kFixedExitSizeMarker));
  // Get the address of the location in the code object (r3) (return
  // address for lazy deoptimization) and compute the fp-to-sp delta in
  // register r4.
  __ mov(r3, lr);
  __ add(r4, sp, Operand(kSavedRegistersAreaSize));
  __ sub(r4, fp, r4);

  // Allocate a new deoptimizer object.
  // Pass four arguments in r0 to r3 and fifth argument on stack.
  __ PrepareCallCFunction(6);
  __ mov(r0, Operand(0));
  Label context_check;
  __ ldr(r1, MemOperand(fp, CommonFrameConstants::kContextOrFrameTypeOffset));
  __ JumpIfSmi(r1, &context_check);
  __ ldr(r0, MemOperand(fp, StandardFrameConstants::kFunctionOffset));
  __ bind(&context_check);
  __ mov(r1, Operand(static_cast<int>(deopt_kind)));
  // r2: bailout id already loaded.
  // r3: code address or 0 already loaded.
  __ str(r4, MemOperand(sp, 0 * kPointerSize));  // Fp-to-sp delta.
  __ Move(r5, ExternalReference::isolate_address(isolate));
  __ str(r5, MemOperand(sp, 1 * kPointerSize));  // Isolate.
  // Call Deoptimizer::New().
  {
    AllowExternalCallThatCantCauseGC scope(masm);
    __ CallCFunction(ExternalReference::new_deoptimizer_function(), 6);
  }

  // Preserve "deoptimizer" object in register r0 and get the input
  // frame descriptor pointer to r1 (deoptimizer->input_);
  __ ldr(r1, MemOperand(r0, Deoptimizer::input_offset()));

  // Copy core registers into FrameDescription::registers_.
  DCHECK_EQ(Register::kNumRegisters, kNumberOfRegisters);
  for (int i = 0; i < kNumberOfRegisters; i++) {
    int offset = (i * kPointerSize) + FrameDescription::registers_offset();
    __ ldr(r2, MemOperand(sp, i * kPointerSize));
    __ str(r2, MemOperand(r1, offset));
  }

  // Copy double registers to double_registers_.
  static constexpr int kDoubleRegsOffset =
      FrameDescription::double_registers_offset();
  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    Register src_location = r4;
    __ add(src_location, sp, Operand(kNumberOfRegisters * kPointerSize));
    __ RestoreFPRegs(src_location, scratch);

    Register dst_location = r4;
    __ add(dst_location, r1, Operand(kDoubleRegsOffset));
    __ SaveFPRegsToHeap(dst_location, scratch);
  }

  // Mark the stack as not iterable for the CPU profiler which won't be able to
  // walk the stack without the return address.
  {
    UseScratchRegisterScope temps(masm);
    Register is_iterable = temps.Acquire();
    Register zero = r4;
    __ Move(is_iterable, ExternalReference::stack_is_iterable_address(isolate));
    __ mov(zero, Operand(0));
    __ strb(zero, MemOperand(is_iterable));
  }

  // Remove the saved registers from the stack.
  __ add(sp, sp, Operand(kSavedRegistersAreaSize));

  // Compute a pointer to the unwinding limit in register r2; that is
  // the first stack slot not part of the input frame.
  __ ldr(r2, MemOperand(r1, FrameDescription::frame_size_offset()));
  __ add(r2, r2, sp);

  // Unwind the stack down to - but not including - the unwinding
  // limit and copy the contents of the activation frame to the input
  // frame description.
  __ add(r3, r1, Operand(FrameDescription::frame_content_offset()));
  Label pop_loop;
  Label pop_loop_header;
  __ b(&pop_loop_header);
  __ bind(&pop_loop);
  __ pop(r4);
  __ str(r4, MemOperand(r3, 0));
  __ add(r3, r3, Operand(sizeof(uint32_t)));
  __ bind(&pop_loop_header);
  __ cmp(r2, sp);
  __ b(ne, &pop_loop);

  // Compute the output frame in the deoptimizer.
  __ push(r0);  // Preserve deoptimizer object across call.
  // r0: deoptimizer object; r1: scratch.
  __ PrepareCallCFunction(1);
  // Call Deoptimizer::ComputeOutputFrames().
  {
    AllowExternalCallThatCantCauseGC scope(masm);
    __ CallCFunction(ExternalReference::compute_output_frames_function(), 1);
  }
  __ pop(r0);  // Restore deoptimizer object (class Deoptimizer).

  __ ldr(sp, MemOperand(r0, Deoptimizer::caller_frame_top_offset()));

  // Replace the current (input) frame with the output frames.
  Label outer_push_loop, inner_push_loop, outer_loop_header, inner_loop_header;
  // Outer loop state: r4 = current "FrameDescription** output_",
  // r1 = one past the last FrameDescription**.
  __ ldr(r1, MemOperand(r0, Deoptimizer::output_count_offset()));
  __ ldr(r4, MemOperand(r0, Deoptimizer::output_offset()));  // r4 is output_.
  __ add(r1, r4, Operand(r1, LSL, 2));
  __ jmp(&outer_loop_header);
  __ bind(&outer_push_loop);
  // Inner loop state: r2 = current FrameDescription*, r3 = loop index.
  __ ldr(r2, MemOperand(r4, 0));  // output_[ix]
  __ ldr(r3, MemOperand(r2, FrameDescription::frame_size_offset()));
  __ jmp(&inner_loop_header);
  __ bind(&inner_push_loop);
  __ sub(r3, r3, Operand(sizeof(uint32_t)));
  __ add(r6, r2, Operand(r3));
  __ ldr(r6, MemOperand(r6, FrameDescription::frame_content_offset()));
  __ push(r6);
  __ bind(&inner_loop_header);
  __ cmp(r3, Operand::Zero());
  __ b(ne, &inner_push_loop);  // test for gt?
  __ add(r4, r4, Operand(kPointerSize));
  __ bind(&outer_loop_header);
  __ cmp(r4, r1);
  __ b(lt, &outer_push_loop);

  __ ldr(r1, MemOperand(r0, Deoptimizer::input_offset()));

  // State:
  // r1: Deoptimizer::input_ (FrameDescription*).
  // r2: The last output FrameDescription pointer (FrameDescription*).

  // Restore double registers from the input frame description.
  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    Register src_location = r6;
    __ add(src_location, r1, Operand(kDoubleRegsOffset));
    __ RestoreFPRegsFromHeap(src_location, scratch);
  }

  // Push pc and continuation from the last output frame.
  __ ldr(r6, MemOperand(r2, FrameDescription::pc_offset()));
  __ push(r6);
  __ ldr(r6, MemOperand(r2, FrameDescription::continuation_offset()));
  __ push(r6);

  // Push the registers from the last output frame.
  for (int i = kNumberOfRegisters - 1; i >= 0; i--) {
    int offset = (i * kPointerSize) + FrameDescription::registers_offset();
    __ ldr(r6, MemOperand(r2, offset));
    __ push(r6);
  }

  // Restore the registers from the stack.
  __ ldm(ia_w, sp, restored_regs);  // all but pc registers.

  {
    UseScratchRegisterScope temps(masm);
    Register is_iterable = temps.Acquire();
    Register one = r4;
    __ Move(is_iterable, ExternalReference::stack_is_iterable_address(isolate));
    __ mov(one, Operand(1));
    __ strb(one, MemOperand(is_iterable));
  }

  // Remove sp, lr and pc.
  __ Drop(3);
  {
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ pop(scratch);  // get continuation, leave pc on stack
    __ pop(lr);
    __ Jump(scratch);
  }

  __ stop();
}

}  // namespace

void Builtins::Generate_DeoptimizationEntry_Eager(MacroAssembler* masm) {
  Generate_DeoptimizationEntry(masm, DeoptimizeKind::kEager);
}

void Builtins::Generate_DeoptimizationEntry_Soft(MacroAssembler* masm) {
  Generate_DeoptimizationEntry(masm, DeoptimizeKind::kSoft);
}

void Builtins::Generate_DeoptimizationEntry_Bailout(MacroAssembler* masm) {
  Generate_DeoptimizationEntry(masm, DeoptimizeKind::kBailout);
}

void Builtins::Generate_DeoptimizationEntry_Lazy(MacroAssembler* masm) {
  Generate_DeoptimizationEntry(masm, DeoptimizeKind::kLazy);
}

namespace {

// Converts an interpreter frame into a baseline frame and continues execution
// in baseline code (baseline code has to exist on the shared function info),
// either at the current or next (in execution order) bytecode.
void Generate_BaselineEntry(MacroAssembler* masm, bool next_bytecode,
                            bool is_osr = false) {
  __ Push(kInterpreterAccumulatorRegister);
  Label start;
  __ bind(&start);

  // Get function from the frame.
  Register closure = r1;
  __ ldr(closure, MemOperand(fp, StandardFrameConstants::kFunctionOffset));

  // Load the feedback vector.
  Register feedback_vector = r2;
  __ ldr(feedback_vector,
         FieldMemOperand(closure, JSFunction::kFeedbackCellOffset));
  __ ldr(feedback_vector, FieldMemOperand(feedback_vector, Cell::kValueOffset));

  Label install_baseline_code;
  // Check if feedback vector is valid. If not, call prepare for baseline to
  // allocate it.
  __ CompareObjectType(feedback_vector, r3, r3, FEEDBACK_VECTOR_TYPE);
  __ b(ne, &install_baseline_code);

  // Save BytecodeOffset from the stack frame.
  __ ldr(kInterpreterBytecodeOffsetRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  __ SmiUntag(kInterpreterBytecodeOffsetRegister);
  // Replace BytecodeOffset with the feedback vector.
  __ str(feedback_vector,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  feedback_vector = no_reg;

  // Get the Code object from the shared function info.
  Register code_obj = r4;
  __ ldr(code_obj,
         FieldMemOperand(closure, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(code_obj,
         FieldMemOperand(code_obj, SharedFunctionInfo::kFunctionDataOffset));
  __ ldr(code_obj,
         FieldMemOperand(code_obj, BaselineData::kBaselineCodeOffset));

  // Compute baseline pc for bytecode offset.
  ExternalReference get_baseline_pc_extref;
  if (next_bytecode || is_osr) {
    get_baseline_pc_extref =
        ExternalReference::baseline_pc_for_next_executed_bytecode();
  } else {
    get_baseline_pc_extref =
        ExternalReference::baseline_pc_for_bytecode_offset();
  }
  Register get_baseline_pc = r3;
  __ Move(get_baseline_pc, get_baseline_pc_extref);

  // If the code deoptimizes during the implicit function entry stack interrupt
  // check, it will have a bailout ID of kFunctionEntryBytecodeOffset, which is
  // not a valid bytecode offset.
  // TODO(pthier): Investigate if it is feasible to handle this special case
  // in TurboFan instead of here.
  Label valid_bytecode_offset, function_entry_bytecode;
  if (!is_osr) {
    __ cmp(kInterpreterBytecodeOffsetRegister,
           Operand(BytecodeArray::kHeaderSize - kHeapObjectTag +
                   kFunctionEntryBytecodeOffset));
    __ b(eq, &function_entry_bytecode);
  }

  __ sub(kInterpreterBytecodeOffsetRegister, kInterpreterBytecodeOffsetRegister,
         Operand(BytecodeArray::kHeaderSize - kHeapObjectTag));

  __ bind(&valid_bytecode_offset);
  // Get bytecode array from the stack frame.
  __ ldr(kInterpreterBytecodeArrayRegister,
         MemOperand(fp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  {
    Register arg_reg_1 = r0;
    Register arg_reg_2 = r1;
    Register arg_reg_3 = r2;
    __ mov(arg_reg_1, code_obj);
    __ mov(arg_reg_2, kInterpreterBytecodeOffsetRegister);
    __ mov(arg_reg_3, kInterpreterBytecodeArrayRegister);
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ PrepareCallCFunction(3, 0);
    __ CallCFunction(get_baseline_pc, 3, 0);
  }
  __ add(code_obj, code_obj, kReturnRegister0);
  __ Pop(kInterpreterAccumulatorRegister);

  if (is_osr) {
    // Reset the OSR loop nesting depth to disarm back edges.
    // TODO(pthier): Separate baseline Sparkplug from TF arming and don't disarm
    // Sparkplug here.
    UseScratchRegisterScope temps(masm);
    Register scratch = temps.Acquire();
    __ mov(scratch, Operand(0));
    __ strh(scratch, FieldMemOperand(kInterpreterBytecodeArrayRegister,
                                     BytecodeArray::kOsrNestingLevelOffset));
    Generate_OSREntry(masm, code_obj,
                      Operand(Code::kHeaderSize - kHeapObjectTag));
  } else {
    __ add(code_obj, code_obj, Operand(Code::kHeaderSize - kHeapObjectTag));
    __ Jump(code_obj);
  }
  __ Trap();  // Unreachable.

  if (!is_osr) {
    __ bind(&function_entry_bytecode);
    // If the bytecode offset is kFunctionEntryOffset, get the start address of
    // the first bytecode.
    __ mov(kInterpreterBytecodeOffsetRegister, Operand(0));
    if (next_bytecode) {
      __ Move(get_baseline_pc,
              ExternalReference::baseline_pc_for_bytecode_offset());
    }
    __ b(&valid_bytecode_offset);
  }

  __ bind(&install_baseline_code);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ Push(closure);
    __ CallRuntime(Runtime::kInstallBaselineCode, 1);
  }
  // Retry from the start after installing baseline code.
  __ b(&start);
}

}  // namespace

void Builtins::Generate_BaselineEnterAtBytecode(MacroAssembler* masm) {
  Generate_BaselineEntry(masm, false);
}

void Builtins::Generate_BaselineEnterAtNextBytecode(MacroAssembler* masm) {
  Generate_BaselineEntry(masm, true);
}

void Builtins::Generate_InterpreterOnStackReplacement_ToBaseline(
    MacroAssembler* masm) {
  Generate_BaselineEntry(masm, false, true);
}

void Builtins::Generate_DynamicCheckMapsTrampoline(MacroAssembler* masm) {
  FrameScope scope(masm, StackFrame::MANUAL);
  __ EnterFrame(StackFrame::INTERNAL);

  // Only save the registers that the DynamicCheckMaps builtin can clobber.
  DynamicCheckMapsDescriptor descriptor;
  RegList registers = descriptor.allocatable_registers();
  // FLAG_debug_code is enabled CSA checks will call C function and so we need
  // to save all CallerSaved registers too.
  if (FLAG_debug_code) registers |= kCallerSaved;
  __ SaveRegisters(registers);

  // Load the immediate arguments from the deopt exit to pass to the builtin.
  Register slot_arg =
      descriptor.GetRegisterParameter(DynamicCheckMapsDescriptor::kSlot);
  Register handler_arg =
      descriptor.GetRegisterParameter(DynamicCheckMapsDescriptor::kHandler);
  __ ldr(handler_arg, MemOperand(fp, CommonFrameConstants::kCallerPCOffset));
  __ ldr(slot_arg, MemOperand(handler_arg,
                              Deoptimizer::kEagerWithResumeImmedArgs1PcOffset));
  __ ldr(
      handler_arg,
      MemOperand(handler_arg, Deoptimizer::kEagerWithResumeImmedArgs2PcOffset));

  __ Call(BUILTIN_CODE(masm->isolate(), DynamicCheckMaps),
          RelocInfo::CODE_TARGET);

  Label deopt, bailout;
  __ cmp_raw_immediate(r0, static_cast<int>(DynamicCheckMapsStatus::kSuccess));
  __ b(ne, &deopt);

  __ RestoreRegisters(registers);
  __ LeaveFrame(StackFrame::INTERNAL);
  __ Ret();

  __ bind(&deopt);
  __ cmp_raw_immediate(r0, static_cast<int>(DynamicCheckMapsStatus::kBailout));
  __ b(eq, &bailout);

  if (FLAG_debug_code) {
    __ cmp_raw_immediate(r0, static_cast<int>(DynamicCheckMapsStatus::kDeopt));
    __ Assert(eq, AbortReason::kUnexpectedDynamicCheckMapsStatus);
  }
  __ RestoreRegisters(registers);
  __ LeaveFrame(StackFrame::INTERNAL);
  Handle<Code> deopt_eager = masm->isolate()->builtins()->builtin_handle(
      Deoptimizer::GetDeoptimizationEntry(DeoptimizeKind::kEager));
  __ Jump(deopt_eager, RelocInfo::CODE_TARGET);

  __ bind(&bailout);
  __ RestoreRegisters(registers);
  __ LeaveFrame(StackFrame::INTERNAL);
  Handle<Code> deopt_bailout = masm->isolate()->builtins()->builtin_handle(
      Deoptimizer::GetDeoptimizationEntry(DeoptimizeKind::kBailout));
  __ Jump(deopt_bailout, RelocInfo::CODE_TARGET);
}

#undef __

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_ARM
