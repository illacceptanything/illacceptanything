// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if V8_TARGET_ARCH_X64

#include "src/api/api-arguments.h"
#include "src/base/bits-iterator.h"
#include "src/base/iterator.h"
#include "src/codegen/code-factory.h"
#include "src/codegen/interface-descriptors-inl.h"
// For interpreter_entry_return_pc_offset. TODO(jkummerow): Drop.
#include "src/codegen/macro-assembler-inl.h"
#include "src/codegen/register-configuration.h"
#include "src/codegen/x64/assembler-x64.h"
#include "src/common/globals.h"
#include "src/deoptimizer/deoptimizer.h"
#include "src/execution/frame-constants.h"
#include "src/execution/frames.h"
#include "src/heap/heap-inl.h"
#include "src/logging/counters.h"
#include "src/objects/cell.h"
#include "src/objects/code.h"
#include "src/objects/debug-objects.h"
#include "src/objects/foreign.h"
#include "src/objects/heap-number.h"
#include "src/objects/js-generator.h"
#include "src/objects/objects-inl.h"
#include "src/objects/smi.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/baseline/liftoff-assembler-defs.h"
#include "src/wasm/object-access.h"
#include "src/wasm/wasm-constants.h"
#include "src/wasm/wasm-linkage.h"
#include "src/wasm/wasm-objects.h"
#endif  // V8_ENABLE_WEBASSEMBLY

namespace v8 {
namespace internal {

#define __ ACCESS_MASM(masm)

void Builtins::Generate_Adaptor(MacroAssembler* masm, Address address) {
  __ LoadAddress(kJavaScriptCallExtraArg1Register,
                 ExternalReference::Create(address));
  __ Jump(BUILTIN_CODE(masm->isolate(), AdaptorWithBuiltinExitFrame),
          RelocInfo::CODE_TARGET);
}

static void GenerateTailCallToReturnedCode(
    MacroAssembler* masm, Runtime::FunctionId function_id,
    JumpMode jump_mode = JumpMode::kJump) {
  // ----------- S t a t e -------------
  //  -- rax : actual argument count
  //  -- rdx : new target (preserved for callee)
  //  -- rdi : target function (preserved for callee)
  // -----------------------------------
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    // Push a copy of the target function, the new target and the actual
    // argument count.
    __ Push(kJavaScriptCallTargetRegister);
    __ Push(kJavaScriptCallNewTargetRegister);
    __ SmiTag(kJavaScriptCallArgCountRegister);
    __ Push(kJavaScriptCallArgCountRegister);
    // Function is also the parameter to the runtime call.
    __ Push(kJavaScriptCallTargetRegister);

    __ CallRuntime(function_id, 1);
    __ movq(rcx, rax);

    // Restore target function, new target and actual argument count.
    __ Pop(kJavaScriptCallArgCountRegister);
    __ SmiUntag(kJavaScriptCallArgCountRegister);
    __ Pop(kJavaScriptCallNewTargetRegister);
    __ Pop(kJavaScriptCallTargetRegister);
  }
  static_assert(kJavaScriptCallCodeStartRegister == rcx, "ABI mismatch");
  __ JumpCodeObject(rcx, jump_mode);
}

namespace {

void Generate_JSBuiltinsConstructStubHelper(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax: number of arguments
  //  -- rdi: constructor function
  //  -- rdx: new target
  //  -- rsi: context
  // -----------------------------------

  Label stack_overflow;
  __ StackOverflowCheck(rax, rcx, &stack_overflow, Label::kFar);

  // Enter a construct frame.
  {
    FrameScope scope(masm, StackFrame::CONSTRUCT);

    // Preserve the incoming parameters on the stack.
    __ SmiTag(rcx, rax);
    __ Push(rsi);
    __ Push(rcx);

    // TODO(victorgomes): When the arguments adaptor is completely removed, we
    // should get the formal parameter count and copy the arguments in its
    // correct position (including any undefined), instead of delaying this to
    // InvokeFunction.

    // Set up pointer to first argument (skip receiver).
    __ leaq(rbx, Operand(rbp, StandardFrameConstants::kCallerSPOffset +
                                  kSystemPointerSize));
    // Copy arguments to the expression stack.
    __ PushArray(rbx, rax, rcx);
    // The receiver for the builtin/api call.
    __ PushRoot(RootIndex::kTheHoleValue);

    // Call the function.
    // rax: number of arguments (untagged)
    // rdi: constructor function
    // rdx: new target
    __ InvokeFunction(rdi, rdx, rax, InvokeType::kCall);

    // Restore smi-tagged arguments count from the frame.
    __ movq(rbx, Operand(rbp, ConstructFrameConstants::kLengthOffset));

    // Leave construct frame.
  }

  // Remove caller arguments from the stack and return.
  __ PopReturnAddressTo(rcx);
  SmiIndex index = masm->SmiToIndex(rbx, rbx, kSystemPointerSizeLog2);
  __ leaq(rsp, Operand(rsp, index.reg, index.scale, 1 * kSystemPointerSize));
  __ PushReturnAddressFrom(rcx);

  __ ret(0);

  __ bind(&stack_overflow);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kThrowStackOverflow);
    __ int3();  // This should be unreachable.
  }
}

}  // namespace

// The construct stub for ES5 constructor functions and ES6 class constructors.
void Builtins::Generate_JSConstructStubGeneric(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax: number of arguments (untagged)
  //  -- rdi: constructor function
  //  -- rdx: new target
  //  -- rsi: context
  //  -- sp[...]: constructor arguments
  // -----------------------------------

  FrameScope scope(masm, StackFrame::MANUAL);
  // Enter a construct frame.
  __ EnterFrame(StackFrame::CONSTRUCT);
  Label post_instantiation_deopt_entry, not_create_implicit_receiver;

  // Preserve the incoming parameters on the stack.
  __ SmiTag(rcx, rax);
  __ Push(rsi);
  __ Push(rcx);
  __ Push(rdi);
  __ PushRoot(RootIndex::kTheHoleValue);
  __ Push(rdx);

  // ----------- S t a t e -------------
  //  --         sp[0*kSystemPointerSize]: new target
  //  --         sp[1*kSystemPointerSize]: padding
  //  -- rdi and sp[2*kSystemPointerSize]: constructor function
  //  --         sp[3*kSystemPointerSize]: argument count
  //  --         sp[4*kSystemPointerSize]: context
  // -----------------------------------

  __ LoadTaggedPointerField(
      rbx, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
  __ movl(rbx, FieldOperand(rbx, SharedFunctionInfo::kFlagsOffset));
  __ DecodeField<SharedFunctionInfo::FunctionKindBits>(rbx);
  __ JumpIfIsInRange(rbx, kDefaultDerivedConstructor, kDerivedConstructor,
                     &not_create_implicit_receiver, Label::kNear);

  // If not derived class constructor: Allocate the new receiver object.
  __ IncrementCounter(masm->isolate()->counters()->constructed_objects(), 1);
  __ Call(BUILTIN_CODE(masm->isolate(), FastNewObject), RelocInfo::CODE_TARGET);
  __ jmp(&post_instantiation_deopt_entry, Label::kNear);

  // Else: use TheHoleValue as receiver for constructor call
  __ bind(&not_create_implicit_receiver);
  __ LoadRoot(rax, RootIndex::kTheHoleValue);

  // ----------- S t a t e -------------
  //  -- rax                          implicit receiver
  //  -- Slot 4 / sp[0*kSystemPointerSize]  new target
  //  -- Slot 3 / sp[1*kSystemPointerSize]  padding
  //  -- Slot 2 / sp[2*kSystemPointerSize]  constructor function
  //  -- Slot 1 / sp[3*kSystemPointerSize]  number of arguments (tagged)
  //  -- Slot 0 / sp[4*kSystemPointerSize]  context
  // -----------------------------------
  // Deoptimizer enters here.
  masm->isolate()->heap()->SetConstructStubCreateDeoptPCOffset(
      masm->pc_offset());
  __ bind(&post_instantiation_deopt_entry);

  // Restore new target.
  __ Pop(rdx);

  // Push the allocated receiver to the stack.
  __ Push(rax);

  // We need two copies because we may have to return the original one
  // and the calling conventions dictate that the called function pops the
  // receiver. The second copy is pushed after the arguments, we saved in r8
  // since rax needs to store the number of arguments before
  // InvokingFunction.
  __ movq(r8, rax);

  // Set up pointer to first argument (skip receiver).
  __ leaq(rbx, Operand(rbp, StandardFrameConstants::kCallerSPOffset +
                                kSystemPointerSize));

  // Restore constructor function and argument count.
  __ movq(rdi, Operand(rbp, ConstructFrameConstants::kConstructorOffset));
  __ SmiUntag(rax, Operand(rbp, ConstructFrameConstants::kLengthOffset));

  // Check if we have enough stack space to push all arguments.
  // Argument count in rax. Clobbers rcx.
  Label stack_overflow;
  __ StackOverflowCheck(rax, rcx, &stack_overflow);

  // TODO(victorgomes): When the arguments adaptor is completely removed, we
  // should get the formal parameter count and copy the arguments in its
  // correct position (including any undefined), instead of delaying this to
  // InvokeFunction.

  // Copy arguments to the expression stack.
  __ PushArray(rbx, rax, rcx);

  // Push implicit receiver.
  __ Push(r8);

  // Call the function.
  __ InvokeFunction(rdi, rdx, rax, InvokeType::kCall);

  // ----------- S t a t e -------------
  //  -- rax                 constructor result
  //  -- sp[0*kSystemPointerSize]  implicit receiver
  //  -- sp[1*kSystemPointerSize]  padding
  //  -- sp[2*kSystemPointerSize]  constructor function
  //  -- sp[3*kSystemPointerSize]  number of arguments
  //  -- sp[4*kSystemPointerSize]  context
  // -----------------------------------

  // Store offset of return address for deoptimizer.
  masm->isolate()->heap()->SetConstructStubInvokeDeoptPCOffset(
      masm->pc_offset());

  // If the result is an object (in the ECMA sense), we should get rid
  // of the receiver and use the result; see ECMA-262 section 13.2.2-7
  // on page 74.
  Label use_receiver, do_throw, leave_and_return, check_result;

  // If the result is undefined, we'll use the implicit receiver. Otherwise we
  // do a smi check and fall through to check if the return value is a valid
  // receiver.
  __ JumpIfNotRoot(rax, RootIndex::kUndefinedValue, &check_result,
                   Label::kNear);

  // Throw away the result of the constructor invocation and use the
  // on-stack receiver as the result.
  __ bind(&use_receiver);
  __ movq(rax, Operand(rsp, 0 * kSystemPointerSize));
  __ JumpIfRoot(rax, RootIndex::kTheHoleValue, &do_throw, Label::kNear);

  __ bind(&leave_and_return);
  // Restore the arguments count.
  __ movq(rbx, Operand(rbp, ConstructFrameConstants::kLengthOffset));
  __ LeaveFrame(StackFrame::CONSTRUCT);
  // Remove caller arguments from the stack and return.
  __ PopReturnAddressTo(rcx);
  SmiIndex index = masm->SmiToIndex(rbx, rbx, kSystemPointerSizeLog2);
  __ leaq(rsp, Operand(rsp, index.reg, index.scale, 1 * kSystemPointerSize));
  __ PushReturnAddressFrom(rcx);
  __ ret(0);

  // If the result is a smi, it is *not* an object in the ECMA sense.
  __ bind(&check_result);
  __ JumpIfSmi(rax, &use_receiver, Label::kNear);

  // If the type of the result (stored in its map) is less than
  // FIRST_JS_RECEIVER_TYPE, it is not an object in the ECMA sense.
  STATIC_ASSERT(LAST_JS_RECEIVER_TYPE == LAST_TYPE);
  __ CmpObjectType(rax, FIRST_JS_RECEIVER_TYPE, rcx);
  __ j(above_equal, &leave_and_return, Label::kNear);
  __ jmp(&use_receiver);

  __ bind(&do_throw);
  // Restore context from the frame.
  __ movq(rsi, Operand(rbp, ConstructFrameConstants::kContextOffset));
  __ CallRuntime(Runtime::kThrowConstructorReturnedNonObject);
  // We don't return here.
  __ int3();

  __ bind(&stack_overflow);
  // Restore the context from the frame.
  __ movq(rsi, Operand(rbp, ConstructFrameConstants::kContextOffset));
  __ CallRuntime(Runtime::kThrowStackOverflow);
  // This should be unreachable.
  __ int3();
}

void Builtins::Generate_JSBuiltinsConstructStub(MacroAssembler* masm) {
  Generate_JSBuiltinsConstructStubHelper(masm);
}

void Builtins::Generate_ConstructedNonConstructable(MacroAssembler* masm) {
  FrameScope scope(masm, StackFrame::INTERNAL);
  __ Push(rdi);
  __ CallRuntime(Runtime::kThrowConstructedNonConstructable);
}

namespace {

// Called with the native C calling convention. The corresponding function
// signature is either:
//   using JSEntryFunction = GeneratedCode<Address(
//       Address root_register_value, Address new_target, Address target,
//       Address receiver, intptr_t argc, Address** argv)>;
// or
//   using JSEntryFunction = GeneratedCode<Address(
//       Address root_register_value, MicrotaskQueue* microtask_queue)>;
void Generate_JSEntryVariant(MacroAssembler* masm, StackFrame::Type type,
                             Builtins::Name entry_trampoline) {
  Label invoke, handler_entry, exit;
  Label not_outermost_js, not_outermost_js_2;

  {  // NOLINT. Scope block confuses linter.
    NoRootArrayScope uninitialized_root_register(masm);
    // Set up frame.
    __ pushq(rbp);
    __ movq(rbp, rsp);

    // Push the stack frame type.
    __ Push(Immediate(StackFrame::TypeToMarker(type)));
    // Reserve a slot for the context. It is filled after the root register has
    // been set up.
    __ AllocateStackSpace(kSystemPointerSize);
    // Save callee-saved registers (X64/X32/Win64 calling conventions).
    __ pushq(r12);
    __ pushq(r13);
    __ pushq(r14);
    __ pushq(r15);
#ifdef V8_TARGET_OS_WIN
    __ pushq(rdi);  // Only callee save in Win64 ABI, argument in AMD64 ABI.
    __ pushq(rsi);  // Only callee save in Win64 ABI, argument in AMD64 ABI.
#endif
    __ pushq(rbx);

#ifdef V8_TARGET_OS_WIN
    // On Win64 XMM6-XMM15 are callee-save.
    __ AllocateStackSpace(EntryFrameConstants::kXMMRegistersBlockSize);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 0), xmm6);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 1), xmm7);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 2), xmm8);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 3), xmm9);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 4), xmm10);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 5), xmm11);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 6), xmm12);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 7), xmm13);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 8), xmm14);
    __ movdqu(Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 9), xmm15);
    STATIC_ASSERT(EntryFrameConstants::kCalleeSaveXMMRegisters == 10);
    STATIC_ASSERT(EntryFrameConstants::kXMMRegistersBlockSize ==
                  EntryFrameConstants::kXMMRegisterSize *
                      EntryFrameConstants::kCalleeSaveXMMRegisters);
#endif

    // Initialize the root register.
    // C calling convention. The first argument is passed in arg_reg_1.
    __ movq(kRootRegister, arg_reg_1);

#ifdef V8_COMPRESS_POINTERS_IN_SHARED_CAGE
    // Initialize the pointer cage base register.
    __ LoadRootRelative(kPtrComprCageBaseRegister,
                        IsolateData::cage_base_offset());
#endif
  }

  // Save copies of the top frame descriptor on the stack.
  ExternalReference c_entry_fp = ExternalReference::Create(
      IsolateAddressId::kCEntryFPAddress, masm->isolate());
  {
    Operand c_entry_fp_operand = masm->ExternalReferenceAsOperand(c_entry_fp);
    __ Push(c_entry_fp_operand);

    // Clear c_entry_fp, now we've pushed its previous value to the stack.
    // If the c_entry_fp is not already zero and we don't clear it, the
    // SafeStackFrameIterator will assume we are executing C++ and miss the JS
    // frames on top.
    __ movq(c_entry_fp_operand, Immediate(0));
  }

  // Store the context address in the previously-reserved slot.
  ExternalReference context_address = ExternalReference::Create(
      IsolateAddressId::kContextAddress, masm->isolate());
  __ Load(kScratchRegister, context_address);
  static constexpr int kOffsetToContextSlot = -2 * kSystemPointerSize;
  __ movq(Operand(rbp, kOffsetToContextSlot), kScratchRegister);

  // If this is the outermost JS call, set js_entry_sp value.
  ExternalReference js_entry_sp = ExternalReference::Create(
      IsolateAddressId::kJSEntrySPAddress, masm->isolate());
  __ Load(rax, js_entry_sp);
  __ testq(rax, rax);
  __ j(not_zero, &not_outermost_js);
  __ Push(Immediate(StackFrame::OUTERMOST_JSENTRY_FRAME));
  __ movq(rax, rbp);
  __ Store(js_entry_sp, rax);
  Label cont;
  __ jmp(&cont);
  __ bind(&not_outermost_js);
  __ Push(Immediate(StackFrame::INNER_JSENTRY_FRAME));
  __ bind(&cont);

  // Jump to a faked try block that does the invoke, with a faked catch
  // block that sets the pending exception.
  __ jmp(&invoke);
  __ bind(&handler_entry);

  // Store the current pc as the handler offset. It's used later to create the
  // handler table.
  masm->isolate()->builtins()->SetJSEntryHandlerOffset(handler_entry.pos());

  // Caught exception: Store result (exception) in the pending exception
  // field in the JSEnv and return a failure sentinel.
  ExternalReference pending_exception = ExternalReference::Create(
      IsolateAddressId::kPendingExceptionAddress, masm->isolate());
  __ Store(pending_exception, rax);
  __ LoadRoot(rax, RootIndex::kException);
  __ jmp(&exit);

  // Invoke: Link this frame into the handler chain.
  __ bind(&invoke);
  __ PushStackHandler();

  // Invoke the function by calling through JS entry trampoline builtin and
  // pop the faked function when we return.
  Handle<Code> trampoline_code =
      masm->isolate()->builtins()->builtin_handle(entry_trampoline);
  __ Call(trampoline_code, RelocInfo::CODE_TARGET);

  // Unlink this frame from the handler chain.
  __ PopStackHandler();

  __ bind(&exit);
  // Check if the current stack frame is marked as the outermost JS frame.
  __ Pop(rbx);
  __ cmpq(rbx, Immediate(StackFrame::OUTERMOST_JSENTRY_FRAME));
  __ j(not_equal, &not_outermost_js_2);
  __ Move(kScratchRegister, js_entry_sp);
  __ movq(Operand(kScratchRegister, 0), Immediate(0));
  __ bind(&not_outermost_js_2);

  // Restore the top frame descriptor from the stack.
  {
    Operand c_entry_fp_operand = masm->ExternalReferenceAsOperand(c_entry_fp);
    __ Pop(c_entry_fp_operand);
  }

  // Restore callee-saved registers (X64 conventions).
#ifdef V8_TARGET_OS_WIN
  // On Win64 XMM6-XMM15 are callee-save
  __ movdqu(xmm6, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 0));
  __ movdqu(xmm7, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 1));
  __ movdqu(xmm8, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 2));
  __ movdqu(xmm9, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 3));
  __ movdqu(xmm10, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 4));
  __ movdqu(xmm11, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 5));
  __ movdqu(xmm12, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 6));
  __ movdqu(xmm13, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 7));
  __ movdqu(xmm14, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 8));
  __ movdqu(xmm15, Operand(rsp, EntryFrameConstants::kXMMRegisterSize * 9));
  __ addq(rsp, Immediate(EntryFrameConstants::kXMMRegistersBlockSize));
#endif

  __ popq(rbx);
#ifdef V8_TARGET_OS_WIN
  // Callee save on in Win64 ABI, arguments/volatile in AMD64 ABI.
  __ popq(rsi);
  __ popq(rdi);
#endif
  __ popq(r15);
  __ popq(r14);
  __ popq(r13);
  __ popq(r12);
  __ addq(rsp, Immediate(2 * kSystemPointerSize));  // remove markers

  // Restore frame pointer and return.
  __ popq(rbp);
  __ ret(0);
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
  // Expects six C++ function parameters.
  // - Address root_register_value
  // - Address new_target (tagged Object pointer)
  // - Address function (tagged JSFunction pointer)
  // - Address receiver (tagged Object pointer)
  // - intptr_t argc
  // - Address** argv (pointer to array of tagged Object pointers)
  // (see Handle::Invoke in execution.cc).

  // Open a C++ scope for the FrameScope.
  {
    // Platform specific argument handling. After this, the stack contains
    // an internal frame and the pushed function and receiver, and
    // register rax and rbx holds the argument count and argument array,
    // while rdi holds the function pointer, rsi the context, and rdx the
    // new.target.

    // MSVC parameters in:
    // rcx        : root_register_value
    // rdx        : new_target
    // r8         : function
    // r9         : receiver
    // [rsp+0x20] : argc
    // [rsp+0x28] : argv
    //
    // GCC parameters in:
    // rdi : root_register_value
    // rsi : new_target
    // rdx : function
    // rcx : receiver
    // r8  : argc
    // r9  : argv

    __ movq(rdi, arg_reg_3);
    __ Move(rdx, arg_reg_2);
    // rdi : function
    // rdx : new_target

    // Clear the context before we push it when entering the internal frame.
    __ Move(rsi, 0);

    // Enter an internal frame.
    FrameScope scope(masm, StackFrame::INTERNAL);

    // Setup the context (we need to use the caller context from the isolate).
    ExternalReference context_address = ExternalReference::Create(
        IsolateAddressId::kContextAddress, masm->isolate());
    __ movq(rsi, masm->ExternalReferenceAsOperand(context_address));

    // Push the function onto the stack.
    __ Push(rdi);

#ifdef V8_TARGET_OS_WIN
    // Load the previous frame pointer to access C arguments on stack
    __ movq(kScratchRegister, Operand(rbp, 0));
    // Load the number of arguments and setup pointer to the arguments.
    __ movq(rax, Operand(kScratchRegister, EntryFrameConstants::kArgcOffset));
    __ movq(rbx, Operand(kScratchRegister, EntryFrameConstants::kArgvOffset));
#else   // V8_TARGET_OS_WIN
    // Load the number of arguments and setup pointer to the arguments.
    __ movq(rax, r8);
    __ movq(rbx, r9);
    __ movq(r9, arg_reg_4);  // Temporarily saving the receiver.
#endif  // V8_TARGET_OS_WIN

    // Current stack contents:
    // [rsp + kSystemPointerSize]     : Internal frame
    // [rsp]                          : function
    // Current register contents:
    // rax : argc
    // rbx : argv
    // rsi : context
    // rdi : function
    // rdx : new.target
    // r9  : receiver

    // Check if we have enough stack space to push all arguments.
    // Argument count in rax. Clobbers rcx.
    Label enough_stack_space, stack_overflow;
    __ StackOverflowCheck(rax, rcx, &stack_overflow, Label::kNear);
    __ jmp(&enough_stack_space, Label::kNear);

    __ bind(&stack_overflow);
    __ CallRuntime(Runtime::kThrowStackOverflow);
    // This should be unreachable.
    __ int3();

    __ bind(&enough_stack_space);

    // Copy arguments to the stack in a loop.
    // Register rbx points to array of pointers to handle locations.
    // Push the values of these handles.
    Label loop, entry;
    __ movq(rcx, rax);
    __ jmp(&entry, Label::kNear);
    __ bind(&loop);
    __ movq(kScratchRegister, Operand(rbx, rcx, times_system_pointer_size, 0));
    __ Push(Operand(kScratchRegister, 0));  // dereference handle
    __ bind(&entry);
    __ decq(rcx);
    __ j(greater_equal, &loop, Label::kNear);

    // Push the receiver.
    __ Push(r9);

    // Invoke the builtin code.
    Handle<Code> builtin = is_construct
                               ? BUILTIN_CODE(masm->isolate(), Construct)
                               : masm->isolate()->builtins()->Call();
    __ Call(builtin, RelocInfo::CODE_TARGET);

    // Exit the internal frame. Notice that this also removes the empty
    // context and the function left on the stack by the code
    // invocation.
  }

  __ ret(0);
}

void Builtins::Generate_JSEntryTrampoline(MacroAssembler* masm) {
  Generate_JSEntryTrampolineHelper(masm, false);
}

void Builtins::Generate_JSConstructEntryTrampoline(MacroAssembler* masm) {
  Generate_JSEntryTrampolineHelper(masm, true);
}

void Builtins::Generate_RunMicrotasksTrampoline(MacroAssembler* masm) {
  // arg_reg_2: microtask_queue
  __ movq(RunMicrotasksDescriptor::MicrotaskQueueRegister(), arg_reg_2);
  __ Jump(BUILTIN_CODE(masm->isolate(), RunMicrotasks), RelocInfo::CODE_TARGET);
}

static void GetSharedFunctionInfoBytecodeOrBaseline(MacroAssembler* masm,
                                                    Register sfi_data,
                                                    Register scratch1,
                                                    Label* is_baseline) {
  Label done;

  __ LoadMap(scratch1, sfi_data);

  __ CmpInstanceType(scratch1, BASELINE_DATA_TYPE);
  __ j(equal, is_baseline);

  __ CmpInstanceType(scratch1, INTERPRETER_DATA_TYPE);
  __ j(not_equal, &done, Label::kNear);

  __ LoadTaggedPointerField(
      sfi_data, FieldOperand(sfi_data, InterpreterData::kBytecodeArrayOffset));

  __ bind(&done);
}

// static
void Builtins::Generate_ResumeGeneratorTrampoline(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax    : the value to pass to the generator
  //  -- rdx    : the JSGeneratorObject to resume
  //  -- rsp[0] : return address
  // -----------------------------------
  __ AssertGeneratorObject(rdx);

  // Store input value into generator object.
  __ StoreTaggedField(
      FieldOperand(rdx, JSGeneratorObject::kInputOrDebugPosOffset), rax);
  __ RecordWriteField(rdx, JSGeneratorObject::kInputOrDebugPosOffset, rax, rcx,
                      SaveFPRegsMode::kIgnore);

  Register decompr_scratch1 = COMPRESS_POINTERS_BOOL ? r8 : no_reg;

  // Load suspended function and context.
  __ LoadTaggedPointerField(
      rdi, FieldOperand(rdx, JSGeneratorObject::kFunctionOffset));
  __ LoadTaggedPointerField(rsi, FieldOperand(rdi, JSFunction::kContextOffset));

  // Flood function if we are stepping.
  Label prepare_step_in_if_stepping, prepare_step_in_suspended_generator;
  Label stepping_prepared;
  ExternalReference debug_hook =
      ExternalReference::debug_hook_on_function_call_address(masm->isolate());
  Operand debug_hook_operand = masm->ExternalReferenceAsOperand(debug_hook);
  __ cmpb(debug_hook_operand, Immediate(0));
  __ j(not_equal, &prepare_step_in_if_stepping);

  // Flood function if we need to continue stepping in the suspended generator.
  ExternalReference debug_suspended_generator =
      ExternalReference::debug_suspended_generator_address(masm->isolate());
  Operand debug_suspended_generator_operand =
      masm->ExternalReferenceAsOperand(debug_suspended_generator);
  __ cmpq(rdx, debug_suspended_generator_operand);
  __ j(equal, &prepare_step_in_suspended_generator);
  __ bind(&stepping_prepared);

  // Check the stack for overflow. We are not trying to catch interruptions
  // (i.e. debug break and preemption) here, so check the "real stack limit".
  Label stack_overflow;
  __ cmpq(rsp, __ StackLimitAsOperand(StackLimitKind::kRealStackLimit));
  __ j(below, &stack_overflow);

  // Pop return address.
  __ PopReturnAddressTo(rax);

  // ----------- S t a t e -------------
  //  -- rax    : return address
  //  -- rdx    : the JSGeneratorObject to resume
  //  -- rdi    : generator function
  //  -- rsi    : generator context
  // -----------------------------------

  // Copy the function arguments from the generator object's register file.
  __ LoadTaggedPointerField(
      rcx, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
  __ movzxwq(
      rcx, FieldOperand(rcx, SharedFunctionInfo::kFormalParameterCountOffset));

  __ LoadTaggedPointerField(
      rbx, FieldOperand(rdx, JSGeneratorObject::kParametersAndRegistersOffset));

  {
    Label done_loop, loop;
    __ bind(&loop);
    __ decq(rcx);
    __ j(less, &done_loop, Label::kNear);
    __ PushTaggedAnyField(
        FieldOperand(rbx, rcx, times_tagged_size, FixedArray::kHeaderSize),
        decompr_scratch1);
    __ jmp(&loop);
    __ bind(&done_loop);

    // Push the receiver.
    __ PushTaggedPointerField(
        FieldOperand(rdx, JSGeneratorObject::kReceiverOffset),
        decompr_scratch1);
  }

  // Underlying function needs to have bytecode available.
  if (FLAG_debug_code) {
    Label is_baseline, ok;
    __ LoadTaggedPointerField(
        rcx, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
    __ LoadTaggedPointerField(
        rcx, FieldOperand(rcx, SharedFunctionInfo::kFunctionDataOffset));
    GetSharedFunctionInfoBytecodeOrBaseline(masm, rcx, kScratchRegister,
                                            &is_baseline);
    __ CmpObjectType(rcx, BYTECODE_ARRAY_TYPE, rcx);
    __ Assert(equal, AbortReason::kMissingBytecodeArray);
    __ jmp(&ok);

    __ bind(&is_baseline);
    __ CmpObjectType(rcx, BASELINE_DATA_TYPE, rcx);
    __ Assert(equal, AbortReason::kMissingBytecodeArray);

    __ bind(&ok);
  }

  // Resume (Ignition/TurboFan) generator object.
  {
    __ PushReturnAddressFrom(rax);
    __ LoadTaggedPointerField(
        rax, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
    __ movzxwq(rax, FieldOperand(
                        rax, SharedFunctionInfo::kFormalParameterCountOffset));
    // We abuse new.target both to indicate that this is a resume call and to
    // pass in the generator object.  In ordinary calls, new.target is always
    // undefined because generator functions are non-constructable.
    static_assert(kJavaScriptCallCodeStartRegister == rcx, "ABI mismatch");
    __ LoadTaggedPointerField(rcx, FieldOperand(rdi, JSFunction::kCodeOffset));
    __ JumpCodeObject(rcx);
  }

  __ bind(&prepare_step_in_if_stepping);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ Push(rdx);
    __ Push(rdi);
    // Push hole as receiver since we do not use it for stepping.
    __ PushRoot(RootIndex::kTheHoleValue);
    __ CallRuntime(Runtime::kDebugOnFunctionCall);
    __ Pop(rdx);
    __ LoadTaggedPointerField(
        rdi, FieldOperand(rdx, JSGeneratorObject::kFunctionOffset));
  }
  __ jmp(&stepping_prepared);

  __ bind(&prepare_step_in_suspended_generator);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ Push(rdx);
    __ CallRuntime(Runtime::kDebugPrepareStepInSuspendedGenerator);
    __ Pop(rdx);
    __ LoadTaggedPointerField(
        rdi, FieldOperand(rdx, JSGeneratorObject::kFunctionOffset));
  }
  __ jmp(&stepping_prepared);

  __ bind(&stack_overflow);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kThrowStackOverflow);
    __ int3();  // This should be unreachable.
  }
}

// TODO(juliana): if we remove the code below then we don't need all
// the parameters.
static void ReplaceClosureCodeWithOptimizedCode(MacroAssembler* masm,
                                                Register optimized_code,
                                                Register closure,
                                                Register scratch1,
                                                Register scratch2) {
  // Store the optimized code in the closure.
  __ StoreTaggedField(FieldOperand(closure, JSFunction::kCodeOffset),
                      optimized_code);
  __ movq(scratch1, optimized_code);  // Write barrier clobbers scratch1 below.
  __ RecordWriteField(closure, JSFunction::kCodeOffset, scratch1, scratch2,
                      SaveFPRegsMode::kIgnore, RememberedSetAction::kOmit,
                      SmiCheck::kOmit);
}

static void LeaveInterpreterFrame(MacroAssembler* masm, Register scratch1,
                                  Register scratch2) {
  Register params_size = scratch1;
  // Get the size of the formal parameters + receiver (in bytes).
  __ movq(params_size,
          Operand(rbp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ movl(params_size,
          FieldOperand(params_size, BytecodeArray::kParameterSizeOffset));

  Register actual_params_size = scratch2;
  // Compute the size of the actual parameters + receiver (in bytes).
  __ movq(actual_params_size,
          Operand(rbp, StandardFrameConstants::kArgCOffset));
  __ leaq(actual_params_size,
          Operand(actual_params_size, times_system_pointer_size,
                  kSystemPointerSize));

  // If actual is bigger than formal, then we should use it to free up the stack
  // arguments.
  Label corrected_args_count;
  __ cmpq(params_size, actual_params_size);
  __ j(greater_equal, &corrected_args_count, Label::kNear);
  __ movq(params_size, actual_params_size);
  __ bind(&corrected_args_count);

  // Leave the frame (also dropping the register file).
  __ leave();

  // Drop receiver + arguments.
  Register return_pc = scratch2;
  __ PopReturnAddressTo(return_pc);
  __ addq(rsp, params_size);
  __ PushReturnAddressFrom(return_pc);
}

// Tail-call |function_id| if |actual_marker| == |expected_marker|
static void TailCallRuntimeIfMarkerEquals(MacroAssembler* masm,
                                          Register actual_marker,
                                          OptimizationMarker expected_marker,
                                          Runtime::FunctionId function_id) {
  Label no_match;
  __ Cmp(actual_marker, expected_marker);
  __ j(not_equal, &no_match);
  GenerateTailCallToReturnedCode(masm, function_id);
  __ bind(&no_match);
}

static void MaybeOptimizeCode(MacroAssembler* masm, Register feedback_vector,
                              Register optimization_marker) {
  // ----------- S t a t e -------------
  //  -- rax : actual argument count
  //  -- rdx : new target (preserved for callee if needed, and caller)
  //  -- rdi : target function (preserved for callee if needed, and caller)
  //  -- feedback vector (preserved for caller if needed)
  //  -- optimization_marker : a Smi containing a non-zero optimization marker.
  // -----------------------------------

  DCHECK(!AreAliased(feedback_vector, rdx, rdi, optimization_marker));

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
    __ int3();
  }
}

static void TailCallOptimizedCodeSlot(MacroAssembler* masm,
                                      Register optimized_code_entry,
                                      Register scratch1, Register scratch2,
                                      JumpMode jump_mode) {
  // ----------- S t a t e -------------
  //  -- rax : actual argument count
  //  -- rdx : new target (preserved for callee if needed, and caller)
  //  -- rdi : target function (preserved for callee if needed, and caller)
  // -----------------------------------

  Register closure = rdi;

  Label heal_optimized_code_slot;

  // If the optimized code is cleared, go to runtime to update the optimization
  // marker field.
  __ LoadWeakValue(optimized_code_entry, &heal_optimized_code_slot);

  // Check if the optimized code is marked for deopt. If it is, call the
  // runtime to clear it.
  __ LoadTaggedPointerField(
      scratch1,
      FieldOperand(optimized_code_entry, Code::kCodeDataContainerOffset));
  __ testl(FieldOperand(scratch1, CodeDataContainer::kKindSpecificFlagsOffset),
           Immediate(1 << Code::kMarkedForDeoptimizationBit));
  __ j(not_zero, &heal_optimized_code_slot);

  // Optimized code is good, get it into the closure and link the closure into
  // the optimized functions list, then tail call the optimized code.
  ReplaceClosureCodeWithOptimizedCode(masm, optimized_code_entry, closure,
                                      scratch1, scratch2);
  static_assert(kJavaScriptCallCodeStartRegister == rcx, "ABI mismatch");
  __ Move(rcx, optimized_code_entry);
  __ JumpCodeObject(rcx, jump_mode);

  // Optimized code slot contains deoptimized code or code is cleared and
  // optimized code marker isn't updated. Evict the code, update the marker
  // and re-enter the closure's code.
  __ bind(&heal_optimized_code_slot);
  GenerateTailCallToReturnedCode(masm, Runtime::kHealOptimizedCodeSlot,
                                 jump_mode);
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
  DCHECK(!AreAliased(bytecode_array, bytecode_offset, bytecode,
                     bytecode_size_table, original_bytecode_offset));

  __ movq(original_bytecode_offset, bytecode_offset);

  __ Move(bytecode_size_table,
          ExternalReference::bytecode_size_table_address());

  // Check if the bytecode is a Wide or ExtraWide prefix bytecode.
  Label process_bytecode, extra_wide;
  STATIC_ASSERT(0 == static_cast<int>(interpreter::Bytecode::kWide));
  STATIC_ASSERT(1 == static_cast<int>(interpreter::Bytecode::kExtraWide));
  STATIC_ASSERT(2 == static_cast<int>(interpreter::Bytecode::kDebugBreakWide));
  STATIC_ASSERT(3 ==
                static_cast<int>(interpreter::Bytecode::kDebugBreakExtraWide));
  __ cmpb(bytecode, Immediate(0x3));
  __ j(above, &process_bytecode, Label::kNear);
  // The code to load the next bytecode is common to both wide and extra wide.
  // We can hoist them up here. incl has to happen before testb since it
  // modifies the ZF flag.
  __ incl(bytecode_offset);
  __ testb(bytecode, Immediate(0x1));
  __ movzxbq(bytecode, Operand(bytecode_array, bytecode_offset, times_1, 0));
  __ j(not_equal, &extra_wide, Label::kNear);

  // Update table to the wide scaled table.
  __ addq(bytecode_size_table,
          Immediate(kByteSize * interpreter::Bytecodes::kBytecodeCount));
  __ jmp(&process_bytecode, Label::kNear);

  __ bind(&extra_wide);
  // Update table to the extra wide scaled table.
  __ addq(bytecode_size_table,
          Immediate(2 * kByteSize * interpreter::Bytecodes::kBytecodeCount));

  __ bind(&process_bytecode);

// Bailout to the return label if this is a return bytecode.
#define JUMP_IF_EQUAL(NAME)                                             \
  __ cmpb(bytecode,                                                     \
          Immediate(static_cast<int>(interpreter::Bytecode::k##NAME))); \
  __ j(equal, if_return, Label::kFar);
  RETURN_BYTECODE_LIST(JUMP_IF_EQUAL)
#undef JUMP_IF_EQUAL

  // If this is a JumpLoop, re-execute it to perform the jump to the beginning
  // of the loop.
  Label end, not_jump_loop;
  __ cmpb(bytecode,
          Immediate(static_cast<int>(interpreter::Bytecode::kJumpLoop)));
  __ j(not_equal, &not_jump_loop, Label::kNear);
  // We need to restore the original bytecode_offset since we might have
  // increased it to skip the wide / extra-wide prefix bytecode.
  __ movq(bytecode_offset, original_bytecode_offset);
  __ jmp(&end, Label::kNear);

  __ bind(&not_jump_loop);
  // Otherwise, load the size of the current bytecode and advance the offset.
  __ movzxbl(kScratchRegister,
             Operand(bytecode_size_table, bytecode, times_1, 0));
  __ addl(bytecode_offset, kScratchRegister);

  __ bind(&end);
}

// Read off the optimization state in the feedback vector and check if there
// is optimized code or a optimization marker that needs to be processed.
static void LoadOptimizationStateAndJumpIfNeedsProcessing(
    MacroAssembler* masm, Register optimization_state, Register feedback_vector,
    Label* has_optimized_code_or_marker) {
  __ RecordComment("[ Check optimization state");

  __ movl(optimization_state,
          FieldOperand(feedback_vector, FeedbackVector::kFlagsOffset));
  __ testl(
      optimization_state,
      Immediate(FeedbackVector::kHasOptimizedCodeOrCompileOptimizedMarkerMask));
  __ j(not_zero, has_optimized_code_or_marker);

  __ RecordComment("]");
}

static void MaybeOptimizeCodeOrTailCallOptimizedCodeSlot(
    MacroAssembler* masm, Register optimization_state, Register feedback_vector,
    JumpMode jump_mode = JumpMode::kJump) {
  Label maybe_has_optimized_code;
  __ testl(
      optimization_state,
      Immediate(FeedbackVector::kHasCompileOptimizedOrLogFirstExecutionMarker));
  __ j(zero, &maybe_has_optimized_code);

  Register optimization_marker = optimization_state;
  __ DecodeField<FeedbackVector::OptimizationMarkerBits>(optimization_marker);
  MaybeOptimizeCode(masm, feedback_vector, optimization_marker);

  __ bind(&maybe_has_optimized_code);
  Register optimized_code_entry = optimization_state;
  __ LoadAnyTaggedField(
      optimized_code_entry,
      FieldOperand(feedback_vector, FeedbackVector::kMaybeOptimizedCodeOffset));
  TailCallOptimizedCodeSlot(masm, optimized_code_entry, r8, r15, jump_mode);
}

// Generate code for entering a JS function with the interpreter.
// On entry to the function the receiver and arguments have been pushed on the
// stack left to right.
//
// The live registers are:
//   o rax: actual argument count (not including the receiver)
//   o rdi: the JS function object being called
//   o rdx: the incoming new target or generator object
//   o rsi: our context
//   o rbp: the caller's frame pointer
//   o rsp: stack pointer (pointing to return address)
//
// The function builds an interpreter frame.  See InterpreterFrameConstants in
// frames.h for its layout.
void Builtins::Generate_InterpreterEntryTrampoline(MacroAssembler* masm) {
  Register closure = rdi;
  Register feedback_vector = rbx;

  // Get the bytecode array from the function object and load it into
  // kInterpreterBytecodeArrayRegister.
  __ LoadTaggedPointerField(
      kScratchRegister,
      FieldOperand(closure, JSFunction::kSharedFunctionInfoOffset));
  __ LoadTaggedPointerField(
      kInterpreterBytecodeArrayRegister,
      FieldOperand(kScratchRegister, SharedFunctionInfo::kFunctionDataOffset));

  Label is_baseline;
  GetSharedFunctionInfoBytecodeOrBaseline(
      masm, kInterpreterBytecodeArrayRegister, kScratchRegister, &is_baseline);

  // The bytecode array could have been flushed from the shared function info,
  // if so, call into CompileLazy.
  Label compile_lazy;
  __ CmpObjectType(kInterpreterBytecodeArrayRegister, BYTECODE_ARRAY_TYPE,
                   kScratchRegister);
  __ j(not_equal, &compile_lazy);

  // Load the feedback vector from the closure.
  __ LoadTaggedPointerField(
      feedback_vector, FieldOperand(closure, JSFunction::kFeedbackCellOffset));
  __ LoadTaggedPointerField(feedback_vector,
                            FieldOperand(feedback_vector, Cell::kValueOffset));

  Label push_stack_frame;
  // Check if feedback vector is valid. If valid, check for optimized code
  // and update invocation count. Otherwise, setup the stack frame.
  __ LoadMap(rcx, feedback_vector);
  __ CmpInstanceType(rcx, FEEDBACK_VECTOR_TYPE);
  __ j(not_equal, &push_stack_frame);

  // Check for an optimization marker.
  Label has_optimized_code_or_marker;
  Register optimization_state = rcx;
  LoadOptimizationStateAndJumpIfNeedsProcessing(
      masm, optimization_state, feedback_vector, &has_optimized_code_or_marker);

  Label not_optimized;
  __ bind(&not_optimized);

  // Increment invocation count for the function.
  __ incl(
      FieldOperand(feedback_vector, FeedbackVector::kInvocationCountOffset));

  // Open a frame scope to indicate that there is a frame on the stack.  The
  // MANUAL indicates that the scope shouldn't actually generate code to set up
  // the frame (that is done below).
  __ bind(&push_stack_frame);
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ pushq(rbp);  // Caller's frame pointer.
  __ movq(rbp, rsp);
  __ Push(kContextRegister);                 // Callee's context.
  __ Push(kJavaScriptCallTargetRegister);    // Callee's JS function.
  __ Push(kJavaScriptCallArgCountRegister);  // Actual argument count.

  // Reset code age and the OSR arming. The OSR field and BytecodeAgeOffset are
  // 8-bit fields next to each other, so we could just optimize by writing a
  // 16-bit. These static asserts guard our assumption is valid.
  STATIC_ASSERT(BytecodeArray::kBytecodeAgeOffset ==
                BytecodeArray::kOsrNestingLevelOffset + kCharSize);
  STATIC_ASSERT(BytecodeArray::kNoAgeBytecodeAge == 0);
  __ movw(FieldOperand(kInterpreterBytecodeArrayRegister,
                       BytecodeArray::kOsrNestingLevelOffset),
          Immediate(0));

  // Load initial bytecode offset.
  __ movq(kInterpreterBytecodeOffsetRegister,
          Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag));

  // Push bytecode array and Smi tagged bytecode offset.
  __ Push(kInterpreterBytecodeArrayRegister);
  __ SmiTag(rcx, kInterpreterBytecodeOffsetRegister);
  __ Push(rcx);

  // Allocate the local and temporary register file on the stack.
  Label stack_overflow;
  {
    // Load frame size from the BytecodeArray object.
    __ movl(rcx, FieldOperand(kInterpreterBytecodeArrayRegister,
                              BytecodeArray::kFrameSizeOffset));

    // Do a stack check to ensure we don't go over the limit.
    __ movq(rax, rsp);
    __ subq(rax, rcx);
    __ cmpq(rax, __ StackLimitAsOperand(StackLimitKind::kRealStackLimit));
    __ j(below, &stack_overflow);

    // If ok, push undefined as the initial value for all register file entries.
    Label loop_header;
    Label loop_check;
    __ LoadRoot(kInterpreterAccumulatorRegister, RootIndex::kUndefinedValue);
    __ j(always, &loop_check, Label::kNear);
    __ bind(&loop_header);
    // TODO(rmcilroy): Consider doing more than one push per loop iteration.
    __ Push(kInterpreterAccumulatorRegister);
    // Continue loop if not done.
    __ bind(&loop_check);
    __ subq(rcx, Immediate(kSystemPointerSize));
    __ j(greater_equal, &loop_header, Label::kNear);
  }

  // If the bytecode array has a valid incoming new target or generator object
  // register, initialize it with incoming value which was passed in rdx.
  Label no_incoming_new_target_or_generator_register;
  __ movsxlq(
      rcx,
      FieldOperand(kInterpreterBytecodeArrayRegister,
                   BytecodeArray::kIncomingNewTargetOrGeneratorRegisterOffset));
  __ testl(rcx, rcx);
  __ j(zero, &no_incoming_new_target_or_generator_register, Label::kNear);
  __ movq(Operand(rbp, rcx, times_system_pointer_size, 0), rdx);
  __ bind(&no_incoming_new_target_or_generator_register);

  // Perform interrupt stack check.
  // TODO(solanes): Merge with the real stack limit check above.
  Label stack_check_interrupt, after_stack_check_interrupt;
  __ cmpq(rsp, __ StackLimitAsOperand(StackLimitKind::kInterruptStackLimit));
  __ j(below, &stack_check_interrupt);
  __ bind(&after_stack_check_interrupt);

  // The accumulator is already loaded with undefined.

  // Load the dispatch table into a register and dispatch to the bytecode
  // handler at the current bytecode offset.
  Label do_dispatch;
  __ bind(&do_dispatch);
  __ Move(
      kInterpreterDispatchTableRegister,
      ExternalReference::interpreter_dispatch_table_address(masm->isolate()));
  __ movzxbq(kScratchRegister,
             Operand(kInterpreterBytecodeArrayRegister,
                     kInterpreterBytecodeOffsetRegister, times_1, 0));
  __ movq(kJavaScriptCallCodeStartRegister,
          Operand(kInterpreterDispatchTableRegister, kScratchRegister,
                  times_system_pointer_size, 0));
  __ call(kJavaScriptCallCodeStartRegister);
  masm->isolate()->heap()->SetInterpreterEntryReturnPCOffset(masm->pc_offset());

  // Any returns to the entry trampoline are either due to the return bytecode
  // or the interpreter tail calling a builtin and then a dispatch.

  // Get bytecode array and bytecode offset from the stack frame.
  __ movq(kInterpreterBytecodeArrayRegister,
          Operand(rbp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ SmiUntag(kInterpreterBytecodeOffsetRegister,
              Operand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp));

  // Either return, or advance to the next bytecode and dispatch.
  Label do_return;
  __ movzxbq(rbx, Operand(kInterpreterBytecodeArrayRegister,
                          kInterpreterBytecodeOffsetRegister, times_1, 0));
  AdvanceBytecodeOffsetOrReturn(masm, kInterpreterBytecodeArrayRegister,
                                kInterpreterBytecodeOffsetRegister, rbx, rcx,
                                r8, &do_return);
  __ jmp(&do_dispatch);

  __ bind(&do_return);
  // The return value is in rax.
  LeaveInterpreterFrame(masm, rbx, rcx);
  __ ret(0);

  __ bind(&stack_check_interrupt);
  // Modify the bytecode offset in the stack to be kFunctionEntryBytecodeOffset
  // for the call to the StackGuard.
  __ Move(Operand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp),
          Smi::FromInt(BytecodeArray::kHeaderSize - kHeapObjectTag +
                       kFunctionEntryBytecodeOffset));
  __ CallRuntime(Runtime::kStackGuard);

  // After the call, restore the bytecode array, bytecode offset and accumulator
  // registers again. Also, restore the bytecode offset in the stack to its
  // previous value.
  __ movq(kInterpreterBytecodeArrayRegister,
          Operand(rbp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ movq(kInterpreterBytecodeOffsetRegister,
          Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag));
  __ LoadRoot(kInterpreterAccumulatorRegister, RootIndex::kUndefinedValue);

  __ SmiTag(rcx, kInterpreterBytecodeArrayRegister);
  __ movq(Operand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp), rcx);

  __ jmp(&after_stack_check_interrupt);

  __ bind(&compile_lazy);
  GenerateTailCallToReturnedCode(masm, Runtime::kCompileLazy);
  __ int3();  // Should not return.

  __ bind(&has_optimized_code_or_marker);
  MaybeOptimizeCodeOrTailCallOptimizedCodeSlot(masm, optimization_state,
                                               feedback_vector);

  __ bind(&is_baseline);
  {
    // Load the feedback vector from the closure.
    __ LoadTaggedPointerField(
        feedback_vector,
        FieldOperand(closure, JSFunction::kFeedbackCellOffset));
    __ LoadTaggedPointerField(
        feedback_vector, FieldOperand(feedback_vector, Cell::kValueOffset));

    Label install_baseline_code;
    // Check if feedback vector is valid. If not, call prepare for baseline to
    // allocate it.
    __ LoadMap(rcx, feedback_vector);
    __ CmpInstanceType(rcx, FEEDBACK_VECTOR_TYPE);
    __ j(not_equal, &install_baseline_code);

    // Check for an optimization marker.
    LoadOptimizationStateAndJumpIfNeedsProcessing(
        masm, optimization_state, feedback_vector,
        &has_optimized_code_or_marker);

    // Load the baseline code into the closure.
    __ LoadTaggedPointerField(rcx,
                              FieldOperand(kInterpreterBytecodeArrayRegister,
                                           BaselineData::kBaselineCodeOffset));
    static_assert(kJavaScriptCallCodeStartRegister == rcx, "ABI mismatch");
    ReplaceClosureCodeWithOptimizedCode(masm, rcx, closure,
                                        kInterpreterBytecodeArrayRegister,
                                        kInterpreterBytecodeOffsetRegister);
    __ JumpCodeObject(rcx);

    __ bind(&install_baseline_code);
    GenerateTailCallToReturnedCode(masm, Runtime::kInstallBaselineCode);
  }

  __ bind(&stack_overflow);
  __ CallRuntime(Runtime::kThrowStackOverflow);
  __ int3();  // Should not return.
}

static void Generate_InterpreterPushArgs(MacroAssembler* masm,
                                         Register num_args,
                                         Register start_address,
                                         Register scratch) {
  // Find the argument with lowest address.
  __ movq(scratch, num_args);
  __ negq(scratch);
  __ leaq(start_address,
          Operand(start_address, scratch, times_system_pointer_size,
                  kSystemPointerSize));
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
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rbx : the address of the first argument to be pushed. Subsequent
  //           arguments should be consecutive above this, in the same order as
  //           they are to be pushed onto the stack.
  //  -- rdi : the target to call (can be any Object).
  // -----------------------------------
  Label stack_overflow;

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // The spread argument should not be pushed.
    __ decl(rax);
  }

  __ leal(rcx, Operand(rax, 1));  // Add one for receiver.

  // Add a stack check before pushing arguments.
  __ StackOverflowCheck(rcx, rdx, &stack_overflow);

  // Pop return address to allow tail-call after pushing arguments.
  __ PopReturnAddressTo(kScratchRegister);

  if (receiver_mode == ConvertReceiverMode::kNullOrUndefined) {
    // Don't copy receiver.
    __ decq(rcx);
  }

  // rbx and rdx will be modified.
  Generate_InterpreterPushArgs(masm, rcx, rbx, rdx);

  // Push "undefined" as the receiver arg if we need to.
  if (receiver_mode == ConvertReceiverMode::kNullOrUndefined) {
    __ PushRoot(RootIndex::kUndefinedValue);
  }

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // Pass the spread in the register rbx.
    // rbx already points to the penultime argument, the spread
    // is below that.
    __ movq(rbx, Operand(rbx, -kSystemPointerSize));
  }

  // Call the target.
  __ PushReturnAddressFrom(kScratchRegister);  // Re-push return address.

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    __ Jump(BUILTIN_CODE(masm->isolate(), CallWithSpread),
            RelocInfo::CODE_TARGET);
  } else {
    __ Jump(masm->isolate()->builtins()->Call(receiver_mode),
            RelocInfo::CODE_TARGET);
  }

  // Throw stack overflow exception.
  __ bind(&stack_overflow);
  {
    __ TailCallRuntime(Runtime::kThrowStackOverflow);
    // This should be unreachable.
    __ int3();
  }
}

// static
void Builtins::Generate_InterpreterPushArgsThenConstructImpl(
    MacroAssembler* masm, InterpreterPushArgsMode mode) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the new target (either the same as the constructor or
  //           the JSFunction on which new was invoked initially)
  //  -- rdi : the constructor to call (can be any Object)
  //  -- rbx : the allocation site feedback if available, undefined otherwise
  //  -- rcx : the address of the first argument to be pushed. Subsequent
  //           arguments should be consecutive above this, in the same order as
  //           they are to be pushed onto the stack.
  // -----------------------------------
  Label stack_overflow;

  // Add a stack check before pushing arguments.
  __ StackOverflowCheck(rax, r8, &stack_overflow);

  // Pop return address to allow tail-call after pushing arguments.
  __ PopReturnAddressTo(kScratchRegister);

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // The spread argument should not be pushed.
    __ decl(rax);
  }

  // rcx and r8 will be modified.
  Generate_InterpreterPushArgs(masm, rax, rcx, r8);

  // Push slot for the receiver to be constructed.
  __ Push(Immediate(0));

  if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // Pass the spread in the register rbx.
    __ movq(rbx, Operand(rcx, -kSystemPointerSize));
    // Push return address in preparation for the tail-call.
    __ PushReturnAddressFrom(kScratchRegister);
  } else {
    __ PushReturnAddressFrom(kScratchRegister);
    __ AssertUndefinedOrAllocationSite(rbx);
  }

  if (mode == InterpreterPushArgsMode::kArrayFunction) {
    // Tail call to the array construct stub (still in the caller
    // context at this point).
    __ AssertFunction(rdi);
    // Jump to the constructor function (rax, rbx, rdx passed on).
    Handle<Code> code = BUILTIN_CODE(masm->isolate(), ArrayConstructorImpl);
    __ Jump(code, RelocInfo::CODE_TARGET);
  } else if (mode == InterpreterPushArgsMode::kWithFinalSpread) {
    // Call the constructor (rax, rdx, rdi passed on).
    __ Jump(BUILTIN_CODE(masm->isolate(), ConstructWithSpread),
            RelocInfo::CODE_TARGET);
  } else {
    DCHECK_EQ(InterpreterPushArgsMode::kOther, mode);
    // Call the constructor (rax, rdx, rdi passed on).
    __ Jump(BUILTIN_CODE(masm->isolate(), Construct), RelocInfo::CODE_TARGET);
  }

  // Throw stack overflow exception.
  __ bind(&stack_overflow);
  {
    __ TailCallRuntime(Runtime::kThrowStackOverflow);
    // This should be unreachable.
    __ int3();
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
  __ movq(rbx, Operand(rbp, StandardFrameConstants::kFunctionOffset));
  __ LoadTaggedPointerField(
      rbx, FieldOperand(rbx, JSFunction::kSharedFunctionInfoOffset));
  __ LoadTaggedPointerField(
      rbx, FieldOperand(rbx, SharedFunctionInfo::kFunctionDataOffset));
  __ CmpObjectType(rbx, INTERPRETER_DATA_TYPE, kScratchRegister);
  __ j(not_equal, &builtin_trampoline, Label::kNear);

  __ LoadTaggedPointerField(
      rbx, FieldOperand(rbx, InterpreterData::kInterpreterTrampolineOffset));
  __ addq(rbx, Immediate(Code::kHeaderSize - kHeapObjectTag));
  __ jmp(&trampoline_loaded, Label::kNear);

  __ bind(&builtin_trampoline);
  // TODO(jgruber): Replace this by a lookup in the builtin entry table.
  __ movq(rbx,
          __ ExternalReferenceAsOperand(
              ExternalReference::
                  address_of_interpreter_entry_trampoline_instruction_start(
                      masm->isolate()),
              kScratchRegister));

  __ bind(&trampoline_loaded);
  __ addq(rbx, Immediate(interpreter_entry_return_pc_offset.value()));
  __ Push(rbx);

  // Initialize dispatch table register.
  __ Move(
      kInterpreterDispatchTableRegister,
      ExternalReference::interpreter_dispatch_table_address(masm->isolate()));

  // Get the bytecode array pointer from the frame.
  __ movq(kInterpreterBytecodeArrayRegister,
          Operand(rbp, InterpreterFrameConstants::kBytecodeArrayFromFp));

  if (FLAG_debug_code) {
    // Check function data field is actually a BytecodeArray object.
    __ AssertNotSmi(kInterpreterBytecodeArrayRegister);
    __ CmpObjectType(kInterpreterBytecodeArrayRegister, BYTECODE_ARRAY_TYPE,
                     rbx);
    __ Assert(
        equal,
        AbortReason::kFunctionDataShouldBeBytecodeArrayOnInterpreterEntry);
  }

  // Get the target bytecode offset from the frame.
  __ SmiUntag(kInterpreterBytecodeOffsetRegister,
              Operand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp));

  if (FLAG_debug_code) {
    Label okay;
    __ cmpq(kInterpreterBytecodeOffsetRegister,
            Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag));
    __ j(greater_equal, &okay, Label::kNear);
    __ int3();
    __ bind(&okay);
  }

  // Dispatch to the target bytecode.
  __ movzxbq(kScratchRegister,
             Operand(kInterpreterBytecodeArrayRegister,
                     kInterpreterBytecodeOffsetRegister, times_1, 0));
  __ movq(kJavaScriptCallCodeStartRegister,
          Operand(kInterpreterDispatchTableRegister, kScratchRegister,
                  times_system_pointer_size, 0));
  __ jmp(kJavaScriptCallCodeStartRegister);
}

void Builtins::Generate_InterpreterEnterAtNextBytecode(MacroAssembler* masm) {
  // Get bytecode array and bytecode offset from the stack frame.
  __ movq(kInterpreterBytecodeArrayRegister,
          Operand(rbp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  __ SmiUntag(kInterpreterBytecodeOffsetRegister,
              Operand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp));

  Label enter_bytecode, function_entry_bytecode;
  __ cmpq(kInterpreterBytecodeOffsetRegister,
          Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag +
                    kFunctionEntryBytecodeOffset));
  __ j(equal, &function_entry_bytecode);

  // Load the current bytecode.
  __ movzxbq(rbx, Operand(kInterpreterBytecodeArrayRegister,
                          kInterpreterBytecodeOffsetRegister, times_1, 0));

  // Advance to the next bytecode.
  Label if_return;
  AdvanceBytecodeOffsetOrReturn(masm, kInterpreterBytecodeArrayRegister,
                                kInterpreterBytecodeOffsetRegister, rbx, rcx,
                                r8, &if_return);

  __ bind(&enter_bytecode);
  // Convert new bytecode offset to a Smi and save in the stackframe.
  __ SmiTag(kInterpreterBytecodeOffsetRegister);
  __ movq(Operand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp),
          kInterpreterBytecodeOffsetRegister);

  Generate_InterpreterEnterBytecode(masm);

  __ bind(&function_entry_bytecode);
  // If the code deoptimizes during the implicit function entry stack interrupt
  // check, it will have a bailout ID of kFunctionEntryBytecodeOffset, which is
  // not a valid bytecode offset. Detect this case and advance to the first
  // actual bytecode.
  __ movq(kInterpreterBytecodeOffsetRegister,
          Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag));
  __ jmp(&enter_bytecode);

  // We should never take the if_return path.
  __ bind(&if_return);
  __ Abort(AbortReason::kInvalidBytecodeAdvance);
}

void Builtins::Generate_InterpreterEnterAtBytecode(MacroAssembler* masm) {
  Generate_InterpreterEnterBytecode(masm);
}

// static
void Builtins::Generate_BaselineOutOfLinePrologue(MacroAssembler* masm) {
  Register feedback_vector = r8;
  Register optimization_state = rcx;
  Register return_address = r15;

#ifdef DEBUG
  for (auto reg : BaselineOutOfLinePrologueDescriptor::registers()) {
    DCHECK(
        !AreAliased(feedback_vector, optimization_state, return_address, reg));
  }
#endif

  auto descriptor = Builtins::CallInterfaceDescriptorFor(
      Builtins::kBaselineOutOfLinePrologue);
  Register closure = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kClosure);
  // Load the feedback vector from the closure.
  __ LoadTaggedPointerField(
      feedback_vector, FieldOperand(closure, JSFunction::kFeedbackCellOffset));
  __ LoadTaggedPointerField(feedback_vector,
                            FieldOperand(feedback_vector, Cell::kValueOffset));
  if (FLAG_debug_code) {
    __ CmpObjectType(feedback_vector, FEEDBACK_VECTOR_TYPE, kScratchRegister);
    __ Assert(equal, AbortReason::kExpectedFeedbackVector);
  }

  // Check for an optimization marker.
  Label has_optimized_code_or_marker;
  LoadOptimizationStateAndJumpIfNeedsProcessing(
      masm, optimization_state, feedback_vector, &has_optimized_code_or_marker);

  // Increment invocation count for the function.
  __ incl(
      FieldOperand(feedback_vector, FeedbackVector::kInvocationCountOffset));

  __ RecordComment("[ Frame Setup");
  // Save the return address, so that we can push it to the end of the newly
  // set-up frame once we're done setting it up.
  __ PopReturnAddressTo(return_address);
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterFrame(StackFrame::BASELINE);

  __ Push(descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kCalleeContext));  // Callee's
                                                              // context.
  Register callee_js_function = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kClosure);
  DCHECK_EQ(callee_js_function, kJavaScriptCallTargetRegister);
  DCHECK_EQ(callee_js_function, kJSFunctionRegister);
  __ Push(callee_js_function);  // Callee's JS function.
  __ Push(descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::
          kJavaScriptCallArgCount));  // Actual argument
                                      // count.

  // We'll use the bytecode for both code age/OSR resetting, and pushing onto
  // the frame, so load it into a register.
  Register bytecode_array = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kInterpreterBytecodeArray);

  // Reset code age and the OSR arming. The OSR field and BytecodeAgeOffset
  // are 8-bit fields next to each other, so we could just optimize by writing
  // a 16-bit. These static asserts guard our assumption is valid.
  STATIC_ASSERT(BytecodeArray::kBytecodeAgeOffset ==
                BytecodeArray::kOsrNestingLevelOffset + kCharSize);
  STATIC_ASSERT(BytecodeArray::kNoAgeBytecodeAge == 0);
  __ movw(FieldOperand(bytecode_array, BytecodeArray::kOsrNestingLevelOffset),
          Immediate(0));
  __ Push(bytecode_array);

  // Baseline code frames store the feedback vector where interpreter would
  // store the bytecode offset.
  __ Push(feedback_vector);

  __ RecordComment("]");

  Register new_target = descriptor.GetRegisterParameter(
      BaselineOutOfLinePrologueDescriptor::kJavaScriptCallNewTarget);

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
    //
    // TODO(v8:11429): Backport this folded check to the
    // InterpreterEntryTrampoline.
    __ Move(kScratchRegister, rsp);
    DCHECK_NE(frame_size, new_target);
    __ subq(kScratchRegister, frame_size);
    __ cmpq(kScratchRegister,
            __ StackLimitAsOperand(StackLimitKind::kInterruptStackLimit));
    __ j(below, &call_stack_guard);
    __ RecordComment("]");
  }

  // Push the return address back onto the stack for return.
  __ PushReturnAddressFrom(return_address);
  // Return to caller pushed pc, without any frame teardown.
  __ Ret();

  __ bind(&has_optimized_code_or_marker);
  {
    __ RecordComment("[ Optimized marker check");
    // Drop the return address, rebalancing the return stack buffer by using
    // JumpMode::kPushAndReturn. We can't leave the slot and overwrite it on
    // return since we may do a runtime call along the way that requires the
    // stack to only contain valid frames.
    __ Drop(1);
    MaybeOptimizeCodeOrTailCallOptimizedCodeSlot(
        masm, optimization_state, feedback_vector, JumpMode::kPushAndReturn);
    __ Trap();
    __ RecordComment("]");
  }

  __ bind(&call_stack_guard);
  {
    __ RecordComment("[ Stack/interrupt call");
    {
      // Push the baseline code return address now, as if it had been pushed by
      // the call to this builtin.
      __ PushReturnAddressFrom(return_address);
      FrameScope frame_scope(masm, StackFrame::INTERNAL);
      // Save incoming new target or generator
      __ Push(new_target);
      __ SmiTag(frame_size);
      __ Push(frame_size);
      __ CallRuntime(Runtime::kStackGuardWithGap, 1);
      __ Pop(new_target);
    }

    // Return to caller pushed pc, without any frame teardown.
    __ Ret();
    __ RecordComment("]");
  }
}

namespace {
void Generate_ContinueToBuiltinHelper(MacroAssembler* masm,
                                      bool java_script_builtin,
                                      bool with_result) {
  const RegisterConfiguration* config(RegisterConfiguration::Default());
  int allocatable_register_count = config->num_allocatable_general_registers();
  if (with_result) {
    if (java_script_builtin) {
      // kScratchRegister is not included in the allocateable registers.
      __ movq(kScratchRegister, rax);
    } else {
      // Overwrite the hole inserted by the deoptimizer with the return value
      // from the LAZY deopt point.
      __ movq(
          Operand(rsp, config->num_allocatable_general_registers() *
                               kSystemPointerSize +
                           BuiltinContinuationFrameConstants::kFixedFrameSize),
          rax);
    }
  }
  for (int i = allocatable_register_count - 1; i >= 0; --i) {
    int code = config->GetAllocatableGeneralCode(i);
    __ popq(Register::from_code(code));
    if (java_script_builtin && code == kJavaScriptCallArgCountRegister.code()) {
      __ SmiUntag(Register::from_code(code));
    }
  }
  if (with_result && java_script_builtin) {
    // Overwrite the hole inserted by the deoptimizer with the return value from
    // the LAZY deopt point. rax contains the arguments count, the return value
    // from LAZY is always the last argument.
    __ movq(Operand(rsp, rax, times_system_pointer_size,
                    BuiltinContinuationFrameConstants::kFixedFrameSize),
            kScratchRegister);
  }
  __ movq(
      rbp,
      Operand(rsp, BuiltinContinuationFrameConstants::kFixedFrameSizeFromFp));
  const int offsetToPC =
      BuiltinContinuationFrameConstants::kFixedFrameSizeFromFp -
      kSystemPointerSize;
  __ popq(Operand(rsp, offsetToPC));
  __ Drop(offsetToPC / kSystemPointerSize);

  // Replace the builtin index Smi on the stack with the instruction start
  // address of the builtin from the builtins table, and then Ret to this
  // address
  __ movq(kScratchRegister, Operand(rsp, 0));
  __ movq(kScratchRegister,
          __ EntryFromBuiltinIndexAsOperand(kScratchRegister));
  __ movq(Operand(rsp, 0), kScratchRegister);

  __ Ret();
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
  // Enter an internal frame.
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kNotifyDeoptimized);
    // Tear down internal frame.
  }

  DCHECK_EQ(kInterpreterAccumulatorRegister.code(), rax.code());
  __ movq(rax, Operand(rsp, kPCOnStackSize));
  __ ret(1 * kSystemPointerSize);  // Remove rax.
}

void Builtins::Generate_TailCallOptimizedCodeSlot(MacroAssembler* masm) {
  Register optimized_code_entry = kJavaScriptCallCodeStartRegister;
  TailCallOptimizedCodeSlot(masm, optimized_code_entry, r8, r15,
                            JumpMode::kJump);
}

// static
void Builtins::Generate_FunctionPrototypeApply(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax     : argc
  //  -- rsp[0]  : return address
  //  -- rsp[1]  : receiver
  //  -- rsp[2]  : thisArg
  //  -- rsp[3]  : argArray
  // -----------------------------------

  // 1. Load receiver into rdi, argArray into rbx (if present), remove all
  // arguments from the stack (including the receiver), and push thisArg (if
  // present) instead.
  {
    Label no_arg_array, no_this_arg;
    StackArgumentsAccessor args(rax);
    __ LoadRoot(rdx, RootIndex::kUndefinedValue);
    __ movq(rbx, rdx);
    __ movq(rdi, args[0]);
    __ testq(rax, rax);
    __ j(zero, &no_this_arg, Label::kNear);
    {
      __ movq(rdx, args[1]);
      __ cmpq(rax, Immediate(1));
      __ j(equal, &no_arg_array, Label::kNear);
      __ movq(rbx, args[2]);
      __ bind(&no_arg_array);
    }
    __ bind(&no_this_arg);
    __ PopReturnAddressTo(rcx);
    __ leaq(rsp,
            Operand(rsp, rax, times_system_pointer_size, kSystemPointerSize));
    __ Push(rdx);
    __ PushReturnAddressFrom(rcx);
  }

  // ----------- S t a t e -------------
  //  -- rbx     : argArray
  //  -- rdi     : receiver
  //  -- rsp[0]  : return address
  //  -- rsp[8]  : thisArg
  // -----------------------------------

  // 2. We don't need to check explicitly for callable receiver here,
  // since that's the first thing the Call/CallWithArrayLike builtins
  // will do.

  // 3. Tail call with no arguments if argArray is null or undefined.
  Label no_arguments;
  __ JumpIfRoot(rbx, RootIndex::kNullValue, &no_arguments, Label::kNear);
  __ JumpIfRoot(rbx, RootIndex::kUndefinedValue, &no_arguments, Label::kNear);

  // 4a. Apply the receiver to the given argArray.
  __ Jump(BUILTIN_CODE(masm->isolate(), CallWithArrayLike),
          RelocInfo::CODE_TARGET);

  // 4b. The argArray is either null or undefined, so we tail call without any
  // arguments to the receiver. Since we did not create a frame for
  // Function.prototype.apply() yet, we use a normal Call builtin here.
  __ bind(&no_arguments);
  {
    __ Move(rax, 0);
    __ Jump(masm->isolate()->builtins()->Call(), RelocInfo::CODE_TARGET);
  }
}

// static
void Builtins::Generate_FunctionPrototypeCall(MacroAssembler* masm) {
  // Stack Layout:
  // rsp[0]           : Return address
  // rsp[8]           : Argument 0 (receiver: callable to call)
  // rsp[16]          : Argument 1
  //  ...
  // rsp[8 * n]       : Argument n-1
  // rsp[8 * (n + 1)] : Argument n
  // rax contains the number of arguments, n, not counting the receiver.

  // 1. Get the callable to call (passed as receiver) from the stack.
  {
    StackArgumentsAccessor args(rax);
    __ movq(rdi, args.GetReceiverOperand());
  }

  // 2. Save the return address and drop the callable.
  __ PopReturnAddressTo(rbx);
  __ Pop(kScratchRegister);

  // 3. Make sure we have at least one argument.
  {
    Label done;
    __ testq(rax, rax);
    __ j(not_zero, &done, Label::kNear);
    __ PushRoot(RootIndex::kUndefinedValue);
    __ incq(rax);
    __ bind(&done);
  }

  // 4. Push back the return address one slot down on the stack (overwriting the
  // original callable), making the original first argument the new receiver.
  __ PushReturnAddressFrom(rbx);
  __ decq(rax);  // One fewer argument (first argument is new receiver).

  // 5. Call the callable.
  // Since we did not create a frame for Function.prototype.call() yet,
  // we use a normal Call builtin here.
  __ Jump(masm->isolate()->builtins()->Call(), RelocInfo::CODE_TARGET);
}

void Builtins::Generate_ReflectApply(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax     : argc
  //  -- rsp[0]  : return address
  //  -- rsp[8]  : receiver
  //  -- rsp[16] : target         (if argc >= 1)
  //  -- rsp[24] : thisArgument   (if argc >= 2)
  //  -- rsp[32] : argumentsList  (if argc == 3)
  // -----------------------------------

  // 1. Load target into rdi (if present), argumentsList into rbx (if present),
  // remove all arguments from the stack (including the receiver), and push
  // thisArgument (if present) instead.
  {
    Label done;
    StackArgumentsAccessor args(rax);
    __ LoadRoot(rdi, RootIndex::kUndefinedValue);
    __ movq(rdx, rdi);
    __ movq(rbx, rdi);
    __ cmpq(rax, Immediate(1));
    __ j(below, &done, Label::kNear);
    __ movq(rdi, args[1]);  // target
    __ j(equal, &done, Label::kNear);
    __ movq(rdx, args[2]);  // thisArgument
    __ cmpq(rax, Immediate(3));
    __ j(below, &done, Label::kNear);
    __ movq(rbx, args[3]);  // argumentsList
    __ bind(&done);
    __ PopReturnAddressTo(rcx);
    __ leaq(rsp,
            Operand(rsp, rax, times_system_pointer_size, kSystemPointerSize));
    __ Push(rdx);
    __ PushReturnAddressFrom(rcx);
  }

  // ----------- S t a t e -------------
  //  -- rbx     : argumentsList
  //  -- rdi     : target
  //  -- rsp[0]  : return address
  //  -- rsp[8]  : thisArgument
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
  //  -- rax     : argc
  //  -- rsp[0]  : return address
  //  -- rsp[8]  : receiver
  //  -- rsp[16] : target
  //  -- rsp[24] : argumentsList
  //  -- rsp[32] : new.target (optional)
  // -----------------------------------

  // 1. Load target into rdi (if present), argumentsList into rbx (if present),
  // new.target into rdx (if present, otherwise use target), remove all
  // arguments from the stack (including the receiver), and push thisArgument
  // (if present) instead.
  {
    Label done;
    StackArgumentsAccessor args(rax);
    __ LoadRoot(rdi, RootIndex::kUndefinedValue);
    __ movq(rdx, rdi);
    __ movq(rbx, rdi);
    __ cmpq(rax, Immediate(1));
    __ j(below, &done, Label::kNear);
    __ movq(rdi, args[1]);                     // target
    __ movq(rdx, rdi);                         // new.target defaults to target
    __ j(equal, &done, Label::kNear);
    __ movq(rbx, args[2]);  // argumentsList
    __ cmpq(rax, Immediate(3));
    __ j(below, &done, Label::kNear);
    __ movq(rdx, args[3]);  // new.target
    __ bind(&done);
    __ PopReturnAddressTo(rcx);
    __ leaq(rsp,
            Operand(rsp, rax, times_system_pointer_size, kSystemPointerSize));
    __ PushRoot(RootIndex::kUndefinedValue);
    __ PushReturnAddressFrom(rcx);
  }

  // ----------- S t a t e -------------
  //  -- rbx     : argumentsList
  //  -- rdx     : new.target
  //  -- rdi     : target
  //  -- rsp[0]  : return address
  //  -- rsp[8]  : receiver (undefined)
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
  //  -- rdi    : target
  //  -- rax    : number of parameters on the stack (not including the receiver)
  //  -- rbx    : arguments list (a FixedArray)
  //  -- rcx    : len (number of elements to push from args)
  //  -- rdx    : new.target (for [[Construct]])
  //  -- rsp[0] : return address
  // -----------------------------------

  if (FLAG_debug_code) {
    // Allow rbx to be a FixedArray, or a FixedDoubleArray if rcx == 0.
    Label ok, fail;
    __ AssertNotSmi(rbx);
    Register map = r9;
    __ LoadMap(map, rbx);
    __ CmpInstanceType(map, FIXED_ARRAY_TYPE);
    __ j(equal, &ok);
    __ CmpInstanceType(map, FIXED_DOUBLE_ARRAY_TYPE);
    __ j(not_equal, &fail);
    __ Cmp(rcx, 0);
    __ j(equal, &ok);
    // Fall through.
    __ bind(&fail);
    __ Abort(AbortReason::kOperandIsNotAFixedArray);

    __ bind(&ok);
  }

  Label stack_overflow;
  __ StackOverflowCheck(rcx, r8, &stack_overflow, Label::kNear);

  // Push additional arguments onto the stack.
  // Move the arguments already in the stack,
  // including the receiver and the return address.
  {
    Label copy, check;
    Register src = r8, dest = rsp, num = r9, current = r12;
    __ movq(src, rsp);
    __ leaq(kScratchRegister, Operand(rcx, times_system_pointer_size, 0));
    __ AllocateStackSpace(kScratchRegister);
    __ leaq(num, Operand(rax, 2));  // Number of words to copy.
                                    // +2 for receiver and return address.
    __ Move(current, 0);
    __ jmp(&check);
    __ bind(&copy);
    __ movq(kScratchRegister,
            Operand(src, current, times_system_pointer_size, 0));
    __ movq(Operand(dest, current, times_system_pointer_size, 0),
            kScratchRegister);
    __ incq(current);
    __ bind(&check);
    __ cmpq(current, num);
    __ j(less, &copy);
    __ leaq(r8, Operand(rsp, num, times_system_pointer_size, 0));
  }

  // Copy the additional arguments onto the stack.
  {
    Register value = r12;
    Register src = rbx, dest = r8, num = rcx, current = r9;
    __ Move(current, 0);
    Label done, push, loop;
    __ bind(&loop);
    __ cmpl(current, num);
    __ j(equal, &done, Label::kNear);
    // Turn the hole into undefined as we go.
    __ LoadAnyTaggedField(value, FieldOperand(src, current, times_tagged_size,
                                              FixedArray::kHeaderSize));
    __ CompareRoot(value, RootIndex::kTheHoleValue);
    __ j(not_equal, &push, Label::kNear);
    __ LoadRoot(value, RootIndex::kUndefinedValue);
    __ bind(&push);
    __ movq(Operand(dest, current, times_system_pointer_size, 0), value);
    __ incl(current);
    __ jmp(&loop);
    __ bind(&done);
    __ addq(rax, current);
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
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the new target (for [[Construct]] calls)
  //  -- rdi : the target to call (can be any Object)
  //  -- rcx : start index (to support rest parameters)
  // -----------------------------------

  // Check if new.target has a [[Construct]] internal method.
  if (mode == CallOrConstructMode::kConstruct) {
    Label new_target_constructor, new_target_not_constructor;
    __ JumpIfSmi(rdx, &new_target_not_constructor, Label::kNear);
    __ LoadMap(rbx, rdx);
    __ testb(FieldOperand(rbx, Map::kBitFieldOffset),
             Immediate(Map::Bits1::IsConstructorBit::kMask));
    __ j(not_zero, &new_target_constructor, Label::kNear);
    __ bind(&new_target_not_constructor);
    {
      FrameScope scope(masm, StackFrame::MANUAL);
      __ EnterFrame(StackFrame::INTERNAL);
      __ Push(rdx);
      __ CallRuntime(Runtime::kThrowNotConstructor);
    }
    __ bind(&new_target_constructor);
  }

  Label stack_done, stack_overflow;
  __ movq(r8, Operand(rbp, StandardFrameConstants::kArgCOffset));
  __ subl(r8, rcx);
  __ j(less_equal, &stack_done);
  {
    // ----------- S t a t e -------------
    //  -- rax : the number of arguments already in the stack (not including the
    //  receiver)
    //  -- rbp : point to the caller stack frame
    //  -- rcx : start index (to support rest parameters)
    //  -- rdx : the new target (for [[Construct]] calls)
    //  -- rdi : the target to call (can be any Object)
    //  -- r8  : number of arguments to copy, i.e. arguments count - start index
    // -----------------------------------

    // Check for stack overflow.
    __ StackOverflowCheck(r8, r12, &stack_overflow, Label::kNear);

    // Forward the arguments from the caller frame.
    // Move the arguments already in the stack,
    // including the receiver and the return address.
    {
      Label copy, check;
      Register src = r9, dest = rsp, num = r12, current = r15;
      __ movq(src, rsp);
      __ leaq(kScratchRegister, Operand(r8, times_system_pointer_size, 0));
      __ AllocateStackSpace(kScratchRegister);
      __ leaq(num, Operand(rax, 2));  // Number of words to copy.
                                      // +2 for receiver and return address.
      __ Move(current, 0);
      __ jmp(&check);
      __ bind(&copy);
      __ movq(kScratchRegister,
              Operand(src, current, times_system_pointer_size, 0));
      __ movq(Operand(dest, current, times_system_pointer_size, 0),
              kScratchRegister);
      __ incq(current);
      __ bind(&check);
      __ cmpq(current, num);
      __ j(less, &copy);
      __ leaq(r9, Operand(rsp, num, times_system_pointer_size, 0));
    }

    __ addl(rax, r8);  // Update total number of arguments.

    // Point to the first argument to copy (skipping receiver).
    __ leaq(rcx, Operand(rcx, times_system_pointer_size,
                         CommonFrameConstants::kFixedFrameSizeAboveFp +
                             kSystemPointerSize));
    __ addq(rcx, rbp);

    // Copy the additional caller arguments onto the stack.
    // TODO(victorgomes): Consider using forward order as potentially more cache
    // friendly.
    {
      Register src = rcx, dest = r9, num = r8;
      Label loop;
      __ bind(&loop);
      __ decq(num);
      __ movq(kScratchRegister,
              Operand(src, num, times_system_pointer_size, 0));
      __ movq(Operand(dest, num, times_system_pointer_size, 0),
              kScratchRegister);
      __ j(not_zero, &loop);
    }
  }
  __ jmp(&stack_done, Label::kNear);
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
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdi : the function to call (checked to be a JSFunction)
  // -----------------------------------

  StackArgumentsAccessor args(rax);
  __ AssertFunction(rdi);

  // ES6 section 9.2.1 [[Call]] ( thisArgument, argumentsList)
  // Check that the function is not a "classConstructor".
  Label class_constructor;
  __ LoadTaggedPointerField(
      rdx, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
  __ testl(FieldOperand(rdx, SharedFunctionInfo::kFlagsOffset),
           Immediate(SharedFunctionInfo::IsClassConstructorBit::kMask));
  __ j(not_zero, &class_constructor);

  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the shared function info.
  //  -- rdi : the function to call (checked to be a JSFunction)
  // -----------------------------------

  // Enter the context of the function; ToObject has to run in the function
  // context, and we also need to take the global proxy from the function
  // context in case of conversion.
  __ LoadTaggedPointerField(rsi, FieldOperand(rdi, JSFunction::kContextOffset));
  // We need to convert the receiver for non-native sloppy mode functions.
  Label done_convert;
  __ testl(FieldOperand(rdx, SharedFunctionInfo::kFlagsOffset),
           Immediate(SharedFunctionInfo::IsNativeBit::kMask |
                     SharedFunctionInfo::IsStrictBit::kMask));
  __ j(not_zero, &done_convert);
  {
    // ----------- S t a t e -------------
    //  -- rax : the number of arguments (not including the receiver)
    //  -- rdx : the shared function info.
    //  -- rdi : the function to call (checked to be a JSFunction)
    //  -- rsi : the function context.
    // -----------------------------------

    if (mode == ConvertReceiverMode::kNullOrUndefined) {
      // Patch receiver to global proxy.
      __ LoadGlobalProxy(rcx);
    } else {
      Label convert_to_object, convert_receiver;
      __ movq(rcx, args.GetReceiverOperand());
      __ JumpIfSmi(rcx, &convert_to_object, Label::kNear);
      STATIC_ASSERT(LAST_JS_RECEIVER_TYPE == LAST_TYPE);
      __ CmpObjectType(rcx, FIRST_JS_RECEIVER_TYPE, rbx);
      __ j(above_equal, &done_convert);
      if (mode != ConvertReceiverMode::kNotNullOrUndefined) {
        Label convert_global_proxy;
        __ JumpIfRoot(rcx, RootIndex::kUndefinedValue, &convert_global_proxy,
                      Label::kNear);
        __ JumpIfNotRoot(rcx, RootIndex::kNullValue, &convert_to_object,
                         Label::kNear);
        __ bind(&convert_global_proxy);
        {
          // Patch receiver to global proxy.
          __ LoadGlobalProxy(rcx);
        }
        __ jmp(&convert_receiver);
      }
      __ bind(&convert_to_object);
      {
        // Convert receiver using ToObject.
        // TODO(bmeurer): Inline the allocation here to avoid building the frame
        // in the fast case? (fall back to AllocateInNewSpace?)
        FrameScope scope(masm, StackFrame::INTERNAL);
        __ SmiTag(rax);
        __ Push(rax);
        __ Push(rdi);
        __ movq(rax, rcx);
        __ Push(rsi);
        __ Call(BUILTIN_CODE(masm->isolate(), ToObject),
                RelocInfo::CODE_TARGET);
        __ Pop(rsi);
        __ movq(rcx, rax);
        __ Pop(rdi);
        __ Pop(rax);
        __ SmiUntag(rax);
      }
      __ LoadTaggedPointerField(
          rdx, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
      __ bind(&convert_receiver);
    }
    __ movq(args.GetReceiverOperand(), rcx);
  }
  __ bind(&done_convert);

  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the shared function info.
  //  -- rdi : the function to call (checked to be a JSFunction)
  //  -- rsi : the function context.
  // -----------------------------------

  __ movzxwq(
      rbx, FieldOperand(rdx, SharedFunctionInfo::kFormalParameterCountOffset));

  __ InvokeFunctionCode(rdi, no_reg, rbx, rax, InvokeType::kJump);

  // The function is a "classConstructor", need to raise an exception.
  __ bind(&class_constructor);
  {
    FrameScope frame(masm, StackFrame::INTERNAL);
    __ Push(rdi);
    __ CallRuntime(Runtime::kThrowConstructorNonCallableError);
  }
}

namespace {

void Generate_PushBoundArguments(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : new.target (only in case of [[Construct]])
  //  -- rdi : target (checked to be a JSBoundFunction)
  // -----------------------------------

  // Load [[BoundArguments]] into rcx and length of that into rbx.
  Label no_bound_arguments;
  __ LoadTaggedPointerField(
      rcx, FieldOperand(rdi, JSBoundFunction::kBoundArgumentsOffset));
  __ SmiUntagField(rbx, FieldOperand(rcx, FixedArray::kLengthOffset));
  __ testl(rbx, rbx);
  __ j(zero, &no_bound_arguments);
  {
    // ----------- S t a t e -------------
    //  -- rax : the number of arguments (not including the receiver)
    //  -- rdx : new.target (only in case of [[Construct]])
    //  -- rdi : target (checked to be a JSBoundFunction)
    //  -- rcx : the [[BoundArguments]] (implemented as FixedArray)
    //  -- rbx : the number of [[BoundArguments]] (checked to be non-zero)
    // -----------------------------------

    // TODO(victor): Use Generate_StackOverflowCheck here.
    // Check the stack for overflow.
    {
      Label done;
      __ shlq(rbx, Immediate(kSystemPointerSizeLog2));
      __ movq(kScratchRegister, rsp);
      __ subq(kScratchRegister, rbx);

      // We are not trying to catch interruptions (i.e. debug break and
      // preemption) here, so check the "real stack limit".
      __ cmpq(kScratchRegister,
              __ StackLimitAsOperand(StackLimitKind::kRealStackLimit));
      __ j(above_equal, &done, Label::kNear);
      {
        FrameScope scope(masm, StackFrame::MANUAL);
        __ EnterFrame(StackFrame::INTERNAL);
        __ CallRuntime(Runtime::kThrowStackOverflow);
      }
      __ bind(&done);
    }

    // Save Return Address and Receiver into registers.
    __ Pop(r8);
    __ Pop(r10);

    // Push [[BoundArguments]] to the stack.
    {
      Label loop;
      __ LoadTaggedPointerField(
          rcx, FieldOperand(rdi, JSBoundFunction::kBoundArgumentsOffset));
      __ SmiUntagField(rbx, FieldOperand(rcx, FixedArray::kLengthOffset));
      __ addq(rax, rbx);  // Adjust effective number of arguments.
      __ bind(&loop);
      // Instead of doing decl(rbx) here subtract kTaggedSize from the header
      // offset in order to be able to move decl(rbx) right before the loop
      // condition. This is necessary in order to avoid flags corruption by
      // pointer decompression code.
      __ LoadAnyTaggedField(
          r12, FieldOperand(rcx, rbx, times_tagged_size,
                            FixedArray::kHeaderSize - kTaggedSize));
      __ Push(r12);
      __ decl(rbx);
      __ j(greater, &loop);
    }

    // Recover Receiver and Return Address.
    __ Push(r10);
    __ Push(r8);
  }
  __ bind(&no_bound_arguments);
}

}  // namespace

// static
void Builtins::Generate_CallBoundFunctionImpl(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdi : the function to call (checked to be a JSBoundFunction)
  // -----------------------------------
  __ AssertBoundFunction(rdi);

  // Patch the receiver to [[BoundThis]].
  StackArgumentsAccessor args(rax);
  __ LoadAnyTaggedField(rbx,
                        FieldOperand(rdi, JSBoundFunction::kBoundThisOffset));
  __ movq(args.GetReceiverOperand(), rbx);

  // Push the [[BoundArguments]] onto the stack.
  Generate_PushBoundArguments(masm);

  // Call the [[BoundTargetFunction]] via the Call builtin.
  __ LoadTaggedPointerField(
      rdi, FieldOperand(rdi, JSBoundFunction::kBoundTargetFunctionOffset));
  __ Jump(BUILTIN_CODE(masm->isolate(), Call_ReceiverIsAny),
          RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_Call(MacroAssembler* masm, ConvertReceiverMode mode) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdi : the target to call (can be any Object)
  // -----------------------------------
  StackArgumentsAccessor args(rax);

  Label non_callable;
  __ JumpIfSmi(rdi, &non_callable);
  __ LoadMap(rcx, rdi);
  __ CmpInstanceTypeRange(rcx, FIRST_JS_FUNCTION_TYPE, LAST_JS_FUNCTION_TYPE);
  __ Jump(masm->isolate()->builtins()->CallFunction(mode),
          RelocInfo::CODE_TARGET, below_equal);

  __ CmpInstanceType(rcx, JS_BOUND_FUNCTION_TYPE);
  __ Jump(BUILTIN_CODE(masm->isolate(), CallBoundFunction),
          RelocInfo::CODE_TARGET, equal);

  // Check if target has a [[Call]] internal method.
  __ testb(FieldOperand(rcx, Map::kBitFieldOffset),
           Immediate(Map::Bits1::IsCallableBit::kMask));
  __ j(zero, &non_callable, Label::kNear);

  // Check if target is a proxy and call CallProxy external builtin
  __ CmpInstanceType(rcx, JS_PROXY_TYPE);
  __ Jump(BUILTIN_CODE(masm->isolate(), CallProxy), RelocInfo::CODE_TARGET,
          equal);

  // 2. Call to something else, which might have a [[Call]] internal method (if
  // not we raise an exception).

  // Overwrite the original receiver with the (original) target.
  __ movq(args.GetReceiverOperand(), rdi);
  // Let the "call_as_function_delegate" take care of the rest.
  __ LoadNativeContextSlot(rdi, Context::CALL_AS_FUNCTION_DELEGATE_INDEX);
  __ Jump(masm->isolate()->builtins()->CallFunction(
              ConvertReceiverMode::kNotNullOrUndefined),
          RelocInfo::CODE_TARGET);

  // 3. Call to something that is not callable.
  __ bind(&non_callable);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ Push(rdi);
    __ CallRuntime(Runtime::kThrowCalledNonCallable);
  }
}

// static
void Builtins::Generate_ConstructFunction(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the new target (checked to be a constructor)
  //  -- rdi : the constructor to call (checked to be a JSFunction)
  // -----------------------------------
  __ AssertConstructor(rdi);
  __ AssertFunction(rdi);

  // Calling convention for function specific ConstructStubs require
  // rbx to contain either an AllocationSite or undefined.
  __ LoadRoot(rbx, RootIndex::kUndefinedValue);

  // Jump to JSBuiltinsConstructStub or JSConstructStubGeneric.
  __ LoadTaggedPointerField(
      rcx, FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset));
  __ testl(FieldOperand(rcx, SharedFunctionInfo::kFlagsOffset),
           Immediate(SharedFunctionInfo::ConstructAsBuiltinBit::kMask));
  __ Jump(BUILTIN_CODE(masm->isolate(), JSBuiltinsConstructStub),
          RelocInfo::CODE_TARGET, not_zero);

  __ Jump(BUILTIN_CODE(masm->isolate(), JSConstructStubGeneric),
          RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_ConstructBoundFunction(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the new target (checked to be a constructor)
  //  -- rdi : the constructor to call (checked to be a JSBoundFunction)
  // -----------------------------------
  __ AssertConstructor(rdi);
  __ AssertBoundFunction(rdi);

  // Push the [[BoundArguments]] onto the stack.
  Generate_PushBoundArguments(masm);

  // Patch new.target to [[BoundTargetFunction]] if new.target equals target.
  {
    Label done;
    __ cmpq(rdi, rdx);
    __ j(not_equal, &done, Label::kNear);
    __ LoadTaggedPointerField(
        rdx, FieldOperand(rdi, JSBoundFunction::kBoundTargetFunctionOffset));
    __ bind(&done);
  }

  // Construct the [[BoundTargetFunction]] via the Construct builtin.
  __ LoadTaggedPointerField(
      rdi, FieldOperand(rdi, JSBoundFunction::kBoundTargetFunctionOffset));
  __ Jump(BUILTIN_CODE(masm->isolate(), Construct), RelocInfo::CODE_TARGET);
}

// static
void Builtins::Generate_Construct(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax : the number of arguments (not including the receiver)
  //  -- rdx : the new target (either the same as the constructor or
  //           the JSFunction on which new was invoked initially)
  //  -- rdi : the constructor to call (can be any Object)
  // -----------------------------------
  StackArgumentsAccessor args(rax);

  // Check if target is a Smi.
  Label non_constructor;
  __ JumpIfSmi(rdi, &non_constructor);

  // Check if target has a [[Construct]] internal method.
  __ LoadMap(rcx, rdi);
  __ testb(FieldOperand(rcx, Map::kBitFieldOffset),
           Immediate(Map::Bits1::IsConstructorBit::kMask));
  __ j(zero, &non_constructor);

  // Dispatch based on instance type.
  __ CmpInstanceTypeRange(rcx, FIRST_JS_FUNCTION_TYPE, LAST_JS_FUNCTION_TYPE);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructFunction),
          RelocInfo::CODE_TARGET, below_equal);

  // Only dispatch to bound functions after checking whether they are
  // constructors.
  __ CmpInstanceType(rcx, JS_BOUND_FUNCTION_TYPE);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructBoundFunction),
          RelocInfo::CODE_TARGET, equal);

  // Only dispatch to proxies after checking whether they are constructors.
  __ CmpInstanceType(rcx, JS_PROXY_TYPE);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructProxy), RelocInfo::CODE_TARGET,
          equal);

  // Called Construct on an exotic Object with a [[Construct]] internal method.
  {
    // Overwrite the original receiver with the (original) target.
    __ movq(args.GetReceiverOperand(), rdi);
    // Let the "call_as_constructor_delegate" take care of the rest.
    __ LoadNativeContextSlot(rdi, Context::CALL_AS_CONSTRUCTOR_DELEGATE_INDEX);
    __ Jump(masm->isolate()->builtins()->CallFunction(),
            RelocInfo::CODE_TARGET);
  }

  // Called Construct on an Object that doesn't have a [[Construct]] internal
  // method.
  __ bind(&non_constructor);
  __ Jump(BUILTIN_CODE(masm->isolate(), ConstructedNonConstructable),
          RelocInfo::CODE_TARGET);
}

namespace {

void Generate_OSREntry(MacroAssembler* masm, Register entry_address) {
  // Overwrite the return address on the stack.
  __ movq(StackOperandForReturnAddress(0), entry_address);

  // And "return" to the OSR entry point of the function.
  __ ret(0);
}

void OnStackReplacement(MacroAssembler* masm, bool is_interpreter) {
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ CallRuntime(Runtime::kCompileForOnStackReplacement);
  }

  Label skip;
  // If the code object is null, just return to the caller.
  __ testq(rax, rax);
  __ j(not_equal, &skip, Label::kNear);
  __ ret(0);

  __ bind(&skip);

  if (is_interpreter) {
    // Drop the handler frame that is be sitting on top of the actual
    // JavaScript frame. This is the case then OSR is triggered from bytecode.
    __ leave();
  }

  // Load deoptimization data from the code object.
  __ LoadTaggedPointerField(rbx,
                            FieldOperand(rax, Code::kDeoptimizationDataOffset));

  // Load the OSR entrypoint offset from the deoptimization data.
  __ SmiUntagField(
      rbx, FieldOperand(rbx, FixedArray::OffsetOfElementAt(
                                 DeoptimizationData::kOsrPcOffsetIndex)));

  // Compute the target address = code_obj + header_size + osr_offset
  __ leaq(rax, FieldOperand(rax, rbx, times_1, Code::kHeaderSize));

  Generate_OSREntry(masm, rax);
}

}  // namespace

void Builtins::Generate_InterpreterOnStackReplacement(MacroAssembler* masm) {
  return OnStackReplacement(masm, true);
}

void Builtins::Generate_BaselineOnStackReplacement(MacroAssembler* masm) {
  __ movq(kContextRegister,
          MemOperand(rbp, BaselineFrameConstants::kContextOffset));
  return OnStackReplacement(masm, false);
}

#if V8_ENABLE_WEBASSEMBLY
void Builtins::Generate_WasmCompileLazy(MacroAssembler* masm) {
  // The function index was pushed to the stack by the caller as int32.
  __ Pop(r15);
  // Convert to Smi for the runtime call.
  __ SmiTag(r15);
  {
    HardAbortScope hard_abort(masm);  // Avoid calls to Abort.
    FrameScope scope(masm, StackFrame::WASM_COMPILE_LAZY);

    // Save all parameter registers (see wasm-linkage.h). They might be
    // overwritten in the runtime call below. We don't have any callee-saved
    // registers in wasm, so no need to store anything else.
    static_assert(WasmCompileLazyFrameConstants::kNumberOfSavedGpParamRegs ==
                      arraysize(wasm::kGpParamRegisters),
                  "frame size mismatch");
    for (Register reg : wasm::kGpParamRegisters) {
      __ Push(reg);
    }
    static_assert(WasmCompileLazyFrameConstants::kNumberOfSavedFpParamRegs ==
                      arraysize(wasm::kFpParamRegisters),
                  "frame size mismatch");
    __ AllocateStackSpace(kSimd128Size * arraysize(wasm::kFpParamRegisters));
    int offset = 0;
    for (DoubleRegister reg : wasm::kFpParamRegisters) {
      __ movdqu(Operand(rsp, offset), reg);
      offset += kSimd128Size;
    }

    // Push the Wasm instance as an explicit argument to WasmCompileLazy.
    __ Push(kWasmInstanceRegister);
    // Push the function index as second argument.
    __ Push(r15);
    // Initialize the JavaScript context with 0. CEntry will use it to
    // set the current context on the isolate.
    __ Move(kContextRegister, Smi::zero());
    __ CallRuntime(Runtime::kWasmCompileLazy, 2);
    // The entrypoint address is the return value.
    __ movq(r15, kReturnRegister0);

    // Restore registers.
    for (DoubleRegister reg : base::Reversed(wasm::kFpParamRegisters)) {
      offset -= kSimd128Size;
      __ movdqu(reg, Operand(rsp, offset));
    }
    DCHECK_EQ(0, offset);
    __ addq(rsp, Immediate(kSimd128Size * arraysize(wasm::kFpParamRegisters)));
    for (Register reg : base::Reversed(wasm::kGpParamRegisters)) {
      __ Pop(reg);
    }
  }
  // Finally, jump to the entrypoint.
  __ jmp(r15);
}

void Builtins::Generate_WasmDebugBreak(MacroAssembler* masm) {
  HardAbortScope hard_abort(masm);  // Avoid calls to Abort.
  {
    FrameScope scope(masm, StackFrame::WASM_DEBUG_BREAK);

    // Save all parameter registers. They might hold live values, we restore
    // them after the runtime call.
    for (int reg_code : base::bits::IterateBitsBackwards(
             WasmDebugBreakFrameConstants::kPushedGpRegs)) {
      __ Push(Register::from_code(reg_code));
    }

    constexpr int kFpStackSize =
        kSimd128Size * WasmDebugBreakFrameConstants::kNumPushedFpRegisters;
    __ AllocateStackSpace(kFpStackSize);
    int offset = kFpStackSize;
    for (int reg_code : base::bits::IterateBitsBackwards(
             WasmDebugBreakFrameConstants::kPushedFpRegs)) {
      offset -= kSimd128Size;
      __ movdqu(Operand(rsp, offset), DoubleRegister::from_code(reg_code));
    }

    // Initialize the JavaScript context with 0. CEntry will use it to
    // set the current context on the isolate.
    __ Move(kContextRegister, Smi::zero());
    __ CallRuntime(Runtime::kWasmDebugBreak, 0);

    // Restore registers.
    for (int reg_code :
         base::bits::IterateBits(WasmDebugBreakFrameConstants::kPushedFpRegs)) {
      __ movdqu(DoubleRegister::from_code(reg_code), Operand(rsp, offset));
      offset += kSimd128Size;
    }
    __ addq(rsp, Immediate(kFpStackSize));
    for (int reg_code :
         base::bits::IterateBits(WasmDebugBreakFrameConstants::kPushedGpRegs)) {
      __ Pop(Register::from_code(reg_code));
    }
  }

  __ ret(0);
}

namespace {
// Helper functions for the GenericJSToWasmWrapper.
void PrepareForBuiltinCall(MacroAssembler* masm, MemOperand GCScanSlotPlace,
                           const int GCScanSlotCount, Register current_param,
                           Register param_limit,
                           Register current_int_param_slot,
                           Register current_float_param_slot,
                           Register valuetypes_array_ptr,
                           Register wasm_instance, Register function_data) {
  // Pushes and puts the values in order onto the stack before builtin calls for
  // the GenericJSToWasmWrapper.
  __ movq(GCScanSlotPlace, Immediate(GCScanSlotCount));
  __ pushq(current_param);
  __ pushq(param_limit);
  __ pushq(current_int_param_slot);
  __ pushq(current_float_param_slot);
  __ pushq(valuetypes_array_ptr);
  __ pushq(wasm_instance);
  __ pushq(function_data);
  // We had to prepare the parameters for the Call: we have to put the context
  // into rsi.
  __ LoadAnyTaggedField(
      rsi,
      MemOperand(wasm_instance, wasm::ObjectAccess::ToTagged(
                                    WasmInstanceObject::kNativeContextOffset)));
}

void RestoreAfterBuiltinCall(MacroAssembler* masm, Register function_data,
                             Register wasm_instance,
                             Register valuetypes_array_ptr,
                             Register current_float_param_slot,
                             Register current_int_param_slot,
                             Register param_limit, Register current_param) {
  // Pop and load values from the stack in order into the registers after
  // builtin calls for the GenericJSToWasmWrapper.
  __ popq(function_data);
  __ popq(wasm_instance);
  __ popq(valuetypes_array_ptr);
  __ popq(current_float_param_slot);
  __ popq(current_int_param_slot);
  __ popq(param_limit);
  __ popq(current_param);
}
}  // namespace

void Builtins::Generate_GenericJSToWasmWrapper(MacroAssembler* masm) {
  // Set up the stackframe.
  __ EnterFrame(StackFrame::JS_TO_WASM);

  // -------------------------------------------
  // Compute offsets and prepare for GC.
  // -------------------------------------------
  // We will have to save a value indicating the GC the number
  // of values on the top of the stack that have to be scanned before calling
  // the Wasm function.
  constexpr int kFrameMarkerOffset = -kSystemPointerSize;
  constexpr int kGCScanSlotCountOffset =
      kFrameMarkerOffset - kSystemPointerSize;
  // The number of parameters passed to this function.
  constexpr int kInParamCountOffset =
      kGCScanSlotCountOffset - kSystemPointerSize;
  // The number of parameters according to the signature.
  constexpr int kParamCountOffset = kInParamCountOffset - kSystemPointerSize;
  constexpr int kReturnCountOffset = kParamCountOffset - kSystemPointerSize;
  constexpr int kValueTypesArrayStartOffset =
      kReturnCountOffset - kSystemPointerSize;
  // We set and use this slot only when moving parameters into the parameter
  // registers (so no GC scan is needed).
  constexpr int kFunctionDataOffset =
      kValueTypesArrayStartOffset - kSystemPointerSize;
  constexpr int kLastSpillOffset = kFunctionDataOffset;
  constexpr int kNumSpillSlots = 6;
  __ subq(rsp, Immediate(kNumSpillSlots * kSystemPointerSize));
  // Put the in_parameter count on the stack, we only  need it at the very end
  // when we pop the parameters off the stack.
  Register in_param_count = rax;
  __ movq(MemOperand(rbp, kInParamCountOffset), in_param_count);
  in_param_count = no_reg;

  // -------------------------------------------
  // Load the Wasm exported function data and the Wasm instance.
  // -------------------------------------------
  Register closure = rdi;
  Register shared_function_info = closure;
  __ LoadAnyTaggedField(
      shared_function_info,
      MemOperand(
          closure,
          wasm::ObjectAccess::SharedFunctionInfoOffsetInTaggedJSFunction()));
  closure = no_reg;
  Register function_data = shared_function_info;
  __ LoadAnyTaggedField(
      function_data,
      MemOperand(shared_function_info,
                 SharedFunctionInfo::kFunctionDataOffset - kHeapObjectTag));
  shared_function_info = no_reg;

  Register wasm_instance = rsi;
  __ LoadAnyTaggedField(
      wasm_instance,
      MemOperand(function_data,
                 WasmExportedFunctionData::kInstanceOffset - kHeapObjectTag));

  // -------------------------------------------
  // Decrement the budget of the generic wrapper in function data.
  // -------------------------------------------
  __ SmiAddConstant(
      MemOperand(function_data, WasmExportedFunctionData::kWrapperBudgetOffset -
                                    kHeapObjectTag),
      Smi::FromInt(-1));

  // -------------------------------------------
  // Check if the budget of the generic wrapper reached 0 (zero).
  // -------------------------------------------
  // Instead of a specific comparison, we can directly use the flags set
  // from the previous addition.
  Label compile_wrapper, compile_wrapper_done;
  __ j(less_equal, &compile_wrapper);
  __ bind(&compile_wrapper_done);

  // -------------------------------------------
  // Load values from the signature.
  // -------------------------------------------
  Register foreign_signature = r11;
  __ LoadAnyTaggedField(
      foreign_signature,
      MemOperand(function_data,
                 WasmExportedFunctionData::kSignatureOffset - kHeapObjectTag));
  Register signature = foreign_signature;
  __ LoadExternalPointerField(
      signature,
      FieldOperand(foreign_signature, Foreign::kForeignAddressOffset),
      kForeignForeignAddressTag, kScratchRegister);
  foreign_signature = no_reg;
  Register return_count = r8;
  __ movq(return_count,
          MemOperand(signature, wasm::FunctionSig::kReturnCountOffset));
  Register param_count = rcx;
  __ movq(param_count,
          MemOperand(signature, wasm::FunctionSig::kParameterCountOffset));
  Register valuetypes_array_ptr = signature;
  __ movq(valuetypes_array_ptr,
          MemOperand(signature, wasm::FunctionSig::kRepsOffset));
  signature = no_reg;

  // -------------------------------------------
  // Store signature-related values to the stack.
  // -------------------------------------------
  // We store values on the stack to restore them after function calls.
  // We cannot push values onto the stack right before the wasm call. The wasm
  // function expects the parameters, that didn't fit into the registers, on the
  // top of the stack.
  __ movq(MemOperand(rbp, kParamCountOffset), param_count);
  __ movq(MemOperand(rbp, kReturnCountOffset), return_count);
  __ movq(MemOperand(rbp, kValueTypesArrayStartOffset), valuetypes_array_ptr);

  // -------------------------------------------
  // Parameter handling.
  // -------------------------------------------
  Label prepare_for_wasm_call;
  __ Cmp(param_count, 0);

  // IF we have 0 params: jump through parameter handling.
  __ j(equal, &prepare_for_wasm_call);

  // -------------------------------------------
  // Create 2 sections for integer and float params.
  // -------------------------------------------
  // We will create 2 sections on the stack for the evaluated parameters:
  // Integer and Float section, both with parameter count size. We will place
  // the parameters into these sections depending on their valuetype. This way
  // we can easily fill the general purpose and floating point parameter
  // registers and place the remaining parameters onto the stack in proper order
  // for the Wasm function. These remaining params are the final stack
  // parameters for the call to WebAssembly. Example of the stack layout after
  // processing 2 int and 1 float parameters when param_count is 4.
  //   +-----------------+
  //   |      rbp        |
  //   |-----------------|-------------------------------
  //   |                 |   Slots we defined
  //   |   Saved values  |    when setting up
  //   |                 |     the stack
  //   |                 |
  //   +-Integer section-+--- <--- start_int_section ----
  //   |  1st int param  |
  //   |- - - - - - - - -|
  //   |  2nd int param  |
  //   |- - - - - - - - -|  <----- current_int_param_slot
  //   |                 |       (points to the stackslot
  //   |- - - - - - - - -|  where the next int param should be placed)
  //   |                 |
  //   +--Float section--+--- <--- start_float_section --
  //   | 1st float param |
  //   |- - - - - - - - -|  <----  current_float_param_slot
  //   |                 |       (points to the stackslot
  //   |- - - - - - - - -|  where the next float param should be placed)
  //   |                 |
  //   |- - - - - - - - -|
  //   |                 |
  //   +---Final stack---+------------------------------
  //   +-parameters for--+------------------------------
  //   +-the Wasm call---+------------------------------
  //   |      . . .      |

  constexpr int kIntegerSectionStartOffset =
      kLastSpillOffset - kSystemPointerSize;
  // For Integer section.
  // Set the current_int_param_slot to point to the start of the section.
  Register current_int_param_slot = r10;
  __ leaq(current_int_param_slot, MemOperand(rsp, -kSystemPointerSize));
  Register params_size = param_count;
  param_count = no_reg;
  __ shlq(params_size, Immediate(kSystemPointerSizeLog2));
  __ subq(rsp, params_size);

  // For Float section.
  // Set the current_float_param_slot to point to the start of the section.
  Register current_float_param_slot = r15;
  __ leaq(current_float_param_slot, MemOperand(rsp, -kSystemPointerSize));
  __ subq(rsp, params_size);
  params_size = no_reg;
  param_count = rcx;
  __ movq(param_count, MemOperand(rbp, kParamCountOffset));

  // -------------------------------------------
  // Set up for the param evaluation loop.
  // -------------------------------------------
  // We will loop through the params starting with the 1st param.
  // The order of processing the params is important. We have to evaluate them
  // in an increasing order.
  //       +-----------------+---------------
  //       |     param n     |
  //       |- - - - - - - - -|
  //       |    param n-1    |   Caller
  //       |       ...       | frame slots
  //       |     param 1     |
  //       |- - - - - - - - -|
  //       |    receiver     |
  //       +-----------------+---------------
  //       |  return addr    |
  //   FP->|- - - - - - - - -|
  //       |      rbp        |   Spill slots
  //       |- - - - - - - - -|
  //
  // [rbp + current_param] gives us the parameter we are processing.
  // We iterate through half-open interval <1st param, [rbp + param_limit]).

  Register current_param = rbx;
  Register param_limit = rdx;
  constexpr int kReceiverOnStackSize = kSystemPointerSize;
  __ movq(current_param,
          Immediate(kFPOnStackSize + kPCOnStackSize + kReceiverOnStackSize));
  __ movq(param_limit, param_count);
  __ shlq(param_limit, Immediate(kSystemPointerSizeLog2));
  __ addq(param_limit,
          Immediate(kFPOnStackSize + kPCOnStackSize + kReceiverOnStackSize));
  const int increment = kSystemPointerSize;
  Register param = rax;
  // We have to check the types of the params. The ValueType array contains
  // first the return then the param types.
  constexpr int kValueTypeSize = sizeof(wasm::ValueType);
  STATIC_ASSERT(kValueTypeSize == 4);
  const int32_t kValueTypeSizeLog2 = log2(kValueTypeSize);
  // Set the ValueType array pointer to point to the first parameter.
  Register returns_size = return_count;
  return_count = no_reg;
  __ shlq(returns_size, Immediate(kValueTypeSizeLog2));
  __ addq(valuetypes_array_ptr, returns_size);
  returns_size = no_reg;
  Register valuetype = r12;

  // -------------------------------------------
  // Param evaluation loop.
  // -------------------------------------------
  Label loop_through_params;
  __ bind(&loop_through_params);

  __ movq(param, MemOperand(rbp, current_param, times_1, 0));
  __ movl(valuetype,
          Operand(valuetypes_array_ptr, wasm::ValueType::bit_field_offset()));

  // -------------------------------------------
  // Param conversion.
  // -------------------------------------------
  // If param is a Smi we can easily convert it. Otherwise we'll call a builtin
  // for conversion.
  Label convert_param;
  __ cmpq(valuetype, Immediate(wasm::kWasmI32.raw_bit_field()));
  __ j(not_equal, &convert_param);
  __ JumpIfNotSmi(param, &convert_param);
  // Change the paramfrom Smi to int32.
  __ SmiUntag(param);
  // Zero extend.
  __ movl(param, param);
  // Place the param into the proper slot in Integer section.
  __ movq(MemOperand(current_int_param_slot, 0), param);
  __ subq(current_int_param_slot, Immediate(kSystemPointerSize));

  // -------------------------------------------
  // Param conversion done.
  // -------------------------------------------
  Label param_conversion_done;
  __ bind(&param_conversion_done);

  __ addq(current_param, Immediate(increment));
  __ addq(valuetypes_array_ptr, Immediate(kValueTypeSize));

  __ cmpq(current_param, param_limit);
  __ j(not_equal, &loop_through_params);

  // -------------------------------------------
  // Move the parameters into the proper param registers.
  // -------------------------------------------
  // The Wasm function expects that the params can be popped from the top of the
  // stack in an increasing order.
  // We can always move the values on the beginning of the sections into the GP
  // or FP parameter registers. If the parameter count is less than the number
  // of parameter registers, we may move values into the registers that are not
  // in the section.
  // ----------- S t a t e -------------
  //  -- r8  : start_int_section
  //  -- rdi : start_float_section
  //  -- r10 : current_int_param_slot
  //  -- r15 : current_float_param_slot
  //  -- r11 : valuetypes_array_ptr
  //  -- r12 : valuetype
  //  -- rsi : wasm_instance
  //  -- GpParamRegisters = rax, rdx, rcx, rbx, r9
  // -----------------------------------

  Register temp_params_size = rax;
  __ movq(temp_params_size, MemOperand(rbp, kParamCountOffset));
  __ shlq(temp_params_size, Immediate(kSystemPointerSizeLog2));
  // We want to use the register of the function_data = rdi.
  __ movq(MemOperand(rbp, kFunctionDataOffset), function_data);
  Register start_float_section = function_data;
  function_data = no_reg;
  __ movq(start_float_section, rbp);
  __ addq(start_float_section, Immediate(kIntegerSectionStartOffset));
  __ subq(start_float_section, temp_params_size);
  temp_params_size = no_reg;
  // Fill the FP param registers.
  __ Movsd(xmm1, MemOperand(start_float_section, 0));
  __ Movsd(xmm2, MemOperand(start_float_section, -kSystemPointerSize));
  __ Movsd(xmm3, MemOperand(start_float_section, -2 * kSystemPointerSize));
  __ Movsd(xmm4, MemOperand(start_float_section, -3 * kSystemPointerSize));
  __ Movsd(xmm5, MemOperand(start_float_section, -4 * kSystemPointerSize));
  __ Movsd(xmm6, MemOperand(start_float_section, -5 * kSystemPointerSize));
  // We want the start to point to the last properly placed param.
  __ subq(start_float_section, Immediate(5 * kSystemPointerSize));

  Register start_int_section = r8;
  __ movq(start_int_section, rbp);
  __ addq(start_int_section, Immediate(kIntegerSectionStartOffset));
  // Fill the GP param registers.
  __ movq(rax, MemOperand(start_int_section, 0));
  __ movq(rdx, MemOperand(start_int_section, -kSystemPointerSize));
  __ movq(rcx, MemOperand(start_int_section, -2 * kSystemPointerSize));
  __ movq(rbx, MemOperand(start_int_section, -3 * kSystemPointerSize));
  __ movq(r9, MemOperand(start_int_section, -4 * kSystemPointerSize));
  // We want the start to point to the last properly placed param.
  __ subq(start_int_section, Immediate(4 * kSystemPointerSize));

  // -------------------------------------------
  // Place the final stack parameters to the proper place.
  // -------------------------------------------
  // We want the current_param_slot (insertion) pointers to point at the last
  // param of the section instead of the next free slot.
  __ addq(current_int_param_slot, Immediate(kSystemPointerSize));
  __ addq(current_float_param_slot, Immediate(kSystemPointerSize));

  // -------------------------------------------
  // Final stack parameters loop.
  // -------------------------------------------
  // The parameters that didn't fit into the registers should be placed on the
  // top of the stack contiguously. The interval of parameters between the
  // start_section and the current_param_slot pointers define the remaining
  // parameters of the section.
  // We can iterate through the valuetypes array to decide from which section we
  // need to push the parameter onto the top of the stack. By iterating in a
  // reversed order we can easily pick the last parameter of the proper section.
  // The parameter of the section is pushed on the top of the stack only if the
  // interval of remaining params is not empty. This way we ensure that only
  // params that didn't fit into param registers are pushed again.

  Label loop_through_valuetypes;
  __ bind(&loop_through_valuetypes);

  // We iterated through the valuetypes array, we are one field over the end in
  // the beginning. Also, we have to decrement it in each iteration.
  __ subq(valuetypes_array_ptr, Immediate(kValueTypeSize));

  // Check if there are still remaining integer params.
  Label continue_loop;
  __ cmpq(start_int_section, current_int_param_slot);
  // If there are remaining integer params.
  __ j(greater, &continue_loop);

  // Check if there are still remaining float params.
  __ cmpq(start_float_section, current_float_param_slot);
  // If there aren't any params remaining.
  Label params_done;
  __ j(less_equal, &params_done);

  __ bind(&continue_loop);
  __ movl(valuetype,
          Operand(valuetypes_array_ptr, wasm::ValueType::bit_field_offset()));
  Label place_integer_param;
  Label place_float_param;
  __ cmpq(valuetype, Immediate(wasm::kWasmI32.raw_bit_field()));
  __ j(equal, &place_integer_param);

  __ cmpq(valuetype, Immediate(wasm::kWasmI64.raw_bit_field()));
  __ j(equal, &place_integer_param);

  __ cmpq(valuetype, Immediate(wasm::kWasmF32.raw_bit_field()));
  __ j(equal, &place_float_param);

  __ cmpq(valuetype, Immediate(wasm::kWasmF64.raw_bit_field()));
  __ j(equal, &place_float_param);

  __ int3();

  __ bind(&place_integer_param);
  __ cmpq(start_int_section, current_int_param_slot);
  // If there aren't any integer params remaining, just floats, then go to the
  // next valuetype.
  __ j(less_equal, &loop_through_valuetypes);

  // Copy the param from the integer section to the actual parameter area.
  __ pushq(MemOperand(current_int_param_slot, 0));
  __ addq(current_int_param_slot, Immediate(kSystemPointerSize));
  __ jmp(&loop_through_valuetypes);

  __ bind(&place_float_param);
  __ cmpq(start_float_section, current_float_param_slot);
  // If there aren't any float params remaining, just integers, then go to the
  // next valuetype.
  __ j(less_equal, &loop_through_valuetypes);

  // Copy the param from the float section to the actual parameter area.
  __ pushq(MemOperand(current_float_param_slot, 0));
  __ addq(current_float_param_slot, Immediate(kSystemPointerSize));
  __ jmp(&loop_through_valuetypes);

  __ bind(&params_done);
  // Restore function_data after we are done with parameter placement.
  function_data = rdi;
  __ movq(function_data, MemOperand(rbp, kFunctionDataOffset));

  __ bind(&prepare_for_wasm_call);
  // -------------------------------------------
  // Prepare for the Wasm call.
  // -------------------------------------------
  // Set thread_in_wasm_flag.
  Register thread_in_wasm_flag_addr = r12;
  __ movq(
      thread_in_wasm_flag_addr,
      MemOperand(kRootRegister, Isolate::thread_in_wasm_flag_address_offset()));
  __ movl(MemOperand(thread_in_wasm_flag_addr, 0), Immediate(1));
  thread_in_wasm_flag_addr = no_reg;

  Register function_entry = function_data;
  Register scratch = r12;
  __ LoadExternalPointerField(
      function_entry,
      FieldOperand(function_data,
                   WasmExportedFunctionData::kForeignAddressOffset),
      kForeignForeignAddressTag, scratch);
  function_data = no_reg;
  scratch = no_reg;

  // We set the indicating value for the GC to the proper one for Wasm call.
  constexpr int kWasmCallGCScanSlotCount = 0;
  __ movq(MemOperand(rbp, kGCScanSlotCountOffset),
          Immediate(kWasmCallGCScanSlotCount));

  // -------------------------------------------
  // Call the Wasm function.
  // -------------------------------------------
  __ call(function_entry);
  function_entry = no_reg;

  // -------------------------------------------
  // Resetting after the Wasm call.
  // -------------------------------------------
  // Restore rsp to free the reserved stack slots for the sections.
  __ leaq(rsp, MemOperand(rbp, kLastSpillOffset));

  // Unset thread_in_wasm_flag.
  thread_in_wasm_flag_addr = r8;
  __ movq(
      thread_in_wasm_flag_addr,
      MemOperand(kRootRegister, Isolate::thread_in_wasm_flag_address_offset()));
  __ movl(MemOperand(thread_in_wasm_flag_addr, 0), Immediate(0));
  thread_in_wasm_flag_addr = no_reg;

  // -------------------------------------------
  // Return handling.
  // -------------------------------------------
  return_count = r8;
  __ movq(return_count, MemOperand(rbp, kReturnCountOffset));
  Register return_reg = rax;

  // If we have 1 return value, then jump to conversion.
  __ cmpl(return_count, Immediate(1));
  Label convert_return;
  __ j(equal, &convert_return);

  // Otherwise load undefined.
  __ LoadRoot(return_reg, RootIndex::kUndefinedValue);

  Label return_done;
  __ bind(&return_done);
  __ movq(param_count, MemOperand(rbp, kParamCountOffset));

  // Calculate the number of parameters we have to pop off the stack. This
  // number is max(in_param_count, param_count).
  in_param_count = rdx;
  __ movq(in_param_count, MemOperand(rbp, kInParamCountOffset));
  __ cmpq(param_count, in_param_count);
  __ cmovq(less, param_count, in_param_count);

  // -------------------------------------------
  // Deconstrunct the stack frame.
  // -------------------------------------------
  __ LeaveFrame(StackFrame::JS_TO_WASM);

  // We have to remove the caller frame slots:
  //  - JS arguments
  //  - the receiver
  // and transfer the control to the return address (the return address is
  // expected to be on the top of the stack).
  // We cannot use just the ret instruction for this, because we cannot pass the
  // number of slots to remove in a Register as an argument.
  Register return_addr = rbx;
  __ popq(return_addr);
  Register caller_frame_slots_count = param_count;
  // Add one to also pop the receiver. The receiver is passed to a JSFunction
  // over the stack but is neither included in the number of parameters passed
  // to this function nor in the number of parameters expected in this function.
  __ addq(caller_frame_slots_count, Immediate(1));
  __ shlq(caller_frame_slots_count, Immediate(kSystemPointerSizeLog2));
  __ addq(rsp, caller_frame_slots_count);
  __ pushq(return_addr);
  __ ret(0);

  // --------------------------------------------------------------------------
  //                          Deferred code.
  // --------------------------------------------------------------------------

  // -------------------------------------------
  // Param conversion builtins.
  // -------------------------------------------
  __ bind(&convert_param);
  // Restore function_data register (which was clobbered by the code above,
  // but was valid when jumping here earlier).
  function_data = rdi;
  // The order of pushes is important. We want the heap objects, that should be
  // scanned by GC, to be on the top of the stack.
  // We have to set the indicating value for the GC to the number of values on
  // the top of the stack that have to be scanned before calling the builtin
  // function.
  // The builtin expects the parameter to be in register param = rax.

  constexpr int kBuiltinCallGCScanSlotCount = 2;
  PrepareForBuiltinCall(masm, MemOperand(rbp, kGCScanSlotCountOffset),
                        kBuiltinCallGCScanSlotCount, current_param, param_limit,
                        current_int_param_slot, current_float_param_slot,
                        valuetypes_array_ptr, wasm_instance, function_data);

  Label param_kWasmI32_not_smi;
  Label param_kWasmI64;
  Label param_kWasmF32;
  Label param_kWasmF64;

  __ cmpq(valuetype, Immediate(wasm::kWasmI32.raw_bit_field()));
  __ j(equal, &param_kWasmI32_not_smi);

  __ cmpq(valuetype, Immediate(wasm::kWasmI64.raw_bit_field()));
  __ j(equal, &param_kWasmI64);

  __ cmpq(valuetype, Immediate(wasm::kWasmF32.raw_bit_field()));
  __ j(equal, &param_kWasmF32);

  __ cmpq(valuetype, Immediate(wasm::kWasmF64.raw_bit_field()));
  __ j(equal, &param_kWasmF64);

  __ int3();

  __ bind(&param_kWasmI32_not_smi);
  __ Call(BUILTIN_CODE(masm->isolate(), WasmTaggedNonSmiToInt32),
          RelocInfo::CODE_TARGET);
  // Param is the result of the builtin.
  __ AssertZeroExtended(param);
  RestoreAfterBuiltinCall(masm, function_data, wasm_instance,
                          valuetypes_array_ptr, current_float_param_slot,
                          current_int_param_slot, param_limit, current_param);
  __ movq(MemOperand(current_int_param_slot, 0), param);
  __ subq(current_int_param_slot, Immediate(kSystemPointerSize));
  __ jmp(&param_conversion_done);

  __ bind(&param_kWasmI64);
  __ Call(BUILTIN_CODE(masm->isolate(), BigIntToI64), RelocInfo::CODE_TARGET);
  RestoreAfterBuiltinCall(masm, function_data, wasm_instance,
                          valuetypes_array_ptr, current_float_param_slot,
                          current_int_param_slot, param_limit, current_param);
  __ movq(MemOperand(current_int_param_slot, 0), param);
  __ subq(current_int_param_slot, Immediate(kSystemPointerSize));
  __ jmp(&param_conversion_done);

  __ bind(&param_kWasmF32);
  __ Call(BUILTIN_CODE(masm->isolate(), WasmTaggedToFloat64),
          RelocInfo::CODE_TARGET);
  RestoreAfterBuiltinCall(masm, function_data, wasm_instance,
                          valuetypes_array_ptr, current_float_param_slot,
                          current_int_param_slot, param_limit, current_param);
  // Clear higher bits.
  __ Xorpd(xmm1, xmm1);
  // Truncate float64 to float32.
  __ Cvtsd2ss(xmm1, xmm0);
  __ Movsd(MemOperand(current_float_param_slot, 0), xmm1);
  __ subq(current_float_param_slot, Immediate(kSystemPointerSize));
  __ jmp(&param_conversion_done);

  __ bind(&param_kWasmF64);
  __ Call(BUILTIN_CODE(masm->isolate(), WasmTaggedToFloat64),
          RelocInfo::CODE_TARGET);
  RestoreAfterBuiltinCall(masm, function_data, wasm_instance,
                          valuetypes_array_ptr, current_float_param_slot,
                          current_int_param_slot, param_limit, current_param);
  __ Movsd(MemOperand(current_float_param_slot, 0), xmm0);
  __ subq(current_float_param_slot, Immediate(kSystemPointerSize));
  __ jmp(&param_conversion_done);

  // -------------------------------------------
  // Return conversions.
  // -------------------------------------------
  __ bind(&convert_return);
  // We have to make sure that the kGCScanSlotCount is set correctly when we
  // call the builtins for conversion. For these builtins it's the same as for
  // the Wasm call, that is, kGCScanSlotCount = 0, so we don't have to reset it.
  // We don't need the JS context for these builtin calls.

  __ movq(valuetypes_array_ptr, MemOperand(rbp, kValueTypesArrayStartOffset));
  // The first valuetype of the array is the return's valuetype.
  __ movl(valuetype,
          Operand(valuetypes_array_ptr, wasm::ValueType::bit_field_offset()));

  Label return_kWasmI32;
  Label return_kWasmI64;
  Label return_kWasmF32;
  Label return_kWasmF64;

  __ cmpq(valuetype, Immediate(wasm::kWasmI32.raw_bit_field()));
  __ j(equal, &return_kWasmI32);

  __ cmpq(valuetype, Immediate(wasm::kWasmI64.raw_bit_field()));
  __ j(equal, &return_kWasmI64);

  __ cmpq(valuetype, Immediate(wasm::kWasmF32.raw_bit_field()));
  __ j(equal, &return_kWasmF32);

  __ cmpq(valuetype, Immediate(wasm::kWasmF64.raw_bit_field()));
  __ j(equal, &return_kWasmF64);

  __ int3();

  __ bind(&return_kWasmI32);
  Label to_heapnumber;
  // If pointer compression is disabled, we can convert the return to a smi.
  if (SmiValuesAre32Bits()) {
    __ SmiTag(return_reg);
  } else {
    Register temp = rbx;
    __ movq(temp, return_reg);
    // Double the return value to test if it can be a Smi.
    __ addl(temp, return_reg);
    temp = no_reg;
    // If there was overflow, convert the return value to a HeapNumber.
    __ j(overflow, &to_heapnumber);
    // If there was no overflow, we can convert to Smi.
    __ SmiTag(return_reg);
  }
  __ jmp(&return_done);

  // Handle the conversion of the I32 return value to HeapNumber when it cannot
  // be a smi.
  __ bind(&to_heapnumber);
  __ Call(BUILTIN_CODE(masm->isolate(), WasmInt32ToHeapNumber),
          RelocInfo::CODE_TARGET);
  __ jmp(&return_done);

  __ bind(&return_kWasmI64);
  __ Call(BUILTIN_CODE(masm->isolate(), I64ToBigInt), RelocInfo::CODE_TARGET);
  __ jmp(&return_done);

  __ bind(&return_kWasmF32);
  // The builtin expects the value to be in xmm0.
  __ Movss(xmm0, xmm1);
  __ Call(BUILTIN_CODE(masm->isolate(), WasmFloat32ToNumber),
          RelocInfo::CODE_TARGET);
  __ jmp(&return_done);

  __ bind(&return_kWasmF64);
  // The builtin expects the value to be in xmm0.
  __ Movsd(xmm0, xmm1);
  __ Call(BUILTIN_CODE(masm->isolate(), WasmFloat64ToNumber),
          RelocInfo::CODE_TARGET);
  __ jmp(&return_done);

  // -------------------------------------------
  // Kick off compilation.
  // -------------------------------------------
  __ bind(&compile_wrapper);
  // Enable GC.
  MemOperand GCScanSlotPlace = MemOperand(rbp, kGCScanSlotCountOffset);
  __ movq(GCScanSlotPlace, Immediate(4));
  // Save registers to the stack.
  __ pushq(wasm_instance);
  __ pushq(function_data);
  // Push the arguments for the runtime call.
  __ Push(wasm_instance);  // first argument
  __ Push(function_data);  // second argument
  // Set up context.
  __ Move(kContextRegister, Smi::zero());
  // Call the runtime function that kicks off compilation.
  __ CallRuntime(Runtime::kWasmCompileWrapper, 2);
  // Pop the result.
  __ movq(r9, kReturnRegister0);
  // Restore registers from the stack.
  __ popq(function_data);
  __ popq(wasm_instance);
  __ jmp(&compile_wrapper_done);
}

void Builtins::Generate_WasmOnStackReplace(MacroAssembler* masm) {
  MemOperand OSRTargetSlot(rbp, -wasm::kOSRTargetOffset);
  __ movq(kScratchRegister, OSRTargetSlot);
  __ movq(OSRTargetSlot, Immediate(0));
  __ jmp(kScratchRegister);
}

#endif  // V8_ENABLE_WEBASSEMBLY

void Builtins::Generate_CEntry(MacroAssembler* masm, int result_size,
                               SaveFPRegsMode save_doubles, ArgvMode argv_mode,
                               bool builtin_exit_frame) {
  // rax: number of arguments including receiver
  // rbx: pointer to C function  (C callee-saved)
  // rbp: frame pointer of calling JS frame (restored after C call)
  // rsp: stack pointer  (restored after C call)
  // rsi: current context (restored)
  //
  // If argv_mode == ArgvMode::kRegister:
  // r15: pointer to the first argument

#ifdef V8_TARGET_OS_WIN
  // Windows 64-bit ABI passes arguments in rcx, rdx, r8, r9. It requires the
  // stack to be aligned to 16 bytes. It only allows a single-word to be
  // returned in register rax. Larger return sizes must be written to an address
  // passed as a hidden first argument.
  const Register kCCallArg0 = rcx;
  const Register kCCallArg1 = rdx;
  const Register kCCallArg2 = r8;
  const Register kCCallArg3 = r9;
  const int kArgExtraStackSpace = 2;
  const int kMaxRegisterResultSize = 1;
#else
  // GCC / Clang passes arguments in rdi, rsi, rdx, rcx, r8, r9. Simple results
  // are returned in rax, and a struct of two pointers are returned in rax+rdx.
  // Larger return sizes must be written to an address passed as a hidden first
  // argument.
  const Register kCCallArg0 = rdi;
  const Register kCCallArg1 = rsi;
  const Register kCCallArg2 = rdx;
  const Register kCCallArg3 = rcx;
  const int kArgExtraStackSpace = 0;
  const int kMaxRegisterResultSize = 2;
#endif  // V8_TARGET_OS_WIN

  // Enter the exit frame that transitions from JavaScript to C++.
  int arg_stack_space =
      kArgExtraStackSpace +
      (result_size <= kMaxRegisterResultSize ? 0 : result_size);
  if (argv_mode == ArgvMode::kRegister) {
    DCHECK(save_doubles == SaveFPRegsMode::kIgnore);
    DCHECK(!builtin_exit_frame);
    __ EnterApiExitFrame(arg_stack_space);
    // Move argc into r12 (argv is already in r15).
    __ movq(r12, rax);
  } else {
    __ EnterExitFrame(
        arg_stack_space, save_doubles == SaveFPRegsMode::kSave,
        builtin_exit_frame ? StackFrame::BUILTIN_EXIT : StackFrame::EXIT);
  }

  // rbx: pointer to builtin function  (C callee-saved).
  // rbp: frame pointer of exit frame  (restored after C call).
  // rsp: stack pointer (restored after C call).
  // r12: number of arguments including receiver (C callee-saved).
  // r15: argv pointer (C callee-saved).

  // Check stack alignment.
  if (FLAG_debug_code) {
    __ CheckStackAlignment();
  }

  // Call C function. The arguments object will be created by stubs declared by
  // DECLARE_RUNTIME_FUNCTION().
  if (result_size <= kMaxRegisterResultSize) {
    // Pass a pointer to the Arguments object as the first argument.
    // Return result in single register (rax), or a register pair (rax, rdx).
    __ movq(kCCallArg0, r12);  // argc.
    __ movq(kCCallArg1, r15);  // argv.
    __ Move(kCCallArg2, ExternalReference::isolate_address(masm->isolate()));
  } else {
    DCHECK_LE(result_size, 2);
    // Pass a pointer to the result location as the first argument.
    __ leaq(kCCallArg0, StackSpaceOperand(kArgExtraStackSpace));
    // Pass a pointer to the Arguments object as the second argument.
    __ movq(kCCallArg1, r12);  // argc.
    __ movq(kCCallArg2, r15);  // argv.
    __ Move(kCCallArg3, ExternalReference::isolate_address(masm->isolate()));
  }
  __ call(rbx);

  if (result_size > kMaxRegisterResultSize) {
    // Read result values stored on stack. Result is stored
    // above the the two Arguments object slots on Win64.
    DCHECK_LE(result_size, 2);
    __ movq(kReturnRegister0, StackSpaceOperand(kArgExtraStackSpace + 0));
    __ movq(kReturnRegister1, StackSpaceOperand(kArgExtraStackSpace + 1));
  }
  // Result is in rax or rdx:rax - do not destroy these registers!

  // Check result for exception sentinel.
  Label exception_returned;
  __ CompareRoot(rax, RootIndex::kException);
  __ j(equal, &exception_returned);

  // Check that there is no pending exception, otherwise we
  // should have returned the exception sentinel.
  if (FLAG_debug_code) {
    Label okay;
    __ LoadRoot(kScratchRegister, RootIndex::kTheHoleValue);
    ExternalReference pending_exception_address = ExternalReference::Create(
        IsolateAddressId::kPendingExceptionAddress, masm->isolate());
    Operand pending_exception_operand =
        masm->ExternalReferenceAsOperand(pending_exception_address);
    __ cmp_tagged(kScratchRegister, pending_exception_operand);
    __ j(equal, &okay, Label::kNear);
    __ int3();
    __ bind(&okay);
  }

  // Exit the JavaScript to C++ exit frame.
  __ LeaveExitFrame(save_doubles == SaveFPRegsMode::kSave,
                    argv_mode == ArgvMode::kStack);
  __ ret(0);

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

  // Ask the runtime for help to determine the handler. This will set rax to
  // contain the current pending exception, don't clobber it.
  ExternalReference find_handler =
      ExternalReference::Create(Runtime::kUnwindAndFindExceptionHandler);
  {
    FrameScope scope(masm, StackFrame::MANUAL);
    __ movq(arg_reg_1, Immediate(0));  // argc.
    __ movq(arg_reg_2, Immediate(0));  // argv.
    __ Move(arg_reg_3, ExternalReference::isolate_address(masm->isolate()));
    __ PrepareCallCFunction(3);
    __ CallCFunction(find_handler, 3);
  }
  // Retrieve the handler context, SP and FP.
  __ movq(rsi,
          masm->ExternalReferenceAsOperand(pending_handler_context_address));
  __ movq(rsp, masm->ExternalReferenceAsOperand(pending_handler_sp_address));
  __ movq(rbp, masm->ExternalReferenceAsOperand(pending_handler_fp_address));

  // If the handler is a JS frame, restore the context to the frame. Note that
  // the context will be set to (rsi == 0) for non-JS frames.
  Label skip;
  __ testq(rsi, rsi);
  __ j(zero, &skip, Label::kNear);
  __ movq(Operand(rbp, StandardFrameConstants::kContextOffset), rsi);
  __ bind(&skip);

  // Reset the masking register. This is done independent of the underlying
  // feature flag {FLAG_untrusted_code_mitigations} to make the snapshot work
  // with both configurations. It is safe to always do this, because the
  // underlying register is caller-saved and can be arbitrarily clobbered.
  __ ResetSpeculationPoisonRegister();

  // Clear c_entry_fp, like we do in `LeaveExitFrame`.
  ExternalReference c_entry_fp_address = ExternalReference::Create(
      IsolateAddressId::kCEntryFPAddress, masm->isolate());
  Operand c_entry_fp_operand =
      masm->ExternalReferenceAsOperand(c_entry_fp_address);
  __ movq(c_entry_fp_operand, Immediate(0));

  // Compute the handler entry address and jump to it.
  __ movq(rdi,
          masm->ExternalReferenceAsOperand(pending_handler_entrypoint_address));
  __ jmp(rdi);
}

void Builtins::Generate_DoubleToI(MacroAssembler* masm) {
  Label check_negative, process_64_bits, done;

  // Account for return address and saved regs.
  const int kArgumentOffset = 4 * kSystemPointerSize;

  MemOperand mantissa_operand(MemOperand(rsp, kArgumentOffset));
  MemOperand exponent_operand(
      MemOperand(rsp, kArgumentOffset + kDoubleSize / 2));

  // The result is returned on the stack.
  MemOperand return_operand = mantissa_operand;

  Register scratch1 = rbx;

  // Since we must use rcx for shifts below, use some other register (rax)
  // to calculate the result if ecx is the requested return register.
  Register result_reg = rax;
  // Save ecx if it isn't the return register and therefore volatile, or if it
  // is the return register, then save the temp register we use in its stead
  // for the result.
  Register save_reg = rax;
  __ pushq(rcx);
  __ pushq(scratch1);
  __ pushq(save_reg);

  __ movl(scratch1, mantissa_operand);
  __ Movsd(kScratchDoubleReg, mantissa_operand);
  __ movl(rcx, exponent_operand);

  __ andl(rcx, Immediate(HeapNumber::kExponentMask));
  __ shrl(rcx, Immediate(HeapNumber::kExponentShift));
  __ leal(result_reg, MemOperand(rcx, -HeapNumber::kExponentBias));
  __ cmpl(result_reg, Immediate(HeapNumber::kMantissaBits));
  __ j(below, &process_64_bits, Label::kNear);

  // Result is entirely in lower 32-bits of mantissa
  int delta = HeapNumber::kExponentBias + Double::kPhysicalSignificandSize;
  __ subl(rcx, Immediate(delta));
  __ xorl(result_reg, result_reg);
  __ cmpl(rcx, Immediate(31));
  __ j(above, &done, Label::kNear);
  __ shll_cl(scratch1);
  __ jmp(&check_negative, Label::kNear);

  __ bind(&process_64_bits);
  __ Cvttsd2siq(result_reg, kScratchDoubleReg);
  __ jmp(&done, Label::kNear);

  // If the double was negative, negate the integer result.
  __ bind(&check_negative);
  __ movl(result_reg, scratch1);
  __ negl(result_reg);
  __ cmpl(exponent_operand, Immediate(0));
  __ cmovl(greater, result_reg, scratch1);

  // Restore registers
  __ bind(&done);
  __ movl(return_operand, result_reg);
  __ popq(save_reg);
  __ popq(scratch1);
  __ popq(rcx);
  __ ret(0);
}

namespace {

int Offset(ExternalReference ref0, ExternalReference ref1) {
  int64_t offset = (ref0.address() - ref1.address());
  // Check that fits into int.
  DCHECK(static_cast<int>(offset) == offset);
  return static_cast<int>(offset);
}

// Calls an API function.  Allocates HandleScope, extracts returned value
// from handle and propagates exceptions.  Clobbers r12, r15, rbx and
// caller-save registers.  Restores context.  On return removes
// stack_space * kSystemPointerSize (GCed).
void CallApiFunctionAndReturn(MacroAssembler* masm, Register function_address,
                              ExternalReference thunk_ref,
                              Register thunk_last_arg, int stack_space,
                              Operand* stack_space_operand,
                              Operand return_value_operand) {
  Label prologue;
  Label promote_scheduled_exception;
  Label delete_allocated_handles;
  Label leave_exit_frame;

  Isolate* isolate = masm->isolate();
  Factory* factory = isolate->factory();
  ExternalReference next_address =
      ExternalReference::handle_scope_next_address(isolate);
  const int kNextOffset = 0;
  const int kLimitOffset = Offset(
      ExternalReference::handle_scope_limit_address(isolate), next_address);
  const int kLevelOffset = Offset(
      ExternalReference::handle_scope_level_address(isolate), next_address);
  ExternalReference scheduled_exception_address =
      ExternalReference::scheduled_exception_address(isolate);

  DCHECK(rdx == function_address || r8 == function_address);
  // Allocate HandleScope in callee-save registers.
  Register prev_next_address_reg = r12;
  Register prev_limit_reg = rbx;
  Register base_reg = r15;
  __ Move(base_reg, next_address);
  __ movq(prev_next_address_reg, Operand(base_reg, kNextOffset));
  __ movq(prev_limit_reg, Operand(base_reg, kLimitOffset));
  __ addl(Operand(base_reg, kLevelOffset), Immediate(1));

  Label profiler_enabled, end_profiler_check;
  __ Move(rax, ExternalReference::is_profiling_address(isolate));
  __ cmpb(Operand(rax, 0), Immediate(0));
  __ j(not_zero, &profiler_enabled);
  __ Move(rax, ExternalReference::address_of_runtime_stats_flag());
  __ cmpl(Operand(rax, 0), Immediate(0));
  __ j(not_zero, &profiler_enabled);
  {
    // Call the api function directly.
    __ Move(rax, function_address);
    __ jmp(&end_profiler_check);
  }
  __ bind(&profiler_enabled);
  {
    // Third parameter is the address of the actual getter function.
    __ Move(thunk_last_arg, function_address);
    __ Move(rax, thunk_ref);
  }
  __ bind(&end_profiler_check);

  // Call the api function!
  __ call(rax);

  // Load the value from ReturnValue
  __ movq(rax, return_value_operand);
  __ bind(&prologue);

  // No more valid handles (the result handle was the last one). Restore
  // previous handle scope.
  __ subl(Operand(base_reg, kLevelOffset), Immediate(1));
  __ movq(Operand(base_reg, kNextOffset), prev_next_address_reg);
  __ cmpq(prev_limit_reg, Operand(base_reg, kLimitOffset));
  __ j(not_equal, &delete_allocated_handles);

  // Leave the API exit frame.
  __ bind(&leave_exit_frame);
  if (stack_space_operand != nullptr) {
    DCHECK_EQ(stack_space, 0);
    __ movq(rbx, *stack_space_operand);
  }
  __ LeaveApiExitFrame();

  // Check if the function scheduled an exception.
  __ Move(rdi, scheduled_exception_address);
  __ Cmp(Operand(rdi, 0), factory->the_hole_value());
  __ j(not_equal, &promote_scheduled_exception);

#if DEBUG
  // Check if the function returned a valid JavaScript value.
  Label ok;
  Register return_value = rax;
  Register map = rcx;

  __ JumpIfSmi(return_value, &ok, Label::kNear);
  __ LoadMap(map, return_value);
  __ CmpInstanceType(map, LAST_NAME_TYPE);
  __ j(below_equal, &ok, Label::kNear);

  __ CmpInstanceType(map, FIRST_JS_RECEIVER_TYPE);
  __ j(above_equal, &ok, Label::kNear);

  __ CompareRoot(map, RootIndex::kHeapNumberMap);
  __ j(equal, &ok, Label::kNear);

  __ CompareRoot(map, RootIndex::kBigIntMap);
  __ j(equal, &ok, Label::kNear);

  __ CompareRoot(return_value, RootIndex::kUndefinedValue);
  __ j(equal, &ok, Label::kNear);

  __ CompareRoot(return_value, RootIndex::kTrueValue);
  __ j(equal, &ok, Label::kNear);

  __ CompareRoot(return_value, RootIndex::kFalseValue);
  __ j(equal, &ok, Label::kNear);

  __ CompareRoot(return_value, RootIndex::kNullValue);
  __ j(equal, &ok, Label::kNear);

  __ Abort(AbortReason::kAPICallReturnedInvalidObject);

  __ bind(&ok);
#endif

  if (stack_space_operand == nullptr) {
    DCHECK_NE(stack_space, 0);
    __ ret(stack_space * kSystemPointerSize);
  } else {
    DCHECK_EQ(stack_space, 0);
    __ PopReturnAddressTo(rcx);
    __ addq(rsp, rbx);
    __ jmp(rcx);
  }

  // Re-throw by promoting a scheduled exception.
  __ bind(&promote_scheduled_exception);
  __ TailCallRuntime(Runtime::kPromoteScheduledException);

  // HandleScope limit has changed. Delete allocated extensions.
  __ bind(&delete_allocated_handles);
  __ movq(Operand(base_reg, kLimitOffset), prev_limit_reg);
  __ movq(prev_limit_reg, rax);
  __ LoadAddress(arg_reg_1, ExternalReference::isolate_address(isolate));
  __ LoadAddress(rax, ExternalReference::delete_handle_scope_extensions());
  __ call(rax);
  __ movq(rax, prev_limit_reg);
  __ jmp(&leave_exit_frame);
}

}  // namespace

// TODO(jgruber): Instead of explicitly setting up implicit_args_ on the stack
// in CallApiCallback, we could use the calling convention to set up the stack
// correctly in the first place.
//
// TODO(jgruber): I suspect that most of CallApiCallback could be implemented
// as a C++ trampoline, vastly simplifying the assembly implementation.

void Builtins::Generate_CallApiCallback(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rsi                 : context
  //  -- rdx                 : api function address
  //  -- rcx                 : arguments count (not including the receiver)
  //  -- rbx                 : call data
  //  -- rdi                 : holder
  //  -- rsp[0]              : return address
  //  -- rsp[8]              : argument 0 (receiver)
  //  -- rsp[16]             : argument 1
  //  -- ...
  //  -- rsp[argc * 8]       : argument (argc - 1)
  //  -- rsp[(argc + 1) * 8] : argument argc
  // -----------------------------------

  Register api_function_address = rdx;
  Register argc = rcx;
  Register call_data = rbx;
  Register holder = rdi;

  DCHECK(!AreAliased(api_function_address, argc, holder, call_data,
                     kScratchRegister));

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
  // Current state:
  //   rsp[0]: return address
  //
  // Target state:
  //   rsp[0 * kSystemPointerSize]: return address
  //   rsp[1 * kSystemPointerSize]: kHolder
  //   rsp[2 * kSystemPointerSize]: kIsolate
  //   rsp[3 * kSystemPointerSize]: undefined (kReturnValueDefaultValue)
  //   rsp[4 * kSystemPointerSize]: undefined (kReturnValue)
  //   rsp[5 * kSystemPointerSize]: kData
  //   rsp[6 * kSystemPointerSize]: undefined (kNewTarget)

  __ PopReturnAddressTo(rax);
  __ LoadRoot(kScratchRegister, RootIndex::kUndefinedValue);
  __ Push(kScratchRegister);
  __ Push(call_data);
  __ Push(kScratchRegister);
  __ Push(kScratchRegister);
  __ PushAddress(ExternalReference::isolate_address(masm->isolate()));
  __ Push(holder);
  __ PushReturnAddressFrom(rax);

  // Keep a pointer to kHolder (= implicit_args) in a scratch register.
  // We use it below to set up the FunctionCallbackInfo object.
  Register scratch = rbx;
  __ leaq(scratch, Operand(rsp, 1 * kSystemPointerSize));

  // Allocate the v8::Arguments structure in the arguments' space since
  // it's not controlled by GC.
  static constexpr int kApiStackSpace = 4;
  __ EnterApiExitFrame(kApiStackSpace);

  // FunctionCallbackInfo::implicit_args_ (points at kHolder as set up above).
  __ movq(StackSpaceOperand(0), scratch);

  // FunctionCallbackInfo::values_ (points at the first varargs argument passed
  // on the stack).
  __ leaq(scratch,
          Operand(scratch, (FCA::kArgsLength + 1) * kSystemPointerSize));
  __ movq(StackSpaceOperand(1), scratch);

  // FunctionCallbackInfo::length_.
  __ movq(StackSpaceOperand(2), argc);

  // We also store the number of bytes to drop from the stack after returning
  // from the API function here.
  __ leaq(kScratchRegister,
          Operand(argc, times_system_pointer_size,
                  (FCA::kArgsLength + 1 /* receiver */) * kSystemPointerSize));
  __ movq(StackSpaceOperand(3), kScratchRegister);

  Register arguments_arg = arg_reg_1;
  Register callback_arg = arg_reg_2;

  // It's okay if api_function_address == callback_arg
  // but not arguments_arg
  DCHECK(api_function_address != arguments_arg);

  // v8::InvocationCallback's argument.
  __ leaq(arguments_arg, StackSpaceOperand(0));

  ExternalReference thunk_ref = ExternalReference::invoke_function_callback();

  // There are two stack slots above the arguments we constructed on the stack:
  // the stored ebp (pushed by EnterApiExitFrame), and the return address.
  static constexpr int kStackSlotsAboveFCA = 2;
  Operand return_value_operand(
      rbp,
      (kStackSlotsAboveFCA + FCA::kReturnValueOffset) * kSystemPointerSize);

  static constexpr int kUseStackSpaceOperand = 0;
  Operand stack_space_operand = StackSpaceOperand(3);
  CallApiFunctionAndReturn(masm, api_function_address, thunk_ref, callback_arg,
                           kUseStackSpaceOperand, &stack_space_operand,
                           return_value_operand);
}

void Builtins::Generate_CallApiGetter(MacroAssembler* masm) {
  Register name_arg = arg_reg_1;
  Register accessor_info_arg = arg_reg_2;
  Register getter_arg = arg_reg_3;
  Register api_function_address = r8;
  Register receiver = ApiGetterDescriptor::ReceiverRegister();
  Register holder = ApiGetterDescriptor::HolderRegister();
  Register callback = ApiGetterDescriptor::CallbackRegister();
  Register scratch = rax;
  Register decompr_scratch1 = COMPRESS_POINTERS_BOOL ? r15 : no_reg;

  DCHECK(!AreAliased(receiver, holder, callback, scratch, decompr_scratch1));

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

  // Insert additional parameters into the stack frame above return address.
  __ PopReturnAddressTo(scratch);
  __ Push(receiver);
  __ PushTaggedAnyField(FieldOperand(callback, AccessorInfo::kDataOffset),
                        decompr_scratch1);
  __ LoadRoot(kScratchRegister, RootIndex::kUndefinedValue);
  __ Push(kScratchRegister);  // return value
  __ Push(kScratchRegister);  // return value default
  __ PushAddress(ExternalReference::isolate_address(masm->isolate()));
  __ Push(holder);
  __ Push(Smi::zero());  // should_throw_on_error -> false
  __ PushTaggedPointerField(FieldOperand(callback, AccessorInfo::kNameOffset),
                            decompr_scratch1);
  __ PushReturnAddressFrom(scratch);

  // v8::PropertyCallbackInfo::args_ array and name handle.
  const int kStackUnwindSpace = PropertyCallbackArguments::kArgsLength + 1;

  // Allocate v8::PropertyCallbackInfo in non-GCed stack space.
  const int kArgStackSpace = 1;

  // Load address of v8::PropertyAccessorInfo::args_ array.
  __ leaq(scratch, Operand(rsp, 2 * kSystemPointerSize));

  __ EnterApiExitFrame(kArgStackSpace);

  // Create v8::PropertyCallbackInfo object on the stack and initialize
  // it's args_ field.
  Operand info_object = StackSpaceOperand(0);
  __ movq(info_object, scratch);

  __ leaq(name_arg, Operand(scratch, -kSystemPointerSize));
  // The context register (rsi) has been saved in EnterApiExitFrame and
  // could be used to pass arguments.
  __ leaq(accessor_info_arg, info_object);

  ExternalReference thunk_ref =
      ExternalReference::invoke_accessor_getter_callback();

  // It's okay if api_function_address == getter_arg
  // but not accessor_info_arg or name_arg
  DCHECK(api_function_address != accessor_info_arg);
  DCHECK(api_function_address != name_arg);
  __ LoadTaggedPointerField(
      scratch, FieldOperand(callback, AccessorInfo::kJsGetterOffset));
  __ LoadExternalPointerField(
      api_function_address,
      FieldOperand(scratch, Foreign::kForeignAddressOffset),
      kForeignForeignAddressTag, kScratchRegister);

  // +3 is to skip prolog, return address and name handle.
  Operand return_value_operand(
      rbp,
      (PropertyCallbackArguments::kReturnValueOffset + 3) * kSystemPointerSize);
  Operand* const kUseStackSpaceConstant = nullptr;
  CallApiFunctionAndReturn(masm, api_function_address, thunk_ref, getter_arg,
                           kStackUnwindSpace, kUseStackSpaceConstant,
                           return_value_operand);
}

void Builtins::Generate_DirectCEntry(MacroAssembler* masm) {
  __ int3();  // Unused on this architecture.
}

namespace {

void Generate_DeoptimizationEntry(MacroAssembler* masm,
                                  DeoptimizeKind deopt_kind) {
  Isolate* isolate = masm->isolate();

  // Save all double registers, they will later be copied to the deoptimizer's
  // FrameDescription.
  static constexpr int kDoubleRegsSize =
      kDoubleSize * XMMRegister::kNumRegisters;
  __ AllocateStackSpace(kDoubleRegsSize);

  const RegisterConfiguration* config = RegisterConfiguration::Default();
  for (int i = 0; i < config->num_allocatable_double_registers(); ++i) {
    int code = config->GetAllocatableDoubleCode(i);
    XMMRegister xmm_reg = XMMRegister::from_code(code);
    int offset = code * kDoubleSize;
    __ Movsd(Operand(rsp, offset), xmm_reg);
  }

  // Save all general purpose registers, they will later be copied to the
  // deoptimizer's FrameDescription.
  static constexpr int kNumberOfRegisters = Register::kNumRegisters;
  for (int i = 0; i < kNumberOfRegisters; i++) {
    __ pushq(Register::from_code(i));
  }

  static constexpr int kSavedRegistersAreaSize =
      kNumberOfRegisters * kSystemPointerSize + kDoubleRegsSize;
  static constexpr int kCurrentOffsetToReturnAddress = kSavedRegistersAreaSize;
  static constexpr int kCurrentOffsetToParentSP =
      kCurrentOffsetToReturnAddress + kPCOnStackSize;

  __ Store(
      ExternalReference::Create(IsolateAddressId::kCEntryFPAddress, isolate),
      rbp);

  // We use this to keep the value of the fifth argument temporarily.
  // Unfortunately we can't store it directly in r8 (used for passing
  // this on linux), since it is another parameter passing register on windows.
  Register arg5 = r15;

  __ movq(arg_reg_3, Immediate(Deoptimizer::kFixedExitSizeMarker));
  // Get the address of the location in the code object
  // and compute the fp-to-sp delta in register arg5.
  __ movq(arg_reg_4, Operand(rsp, kCurrentOffsetToReturnAddress));
  // Load the fp-to-sp-delta.
  __ leaq(arg5, Operand(rsp, kCurrentOffsetToParentSP));
  __ subq(arg5, rbp);
  __ negq(arg5);

  // Allocate a new deoptimizer object.
  __ PrepareCallCFunction(6);
  __ movq(rax, Immediate(0));
  Label context_check;
  __ movq(rdi, Operand(rbp, CommonFrameConstants::kContextOrFrameTypeOffset));
  __ JumpIfSmi(rdi, &context_check);
  __ movq(rax, Operand(rbp, StandardFrameConstants::kFunctionOffset));
  __ bind(&context_check);
  __ movq(arg_reg_1, rax);
  __ Move(arg_reg_2, static_cast<int>(deopt_kind));
  // Args 3 and 4 are already in the right registers.

  // On windows put the arguments on the stack (PrepareCallCFunction
  // has created space for this). On linux pass the arguments in r8 and r9.
#ifdef V8_TARGET_OS_WIN
  __ movq(Operand(rsp, 4 * kSystemPointerSize), arg5);
  __ LoadAddress(arg5, ExternalReference::isolate_address(isolate));
  __ movq(Operand(rsp, 5 * kSystemPointerSize), arg5);
#else
  __ movq(r8, arg5);
  __ LoadAddress(r9, ExternalReference::isolate_address(isolate));
#endif

  {
    AllowExternalCallThatCantCauseGC scope(masm);
    __ CallCFunction(ExternalReference::new_deoptimizer_function(), 6);
  }
  // Preserve deoptimizer object in register rax and get the input
  // frame descriptor pointer.
  __ movq(rbx, Operand(rax, Deoptimizer::input_offset()));

  // Fill in the input registers.
  for (int i = kNumberOfRegisters - 1; i >= 0; i--) {
    int offset =
        (i * kSystemPointerSize) + FrameDescription::registers_offset();
    __ PopQuad(Operand(rbx, offset));
  }

  // Fill in the double input registers.
  int double_regs_offset = FrameDescription::double_registers_offset();
  for (int i = 0; i < XMMRegister::kNumRegisters; i++) {
    int dst_offset = i * kDoubleSize + double_regs_offset;
    __ popq(Operand(rbx, dst_offset));
  }

  // Mark the stack as not iterable for the CPU profiler which won't be able to
  // walk the stack without the return address.
  __ movb(__ ExternalReferenceAsOperand(
              ExternalReference::stack_is_iterable_address(isolate)),
          Immediate(0));

  // Remove the return address from the stack.
  __ addq(rsp, Immediate(kPCOnStackSize));

  // Compute a pointer to the unwinding limit in register rcx; that is
  // the first stack slot not part of the input frame.
  __ movq(rcx, Operand(rbx, FrameDescription::frame_size_offset()));
  __ addq(rcx, rsp);

  // Unwind the stack down to - but not including - the unwinding
  // limit and copy the contents of the activation frame to the input
  // frame description.
  __ leaq(rdx, Operand(rbx, FrameDescription::frame_content_offset()));
  Label pop_loop_header;
  __ jmp(&pop_loop_header);
  Label pop_loop;
  __ bind(&pop_loop);
  __ Pop(Operand(rdx, 0));
  __ addq(rdx, Immediate(sizeof(intptr_t)));
  __ bind(&pop_loop_header);
  __ cmpq(rcx, rsp);
  __ j(not_equal, &pop_loop);

  // Compute the output frame in the deoptimizer.
  __ pushq(rax);
  __ PrepareCallCFunction(2);
  __ movq(arg_reg_1, rax);
  __ LoadAddress(arg_reg_2, ExternalReference::isolate_address(isolate));
  {
    AllowExternalCallThatCantCauseGC scope(masm);
    __ CallCFunction(ExternalReference::compute_output_frames_function(), 2);
  }
  __ popq(rax);

  __ movq(rsp, Operand(rax, Deoptimizer::caller_frame_top_offset()));

  // Replace the current (input) frame with the output frames.
  Label outer_push_loop, inner_push_loop, outer_loop_header, inner_loop_header;
  // Outer loop state: rax = current FrameDescription**, rdx = one past the
  // last FrameDescription**.
  __ movl(rdx, Operand(rax, Deoptimizer::output_count_offset()));
  __ movq(rax, Operand(rax, Deoptimizer::output_offset()));
  __ leaq(rdx, Operand(rax, rdx, times_system_pointer_size, 0));
  __ jmp(&outer_loop_header);
  __ bind(&outer_push_loop);
  // Inner loop state: rbx = current FrameDescription*, rcx = loop index.
  __ movq(rbx, Operand(rax, 0));
  __ movq(rcx, Operand(rbx, FrameDescription::frame_size_offset()));
  __ jmp(&inner_loop_header);
  __ bind(&inner_push_loop);
  __ subq(rcx, Immediate(sizeof(intptr_t)));
  __ Push(Operand(rbx, rcx, times_1, FrameDescription::frame_content_offset()));
  __ bind(&inner_loop_header);
  __ testq(rcx, rcx);
  __ j(not_zero, &inner_push_loop);
  __ addq(rax, Immediate(kSystemPointerSize));
  __ bind(&outer_loop_header);
  __ cmpq(rax, rdx);
  __ j(below, &outer_push_loop);

  for (int i = 0; i < config->num_allocatable_double_registers(); ++i) {
    int code = config->GetAllocatableDoubleCode(i);
    XMMRegister xmm_reg = XMMRegister::from_code(code);
    int src_offset = code * kDoubleSize + double_regs_offset;
    __ Movsd(xmm_reg, Operand(rbx, src_offset));
  }

  // Push pc and continuation from the last output frame.
  __ PushQuad(Operand(rbx, FrameDescription::pc_offset()));
  __ PushQuad(Operand(rbx, FrameDescription::continuation_offset()));

  // Push the registers from the last output frame.
  for (int i = 0; i < kNumberOfRegisters; i++) {
    int offset =
        (i * kSystemPointerSize) + FrameDescription::registers_offset();
    __ PushQuad(Operand(rbx, offset));
  }

  // Restore the registers from the stack.
  for (int i = kNumberOfRegisters - 1; i >= 0; i--) {
    Register r = Register::from_code(i);
    // Do not restore rsp, simply pop the value into the next register
    // and overwrite this afterwards.
    if (r == rsp) {
      DCHECK_GT(i, 0);
      r = Register::from_code(i - 1);
    }
    __ popq(r);
  }

  __ movb(__ ExternalReferenceAsOperand(
              ExternalReference::stack_is_iterable_address(isolate)),
          Immediate(1));

  // Return to the continuation point.
  __ ret(0);
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
  __ pushq(kInterpreterAccumulatorRegister);
  Label start;
  __ bind(&start);

  // Get function from the frame.
  Register closure = rdi;
  __ movq(closure, MemOperand(rbp, StandardFrameConstants::kFunctionOffset));

  // Load the feedback vector.
  Register feedback_vector = rbx;
  __ LoadTaggedPointerField(
      feedback_vector, FieldOperand(closure, JSFunction::kFeedbackCellOffset));
  __ LoadTaggedPointerField(feedback_vector,
                            FieldOperand(feedback_vector, Cell::kValueOffset));

  Label install_baseline_code;
  // Check if feedback vector is valid. If not, call prepare for baseline to
  // allocate it.
  __ CmpObjectType(feedback_vector, FEEDBACK_VECTOR_TYPE, kScratchRegister);
  __ j(not_equal, &install_baseline_code);

  // Save BytecodeOffset from the stack frame.
  __ SmiUntag(
      kInterpreterBytecodeOffsetRegister,
      MemOperand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp));
  // Replace BytecodeOffset with the feedback vector.
  __ movq(MemOperand(rbp, InterpreterFrameConstants::kBytecodeOffsetFromFp),
          feedback_vector);
  feedback_vector = no_reg;

  // Get the Code object from the shared function info.
  Register code_obj = rbx;
  __ LoadTaggedPointerField(
      code_obj, FieldOperand(closure, JSFunction::kSharedFunctionInfoOffset));
  __ LoadTaggedPointerField(
      code_obj,
      FieldOperand(code_obj, SharedFunctionInfo::kFunctionDataOffset));
  __ LoadTaggedPointerField(
      code_obj, FieldOperand(code_obj, BaselineData::kBaselineCodeOffset));

  // Compute baseline pc for bytecode offset.
  ExternalReference get_baseline_pc_extref;
  if (next_bytecode || is_osr) {
    get_baseline_pc_extref =
        ExternalReference::baseline_pc_for_next_executed_bytecode();
  } else {
    get_baseline_pc_extref =
        ExternalReference::baseline_pc_for_bytecode_offset();
  }
  Register get_baseline_pc = rax;
  __ LoadAddress(get_baseline_pc, get_baseline_pc_extref);

  // If the code deoptimizes during the implicit function entry stack interrupt
  // check, it will have a bailout ID of kFunctionEntryBytecodeOffset, which is
  // not a valid bytecode offset.
  // TODO(pthier): Investigate if it is feasible to handle this special case
  // in TurboFan instead of here.
  Label valid_bytecode_offset, function_entry_bytecode;
  if (!is_osr) {
    __ cmpq(kInterpreterBytecodeOffsetRegister,
            Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag +
                      kFunctionEntryBytecodeOffset));
    __ j(equal, &function_entry_bytecode);
  }

  __ subq(kInterpreterBytecodeOffsetRegister,
          Immediate(BytecodeArray::kHeaderSize - kHeapObjectTag));

  __ bind(&valid_bytecode_offset);
  // Get bytecode array from the stack frame.
  __ movq(kInterpreterBytecodeArrayRegister,
          MemOperand(rbp, InterpreterFrameConstants::kBytecodeArrayFromFp));
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ PrepareCallCFunction(3);
    __ movq(arg_reg_1, code_obj);
    __ movq(arg_reg_2, kInterpreterBytecodeOffsetRegister);
    __ movq(arg_reg_3, kInterpreterBytecodeArrayRegister);
    __ CallCFunction(get_baseline_pc, 3);
  }
  __ leaq(code_obj,
          FieldOperand(code_obj, kReturnRegister0, times_1, Code::kHeaderSize));
  __ popq(kInterpreterAccumulatorRegister);

  if (is_osr) {
    // Reset the OSR loop nesting depth to disarm back edges.
    // TODO(pthier): Separate baseline Sparkplug from TF arming and don't disarm
    // Sparkplug here.
    __ movw(FieldOperand(kInterpreterBytecodeArrayRegister,
                         BytecodeArray::kOsrNestingLevelOffset),
            Immediate(0));
    Generate_OSREntry(masm, code_obj);
  } else {
    __ jmp(code_obj);
  }
  __ Trap();  // Unreachable.

  if (!is_osr) {
    __ bind(&function_entry_bytecode);
    // If the bytecode offset is kFunctionEntryOffset, get the start address of
    // the first bytecode.
    __ movq(kInterpreterBytecodeOffsetRegister, Immediate(0));
    if (next_bytecode) {
      __ LoadAddress(get_baseline_pc,
                     ExternalReference::baseline_pc_for_bytecode_offset());
    }
    __ jmp(&valid_bytecode_offset);
  }

  __ bind(&install_baseline_code);
  {
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ Push(closure);
    __ CallRuntime(Runtime::kInstallBaselineCode, 1);
  }
  // Retry from the start after installing baseline code.
  __ jmp(&start);
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
  __ movq(handler_arg, Operand(rbp, CommonFrameConstants::kCallerPCOffset));
  __ movq(slot_arg, Operand(handler_arg,
                            Deoptimizer::kEagerWithResumeImmedArgs1PcOffset));
  __ movq(
      handler_arg,
      Operand(handler_arg, Deoptimizer::kEagerWithResumeImmedArgs2PcOffset));

  __ Call(BUILTIN_CODE(masm->isolate(), DynamicCheckMaps),
          RelocInfo::CODE_TARGET);

  Label deopt, bailout;
  __ cmpq(rax, Immediate(static_cast<int>(DynamicCheckMapsStatus::kSuccess)));
  __ j(not_equal, &deopt);

  __ RestoreRegisters(registers);
  __ LeaveFrame(StackFrame::INTERNAL);
  __ Ret();

  __ bind(&deopt);
  __ cmpq(rax, Immediate(static_cast<int>(DynamicCheckMapsStatus::kBailout)));
  __ j(equal, &bailout);

  if (FLAG_debug_code) {
    __ cmpq(rax, Immediate(static_cast<int>(DynamicCheckMapsStatus::kDeopt)));
    __ Assert(equal, AbortReason::kUnexpectedDynamicCheckMapsStatus);
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

#endif  // V8_TARGET_ARCH_X64
