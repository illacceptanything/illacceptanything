// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/interpreter/bytecode-generator.h"

#include "src/api/api-inl.h"
#include "src/ast/ast-source-ranges.h"
#include "src/ast/ast.h"
#include "src/ast/scopes.h"
#include "src/builtins/builtins-constructor.h"
#include "src/codegen/compiler.h"
#include "src/codegen/unoptimized-compilation-info.h"
#include "src/common/globals.h"
#include "src/interpreter/bytecode-flags.h"
#include "src/interpreter/bytecode-jump-table.h"
#include "src/interpreter/bytecode-label.h"
#include "src/interpreter/bytecode-register-allocator.h"
#include "src/interpreter/bytecode-register.h"
#include "src/interpreter/control-flow-builders.h"
#include "src/logging/local-logger.h"
#include "src/logging/log.h"
#include "src/objects/debug-objects.h"
#include "src/objects/literal-objects-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/smi.h"
#include "src/objects/template-objects-inl.h"
#include "src/parsing/parse-info.h"
#include "src/parsing/token.h"
#include "src/utils/ostreams.h"

namespace v8 {
namespace internal {
namespace interpreter {

// Scoped class tracking context objects created by the visitor. Represents
// mutations of the context chain within the function body, allowing pushing and
// popping of the current {context_register} during visitation.
class V8_NODISCARD BytecodeGenerator::ContextScope {
 public:
  ContextScope(BytecodeGenerator* generator, Scope* scope,
               Register outer_context_reg = Register())
      : generator_(generator),
        scope_(scope),
        outer_(generator_->execution_context()),
        register_(Register::current_context()),
        depth_(0) {
    DCHECK(scope->NeedsContext() || outer_ == nullptr);
    if (outer_) {
      depth_ = outer_->depth_ + 1;

      // Push the outer context into a new context register.
      if (!outer_context_reg.is_valid()) {
        outer_context_reg = generator_->register_allocator()->NewRegister();
      }
      outer_->set_register(outer_context_reg);
      generator_->builder()->PushContext(outer_context_reg);
    }
    generator_->set_execution_context(this);
  }

  ~ContextScope() {
    if (outer_) {
      DCHECK_EQ(register_.index(), Register::current_context().index());
      generator_->builder()->PopContext(outer_->reg());
      outer_->set_register(register_);
    }
    generator_->set_execution_context(outer_);
  }

  ContextScope(const ContextScope&) = delete;
  ContextScope& operator=(const ContextScope&) = delete;

  // Returns the depth of the given |scope| for the current execution context.
  int ContextChainDepth(Scope* scope) {
    return scope_->ContextChainLength(scope);
  }

  // Returns the execution context at |depth| in the current context chain if it
  // is a function local execution context, otherwise returns nullptr.
  ContextScope* Previous(int depth) {
    if (depth > depth_) {
      return nullptr;
    }

    ContextScope* previous = this;
    for (int i = depth; i > 0; --i) {
      previous = previous->outer_;
    }
    return previous;
  }

  Register reg() const { return register_; }

 private:
  const BytecodeArrayBuilder* builder() const { return generator_->builder(); }

  void set_register(Register reg) { register_ = reg; }

  BytecodeGenerator* generator_;
  Scope* scope_;
  ContextScope* outer_;
  Register register_;
  int depth_;
};

// Scoped class for tracking control statements entered by the
// visitor.
class V8_NODISCARD BytecodeGenerator::ControlScope {
 public:
  explicit ControlScope(BytecodeGenerator* generator)
      : generator_(generator),
        outer_(generator->execution_control()),
        context_(generator->execution_context()) {
    generator_->set_execution_control(this);
  }
  ~ControlScope() { generator_->set_execution_control(outer()); }
  ControlScope(const ControlScope&) = delete;
  ControlScope& operator=(const ControlScope&) = delete;

  void Break(Statement* stmt) {
    PerformCommand(CMD_BREAK, stmt, kNoSourcePosition);
  }
  void Continue(Statement* stmt) {
    PerformCommand(CMD_CONTINUE, stmt, kNoSourcePosition);
  }
  void ReturnAccumulator(int source_position) {
    PerformCommand(CMD_RETURN, nullptr, source_position);
  }
  void AsyncReturnAccumulator(int source_position) {
    PerformCommand(CMD_ASYNC_RETURN, nullptr, source_position);
  }

  class DeferredCommands;

 protected:
  enum Command {
    CMD_BREAK,
    CMD_CONTINUE,
    CMD_RETURN,
    CMD_ASYNC_RETURN,
    CMD_RETHROW
  };
  static constexpr bool CommandUsesAccumulator(Command command) {
    return command != CMD_BREAK && command != CMD_CONTINUE;
  }

  void PerformCommand(Command command, Statement* statement,
                      int source_position);
  virtual bool Execute(Command command, Statement* statement,
                       int source_position) = 0;

  // Helper to pop the context chain to a depth expected by this control scope.
  // Note that it is the responsibility of each individual {Execute} method to
  // trigger this when commands are handled and control-flow continues locally.
  void PopContextToExpectedDepth();

  BytecodeGenerator* generator() const { return generator_; }
  ControlScope* outer() const { return outer_; }
  ContextScope* context() const { return context_; }

 private:
  BytecodeGenerator* generator_;
  ControlScope* outer_;
  ContextScope* context_;
};

// Helper class for a try-finally control scope. It can record intercepted
// control-flow commands that cause entry into a finally-block, and re-apply
// them after again leaving that block. Special tokens are used to identify
// paths going through the finally-block to dispatch after leaving the block.
class V8_NODISCARD BytecodeGenerator::ControlScope::DeferredCommands final {
 public:
  // Fixed value tokens for paths we know we need.
  // Fallthrough is set to -1 to make it the fallthrough case of the jump table,
  // where the remaining cases start at 0.
  static const int kFallthroughToken = -1;
  // TODO(leszeks): Rethrow being 0 makes it use up a valuable LdaZero, which
  // means that other commands (such as break or return) have to use LdaSmi.
  // This can very slightly bloat bytecode, so perhaps token values should all
  // be shifted down by 1.
  static const int kRethrowToken = 0;

  DeferredCommands(BytecodeGenerator* generator, Register token_register,
                   Register result_register)
      : generator_(generator),
        deferred_(generator->zone()),
        token_register_(token_register),
        result_register_(result_register),
        return_token_(-1),
        async_return_token_(-1) {
    // There's always a rethrow path.
    // TODO(leszeks): We could decouple deferred_ index and token to allow us
    // to still push this lazily.
    STATIC_ASSERT(kRethrowToken == 0);
    deferred_.push_back({CMD_RETHROW, nullptr, kRethrowToken});
  }

  // One recorded control-flow command.
  struct Entry {
    Command command;       // The command type being applied on this path.
    Statement* statement;  // The target statement for the command or {nullptr}.
    int token;             // A token identifying this particular path.
  };

  // Records a control-flow command while entering the finally-block. This also
  // generates a new dispatch token that identifies one particular path. This
  // expects the result to be in the accumulator.
  void RecordCommand(Command command, Statement* statement) {
    int token = GetTokenForCommand(command, statement);

    DCHECK_LT(token, deferred_.size());
    DCHECK_EQ(deferred_[token].command, command);
    DCHECK_EQ(deferred_[token].statement, statement);
    DCHECK_EQ(deferred_[token].token, token);

    if (CommandUsesAccumulator(command)) {
      builder()->StoreAccumulatorInRegister(result_register_);
    }
    builder()->LoadLiteral(Smi::FromInt(token));
    builder()->StoreAccumulatorInRegister(token_register_);
    if (!CommandUsesAccumulator(command)) {
      // If we're not saving the accumulator in the result register, shove a
      // harmless value there instead so that it is still considered "killed" in
      // the liveness analysis. Normally we would LdaUndefined first, but the
      // Smi token value is just as good, and by reusing it we save a bytecode.
      builder()->StoreAccumulatorInRegister(result_register_);
    }
  }

  // Records the dispatch token to be used to identify the re-throw path when
  // the finally-block has been entered through the exception handler. This
  // expects the exception to be in the accumulator.
  void RecordHandlerReThrowPath() {
    // The accumulator contains the exception object.
    RecordCommand(CMD_RETHROW, nullptr);
  }

  // Records the dispatch token to be used to identify the implicit fall-through
  // path at the end of a try-block into the corresponding finally-block.
  void RecordFallThroughPath() {
    builder()->LoadLiteral(Smi::FromInt(kFallthroughToken));
    builder()->StoreAccumulatorInRegister(token_register_);
    // Since we're not saving the accumulator in the result register, shove a
    // harmless value there instead so that it is still considered "killed" in
    // the liveness analysis. Normally we would LdaUndefined first, but the Smi
    // token value is just as good, and by reusing it we save a bytecode.
    builder()->StoreAccumulatorInRegister(result_register_);
  }

  // Applies all recorded control-flow commands after the finally-block again.
  // This generates a dynamic dispatch on the token from the entry point.
  void ApplyDeferredCommands() {
    if (deferred_.size() == 0) return;

    BytecodeLabel fall_through;

    if (deferred_.size() == 1) {
      // For a single entry, just jump to the fallthrough if we don't match the
      // entry token.
      const Entry& entry = deferred_[0];

      builder()
          ->LoadLiteral(Smi::FromInt(entry.token))
          .CompareReference(token_register_)
          .JumpIfFalse(ToBooleanMode::kAlreadyBoolean, &fall_through);

      if (CommandUsesAccumulator(entry.command)) {
        builder()->LoadAccumulatorWithRegister(result_register_);
      }
      execution_control()->PerformCommand(entry.command, entry.statement,
                                          kNoSourcePosition);
    } else {
      // For multiple entries, build a jump table and switch on the token,
      // jumping to the fallthrough if none of them match.

      BytecodeJumpTable* jump_table =
          builder()->AllocateJumpTable(static_cast<int>(deferred_.size()), 0);
      builder()
          ->LoadAccumulatorWithRegister(token_register_)
          .SwitchOnSmiNoFeedback(jump_table)
          .Jump(&fall_through);
      for (const Entry& entry : deferred_) {
        builder()->Bind(jump_table, entry.token);

        if (CommandUsesAccumulator(entry.command)) {
          builder()->LoadAccumulatorWithRegister(result_register_);
        }
        execution_control()->PerformCommand(entry.command, entry.statement,
                                            kNoSourcePosition);
      }
    }

    builder()->Bind(&fall_through);
  }

  BytecodeArrayBuilder* builder() { return generator_->builder(); }
  ControlScope* execution_control() { return generator_->execution_control(); }

 private:
  int GetTokenForCommand(Command command, Statement* statement) {
    switch (command) {
      case CMD_RETURN:
        return GetReturnToken();
      case CMD_ASYNC_RETURN:
        return GetAsyncReturnToken();
      case CMD_RETHROW:
        return kRethrowToken;
      default:
        // TODO(leszeks): We could also search for entries with the same
        // command and statement.
        return GetNewTokenForCommand(command, statement);
    }
  }

  int GetReturnToken() {
    if (return_token_ == -1) {
      return_token_ = GetNewTokenForCommand(CMD_RETURN, nullptr);
    }
    return return_token_;
  }

  int GetAsyncReturnToken() {
    if (async_return_token_ == -1) {
      async_return_token_ = GetNewTokenForCommand(CMD_ASYNC_RETURN, nullptr);
    }
    return async_return_token_;
  }

  int GetNewTokenForCommand(Command command, Statement* statement) {
    int token = static_cast<int>(deferred_.size());
    deferred_.push_back({command, statement, token});
    return token;
  }

  BytecodeGenerator* generator_;
  ZoneVector<Entry> deferred_;
  Register token_register_;
  Register result_register_;

  // Tokens for commands that don't need a statement.
  int return_token_;
  int async_return_token_;
};

// Scoped class for dealing with control flow reaching the function level.
class BytecodeGenerator::ControlScopeForTopLevel final
    : public BytecodeGenerator::ControlScope {
 public:
  explicit ControlScopeForTopLevel(BytecodeGenerator* generator)
      : ControlScope(generator) {}

 protected:
  bool Execute(Command command, Statement* statement,
               int source_position) override {
    switch (command) {
      case CMD_BREAK:  // We should never see break/continue in top-level.
      case CMD_CONTINUE:
        UNREACHABLE();
      case CMD_RETURN:
        // No need to pop contexts, execution leaves the method body.
        generator()->BuildReturn(source_position);
        return true;
      case CMD_ASYNC_RETURN:
        // No need to pop contexts, execution leaves the method body.
        generator()->BuildAsyncReturn(source_position);
        return true;
      case CMD_RETHROW:
        // No need to pop contexts, execution leaves the method body.
        generator()->BuildReThrow();
        return true;
    }
    return false;
  }
};

// Scoped class for enabling break inside blocks and switch blocks.
class BytecodeGenerator::ControlScopeForBreakable final
    : public BytecodeGenerator::ControlScope {
 public:
  ControlScopeForBreakable(BytecodeGenerator* generator,
                           BreakableStatement* statement,
                           BreakableControlFlowBuilder* control_builder)
      : ControlScope(generator),
        statement_(statement),
        control_builder_(control_builder) {}

 protected:
  bool Execute(Command command, Statement* statement,
               int source_position) override {
    if (statement != statement_) return false;
    switch (command) {
      case CMD_BREAK:
        PopContextToExpectedDepth();
        control_builder_->Break();
        return true;
      case CMD_CONTINUE:
      case CMD_RETURN:
      case CMD_ASYNC_RETURN:
      case CMD_RETHROW:
        break;
    }
    return false;
  }

 private:
  Statement* statement_;
  BreakableControlFlowBuilder* control_builder_;
};

// Scoped class for enabling 'break' and 'continue' in iteration
// constructs, e.g. do...while, while..., for...
class BytecodeGenerator::ControlScopeForIteration final
    : public BytecodeGenerator::ControlScope {
 public:
  ControlScopeForIteration(BytecodeGenerator* generator,
                           IterationStatement* statement,
                           LoopBuilder* loop_builder)
      : ControlScope(generator),
        statement_(statement),
        loop_builder_(loop_builder) {}

 protected:
  bool Execute(Command command, Statement* statement,
               int source_position) override {
    if (statement != statement_) return false;
    switch (command) {
      case CMD_BREAK:
        PopContextToExpectedDepth();
        loop_builder_->Break();
        return true;
      case CMD_CONTINUE:
        PopContextToExpectedDepth();
        loop_builder_->Continue();
        return true;
      case CMD_RETURN:
      case CMD_ASYNC_RETURN:
      case CMD_RETHROW:
        break;
    }
    return false;
  }

 private:
  Statement* statement_;
  LoopBuilder* loop_builder_;
};

// Scoped class for enabling 'throw' in try-catch constructs.
class BytecodeGenerator::ControlScopeForTryCatch final
    : public BytecodeGenerator::ControlScope {
 public:
  ControlScopeForTryCatch(BytecodeGenerator* generator,
                          TryCatchBuilder* try_catch_builder)
      : ControlScope(generator) {}

 protected:
  bool Execute(Command command, Statement* statement,
               int source_position) override {
    switch (command) {
      case CMD_BREAK:
      case CMD_CONTINUE:
      case CMD_RETURN:
      case CMD_ASYNC_RETURN:
        break;
      case CMD_RETHROW:
        // No need to pop contexts, execution re-enters the method body via the
        // stack unwinding mechanism which itself restores contexts correctly.
        generator()->BuildReThrow();
        return true;
    }
    return false;
  }
};

// Scoped class for enabling control flow through try-finally constructs.
class BytecodeGenerator::ControlScopeForTryFinally final
    : public BytecodeGenerator::ControlScope {
 public:
  ControlScopeForTryFinally(BytecodeGenerator* generator,
                            TryFinallyBuilder* try_finally_builder,
                            DeferredCommands* commands)
      : ControlScope(generator),
        try_finally_builder_(try_finally_builder),
        commands_(commands) {}

 protected:
  bool Execute(Command command, Statement* statement,
               int source_position) override {
    switch (command) {
      case CMD_BREAK:
      case CMD_CONTINUE:
      case CMD_RETURN:
      case CMD_ASYNC_RETURN:
      case CMD_RETHROW:
        PopContextToExpectedDepth();
        // We don't record source_position here since we don't generate return
        // bytecode right here and will generate it later as part of finally
        // block. Each return bytecode generated in finally block will get own
        // return source position from corresponded return statement or we'll
        // use end of function if no return statement is presented.
        commands_->RecordCommand(command, statement);
        try_finally_builder_->LeaveTry();
        return true;
    }
    return false;
  }

 private:
  TryFinallyBuilder* try_finally_builder_;
  DeferredCommands* commands_;
};

// Allocate and fetch the coverage indices tracking NaryLogical Expressions.
class BytecodeGenerator::NaryCodeCoverageSlots {
 public:
  NaryCodeCoverageSlots(BytecodeGenerator* generator, NaryOperation* expr)
      : generator_(generator) {
    if (generator_->block_coverage_builder_ == nullptr) return;
    for (size_t i = 0; i < expr->subsequent_length(); i++) {
      coverage_slots_.push_back(
          generator_->AllocateNaryBlockCoverageSlotIfEnabled(expr, i));
    }
  }

  int GetSlotFor(size_t subsequent_expr_index) const {
    if (generator_->block_coverage_builder_ == nullptr) {
      return BlockCoverageBuilder::kNoCoverageArraySlot;
    }
    DCHECK(coverage_slots_.size() > subsequent_expr_index);
    return coverage_slots_[subsequent_expr_index];
  }

 private:
  BytecodeGenerator* generator_;
  std::vector<int> coverage_slots_;
};

void BytecodeGenerator::ControlScope::PerformCommand(Command command,
                                                     Statement* statement,
                                                     int source_position) {
  ControlScope* current = this;
  do {
    if (current->Execute(command, statement, source_position)) {
      return;
    }
    current = current->outer();
  } while (current != nullptr);
  UNREACHABLE();
}

void BytecodeGenerator::ControlScope::PopContextToExpectedDepth() {
  // Pop context to the expected depth. Note that this can in fact pop multiple
  // contexts at once because the {PopContext} bytecode takes a saved register.
  if (generator()->execution_context() != context()) {
    generator()->builder()->PopContext(context()->reg());
  }
}

class V8_NODISCARD BytecodeGenerator::RegisterAllocationScope final {
 public:
  explicit RegisterAllocationScope(BytecodeGenerator* generator)
      : generator_(generator),
        outer_next_register_index_(
            generator->register_allocator()->next_register_index()) {}

  ~RegisterAllocationScope() {
    generator_->register_allocator()->ReleaseRegisters(
        outer_next_register_index_);
  }

  RegisterAllocationScope(const RegisterAllocationScope&) = delete;
  RegisterAllocationScope& operator=(const RegisterAllocationScope&) = delete;

  BytecodeGenerator* generator() const { return generator_; }

 private:
  BytecodeGenerator* generator_;
  int outer_next_register_index_;
};

class V8_NODISCARD BytecodeGenerator::AccumulatorPreservingScope final {
 public:
  explicit AccumulatorPreservingScope(BytecodeGenerator* generator,
                                      AccumulatorPreservingMode mode)
      : generator_(generator) {
    if (mode == AccumulatorPreservingMode::kPreserve) {
      saved_accumulator_register_ =
          generator_->register_allocator()->NewRegister();
      generator_->builder()->StoreAccumulatorInRegister(
          saved_accumulator_register_);
    }
  }

  ~AccumulatorPreservingScope() {
    if (saved_accumulator_register_.is_valid()) {
      generator_->builder()->LoadAccumulatorWithRegister(
          saved_accumulator_register_);
    }
  }

  AccumulatorPreservingScope(const AccumulatorPreservingScope&) = delete;
  AccumulatorPreservingScope& operator=(const AccumulatorPreservingScope&) =
      delete;

 private:
  BytecodeGenerator* generator_;
  Register saved_accumulator_register_;
};

// Scoped base class for determining how the result of an expression will be
// used.
class V8_NODISCARD BytecodeGenerator::ExpressionResultScope {
 public:
  ExpressionResultScope(BytecodeGenerator* generator, Expression::Context kind)
      : outer_(generator->execution_result()),
        allocator_(generator),
        kind_(kind),
        type_hint_(TypeHint::kAny) {
    generator->set_execution_result(this);
  }

  ~ExpressionResultScope() {
    allocator_.generator()->set_execution_result(outer_);
  }

  ExpressionResultScope(const ExpressionResultScope&) = delete;
  ExpressionResultScope& operator=(const ExpressionResultScope&) = delete;

  bool IsEffect() const { return kind_ == Expression::kEffect; }
  bool IsValue() const { return kind_ == Expression::kValue; }
  bool IsTest() const { return kind_ == Expression::kTest; }

  TestResultScope* AsTest() {
    DCHECK(IsTest());
    return reinterpret_cast<TestResultScope*>(this);
  }

  // Specify expression always returns a Boolean result value.
  void SetResultIsBoolean() {
    DCHECK_EQ(type_hint_, TypeHint::kAny);
    type_hint_ = TypeHint::kBoolean;
  }

  void SetResultIsString() {
    DCHECK_EQ(type_hint_, TypeHint::kAny);
    type_hint_ = TypeHint::kString;
  }

  TypeHint type_hint() const { return type_hint_; }

 private:
  ExpressionResultScope* outer_;
  RegisterAllocationScope allocator_;
  Expression::Context kind_;
  TypeHint type_hint_;
};

// Scoped class used when the result of the current expression is not
// expected to produce a result.
class BytecodeGenerator::EffectResultScope final
    : public ExpressionResultScope {
 public:
  explicit EffectResultScope(BytecodeGenerator* generator)
      : ExpressionResultScope(generator, Expression::kEffect) {}
};

// Scoped class used when the result of the current expression to be
// evaluated should go into the interpreter's accumulator.
class V8_NODISCARD BytecodeGenerator::ValueResultScope final
    : public ExpressionResultScope {
 public:
  explicit ValueResultScope(BytecodeGenerator* generator)
      : ExpressionResultScope(generator, Expression::kValue) {}
};

// Scoped class used when the result of the current expression to be
// evaluated is only tested with jumps to two branches.
class V8_NODISCARD BytecodeGenerator::TestResultScope final
    : public ExpressionResultScope {
 public:
  TestResultScope(BytecodeGenerator* generator, BytecodeLabels* then_labels,
                  BytecodeLabels* else_labels, TestFallthrough fallthrough)
      : ExpressionResultScope(generator, Expression::kTest),
        result_consumed_by_test_(false),
        fallthrough_(fallthrough),
        then_labels_(then_labels),
        else_labels_(else_labels) {}

  TestResultScope(const TestResultScope&) = delete;
  TestResultScope& operator=(const TestResultScope&) = delete;

  // Used when code special cases for TestResultScope and consumes any
  // possible value by testing and jumping to a then/else label.
  void SetResultConsumedByTest() { result_consumed_by_test_ = true; }
  bool result_consumed_by_test() { return result_consumed_by_test_; }

  // Inverts the control flow of the operation, swapping the then and else
  // labels and the fallthrough.
  void InvertControlFlow() {
    std::swap(then_labels_, else_labels_);
    fallthrough_ = inverted_fallthrough();
  }

  BytecodeLabel* NewThenLabel() { return then_labels_->New(); }
  BytecodeLabel* NewElseLabel() { return else_labels_->New(); }

  BytecodeLabels* then_labels() const { return then_labels_; }
  BytecodeLabels* else_labels() const { return else_labels_; }

  void set_then_labels(BytecodeLabels* then_labels) {
    then_labels_ = then_labels;
  }
  void set_else_labels(BytecodeLabels* else_labels) {
    else_labels_ = else_labels;
  }

  TestFallthrough fallthrough() const { return fallthrough_; }
  TestFallthrough inverted_fallthrough() const {
    switch (fallthrough_) {
      case TestFallthrough::kThen:
        return TestFallthrough::kElse;
      case TestFallthrough::kElse:
        return TestFallthrough::kThen;
      default:
        return TestFallthrough::kNone;
    }
  }
  void set_fallthrough(TestFallthrough fallthrough) {
    fallthrough_ = fallthrough;
  }

 private:
  bool result_consumed_by_test_;
  TestFallthrough fallthrough_;
  BytecodeLabels* then_labels_;
  BytecodeLabels* else_labels_;
};

// Used to build a list of toplevel declaration data.
class BytecodeGenerator::TopLevelDeclarationsBuilder final : public ZoneObject {
 public:
  template <typename IsolateT>
  Handle<FixedArray> AllocateDeclarations(UnoptimizedCompilationInfo* info,
                                          BytecodeGenerator* generator,
                                          Handle<Script> script,
                                          IsolateT* isolate) {
    DCHECK(has_constant_pool_entry_);

    Handle<FixedArray> data =
        isolate->factory()->NewFixedArray(entry_slots_, AllocationType::kOld);

    int array_index = 0;
    if (info->scope()->is_module_scope()) {
      for (Declaration* decl : *info->scope()->declarations()) {
        Variable* var = decl->var();
        if (!var->is_used()) continue;
        if (var->location() != VariableLocation::MODULE) continue;
#ifdef DEBUG
        int start = array_index;
#endif
        if (decl->IsFunctionDeclaration()) {
          FunctionLiteral* f = static_cast<FunctionDeclaration*>(decl)->fun();
          Handle<SharedFunctionInfo> sfi(
              Compiler::GetSharedFunctionInfo(f, script, isolate));
          // Return a null handle if any initial values can't be created. Caller
          // will set stack overflow.
          if (sfi.is_null()) return Handle<FixedArray>();
          data->set(array_index++, *sfi);
          int literal_index = generator->GetCachedCreateClosureSlot(f);
          data->set(array_index++, Smi::FromInt(literal_index));
          DCHECK(var->IsExport());
          data->set(array_index++, Smi::FromInt(var->index()));
          DCHECK_EQ(start + kModuleFunctionDeclarationSize, array_index);
        } else if (var->IsExport() && var->binding_needs_init()) {
          data->set(array_index++, Smi::FromInt(var->index()));
          DCHECK_EQ(start + kModuleVariableDeclarationSize, array_index);
        }
      }
    } else {
      for (Declaration* decl : *info->scope()->declarations()) {
        Variable* var = decl->var();
        if (!var->is_used()) continue;
        if (var->location() != VariableLocation::UNALLOCATED) continue;
#ifdef DEBUG
        int start = array_index;
#endif
        if (decl->IsVariableDeclaration()) {
          data->set(array_index++, *var->raw_name()->string());
          DCHECK_EQ(start + kGlobalVariableDeclarationSize, array_index);
        } else {
          FunctionLiteral* f = static_cast<FunctionDeclaration*>(decl)->fun();
          Handle<SharedFunctionInfo> sfi(
              Compiler::GetSharedFunctionInfo(f, script, isolate));
          // Return a null handle if any initial values can't be created. Caller
          // will set stack overflow.
          if (sfi.is_null()) return Handle<FixedArray>();
          data->set(array_index++, *sfi);
          int literal_index = generator->GetCachedCreateClosureSlot(f);
          data->set(array_index++, Smi::FromInt(literal_index));
          DCHECK_EQ(start + kGlobalFunctionDeclarationSize, array_index);
        }
      }
    }
    DCHECK_EQ(array_index, data->length());
    return data;
  }

  size_t constant_pool_entry() {
    DCHECK(has_constant_pool_entry_);
    return constant_pool_entry_;
  }

  void set_constant_pool_entry(size_t constant_pool_entry) {
    DCHECK(has_top_level_declaration());
    DCHECK(!has_constant_pool_entry_);
    constant_pool_entry_ = constant_pool_entry;
    has_constant_pool_entry_ = true;
  }

  void record_global_variable_declaration() {
    entry_slots_ += kGlobalVariableDeclarationSize;
  }
  void record_global_function_declaration() {
    entry_slots_ += kGlobalFunctionDeclarationSize;
  }
  void record_module_variable_declaration() {
    entry_slots_ += kModuleVariableDeclarationSize;
  }
  void record_module_function_declaration() {
    entry_slots_ += kModuleFunctionDeclarationSize;
  }
  bool has_top_level_declaration() { return entry_slots_ > 0; }
  bool processed() { return processed_; }
  void mark_processed() { processed_ = true; }

 private:
  const int kGlobalVariableDeclarationSize = 1;
  const int kGlobalFunctionDeclarationSize = 2;
  const int kModuleVariableDeclarationSize = 1;
  const int kModuleFunctionDeclarationSize = 3;

  size_t constant_pool_entry_ = 0;
  int entry_slots_ = 0;
  bool has_constant_pool_entry_ = false;
  bool processed_ = false;
};

class V8_NODISCARD BytecodeGenerator::CurrentScope final {
 public:
  CurrentScope(BytecodeGenerator* generator, Scope* scope)
      : generator_(generator), outer_scope_(generator->current_scope()) {
    if (scope != nullptr) {
      DCHECK_EQ(outer_scope_, scope->outer_scope());
      generator_->set_current_scope(scope);
    }
  }
  ~CurrentScope() {
    if (outer_scope_ != generator_->current_scope()) {
      generator_->set_current_scope(outer_scope_);
    }
  }
  CurrentScope(const CurrentScope&) = delete;
  CurrentScope& operator=(const CurrentScope&) = delete;

 private:
  BytecodeGenerator* generator_;
  Scope* outer_scope_;
};

class V8_NODISCARD BytecodeGenerator::MultipleEntryBlockContextScope {
 public:
  MultipleEntryBlockContextScope(BytecodeGenerator* generator, Scope* scope)
      : generator_(generator), scope_(scope), is_in_scope_(false) {
    if (scope) {
      inner_context_ = generator->register_allocator()->NewRegister();
      outer_context_ = generator->register_allocator()->NewRegister();
      generator->BuildNewLocalBlockContext(scope_);
      generator->builder()->StoreAccumulatorInRegister(inner_context_);
    }
  }

  void SetEnteredIf(bool condition) {
    RegisterAllocationScope register_scope(generator_);
    if (condition && scope_ != nullptr && !is_in_scope_) {
      EnterScope();
    } else if (!condition && is_in_scope_) {
      ExitScope();
    }
  }

  MultipleEntryBlockContextScope(const MultipleEntryBlockContextScope&) =
      delete;
  MultipleEntryBlockContextScope& operator=(
      const MultipleEntryBlockContextScope&) = delete;

 private:
  void EnterScope() {
    DCHECK(inner_context_.is_valid());
    DCHECK(outer_context_.is_valid());
    DCHECK(!is_in_scope_);
    Register temp = generator_->register_allocator()->NewRegister();
    generator_->builder()->StoreAccumulatorInRegister(temp);
    generator_->builder()->LoadAccumulatorWithRegister(inner_context_);
    current_scope_.emplace(generator_, scope_);
    context_scope_.emplace(generator_, scope_, outer_context_);
    generator_->builder()->LoadAccumulatorWithRegister(temp);
    is_in_scope_ = true;
  }

  void ExitScope() {
    DCHECK(inner_context_.is_valid());
    DCHECK(outer_context_.is_valid());
    DCHECK(is_in_scope_);
    Register temp = generator_->register_allocator()->NewRegister();
    generator_->builder()->StoreAccumulatorInRegister(temp);
    context_scope_ = base::nullopt;
    current_scope_ = base::nullopt;
    generator_->builder()->LoadAccumulatorWithRegister(temp);
    is_in_scope_ = false;
  }

  BytecodeGenerator* generator_;
  Scope* scope_;
  Register inner_context_;
  Register outer_context_;
  bool is_in_scope_;
  base::Optional<CurrentScope> current_scope_;
  base::Optional<ContextScope> context_scope_;
};

class BytecodeGenerator::FeedbackSlotCache : public ZoneObject {
 public:
  enum class SlotKind {
    kStoreGlobalSloppy,
    kStoreGlobalStrict,
    kStoreNamedStrict,
    kStoreNamedSloppy,
    kLoadProperty,
    kLoadSuperProperty,
    kLoadGlobalNotInsideTypeof,
    kLoadGlobalInsideTypeof,
    kClosureFeedbackCell
  };

  explicit FeedbackSlotCache(Zone* zone) : map_(zone) {}

  void Put(SlotKind slot_kind, Variable* variable, int slot_index) {
    PutImpl(slot_kind, 0, variable, slot_index);
  }
  void Put(SlotKind slot_kind, AstNode* node, int slot_index) {
    PutImpl(slot_kind, 0, node, slot_index);
  }
  void Put(SlotKind slot_kind, int variable_index, const AstRawString* name,
           int slot_index) {
    PutImpl(slot_kind, variable_index, name, slot_index);
  }
  void Put(SlotKind slot_kind, const AstRawString* name, int slot_index) {
    PutImpl(slot_kind, 0, name, slot_index);
  }

  int Get(SlotKind slot_kind, Variable* variable) const {
    return GetImpl(slot_kind, 0, variable);
  }
  int Get(SlotKind slot_kind, AstNode* node) const {
    return GetImpl(slot_kind, 0, node);
  }
  int Get(SlotKind slot_kind, int variable_index,
          const AstRawString* name) const {
    return GetImpl(slot_kind, variable_index, name);
  }
  int Get(SlotKind slot_kind, const AstRawString* name) const {
    return GetImpl(slot_kind, 0, name);
  }

 private:
  using Key = std::tuple<SlotKind, int, const void*>;

  void PutImpl(SlotKind slot_kind, int index, const void* node,
               int slot_index) {
    Key key = std::make_tuple(slot_kind, index, node);
    auto entry = std::make_pair(key, slot_index);
    map_.insert(entry);
  }

  int GetImpl(SlotKind slot_kind, int index, const void* node) const {
    Key key = std::make_tuple(slot_kind, index, node);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      return iter->second;
    }
    return -1;
  }

  ZoneMap<Key, int> map_;
};

class BytecodeGenerator::IteratorRecord final {
 public:
  IteratorRecord(Register object_register, Register next_register,
                 IteratorType type = IteratorType::kNormal)
      : type_(type), object_(object_register), next_(next_register) {
    DCHECK(object_.is_valid() && next_.is_valid());
  }

  inline IteratorType type() const { return type_; }
  inline Register object() const { return object_; }
  inline Register next() const { return next_; }

 private:
  IteratorType type_;
  Register object_;
  Register next_;
};

class V8_NODISCARD BytecodeGenerator::OptionalChainNullLabelScope final {
 public:
  explicit OptionalChainNullLabelScope(BytecodeGenerator* bytecode_generator)
      : bytecode_generator_(bytecode_generator),
        labels_(bytecode_generator->zone()) {
    prev_ = bytecode_generator_->optional_chaining_null_labels_;
    bytecode_generator_->optional_chaining_null_labels_ = &labels_;
  }

  ~OptionalChainNullLabelScope() {
    bytecode_generator_->optional_chaining_null_labels_ = prev_;
  }

  BytecodeLabels* labels() { return &labels_; }

 private:
  BytecodeGenerator* bytecode_generator_;
  BytecodeLabels labels_;
  BytecodeLabels* prev_;
};

// LoopScope delimits the scope of {loop}, from its header to its final jump.
// It should be constructed iff a (conceptual) back edge should be produced. In
// the case of creating a LoopBuilder but never emitting the loop, it is valid
// to skip the creation of LoopScope.
class V8_NODISCARD BytecodeGenerator::LoopScope final {
 public:
  explicit LoopScope(BytecodeGenerator* bytecode_generator, LoopBuilder* loop)
      : bytecode_generator_(bytecode_generator),
        parent_loop_scope_(bytecode_generator_->current_loop_scope()),
        loop_builder_(loop) {
    loop_builder_->LoopHeader();
    bytecode_generator_->set_current_loop_scope(this);
    bytecode_generator_->loop_depth_++;
  }

  ~LoopScope() {
    bytecode_generator_->loop_depth_--;
    bytecode_generator_->set_current_loop_scope(parent_loop_scope_);
    DCHECK_GE(bytecode_generator_->loop_depth_, 0);
    loop_builder_->JumpToHeader(
        bytecode_generator_->loop_depth_,
        parent_loop_scope_ ? parent_loop_scope_->loop_builder_ : nullptr);
  }

 private:
  BytecodeGenerator* const bytecode_generator_;
  LoopScope* const parent_loop_scope_;
  LoopBuilder* const loop_builder_;
};

namespace {

template <typename PropertyT>
struct Accessors : public ZoneObject {
  Accessors() : getter(nullptr), setter(nullptr) {}
  PropertyT* getter;
  PropertyT* setter;
};

// A map from property names to getter/setter pairs allocated in the zone that
// also provides a way of accessing the pairs in the order they were first
// added so that the generated bytecode is always the same.
template <typename PropertyT>
class AccessorTable
    : public base::TemplateHashMap<Literal, Accessors<PropertyT>,
                                   bool (*)(void*, void*),
                                   ZoneAllocationPolicy> {
 public:
  explicit AccessorTable(Zone* zone)
      : base::TemplateHashMap<Literal, Accessors<PropertyT>,
                              bool (*)(void*, void*), ZoneAllocationPolicy>(
            Literal::Match, ZoneAllocationPolicy(zone)),
        zone_(zone) {}

  Accessors<PropertyT>* LookupOrInsert(Literal* key) {
    auto it = this->find(key, true);
    if (it->second == nullptr) {
      it->second = zone_->New<Accessors<PropertyT>>();
      ordered_accessors_.push_back({key, it->second});
    }
    return it->second;
  }

  const std::vector<std::pair<Literal*, Accessors<PropertyT>*>>&
  ordered_accessors() {
    return ordered_accessors_;
  }

 private:
  std::vector<std::pair<Literal*, Accessors<PropertyT>*>> ordered_accessors_;

  Zone* zone_;
};

}  // namespace

#ifdef DEBUG

static bool IsInEagerLiterals(
    FunctionLiteral* literal,
    const std::vector<FunctionLiteral*>& eager_literals) {
  for (FunctionLiteral* eager_literal : eager_literals) {
    if (literal == eager_literal) return true;
  }
  return false;
}

#endif  // DEBUG

BytecodeGenerator::BytecodeGenerator(
    Zone* compile_zone, UnoptimizedCompilationInfo* info,
    const AstStringConstants* ast_string_constants,
    std::vector<FunctionLiteral*>* eager_inner_literals)
    : zone_(compile_zone),
      builder_(zone(), info->num_parameters_including_this(),
               info->scope()->num_stack_slots(), info->feedback_vector_spec(),
               info->SourcePositionRecordingMode()),
      info_(info),
      ast_string_constants_(ast_string_constants),
      closure_scope_(info->scope()),
      current_scope_(info->scope()),
      eager_inner_literals_(eager_inner_literals),
      feedback_slot_cache_(zone()->New<FeedbackSlotCache>(zone())),
      top_level_builder_(zone()->New<TopLevelDeclarationsBuilder>()),
      block_coverage_builder_(nullptr),
      function_literals_(0, zone()),
      native_function_literals_(0, zone()),
      object_literals_(0, zone()),
      array_literals_(0, zone()),
      class_literals_(0, zone()),
      template_objects_(0, zone()),
      execution_control_(nullptr),
      execution_context_(nullptr),
      execution_result_(nullptr),
      incoming_new_target_or_generator_(),
      optional_chaining_null_labels_(nullptr),
      dummy_feedback_slot_(feedback_spec(), FeedbackSlotKind::kCompareOp),
      generator_jump_table_(nullptr),
      suspend_count_(0),
      loop_depth_(0),
      current_loop_scope_(nullptr),
      catch_prediction_(HandlerTable::UNCAUGHT) {
  DCHECK_EQ(closure_scope(), closure_scope()->GetClosureScope());
  if (info->has_source_range_map()) {
    block_coverage_builder_ = zone()->New<BlockCoverageBuilder>(
        zone(), builder(), info->source_range_map());
  }
}

namespace {

template <typename Isolate>
struct NullContextScopeHelper;

template <>
struct NullContextScopeHelper<Isolate> {
  using Type = NullContextScope;
};

template <>
struct NullContextScopeHelper<LocalIsolate> {
  class V8_NODISCARD DummyNullContextScope {
   public:
    explicit DummyNullContextScope(LocalIsolate*) {}
  };
  using Type = DummyNullContextScope;
};

template <typename Isolate>
using NullContextScopeFor = typename NullContextScopeHelper<Isolate>::Type;

}  // namespace

template <typename IsolateT>
Handle<BytecodeArray> BytecodeGenerator::FinalizeBytecode(
    IsolateT* isolate, Handle<Script> script) {
  DCHECK_EQ(ThreadId::Current(), isolate->thread_id());
#ifdef DEBUG
  // Unoptimized compilation should be context-independent. Verify that we don't
  // access the native context by nulling it out during finalization.
  NullContextScopeFor<IsolateT> null_context_scope(isolate);
#endif

  AllocateDeferredConstants(isolate, script);

  if (block_coverage_builder_) {
    Handle<CoverageInfo> coverage_info =
        isolate->factory()->NewCoverageInfo(block_coverage_builder_->slots());
    info()->set_coverage_info(coverage_info);
    if (FLAG_trace_block_coverage) {
      StdoutStream os;
      coverage_info->CoverageInfoPrint(os, info()->literal()->GetDebugName());
    }
  }

  if (HasStackOverflow()) return Handle<BytecodeArray>();
  Handle<BytecodeArray> bytecode_array = builder()->ToBytecodeArray(isolate);

  if (incoming_new_target_or_generator_.is_valid()) {
    bytecode_array->set_incoming_new_target_or_generator_register(
        incoming_new_target_or_generator_);
  }

  return bytecode_array;
}

template Handle<BytecodeArray> BytecodeGenerator::FinalizeBytecode(
    Isolate* isolate, Handle<Script> script);
template Handle<BytecodeArray> BytecodeGenerator::FinalizeBytecode(
    LocalIsolate* isolate, Handle<Script> script);

template <typename IsolateT>
Handle<ByteArray> BytecodeGenerator::FinalizeSourcePositionTable(
    IsolateT* isolate) {
  DCHECK_EQ(ThreadId::Current(), isolate->thread_id());
#ifdef DEBUG
  // Unoptimized compilation should be context-independent. Verify that we don't
  // access the native context by nulling it out during finalization.
  NullContextScopeFor<IsolateT> null_context_scope(isolate);
#endif

  Handle<ByteArray> source_position_table =
      builder()->ToSourcePositionTable(isolate);

  LOG_CODE_EVENT(isolate,
                 CodeLinePosInfoRecordEvent(
                     info_->bytecode_array()->GetFirstBytecodeAddress(),
                     *source_position_table));

  return source_position_table;
}

template Handle<ByteArray> BytecodeGenerator::FinalizeSourcePositionTable(
    Isolate* isolate);
template Handle<ByteArray> BytecodeGenerator::FinalizeSourcePositionTable(
    LocalIsolate* isolate);

#ifdef DEBUG
int BytecodeGenerator::CheckBytecodeMatches(BytecodeArray bytecode) {
  return builder()->CheckBytecodeMatches(bytecode);
}
#endif

template <typename IsolateT>
void BytecodeGenerator::AllocateDeferredConstants(IsolateT* isolate,
                                                  Handle<Script> script) {
  if (top_level_builder()->has_top_level_declaration()) {
    // Build global declaration pair array.
    Handle<FixedArray> declarations = top_level_builder()->AllocateDeclarations(
        info(), this, script, isolate);
    if (declarations.is_null()) return SetStackOverflow();
    builder()->SetDeferredConstantPoolEntry(
        top_level_builder()->constant_pool_entry(), declarations);
  }

  // Find or build shared function infos.
  for (std::pair<FunctionLiteral*, size_t> literal : function_literals_) {
    FunctionLiteral* expr = literal.first;
    Handle<SharedFunctionInfo> shared_info =
        Compiler::GetSharedFunctionInfo(expr, script, isolate);
    if (shared_info.is_null()) return SetStackOverflow();
    builder()->SetDeferredConstantPoolEntry(literal.second, shared_info);
  }

  // Find or build shared function infos for the native function templates.
  for (std::pair<NativeFunctionLiteral*, size_t> literal :
       native_function_literals_) {
    // This should only happen for main-thread compilations.
    DCHECK((std::is_same<Isolate, v8::internal::Isolate>::value));

    NativeFunctionLiteral* expr = literal.first;
    v8::Isolate* v8_isolate = reinterpret_cast<v8::Isolate*>(isolate);

    // Compute the function template for the native function.
    v8::Local<v8::FunctionTemplate> info =
        expr->extension()->GetNativeFunctionTemplate(
            v8_isolate, Utils::ToLocal(expr->name()));
    DCHECK(!info.IsEmpty());

    Handle<SharedFunctionInfo> shared_info =
        FunctionTemplateInfo::GetOrCreateSharedFunctionInfo(
            isolate, Utils::OpenHandle(*info), expr->name());
    DCHECK(!shared_info.is_null());
    builder()->SetDeferredConstantPoolEntry(literal.second, shared_info);
  }

  // Build object literal constant properties
  for (std::pair<ObjectLiteral*, size_t> literal : object_literals_) {
    ObjectLiteral* object_literal = literal.first;
    if (object_literal->properties_count() > 0) {
      // If constant properties is an empty fixed array, we've already added it
      // to the constant pool when visiting the object literal.
      Handle<ObjectBoilerplateDescription> constant_properties =
          object_literal->GetOrBuildBoilerplateDescription(isolate);

      builder()->SetDeferredConstantPoolEntry(literal.second,
                                              constant_properties);
    }
  }

  // Build array literal constant elements
  for (std::pair<ArrayLiteral*, size_t> literal : array_literals_) {
    ArrayLiteral* array_literal = literal.first;
    Handle<ArrayBoilerplateDescription> constant_elements =
        array_literal->GetOrBuildBoilerplateDescription(isolate);
    builder()->SetDeferredConstantPoolEntry(literal.second, constant_elements);
  }

  // Build class literal boilerplates.
  for (std::pair<ClassLiteral*, size_t> literal : class_literals_) {
    ClassLiteral* class_literal = literal.first;
    Handle<ClassBoilerplate> class_boilerplate =
        ClassBoilerplate::BuildClassBoilerplate(isolate, class_literal);
    builder()->SetDeferredConstantPoolEntry(literal.second, class_boilerplate);
  }

  // Build template literals.
  for (std::pair<GetTemplateObject*, size_t> literal : template_objects_) {
    GetTemplateObject* get_template_object = literal.first;
    Handle<TemplateObjectDescription> description =
        get_template_object->GetOrBuildDescription(isolate);
    builder()->SetDeferredConstantPoolEntry(literal.second, description);
  }
}

template void BytecodeGenerator::AllocateDeferredConstants(
    Isolate* isolate, Handle<Script> script);
template void BytecodeGenerator::AllocateDeferredConstants(
    LocalIsolate* isolate, Handle<Script> script);

namespace {
bool NeedsContextInitialization(DeclarationScope* scope) {
  return scope->NeedsContext() && !scope->is_script_scope() &&
         !scope->is_module_scope();
}
}  // namespace

void BytecodeGenerator::GenerateBytecode(uintptr_t stack_limit) {
  DisallowGarbageCollection no_gc;
  DisallowHandleAllocation no_handles;
  DisallowHandleDereference no_deref;

  InitializeAstVisitor(stack_limit);

  // Initialize the incoming context.
  ContextScope incoming_context(this, closure_scope());

  // Initialize control scope.
  ControlScopeForTopLevel control(this);

  RegisterAllocationScope register_scope(this);

  AllocateTopLevelRegisters();

  builder()->EmitFunctionStartSourcePosition(
      info()->literal()->start_position());

  if (info()->literal()->CanSuspend()) {
    BuildGeneratorPrologue();
  }

  if (NeedsContextInitialization(closure_scope())) {
    // Push a new inner context scope for the function.
    BuildNewLocalActivationContext();
    ContextScope local_function_context(this, closure_scope());
    BuildLocalActivationContextInitialization();
    GenerateBytecodeBody();
  } else {
    GenerateBytecodeBody();
  }

  // Check that we are not falling off the end.
  DCHECK(builder()->RemainderOfBlockIsDead());
}

void BytecodeGenerator::GenerateBytecodeBody() {
  // Build the arguments object if it is used.
  VisitArgumentsObject(closure_scope()->arguments());

  // Build rest arguments array if it is used.
  Variable* rest_parameter = closure_scope()->rest_parameter();
  VisitRestArgumentsArray(rest_parameter);

  // Build assignment to the function name or {.this_function}
  // variables if used.
  VisitThisFunctionVariable(closure_scope()->function_var());
  VisitThisFunctionVariable(closure_scope()->this_function_var());

  // Build assignment to {new.target} variable if it is used.
  VisitNewTargetVariable(closure_scope()->new_target_var());

  // Create a generator object if necessary and initialize the
  // {.generator_object} variable.
  FunctionLiteral* literal = info()->literal();
  if (IsResumableFunction(literal->kind())) {
    BuildGeneratorObjectVariableInitialization();
  }

  // Emit tracing call if requested to do so.
  if (FLAG_trace) builder()->CallRuntime(Runtime::kTraceEnter);

  // Emit type profile call.
  if (info()->flags().collect_type_profile()) {
    feedback_spec()->AddTypeProfileSlot();
    int num_parameters = closure_scope()->num_parameters();
    for (int i = 0; i < num_parameters; i++) {
      Register parameter(builder()->Parameter(i));
      builder()->LoadAccumulatorWithRegister(parameter).CollectTypeProfile(
          closure_scope()->parameter(i)->initializer_position());
    }
  }

  // Increment the function-scope block coverage counter.
  BuildIncrementBlockCoverageCounterIfEnabled(literal, SourceRangeKind::kBody);

  // Visit declarations within the function scope.
  if (closure_scope()->is_script_scope()) {
    VisitGlobalDeclarations(closure_scope()->declarations());
  } else if (closure_scope()->is_module_scope()) {
    VisitModuleDeclarations(closure_scope()->declarations());
  } else {
    VisitDeclarations(closure_scope()->declarations());
  }

  // Emit initializing assignments for module namespace imports (if any).
  VisitModuleNamespaceImports();

  // The derived constructor case is handled in VisitCallSuper.
  if (IsBaseConstructor(function_kind())) {
    if (literal->class_scope_has_private_brand()) {
      BuildPrivateBrandInitialization(builder()->Receiver());
    }

    if (literal->requires_instance_members_initializer()) {
      BuildInstanceMemberInitialization(Register::function_closure(),
                                        builder()->Receiver());
    }
  }

  // Visit statements in the function body.
  VisitStatements(literal->body());

  // Emit an implicit return instruction in case control flow can fall off the
  // end of the function without an explicit return being present on all paths.
  if (!builder()->RemainderOfBlockIsDead()) {
    builder()->LoadUndefined();
    BuildReturn(literal->return_position());
  }
}

void BytecodeGenerator::AllocateTopLevelRegisters() {
  if (IsResumableFunction(info()->literal()->kind())) {
    // Either directly use generator_object_var or allocate a new register for
    // the incoming generator object.
    Variable* generator_object_var = closure_scope()->generator_object_var();
    if (generator_object_var->location() == VariableLocation::LOCAL) {
      incoming_new_target_or_generator_ =
          GetRegisterForLocalVariable(generator_object_var);
    } else {
      incoming_new_target_or_generator_ = register_allocator()->NewRegister();
    }
  } else if (closure_scope()->new_target_var()) {
    // Either directly use new_target_var or allocate a new register for
    // the incoming new target object.
    Variable* new_target_var = closure_scope()->new_target_var();
    if (new_target_var->location() == VariableLocation::LOCAL) {
      incoming_new_target_or_generator_ =
          GetRegisterForLocalVariable(new_target_var);
    } else {
      incoming_new_target_or_generator_ = register_allocator()->NewRegister();
    }
  }
}

void BytecodeGenerator::BuildGeneratorPrologue() {
  DCHECK_GT(info()->literal()->suspend_count(), 0);
  DCHECK(generator_object().is_valid());
  generator_jump_table_ =
      builder()->AllocateJumpTable(info()->literal()->suspend_count(), 0);

  // If the generator is not undefined, this is a resume, so perform state
  // dispatch.
  builder()->SwitchOnGeneratorState(generator_object(), generator_jump_table_);

  // Otherwise, fall-through to the ordinary function prologue, after which we
  // will run into the generator object creation and other extra code inserted
  // by the parser.
}

void BytecodeGenerator::VisitBlock(Block* stmt) {
  // Visit declarations and statements.
  CurrentScope current_scope(this, stmt->scope());
  if (stmt->scope() != nullptr && stmt->scope()->NeedsContext()) {
    BuildNewLocalBlockContext(stmt->scope());
    ContextScope scope(this, stmt->scope());
    VisitBlockDeclarationsAndStatements(stmt);
  } else {
    VisitBlockDeclarationsAndStatements(stmt);
  }
}

void BytecodeGenerator::VisitBlockDeclarationsAndStatements(Block* stmt) {
  BlockBuilder block_builder(builder(), block_coverage_builder_, stmt);
  ControlScopeForBreakable execution_control(this, stmt, &block_builder);
  if (stmt->scope() != nullptr) {
    VisitDeclarations(stmt->scope()->declarations());
  }
  VisitStatements(stmt->statements());
}

void BytecodeGenerator::VisitVariableDeclaration(VariableDeclaration* decl) {
  Variable* variable = decl->var();
  // Unused variables don't need to be visited.
  if (!variable->is_used()) return;

  switch (variable->location()) {
    case VariableLocation::UNALLOCATED:
    case VariableLocation::MODULE:
      UNREACHABLE();
    case VariableLocation::LOCAL:
      if (variable->binding_needs_init()) {
        Register destination(builder()->Local(variable->index()));
        builder()->LoadTheHole().StoreAccumulatorInRegister(destination);
      }
      break;
    case VariableLocation::PARAMETER:
      if (variable->binding_needs_init()) {
        Register destination(builder()->Parameter(variable->index()));
        builder()->LoadTheHole().StoreAccumulatorInRegister(destination);
      }
      break;
    case VariableLocation::REPL_GLOBAL:
      // REPL let's are stored in script contexts. They get initialized
      // with the hole the same way as normal context allocated variables.
    case VariableLocation::CONTEXT:
      if (variable->binding_needs_init()) {
        DCHECK_EQ(0, execution_context()->ContextChainDepth(variable->scope()));
        builder()->LoadTheHole().StoreContextSlot(execution_context()->reg(),
                                                  variable->index(), 0);
      }
      break;
    case VariableLocation::LOOKUP: {
      DCHECK_EQ(VariableMode::kDynamic, variable->mode());
      DCHECK(!variable->binding_needs_init());

      Register name = register_allocator()->NewRegister();

      builder()
          ->LoadLiteral(variable->raw_name())
          .StoreAccumulatorInRegister(name)
          .CallRuntime(Runtime::kDeclareEvalVar, name);
      break;
    }
  }
}

void BytecodeGenerator::VisitFunctionDeclaration(FunctionDeclaration* decl) {
  Variable* variable = decl->var();
  DCHECK(variable->mode() == VariableMode::kLet ||
         variable->mode() == VariableMode::kVar ||
         variable->mode() == VariableMode::kDynamic);
  // Unused variables don't need to be visited.
  if (!variable->is_used()) return;

  switch (variable->location()) {
    case VariableLocation::UNALLOCATED:
    case VariableLocation::MODULE:
      UNREACHABLE();
    case VariableLocation::PARAMETER:
    case VariableLocation::LOCAL: {
      VisitFunctionLiteral(decl->fun());
      BuildVariableAssignment(variable, Token::INIT, HoleCheckMode::kElided);
      break;
    }
    case VariableLocation::REPL_GLOBAL:
    case VariableLocation::CONTEXT: {
      DCHECK_EQ(0, execution_context()->ContextChainDepth(variable->scope()));
      VisitFunctionLiteral(decl->fun());
      builder()->StoreContextSlot(execution_context()->reg(), variable->index(),
                                  0);
      break;
    }
    case VariableLocation::LOOKUP: {
      RegisterList args = register_allocator()->NewRegisterList(2);
      builder()
          ->LoadLiteral(variable->raw_name())
          .StoreAccumulatorInRegister(args[0]);
      VisitFunctionLiteral(decl->fun());
      builder()->StoreAccumulatorInRegister(args[1]).CallRuntime(
          Runtime::kDeclareEvalFunction, args);
      break;
    }
  }
  DCHECK_IMPLIES(
      eager_inner_literals_ != nullptr && decl->fun()->ShouldEagerCompile(),
      IsInEagerLiterals(decl->fun(), *eager_inner_literals_));
}

void BytecodeGenerator::VisitModuleNamespaceImports() {
  if (!closure_scope()->is_module_scope()) return;

  RegisterAllocationScope register_scope(this);
  Register module_request = register_allocator()->NewRegister();

  SourceTextModuleDescriptor* descriptor =
      closure_scope()->AsModuleScope()->module();
  for (auto entry : descriptor->namespace_imports()) {
    builder()
        ->LoadLiteral(Smi::FromInt(entry->module_request))
        .StoreAccumulatorInRegister(module_request)
        .CallRuntime(Runtime::kGetModuleNamespace, module_request);
    Variable* var = closure_scope()->LookupInModule(entry->local_name);
    BuildVariableAssignment(var, Token::INIT, HoleCheckMode::kElided);
  }
}

void BytecodeGenerator::BuildDeclareCall(Runtime::FunctionId id) {
  if (!top_level_builder()->has_top_level_declaration()) return;
  DCHECK(!top_level_builder()->processed());

  top_level_builder()->set_constant_pool_entry(
      builder()->AllocateDeferredConstantPoolEntry());

  // Emit code to declare globals.
  RegisterList args = register_allocator()->NewRegisterList(2);
  builder()
      ->LoadConstantPoolEntry(top_level_builder()->constant_pool_entry())
      .StoreAccumulatorInRegister(args[0])
      .MoveRegister(Register::function_closure(), args[1])
      .CallRuntime(id, args);

  top_level_builder()->mark_processed();
}

void BytecodeGenerator::VisitModuleDeclarations(Declaration::List* decls) {
  RegisterAllocationScope register_scope(this);
  for (Declaration* decl : *decls) {
    Variable* var = decl->var();
    if (!var->is_used()) continue;
    if (var->location() == VariableLocation::MODULE) {
      if (decl->IsFunctionDeclaration()) {
        DCHECK(var->IsExport());
        FunctionDeclaration* f = static_cast<FunctionDeclaration*>(decl);
        AddToEagerLiteralsIfEager(f->fun());
        top_level_builder()->record_module_function_declaration();
      } else if (var->IsExport() && var->binding_needs_init()) {
        DCHECK(decl->IsVariableDeclaration());
        top_level_builder()->record_module_variable_declaration();
      }
    } else {
      RegisterAllocationScope register_scope(this);
      Visit(decl);
    }
  }
  BuildDeclareCall(Runtime::kDeclareModuleExports);
}

void BytecodeGenerator::VisitGlobalDeclarations(Declaration::List* decls) {
  RegisterAllocationScope register_scope(this);
  for (Declaration* decl : *decls) {
    Variable* var = decl->var();
    DCHECK(var->is_used());
    if (var->location() == VariableLocation::UNALLOCATED) {
      // var or function.
      if (decl->IsFunctionDeclaration()) {
        top_level_builder()->record_global_function_declaration();
        FunctionDeclaration* f = static_cast<FunctionDeclaration*>(decl);
        AddToEagerLiteralsIfEager(f->fun());
      } else {
        top_level_builder()->record_global_variable_declaration();
      }
    } else {
      // let or const. Handled in NewScriptContext.
      DCHECK(decl->IsVariableDeclaration());
      DCHECK(IsLexicalVariableMode(var->mode()));
    }
  }

  BuildDeclareCall(Runtime::kDeclareGlobals);
}

void BytecodeGenerator::VisitDeclarations(Declaration::List* declarations) {
  for (Declaration* decl : *declarations) {
    RegisterAllocationScope register_scope(this);
    Visit(decl);
  }
}

void BytecodeGenerator::VisitStatements(
    const ZonePtrList<Statement>* statements) {
  for (int i = 0; i < statements->length(); i++) {
    // Allocate an outer register allocations scope for the statement.
    RegisterAllocationScope allocation_scope(this);
    Statement* stmt = statements->at(i);
    Visit(stmt);
    if (builder()->RemainderOfBlockIsDead()) break;
  }
}

void BytecodeGenerator::VisitExpressionStatement(ExpressionStatement* stmt) {
  builder()->SetStatementPosition(stmt);
  VisitForEffect(stmt->expression());
}

void BytecodeGenerator::VisitEmptyStatement(EmptyStatement* stmt) {}

void BytecodeGenerator::VisitIfStatement(IfStatement* stmt) {
  ConditionalControlFlowBuilder conditional_builder(
      builder(), block_coverage_builder_, stmt);
  builder()->SetStatementPosition(stmt);

  if (stmt->condition()->ToBooleanIsTrue()) {
    // Generate then block unconditionally as always true.
    conditional_builder.Then();
    Visit(stmt->then_statement());
  } else if (stmt->condition()->ToBooleanIsFalse()) {
    // Generate else block unconditionally if it exists.
    if (stmt->HasElseStatement()) {
      conditional_builder.Else();
      Visit(stmt->else_statement());
    }
  } else {
    // TODO(oth): If then statement is BreakStatement or
    // ContinueStatement we can reduce number of generated
    // jump/jump_ifs here. See BasicLoops test.
    VisitForTest(stmt->condition(), conditional_builder.then_labels(),
                 conditional_builder.else_labels(), TestFallthrough::kThen);

    conditional_builder.Then();
    Visit(stmt->then_statement());

    if (stmt->HasElseStatement()) {
      conditional_builder.JumpToEnd();
      conditional_builder.Else();
      Visit(stmt->else_statement());
    }
  }
}

void BytecodeGenerator::VisitSloppyBlockFunctionStatement(
    SloppyBlockFunctionStatement* stmt) {
  Visit(stmt->statement());
}

void BytecodeGenerator::VisitContinueStatement(ContinueStatement* stmt) {
  AllocateBlockCoverageSlotIfEnabled(stmt, SourceRangeKind::kContinuation);
  builder()->SetStatementPosition(stmt);
  execution_control()->Continue(stmt->target());
}

void BytecodeGenerator::VisitBreakStatement(BreakStatement* stmt) {
  AllocateBlockCoverageSlotIfEnabled(stmt, SourceRangeKind::kContinuation);
  builder()->SetStatementPosition(stmt);
  execution_control()->Break(stmt->target());
}

void BytecodeGenerator::VisitReturnStatement(ReturnStatement* stmt) {
  AllocateBlockCoverageSlotIfEnabled(stmt, SourceRangeKind::kContinuation);
  builder()->SetStatementPosition(stmt);
  VisitForAccumulatorValue(stmt->expression());
  int return_position = stmt->end_position();
  if (return_position == ReturnStatement::kFunctionLiteralReturnPosition) {
    return_position = info()->literal()->return_position();
  }
  if (stmt->is_async_return()) {
    execution_control()->AsyncReturnAccumulator(return_position);
  } else {
    execution_control()->ReturnAccumulator(return_position);
  }
}

void BytecodeGenerator::VisitWithStatement(WithStatement* stmt) {
  builder()->SetStatementPosition(stmt);
  VisitForAccumulatorValue(stmt->expression());
  BuildNewLocalWithContext(stmt->scope());
  VisitInScope(stmt->statement(), stmt->scope());
}

void BytecodeGenerator::VisitSwitchStatement(SwitchStatement* stmt) {
  // We need this scope because we visit for register values. We have to
  // maintain a execution result scope where registers can be allocated.
  ZonePtrList<CaseClause>* clauses = stmt->cases();
  SwitchBuilder switch_builder(builder(), block_coverage_builder_, stmt,
                               clauses->length());
  ControlScopeForBreakable scope(this, stmt, &switch_builder);
  int default_index = -1;

  builder()->SetStatementPosition(stmt);

  // Keep the switch value in a register until a case matches.
  Register tag = VisitForRegisterValue(stmt->tag());
  FeedbackSlot slot = clauses->length() > 0
                          ? feedback_spec()->AddCompareICSlot()
                          : FeedbackSlot::Invalid();

  // Iterate over all cases and create nodes for label comparison.
  for (int i = 0; i < clauses->length(); i++) {
    CaseClause* clause = clauses->at(i);

    // The default is not a test, remember index.
    if (clause->is_default()) {
      default_index = i;
      continue;
    }

    // Perform label comparison as if via '===' with tag.
    VisitForAccumulatorValue(clause->label());
    builder()->CompareOperation(Token::Value::EQ_STRICT, tag,
                                feedback_index(slot));
    switch_builder.Case(ToBooleanMode::kAlreadyBoolean, i);
  }

  if (default_index >= 0) {
    // Emit default jump if there is a default case.
    switch_builder.DefaultAt(default_index);
  } else {
    // Otherwise if we have reached here none of the cases matched, so jump to
    // the end.
    switch_builder.Break();
  }

  // Iterate over all cases and create the case bodies.
  for (int i = 0; i < clauses->length(); i++) {
    CaseClause* clause = clauses->at(i);
    switch_builder.SetCaseTarget(i, clause);
    VisitStatements(clause->statements());
  }
}

template <typename TryBodyFunc, typename CatchBodyFunc>
void BytecodeGenerator::BuildTryCatch(
    TryBodyFunc try_body_func, CatchBodyFunc catch_body_func,
    HandlerTable::CatchPrediction catch_prediction,
    TryCatchStatement* stmt_for_coverage) {
  TryCatchBuilder try_control_builder(
      builder(),
      stmt_for_coverage == nullptr ? nullptr : block_coverage_builder_,
      stmt_for_coverage, catch_prediction);

  // Preserve the context in a dedicated register, so that it can be restored
  // when the handler is entered by the stack-unwinding machinery.
  // TODO(ignition): Be smarter about register allocation.
  Register context = register_allocator()->NewRegister();
  builder()->MoveRegister(Register::current_context(), context);

  // Evaluate the try-block inside a control scope. This simulates a handler
  // that is intercepting 'throw' control commands.
  try_control_builder.BeginTry(context);
  {
    ControlScopeForTryCatch scope(this, &try_control_builder);
    try_body_func();
  }
  try_control_builder.EndTry();

  catch_body_func(context);

  try_control_builder.EndCatch();
}

template <typename TryBodyFunc, typename FinallyBodyFunc>
void BytecodeGenerator::BuildTryFinally(
    TryBodyFunc try_body_func, FinallyBodyFunc finally_body_func,
    HandlerTable::CatchPrediction catch_prediction,
    TryFinallyStatement* stmt_for_coverage) {
  // We can't know whether the finally block will override ("catch") an
  // exception thrown in the try block, so we just adopt the outer prediction.
  TryFinallyBuilder try_control_builder(
      builder(),
      stmt_for_coverage == nullptr ? nullptr : block_coverage_builder_,
      stmt_for_coverage, catch_prediction);

  // We keep a record of all paths that enter the finally-block to be able to
  // dispatch to the correct continuation point after the statements in the
  // finally-block have been evaluated.
  //
  // The try-finally construct can enter the finally-block in three ways:
  // 1. By exiting the try-block normally, falling through at the end.
  // 2. By exiting the try-block with a function-local control flow transfer
  //    (i.e. through break/continue/return statements).
  // 3. By exiting the try-block with a thrown exception.
  //
  // The result register semantics depend on how the block was entered:
  //  - ReturnStatement: It represents the return value being returned.
  //  - ThrowStatement: It represents the exception being thrown.
  //  - BreakStatement/ContinueStatement: Undefined and not used.
  //  - Falling through into finally-block: Undefined and not used.
  Register token = register_allocator()->NewRegister();
  Register result = register_allocator()->NewRegister();
  ControlScope::DeferredCommands commands(this, token, result);

  // Preserve the context in a dedicated register, so that it can be restored
  // when the handler is entered by the stack-unwinding machinery.
  // TODO(ignition): Be smarter about register allocation.
  Register context = register_allocator()->NewRegister();
  builder()->MoveRegister(Register::current_context(), context);

  // Evaluate the try-block inside a control scope. This simulates a handler
  // that is intercepting all control commands.
  try_control_builder.BeginTry(context);
  {
    ControlScopeForTryFinally scope(this, &try_control_builder, &commands);
    try_body_func();
  }
  try_control_builder.EndTry();

  // Record fall-through and exception cases.
  commands.RecordFallThroughPath();
  try_control_builder.LeaveTry();
  try_control_builder.BeginHandler();
  commands.RecordHandlerReThrowPath();

  // Pending message object is saved on entry.
  try_control_builder.BeginFinally();
  Register message = context;  // Reuse register.

  // Clear message object as we enter the finally block.
  builder()->LoadTheHole().SetPendingMessage().StoreAccumulatorInRegister(
      message);

  // Evaluate the finally-block.
  finally_body_func(token);
  try_control_builder.EndFinally();

  // Pending message object is restored on exit.
  builder()->LoadAccumulatorWithRegister(message).SetPendingMessage();

  // Dynamic dispatch after the finally-block.
  commands.ApplyDeferredCommands();
}

void BytecodeGenerator::VisitIterationBody(IterationStatement* stmt,
                                           LoopBuilder* loop_builder) {
  loop_builder->LoopBody();
  ControlScopeForIteration execution_control(this, stmt, loop_builder);
  Visit(stmt->body());
  loop_builder->BindContinueTarget();
}

void BytecodeGenerator::VisitDoWhileStatement(DoWhileStatement* stmt) {
  LoopBuilder loop_builder(builder(), block_coverage_builder_, stmt);
  if (stmt->cond()->ToBooleanIsFalse()) {
    // Since we know that the condition is false, we don't create a loop.
    // Therefore, we don't create a LoopScope (and thus we don't create a header
    // and a JumpToHeader). However, we still need to iterate once through the
    // body.
    VisitIterationBody(stmt, &loop_builder);
  } else if (stmt->cond()->ToBooleanIsTrue()) {
    LoopScope loop_scope(this, &loop_builder);
    VisitIterationBody(stmt, &loop_builder);
  } else {
    LoopScope loop_scope(this, &loop_builder);
    VisitIterationBody(stmt, &loop_builder);
    builder()->SetExpressionAsStatementPosition(stmt->cond());
    BytecodeLabels loop_backbranch(zone());
    VisitForTest(stmt->cond(), &loop_backbranch, loop_builder.break_labels(),
                 TestFallthrough::kThen);
    loop_backbranch.Bind(builder());
  }
}

void BytecodeGenerator::VisitWhileStatement(WhileStatement* stmt) {
  LoopBuilder loop_builder(builder(), block_coverage_builder_, stmt);

  if (stmt->cond()->ToBooleanIsFalse()) {
    // If the condition is false there is no need to generate the loop.
    return;
  }

  LoopScope loop_scope(this, &loop_builder);
  if (!stmt->cond()->ToBooleanIsTrue()) {
    builder()->SetExpressionAsStatementPosition(stmt->cond());
    BytecodeLabels loop_body(zone());
    VisitForTest(stmt->cond(), &loop_body, loop_builder.break_labels(),
                 TestFallthrough::kThen);
    loop_body.Bind(builder());
  }
  VisitIterationBody(stmt, &loop_builder);
}

void BytecodeGenerator::VisitForStatement(ForStatement* stmt) {
  if (stmt->init() != nullptr) {
    Visit(stmt->init());
  }

  LoopBuilder loop_builder(builder(), block_coverage_builder_, stmt);
  if (stmt->cond() && stmt->cond()->ToBooleanIsFalse()) {
    // If the condition is known to be false there is no need to generate
    // body, next or condition blocks. Init block should be generated.
    return;
  }

  LoopScope loop_scope(this, &loop_builder);
  if (stmt->cond() && !stmt->cond()->ToBooleanIsTrue()) {
    builder()->SetExpressionAsStatementPosition(stmt->cond());
    BytecodeLabels loop_body(zone());
    VisitForTest(stmt->cond(), &loop_body, loop_builder.break_labels(),
                 TestFallthrough::kThen);
    loop_body.Bind(builder());
  }
  VisitIterationBody(stmt, &loop_builder);
  if (stmt->next() != nullptr) {
    builder()->SetStatementPosition(stmt->next());
    Visit(stmt->next());
  }
}

void BytecodeGenerator::VisitForInStatement(ForInStatement* stmt) {
  if (stmt->subject()->IsNullLiteral() ||
      stmt->subject()->IsUndefinedLiteral()) {
    // ForIn generates lots of code, skip if it wouldn't produce any effects.
    return;
  }

  BytecodeLabel subject_undefined_label;
  FeedbackSlot slot = feedback_spec()->AddForInSlot();

  // Prepare the state for executing ForIn.
  builder()->SetExpressionAsStatementPosition(stmt->subject());
  VisitForAccumulatorValue(stmt->subject());
  builder()->JumpIfUndefinedOrNull(&subject_undefined_label);
  Register receiver = register_allocator()->NewRegister();
  builder()->ToObject(receiver);

  // Used as kRegTriple and kRegPair in ForInPrepare and ForInNext.
  RegisterList triple = register_allocator()->NewRegisterList(3);
  Register cache_length = triple[2];
  builder()->ForInEnumerate(receiver);
  builder()->ForInPrepare(triple, feedback_index(slot));

  // Set up loop counter
  Register index = register_allocator()->NewRegister();
  builder()->LoadLiteral(Smi::zero());
  builder()->StoreAccumulatorInRegister(index);

  // The loop
  {
    LoopBuilder loop_builder(builder(), block_coverage_builder_, stmt);
    LoopScope loop_scope(this, &loop_builder);
    builder()->SetExpressionAsStatementPosition(stmt->each());
    builder()->ForInContinue(index, cache_length);
    loop_builder.BreakIfFalse(ToBooleanMode::kAlreadyBoolean);
    builder()->ForInNext(receiver, index, triple.Truncate(2),
                         feedback_index(slot));
    loop_builder.ContinueIfUndefined();

    // Assign accumulator value to the 'each' target.
    {
      EffectResultScope scope(this);
      // Make sure to preserve the accumulator across the PrepareAssignmentLhs
      // call.
      AssignmentLhsData lhs_data = PrepareAssignmentLhs(
          stmt->each(), AccumulatorPreservingMode::kPreserve);
      builder()->SetExpressionPosition(stmt->each());
      BuildAssignment(lhs_data, Token::ASSIGN, LookupHoistingMode::kNormal);
    }

    VisitIterationBody(stmt, &loop_builder);
    builder()->ForInStep(index);
    builder()->StoreAccumulatorInRegister(index);
  }
  builder()->Bind(&subject_undefined_label);
}

// Desugar a for-of statement into an application of the iteration protocol.
//
// for (EACH of SUBJECT) BODY
//
//   becomes
//
// iterator = %GetIterator(SUBJECT)
// try {
//
//   loop {
//     // Make sure we are considered 'done' if .next(), .done or .value fail.
//     done = true
//     value = iterator.next()
//     if (value.done) break;
//     value = value.value
//     done = false
//
//     EACH = value
//     BODY
//   }
//   done = true
//
// } catch(e) {
//   iteration_continuation = RETHROW
// } finally {
//   %FinalizeIteration(iterator, done, iteration_continuation)
// }
void BytecodeGenerator::VisitForOfStatement(ForOfStatement* stmt) {
  EffectResultScope effect_scope(this);

  builder()->SetExpressionAsStatementPosition(stmt->subject());
  VisitForAccumulatorValue(stmt->subject());

  // Store the iterator in a dedicated register so that it can be closed on
  // exit, and the 'done' value in a dedicated register so that it can be
  // changed and accessed independently of the iteration result.
  IteratorRecord iterator = BuildGetIteratorRecord(stmt->type());
  Register done = register_allocator()->NewRegister();
  builder()->LoadFalse();
  builder()->StoreAccumulatorInRegister(done);

  BuildTryFinally(
      // Try block.
      [&]() {
        Register next_result = register_allocator()->NewRegister();

        LoopBuilder loop_builder(builder(), block_coverage_builder_, stmt);
        LoopScope loop_scope(this, &loop_builder);

        builder()->LoadTrue().StoreAccumulatorInRegister(done);

        // Call the iterator's .next() method. Break from the loop if the `done`
        // property is truthy, otherwise load the value from the iterator result
        // and append the argument.
        builder()->SetExpressionAsStatementPosition(stmt->each());
        BuildIteratorNext(iterator, next_result);
        builder()->LoadNamedProperty(
            next_result, ast_string_constants()->done_string(),
            feedback_index(feedback_spec()->AddLoadICSlot()));
        loop_builder.BreakIfTrue(ToBooleanMode::kConvertToBoolean);

        builder()
            // value = value.value
            ->LoadNamedProperty(
                next_result, ast_string_constants()->value_string(),
                feedback_index(feedback_spec()->AddLoadICSlot()));
        // done = false, before the assignment to each happens, so that done is
        // false if the assignment throws.
        builder()
            ->StoreAccumulatorInRegister(next_result)
            .LoadFalse()
            .StoreAccumulatorInRegister(done);

        // Assign to the 'each' target.
        AssignmentLhsData lhs_data = PrepareAssignmentLhs(stmt->each());
        builder()->LoadAccumulatorWithRegister(next_result);
        BuildAssignment(lhs_data, Token::ASSIGN, LookupHoistingMode::kNormal);

        VisitIterationBody(stmt, &loop_builder);
      },
      // Finally block.
      [&](Register iteration_continuation_token) {
        // Finish the iteration in the finally block.
        BuildFinalizeIteration(iterator, done, iteration_continuation_token);
      },
      HandlerTable::UNCAUGHT);
}

void BytecodeGenerator::VisitTryCatchStatement(TryCatchStatement* stmt) {
  // Update catch prediction tracking. The updated catch_prediction value lasts
  // until the end of the try_block in the AST node, and does not apply to the
  // catch_block.
  HandlerTable::CatchPrediction outer_catch_prediction = catch_prediction();
  set_catch_prediction(stmt->GetCatchPrediction(outer_catch_prediction));

  BuildTryCatch(
      // Try body.
      [&]() {
        Visit(stmt->try_block());
        set_catch_prediction(outer_catch_prediction);
      },
      // Catch body.
      [&](Register context) {
        if (stmt->scope()) {
          // Create a catch scope that binds the exception.
          BuildNewLocalCatchContext(stmt->scope());
          builder()->StoreAccumulatorInRegister(context);
        }

        // If requested, clear message object as we enter the catch block.
        if (stmt->ShouldClearPendingException(outer_catch_prediction)) {
          builder()->LoadTheHole().SetPendingMessage();
        }

        // Load the catch context into the accumulator.
        builder()->LoadAccumulatorWithRegister(context);

        // Evaluate the catch-block.
        if (stmt->scope()) {
          VisitInScope(stmt->catch_block(), stmt->scope());
        } else {
          VisitBlock(stmt->catch_block());
        }
      },
      catch_prediction(), stmt);
}

void BytecodeGenerator::VisitTryFinallyStatement(TryFinallyStatement* stmt) {
  BuildTryFinally(
      // Try block.
      [&]() { Visit(stmt->try_block()); },
      // Finally block.
      [&](Register body_continuation_token) { Visit(stmt->finally_block()); },
      catch_prediction(), stmt);
}

void BytecodeGenerator::VisitDebuggerStatement(DebuggerStatement* stmt) {
  builder()->SetStatementPosition(stmt);
  builder()->Debugger();
}

void BytecodeGenerator::VisitFunctionLiteral(FunctionLiteral* expr) {
  DCHECK_EQ(expr->scope()->outer_scope(), current_scope());
  uint8_t flags = CreateClosureFlags::Encode(
      expr->pretenure(), closure_scope()->is_function_scope(),
      info()->flags().might_always_opt());
  size_t entry = builder()->AllocateDeferredConstantPoolEntry();
  builder()->CreateClosure(entry, GetCachedCreateClosureSlot(expr), flags);
  function_literals_.push_back(std::make_pair(expr, entry));
  AddToEagerLiteralsIfEager(expr);
}

void BytecodeGenerator::AddToEagerLiteralsIfEager(FunctionLiteral* literal) {
  if (eager_inner_literals_ && literal->ShouldEagerCompile()) {
    DCHECK(!IsInEagerLiterals(literal, *eager_inner_literals_));
    eager_inner_literals_->push_back(literal);
  }
}

bool BytecodeGenerator::ShouldOptimizeAsOneShot() const {
  if (!FLAG_enable_one_shot_optimization) return false;

  if (loop_depth_ > 0) return false;

  return info()->literal()->is_toplevel() ||
         info()->literal()->is_oneshot_iife();
}

void BytecodeGenerator::BuildClassLiteral(ClassLiteral* expr, Register name) {
  size_t class_boilerplate_entry =
      builder()->AllocateDeferredConstantPoolEntry();
  class_literals_.push_back(std::make_pair(expr, class_boilerplate_entry));

  VisitDeclarations(expr->scope()->declarations());
  Register class_constructor = register_allocator()->NewRegister();

  // Create the class brand symbol and store it on the context during class
  // evaluation. This will be stored in the instance later in the constructor.
  // We do this early so that invalid access to private methods or accessors
  // in computed property keys throw.
  if (expr->scope()->brand() != nullptr) {
    Register brand = register_allocator()->NewRegister();
    const AstRawString* class_name =
        expr->scope()->class_variable() != nullptr
            ? expr->scope()->class_variable()->raw_name()
            : ast_string_constants()->empty_string();
    builder()
        ->LoadLiteral(class_name)
        .StoreAccumulatorInRegister(brand)
        .CallRuntime(Runtime::kCreatePrivateBrandSymbol, brand);
    BuildVariableAssignment(expr->scope()->brand(), Token::INIT,
                            HoleCheckMode::kElided);
  }

  AccessorTable<ClassLiteral::Property> private_accessors(zone());
  for (int i = 0; i < expr->private_members()->length(); i++) {
    ClassLiteral::Property* property = expr->private_members()->at(i);
    DCHECK(property->is_private());
    switch (property->kind()) {
      case ClassLiteral::Property::FIELD: {
        // Initialize the private field variables early.
        // Create the private name symbols for fields during class
        // evaluation and store them on the context. These will be
        // used as keys later during instance or static initialization.
        RegisterAllocationScope private_name_register_scope(this);
        Register private_name = register_allocator()->NewRegister();
        VisitForRegisterValue(property->key(), private_name);
        builder()
            ->LoadLiteral(property->key()->AsLiteral()->AsRawPropertyName())
            .StoreAccumulatorInRegister(private_name)
            .CallRuntime(Runtime::kCreatePrivateNameSymbol, private_name);
        DCHECK_NOT_NULL(property->private_name_var());
        BuildVariableAssignment(property->private_name_var(), Token::INIT,
                                HoleCheckMode::kElided);
        break;
      }
      case ClassLiteral::Property::METHOD: {
        RegisterAllocationScope register_scope(this);
        VisitForAccumulatorValue(property->value());
        BuildVariableAssignment(property->private_name_var(), Token::INIT,
                                HoleCheckMode::kElided);
        break;
      }
      // Collect private accessors into a table to merge the creation of
      // those closures later.
      case ClassLiteral::Property::GETTER: {
        Literal* key = property->key()->AsLiteral();
        DCHECK_NULL(private_accessors.LookupOrInsert(key)->getter);
        private_accessors.LookupOrInsert(key)->getter = property;
        break;
      }
      case ClassLiteral::Property::SETTER: {
        Literal* key = property->key()->AsLiteral();
        DCHECK_NULL(private_accessors.LookupOrInsert(key)->setter);
        private_accessors.LookupOrInsert(key)->setter = property;
        break;
      }
      default:
        UNREACHABLE();
    }
  }

  {
    RegisterAllocationScope register_scope(this);
    RegisterList args = register_allocator()->NewGrowableRegisterList();

    Register class_boilerplate = register_allocator()->GrowRegisterList(&args);
    Register class_constructor_in_args =
        register_allocator()->GrowRegisterList(&args);
    Register super_class = register_allocator()->GrowRegisterList(&args);
    DCHECK_EQ(ClassBoilerplate::kFirstDynamicArgumentIndex,
              args.register_count());

    VisitForAccumulatorValueOrTheHole(expr->extends());
    builder()->StoreAccumulatorInRegister(super_class);

    VisitFunctionLiteral(expr->constructor());
    builder()
        ->StoreAccumulatorInRegister(class_constructor)
        .MoveRegister(class_constructor, class_constructor_in_args)
        .LoadConstantPoolEntry(class_boilerplate_entry)
        .StoreAccumulatorInRegister(class_boilerplate);

    // Create computed names and method values nodes to store into the literal.
    for (int i = 0; i < expr->public_members()->length(); i++) {
      ClassLiteral::Property* property = expr->public_members()->at(i);
      if (property->is_computed_name()) {
        Register key = register_allocator()->GrowRegisterList(&args);

        builder()->SetExpressionAsStatementPosition(property->key());
        BuildLoadPropertyKey(property, key);
        if (property->is_static()) {
          // The static prototype property is read only. We handle the non
          // computed property name case in the parser. Since this is the only
          // case where we need to check for an own read only property we
          // special case this so we do not need to do this for every property.

          FeedbackSlot slot = GetDummyCompareICSlot();
          BytecodeLabel done;
          builder()
              ->LoadLiteral(ast_string_constants()->prototype_string())
              .CompareOperation(Token::Value::EQ_STRICT, key,
                                feedback_index(slot))
              .JumpIfFalse(ToBooleanMode::kAlreadyBoolean, &done)
              .CallRuntime(Runtime::kThrowStaticPrototypeError)
              .Bind(&done);
        }

        if (property->kind() == ClassLiteral::Property::FIELD) {
          DCHECK(!property->is_private());
          // Initialize field's name variable with the computed name.
          DCHECK_NOT_NULL(property->computed_name_var());
          builder()->LoadAccumulatorWithRegister(key);
          BuildVariableAssignment(property->computed_name_var(), Token::INIT,
                                  HoleCheckMode::kElided);
        }
      }

      DCHECK(!property->is_private());

      if (property->kind() == ClassLiteral::Property::FIELD) {
        // We don't compute field's value here, but instead do it in the
        // initializer function.
        continue;
      }

      Register value = register_allocator()->GrowRegisterList(&args);
      VisitForRegisterValue(property->value(), value);
    }

    builder()->CallRuntime(Runtime::kDefineClass, args);
  }
  Register prototype = register_allocator()->NewRegister();
  builder()->StoreAccumulatorInRegister(prototype);

  // Assign to the home object variable. Accumulator already contains the
  // prototype.
  Variable* home_object_variable = expr->home_object();
  if (home_object_variable != nullptr) {
    DCHECK(home_object_variable->is_used());
    DCHECK(home_object_variable->IsContextSlot());
    BuildVariableAssignment(home_object_variable, Token::INIT,
                            HoleCheckMode::kElided);
  }
  Variable* static_home_object_variable = expr->static_home_object();
  if (static_home_object_variable != nullptr) {
    DCHECK(static_home_object_variable->is_used());
    DCHECK(static_home_object_variable->IsContextSlot());
    builder()->LoadAccumulatorWithRegister(class_constructor);
    BuildVariableAssignment(static_home_object_variable, Token::INIT,
                            HoleCheckMode::kElided);
  }

  // Assign to class variable.
  Variable* class_variable = expr->scope()->class_variable();
  if (class_variable != nullptr && class_variable->is_used()) {
    DCHECK(class_variable->IsStackLocal() || class_variable->IsContextSlot());
    builder()->LoadAccumulatorWithRegister(class_constructor);
    BuildVariableAssignment(class_variable, Token::INIT,
                            HoleCheckMode::kElided);
  }

  // Define private accessors, using only a single call to the runtime for
  // each pair of corresponding getters and setters, in the order the first
  // component is declared.
  for (auto accessors : private_accessors.ordered_accessors()) {
    RegisterAllocationScope inner_register_scope(this);
    RegisterList accessors_reg = register_allocator()->NewRegisterList(2);
    ClassLiteral::Property* getter = accessors.second->getter;
    ClassLiteral::Property* setter = accessors.second->setter;
    VisitLiteralAccessor(getter, accessors_reg[0]);
    VisitLiteralAccessor(setter, accessors_reg[1]);
    builder()->CallRuntime(Runtime::kCreatePrivateAccessors, accessors_reg);
    Variable* var = getter != nullptr ? getter->private_name_var()
                                      : setter->private_name_var();
    DCHECK_NOT_NULL(var);
    BuildVariableAssignment(var, Token::INIT, HoleCheckMode::kElided);
  }

  if (expr->instance_members_initializer_function() != nullptr) {
    Register initializer =
        VisitForRegisterValue(expr->instance_members_initializer_function());

    FeedbackSlot slot = feedback_spec()->AddStoreICSlot(language_mode());
    builder()
        ->LoadAccumulatorWithRegister(initializer)
        .StoreClassFieldsInitializer(class_constructor, feedback_index(slot))
        .LoadAccumulatorWithRegister(class_constructor);
  }

  if (expr->static_initializer() != nullptr) {
    // TODO(gsathya): This can be optimized away to be a part of the
    // class boilerplate in the future. The name argument can be
    // passed to the DefineClass runtime function and have it set
    // there.
    if (name.is_valid()) {
      Register key = register_allocator()->NewRegister();
      builder()
          ->LoadLiteral(ast_string_constants()->name_string())
          .StoreAccumulatorInRegister(key);

      DataPropertyInLiteralFlags data_property_flags =
          DataPropertyInLiteralFlag::kNoFlags;
      FeedbackSlot slot =
          feedback_spec()->AddStoreDataPropertyInLiteralICSlot();
      builder()->LoadAccumulatorWithRegister(name).StoreDataPropertyInLiteral(
          class_constructor, key, data_property_flags, feedback_index(slot));
    }

    RegisterList args = register_allocator()->NewRegisterList(1);
    Register initializer = VisitForRegisterValue(expr->static_initializer());

    builder()
        ->MoveRegister(class_constructor, args[0])
        .CallProperty(initializer, args,
                      feedback_index(feedback_spec()->AddCallICSlot()));
  }
  builder()->LoadAccumulatorWithRegister(class_constructor);
}

void BytecodeGenerator::VisitClassLiteral(ClassLiteral* expr) {
  VisitClassLiteral(expr, Register::invalid_value());
}

void BytecodeGenerator::VisitClassLiteral(ClassLiteral* expr, Register name) {
  CurrentScope current_scope(this, expr->scope());
  DCHECK_NOT_NULL(expr->scope());
  if (expr->scope()->NeedsContext()) {
    BuildNewLocalBlockContext(expr->scope());
    ContextScope scope(this, expr->scope());
    BuildClassLiteral(expr, name);
  } else {
    BuildClassLiteral(expr, name);
  }
}

void BytecodeGenerator::BuildClassProperty(ClassLiteral::Property* property) {
  RegisterAllocationScope register_scope(this);
  RegisterList args = register_allocator()->NewRegisterList(3);
  Register constructor = args[0], key = args[1], value = args[2];
  builder()->MoveRegister(builder()->Receiver(), constructor);

  // Private methods are not initialized in BuildClassProperty.
  DCHECK_IMPLIES(property->is_private(),
                 property->kind() == ClassLiteral::Property::FIELD);

  if (property->is_computed_name()) {
    DCHECK_EQ(property->kind(), ClassLiteral::Property::FIELD);
    DCHECK(!property->is_private());
    Variable* var = property->computed_name_var();
    DCHECK_NOT_NULL(var);
    // The computed name is already evaluated and stored in a variable at class
    // definition time.
    BuildVariableLoad(var, HoleCheckMode::kElided);
    builder()->StoreAccumulatorInRegister(key);
  } else if (property->is_private()) {
    Variable* private_name_var = property->private_name_var();
    DCHECK_NOT_NULL(private_name_var);
    BuildVariableLoad(private_name_var, HoleCheckMode::kElided);
    builder()->StoreAccumulatorInRegister(key);
  } else {
    BuildLoadPropertyKey(property, key);
  }

  builder()->SetExpressionAsStatementPosition(property->value());
  VisitForRegisterValue(property->value(), value);

  Runtime::FunctionId function_id =
      property->kind() == ClassLiteral::Property::FIELD &&
              !property->is_private()
          ? Runtime::kCreateDataProperty
          : Runtime::kAddPrivateField;
  builder()->CallRuntime(function_id, args);
}

void BytecodeGenerator::VisitInitializeClassMembersStatement(
    InitializeClassMembersStatement* stmt) {
  for (int i = 0; i < stmt->fields()->length(); i++) {
    BuildClassProperty(stmt->fields()->at(i));
  }
}

void BytecodeGenerator::VisitInitializeClassStaticElementsStatement(
    InitializeClassStaticElementsStatement* stmt) {
  for (int i = 0; i < stmt->elements()->length(); i++) {
    ClassLiteral::StaticElement* element = stmt->elements()->at(i);
    switch (element->kind()) {
      case ClassLiteral::StaticElement::PROPERTY:
        BuildClassProperty(element->property());
        break;
      case ClassLiteral::StaticElement::STATIC_BLOCK:
        VisitBlock(element->static_block());
        break;
    }
  }
}

void BytecodeGenerator::BuildInvalidPropertyAccess(MessageTemplate tmpl,
                                                   Property* property) {
  RegisterAllocationScope register_scope(this);
  const AstRawString* name = property->key()->AsVariableProxy()->raw_name();
  RegisterList args = register_allocator()->NewRegisterList(2);
  builder()
      ->LoadLiteral(Smi::FromEnum(tmpl))
      .StoreAccumulatorInRegister(args[0])
      .LoadLiteral(name)
      .StoreAccumulatorInRegister(args[1])
      .CallRuntime(Runtime::kNewTypeError, args)
      .Throw();
}

void BytecodeGenerator::BuildPrivateBrandInitialization(Register receiver) {
  RegisterList brand_args = register_allocator()->NewRegisterList(3);
  Variable* brand = info()->scope()->outer_scope()->AsClassScope()->brand();
  int depth = execution_context()->ContextChainDepth(brand->scope());
  ContextScope* class_context = execution_context()->Previous(depth);

  BuildVariableLoad(brand, HoleCheckMode::kElided);
  builder()
      ->StoreAccumulatorInRegister(brand_args[1])
      .MoveRegister(receiver, brand_args[0])
      .MoveRegister(class_context->reg(), brand_args[2])
      .CallRuntime(Runtime::kAddPrivateBrand, brand_args);
}

void BytecodeGenerator::BuildInstanceMemberInitialization(Register constructor,
                                                          Register instance) {
  RegisterList args = register_allocator()->NewRegisterList(1);
  Register initializer = register_allocator()->NewRegister();

  FeedbackSlot slot = feedback_spec()->AddLoadICSlot();
  BytecodeLabel done;

  builder()
      ->LoadClassFieldsInitializer(constructor, feedback_index(slot))
      // TODO(gsathya): This jump can be elided for the base
      // constructor and derived constructor. This is only required
      // when called from an arrow function.
      .JumpIfUndefined(&done)
      .StoreAccumulatorInRegister(initializer)
      .MoveRegister(instance, args[0])
      .CallProperty(initializer, args,
                    feedback_index(feedback_spec()->AddCallICSlot()))
      .Bind(&done);
}

void BytecodeGenerator::VisitNativeFunctionLiteral(
    NativeFunctionLiteral* expr) {
  size_t entry = builder()->AllocateDeferredConstantPoolEntry();
  int index = feedback_spec()->AddCreateClosureSlot();
  uint8_t flags = CreateClosureFlags::Encode(false, false, false);
  builder()->CreateClosure(entry, index, flags);
  native_function_literals_.push_back(std::make_pair(expr, entry));
}

void BytecodeGenerator::VisitConditional(Conditional* expr) {
  ConditionalControlFlowBuilder conditional_builder(
      builder(), block_coverage_builder_, expr);

  if (expr->condition()->ToBooleanIsTrue()) {
    // Generate then block unconditionally as always true.
    conditional_builder.Then();
    VisitForAccumulatorValue(expr->then_expression());
  } else if (expr->condition()->ToBooleanIsFalse()) {
    // Generate else block unconditionally if it exists.
    conditional_builder.Else();
    VisitForAccumulatorValue(expr->else_expression());
  } else {
    VisitForTest(expr->condition(), conditional_builder.then_labels(),
                 conditional_builder.else_labels(), TestFallthrough::kThen);

    conditional_builder.Then();
    VisitForAccumulatorValue(expr->then_expression());
    conditional_builder.JumpToEnd();

    conditional_builder.Else();
    VisitForAccumulatorValue(expr->else_expression());
  }
}

void BytecodeGenerator::VisitLiteral(Literal* expr) {
  if (execution_result()->IsEffect()) return;
  switch (expr->type()) {
    case Literal::kSmi:
      builder()->LoadLiteral(expr->AsSmiLiteral());
      break;
    case Literal::kHeapNumber:
      builder()->LoadLiteral(expr->AsNumber());
      break;
    case Literal::kUndefined:
      builder()->LoadUndefined();
      break;
    case Literal::kBoolean:
      builder()->LoadBoolean(expr->ToBooleanIsTrue());
      execution_result()->SetResultIsBoolean();
      break;
    case Literal::kNull:
      builder()->LoadNull();
      break;
    case Literal::kTheHole:
      builder()->LoadTheHole();
      break;
    case Literal::kString:
      builder()->LoadLiteral(expr->AsRawString());
      execution_result()->SetResultIsString();
      break;
    case Literal::kBigInt:
      builder()->LoadLiteral(expr->AsBigInt());
      break;
  }
}

void BytecodeGenerator::VisitRegExpLiteral(RegExpLiteral* expr) {
  // Materialize a regular expression literal.
  builder()->CreateRegExpLiteral(
      expr->raw_pattern(), feedback_index(feedback_spec()->AddLiteralSlot()),
      expr->flags());
}

void BytecodeGenerator::BuildCreateObjectLiteral(Register literal,
                                                 uint8_t flags, size_t entry) {
  if (ShouldOptimizeAsOneShot()) {
    RegisterList args = register_allocator()->NewRegisterList(2);
    builder()
        ->LoadConstantPoolEntry(entry)
        .StoreAccumulatorInRegister(args[0])
        .LoadLiteral(Smi::FromInt(flags))
        .StoreAccumulatorInRegister(args[1])
        .CallRuntime(Runtime::kCreateObjectLiteralWithoutAllocationSite, args)
        .StoreAccumulatorInRegister(literal);

  } else {
    // TODO(cbruni): Directly generate runtime call for literals we cannot
    // optimize once the CreateShallowObjectLiteral stub is in sync with the TF
    // optimizations.
    int literal_index = feedback_index(feedback_spec()->AddLiteralSlot());
    builder()
        ->CreateObjectLiteral(entry, literal_index, flags)
        .StoreAccumulatorInRegister(literal);
  }
}

void BytecodeGenerator::VisitObjectLiteral(ObjectLiteral* expr) {
  expr->InitDepthAndFlags();

  // Fast path for the empty object literal which doesn't need an
  // AllocationSite.
  if (expr->IsEmptyObjectLiteral()) {
    DCHECK(expr->IsFastCloningSupported());
    builder()->CreateEmptyObjectLiteral();
    return;
  }

  Variable* home_object = expr->home_object();
  if (home_object != nullptr) {
    DCHECK(home_object->is_used());
    DCHECK(home_object->IsContextSlot());
  }
  MultipleEntryBlockContextScope object_literal_context_scope(
      this, home_object ? home_object->scope() : nullptr);

  // Deep-copy the literal boilerplate.
  uint8_t flags = CreateObjectLiteralFlags::Encode(
      expr->ComputeFlags(), expr->IsFastCloningSupported());

  Register literal = register_allocator()->NewRegister();

  // Create literal object.
  int property_index = 0;
  bool clone_object_spread =
      expr->properties()->first()->kind() == ObjectLiteral::Property::SPREAD;
  if (clone_object_spread) {
    // Avoid the slow path for spreads in the following common cases:
    //   1) `let obj = { ...source }`
    //   2) `let obj = { ...source, override: 1 }`
    //   3) `let obj = { ...source, ...overrides }`
    RegisterAllocationScope register_scope(this);
    Expression* property = expr->properties()->first()->value();
    Register from_value = VisitForRegisterValue(property);
    int clone_index = feedback_index(feedback_spec()->AddCloneObjectSlot());
    builder()->CloneObject(from_value, flags, clone_index);
    builder()->StoreAccumulatorInRegister(literal);
    property_index++;
  } else {
    size_t entry;
    // If constant properties is an empty fixed array, use a cached empty fixed
    // array to ensure it's only added to the constant pool once.
    if (expr->properties_count() == 0) {
      entry = builder()->EmptyObjectBoilerplateDescriptionConstantPoolEntry();
    } else {
      entry = builder()->AllocateDeferredConstantPoolEntry();
      object_literals_.push_back(std::make_pair(expr, entry));
    }
    BuildCreateObjectLiteral(literal, flags, entry);
  }

  // Store computed values into the literal.
  AccessorTable<ObjectLiteral::Property> accessor_table(zone());
  for (; property_index < expr->properties()->length(); property_index++) {
    ObjectLiteral::Property* property = expr->properties()->at(property_index);
    if (property->is_computed_name()) break;
    if (!clone_object_spread && property->IsCompileTimeValue()) continue;

    RegisterAllocationScope inner_register_scope(this);
    Literal* key = property->key()->AsLiteral();
    switch (property->kind()) {
      case ObjectLiteral::Property::SPREAD:
        UNREACHABLE();
      case ObjectLiteral::Property::CONSTANT:
      case ObjectLiteral::Property::MATERIALIZED_LITERAL:
        DCHECK(clone_object_spread || !property->value()->IsCompileTimeValue());
        V8_FALLTHROUGH;
      case ObjectLiteral::Property::COMPUTED: {
        // It is safe to use [[Put]] here because the boilerplate already
        // contains computed properties with an uninitialized value.
        if (key->IsStringLiteral()) {
          DCHECK(key->IsPropertyName());
          object_literal_context_scope.SetEnteredIf(
              property->value()->IsConciseMethodDefinition());
          if (property->emit_store()) {
            builder()->SetExpressionPosition(property->value());
            VisitForAccumulatorValue(property->value());
            FeedbackSlot slot = feedback_spec()->AddStoreOwnICSlot();
            builder()->StoreNamedOwnProperty(literal, key->AsRawPropertyName(),
                                             feedback_index(slot));
          } else {
            builder()->SetExpressionPosition(property->value());
            VisitForEffect(property->value());
          }
        } else {
          RegisterList args = register_allocator()->NewRegisterList(3);

          builder()->MoveRegister(literal, args[0]);
          builder()->SetExpressionPosition(property->key());
          VisitForRegisterValue(property->key(), args[1]);

          object_literal_context_scope.SetEnteredIf(
              property->value()->IsConciseMethodDefinition());
          builder()->SetExpressionPosition(property->value());
          VisitForRegisterValue(property->value(), args[2]);
          if (property->emit_store()) {
            builder()->CallRuntime(Runtime::kSetKeyedProperty, args);
          }
        }
        break;
      }
      case ObjectLiteral::Property::PROTOTYPE: {
        // __proto__:null is handled by CreateObjectLiteral.
        if (property->IsNullPrototype()) break;
        DCHECK(property->emit_store());
        DCHECK(!property->NeedsSetFunctionName());
        RegisterList args = register_allocator()->NewRegisterList(2);
        builder()->MoveRegister(literal, args[0]);
        object_literal_context_scope.SetEnteredIf(false);
        builder()->SetExpressionPosition(property->value());
        VisitForRegisterValue(property->value(), args[1]);
        builder()->CallRuntime(Runtime::kInternalSetPrototype, args);
        break;
      }
      case ObjectLiteral::Property::GETTER:
        if (property->emit_store()) {
          accessor_table.LookupOrInsert(key)->getter = property;
        }
        break;
      case ObjectLiteral::Property::SETTER:
        if (property->emit_store()) {
          accessor_table.LookupOrInsert(key)->setter = property;
        }
        break;
    }
  }

  // Define accessors, using only a single call to the runtime for each pair of
  // corresponding getters and setters.
  object_literal_context_scope.SetEnteredIf(true);
  for (auto accessors : accessor_table.ordered_accessors()) {
    RegisterAllocationScope inner_register_scope(this);
    RegisterList args = register_allocator()->NewRegisterList(5);
    builder()->MoveRegister(literal, args[0]);
    VisitForRegisterValue(accessors.first, args[1]);
    VisitLiteralAccessor(accessors.second->getter, args[2]);
    VisitLiteralAccessor(accessors.second->setter, args[3]);
    builder()
        ->LoadLiteral(Smi::FromInt(NONE))
        .StoreAccumulatorInRegister(args[4])
        .CallRuntime(Runtime::kDefineAccessorPropertyUnchecked, args);
  }

  // Object literals have two parts. The "static" part on the left contains no
  // computed property names, and so we can compute its map ahead of time; see
  // Runtime_CreateObjectLiteralBoilerplate. The second "dynamic" part starts
  // with the first computed property name and continues with all properties to
  // its right. All the code from above initializes the static component of the
  // object literal, and arranges for the map of the result to reflect the
  // static order in which the keys appear. For the dynamic properties, we
  // compile them into a series of "SetOwnProperty" runtime calls. This will
  // preserve insertion order.
  for (; property_index < expr->properties()->length(); property_index++) {
    ObjectLiteral::Property* property = expr->properties()->at(property_index);
    RegisterAllocationScope inner_register_scope(this);

    bool should_be_in_object_literal_scope =
        (property->value()->IsConciseMethodDefinition() ||
         property->value()->IsAccessorFunctionDefinition());

    if (property->IsPrototype()) {
      // __proto__:null is handled by CreateObjectLiteral.
      if (property->IsNullPrototype()) continue;
      DCHECK(property->emit_store());
      DCHECK(!property->NeedsSetFunctionName());
      RegisterList args = register_allocator()->NewRegisterList(2);
      builder()->MoveRegister(literal, args[0]);

      DCHECK(!should_be_in_object_literal_scope);
      object_literal_context_scope.SetEnteredIf(false);
      builder()->SetExpressionPosition(property->value());
      VisitForRegisterValue(property->value(), args[1]);
      builder()->CallRuntime(Runtime::kInternalSetPrototype, args);
      continue;
    }

    switch (property->kind()) {
      case ObjectLiteral::Property::CONSTANT:
      case ObjectLiteral::Property::COMPUTED:
      case ObjectLiteral::Property::MATERIALIZED_LITERAL: {
        // Computed property keys don't belong to the object literal scope (even
        // if they're syntactically inside it).
        if (property->is_computed_name()) {
          object_literal_context_scope.SetEnteredIf(false);
        }
        Register key = register_allocator()->NewRegister();
        BuildLoadPropertyKey(property, key);

        object_literal_context_scope.SetEnteredIf(
            should_be_in_object_literal_scope);
        builder()->SetExpressionPosition(property->value());
        Register value;

        // Static class fields require the name property to be set on
        // the class, meaning we can't wait until the
        // StoreDataPropertyInLiteral call later to set the name.
        if (property->value()->IsClassLiteral() &&
            property->value()->AsClassLiteral()->static_initializer() !=
                nullptr) {
          value = register_allocator()->NewRegister();
          VisitClassLiteral(property->value()->AsClassLiteral(), key);
          builder()->StoreAccumulatorInRegister(value);
        } else {
          value = VisitForRegisterValue(property->value());
        }

        DataPropertyInLiteralFlags data_property_flags =
            DataPropertyInLiteralFlag::kNoFlags;
        if (property->NeedsSetFunctionName()) {
          data_property_flags |= DataPropertyInLiteralFlag::kSetFunctionName;
        }

        FeedbackSlot slot =
            feedback_spec()->AddStoreDataPropertyInLiteralICSlot();
        builder()
            ->LoadAccumulatorWithRegister(value)
            .StoreDataPropertyInLiteral(literal, key, data_property_flags,
                                        feedback_index(slot));
        break;
      }
      case ObjectLiteral::Property::GETTER:
      case ObjectLiteral::Property::SETTER: {
        // Computed property keys don't belong to the object literal scope (even
        // if they're syntactically inside it).
        if (property->is_computed_name()) {
          object_literal_context_scope.SetEnteredIf(false);
        }
        RegisterList args = register_allocator()->NewRegisterList(4);
        builder()->MoveRegister(literal, args[0]);
        BuildLoadPropertyKey(property, args[1]);

        DCHECK(should_be_in_object_literal_scope);
        object_literal_context_scope.SetEnteredIf(true);
        builder()->SetExpressionPosition(property->value());
        VisitForRegisterValue(property->value(), args[2]);
        builder()
            ->LoadLiteral(Smi::FromInt(NONE))
            .StoreAccumulatorInRegister(args[3]);
        Runtime::FunctionId function_id =
            property->kind() == ObjectLiteral::Property::GETTER
                ? Runtime::kDefineGetterPropertyUnchecked
                : Runtime::kDefineSetterPropertyUnchecked;
        builder()->CallRuntime(function_id, args);
        break;
      }
      case ObjectLiteral::Property::SPREAD: {
        RegisterList args = register_allocator()->NewRegisterList(2);
        builder()->MoveRegister(literal, args[0]);
        builder()->SetExpressionPosition(property->value());
        object_literal_context_scope.SetEnteredIf(false);
        VisitForRegisterValue(property->value(), args[1]);
        builder()->CallRuntime(Runtime::kInlineCopyDataProperties, args);
        break;
      }
      case ObjectLiteral::Property::PROTOTYPE:
        UNREACHABLE();  // Handled specially above.
        break;
    }
  }

  builder()->LoadAccumulatorWithRegister(literal);
  if (home_object != nullptr) {
    object_literal_context_scope.SetEnteredIf(true);
    BuildVariableAssignment(home_object, Token::INIT, HoleCheckMode::kElided);
  }
}

// Fill an array with values from an iterator, starting at a given index. It is
// guaranteed that the loop will only terminate if the iterator is exhausted, or
// if one of iterator.next(), value.done, or value.value fail.
//
// In pseudocode:
//
// loop {
//   value = iterator.next()
//   if (value.done) break;
//   value = value.value
//   array[index++] = value
// }
void BytecodeGenerator::BuildFillArrayWithIterator(
    IteratorRecord iterator, Register array, Register index, Register value,
    FeedbackSlot next_value_slot, FeedbackSlot next_done_slot,
    FeedbackSlot index_slot, FeedbackSlot element_slot) {
  DCHECK(array.is_valid());
  DCHECK(index.is_valid());
  DCHECK(value.is_valid());

  LoopBuilder loop_builder(builder(), nullptr, nullptr);
  LoopScope loop_scope(this, &loop_builder);

  // Call the iterator's .next() method. Break from the loop if the `done`
  // property is truthy, otherwise load the value from the iterator result and
  // append the argument.
  BuildIteratorNext(iterator, value);
  builder()->LoadNamedProperty(
      value, ast_string_constants()->done_string(),
      feedback_index(feedback_spec()->AddLoadICSlot()));
  loop_builder.BreakIfTrue(ToBooleanMode::kConvertToBoolean);

  loop_builder.LoopBody();
  builder()
      // value = value.value
      ->LoadNamedProperty(value, ast_string_constants()->value_string(),
                          feedback_index(next_value_slot))
      // array[index] = value
      .StoreInArrayLiteral(array, index, feedback_index(element_slot))
      // index++
      .LoadAccumulatorWithRegister(index)
      .UnaryOperation(Token::INC, feedback_index(index_slot))
      .StoreAccumulatorInRegister(index);
  loop_builder.BindContinueTarget();
}

void BytecodeGenerator::BuildCreateArrayLiteral(
    const ZonePtrList<Expression>* elements, ArrayLiteral* expr) {
  RegisterAllocationScope register_scope(this);
  Register index = register_allocator()->NewRegister();
  Register array = register_allocator()->NewRegister();
  SharedFeedbackSlot element_slot(feedback_spec(),
                                  FeedbackSlotKind::kStoreInArrayLiteral);
  ZonePtrList<Expression>::const_iterator current = elements->begin();
  ZonePtrList<Expression>::const_iterator end = elements->end();
  bool is_empty = elements->is_empty();

  if (!is_empty && (*current)->IsSpread()) {
    // If we have a leading spread, use CreateArrayFromIterable to create
    // an array from it and then add the remaining components to that array.
    VisitForAccumulatorValue(*current);
    builder()->SetExpressionPosition((*current)->AsSpread()->expression());
    builder()->CreateArrayFromIterable().StoreAccumulatorInRegister(array);

    if (++current != end) {
      // If there are remaning elements, prepare the index register that is
      // used for adding those elements. The next index is the length of the
      // newly created array.
      auto length = ast_string_constants()->length_string();
      int length_load_slot = feedback_index(feedback_spec()->AddLoadICSlot());
      builder()
          ->LoadNamedProperty(array, length, length_load_slot)
          .StoreAccumulatorInRegister(index);
    }
  } else if (expr != nullptr) {
    // There are some elements before the first (if any) spread, and we can
    // use a boilerplate when creating the initial array from those elements.

    // First, allocate a constant pool entry for the boilerplate that will
    // be created during finalization, and will contain all the constant
    // elements before the first spread. This also handle the empty array case
    // and one-shot optimization.
    uint8_t flags = CreateArrayLiteralFlags::Encode(
        expr->IsFastCloningSupported(), expr->ComputeFlags());
    bool optimize_as_one_shot = ShouldOptimizeAsOneShot();
    size_t entry;
    if (is_empty && optimize_as_one_shot) {
      entry = builder()->EmptyArrayBoilerplateDescriptionConstantPoolEntry();
    } else if (!is_empty) {
      entry = builder()->AllocateDeferredConstantPoolEntry();
      array_literals_.push_back(std::make_pair(expr, entry));
    }

    if (optimize_as_one_shot) {
      RegisterList args = register_allocator()->NewRegisterList(2);
      builder()
          ->LoadConstantPoolEntry(entry)
          .StoreAccumulatorInRegister(args[0])
          .LoadLiteral(Smi::FromInt(flags))
          .StoreAccumulatorInRegister(args[1])
          .CallRuntime(Runtime::kCreateArrayLiteralWithoutAllocationSite, args);
    } else if (is_empty) {
      // Empty array literal fast-path.
      int literal_index = feedback_index(feedback_spec()->AddLiteralSlot());
      DCHECK(expr->IsFastCloningSupported());
      builder()->CreateEmptyArrayLiteral(literal_index);
    } else {
      // Create array literal from boilerplate.
      int literal_index = feedback_index(feedback_spec()->AddLiteralSlot());
      builder()->CreateArrayLiteral(entry, literal_index, flags);
    }
    builder()->StoreAccumulatorInRegister(array);

    // Insert the missing non-constant elements, up until the first spread
    // index, into the initial array (the remaining elements will be inserted
    // below).
    DCHECK_EQ(current, elements->begin());
    ZonePtrList<Expression>::const_iterator first_spread_or_end =
        expr->first_spread_index() >= 0 ? current + expr->first_spread_index()
                                        : end;
    int array_index = 0;
    for (; current != first_spread_or_end; ++current, array_index++) {
      Expression* subexpr = *current;
      DCHECK(!subexpr->IsSpread());
      // Skip the constants.
      if (subexpr->IsCompileTimeValue()) continue;

      builder()
          ->LoadLiteral(Smi::FromInt(array_index))
          .StoreAccumulatorInRegister(index);
      VisitForAccumulatorValue(subexpr);
      builder()->StoreInArrayLiteral(array, index,
                                     feedback_index(element_slot.Get()));
    }

    if (current != end) {
      // If there are remaining elements, prepare the index register
      // to store the next element, which comes from the first spread.
      builder()
          ->LoadLiteral(Smi::FromInt(array_index))
          .StoreAccumulatorInRegister(index);
    }
  } else {
    // TODO(v8:11582): Support allocating boilerplates here.

    // In other cases, we prepare an empty array to be filled in below.
    DCHECK(!elements->is_empty());
    int literal_index = feedback_index(feedback_spec()->AddLiteralSlot());
    builder()
        ->CreateEmptyArrayLiteral(literal_index)
        .StoreAccumulatorInRegister(array);
    // Prepare the index for the first element.
    builder()->LoadLiteral(Smi::FromInt(0)).StoreAccumulatorInRegister(index);
  }

  // Now build insertions for the remaining elements from current to end.
  SharedFeedbackSlot index_slot(feedback_spec(), FeedbackSlotKind::kBinaryOp);
  SharedFeedbackSlot length_slot(
      feedback_spec(), feedback_spec()->GetStoreICSlot(LanguageMode::kStrict));
  for (; current != end; ++current) {
    Expression* subexpr = *current;
    if (subexpr->IsSpread()) {
      RegisterAllocationScope scope(this);
      builder()->SetExpressionAsStatementPosition(
          subexpr->AsSpread()->expression());
      VisitForAccumulatorValue(subexpr->AsSpread()->expression());
      builder()->SetExpressionPosition(subexpr->AsSpread()->expression());
      IteratorRecord iterator = BuildGetIteratorRecord(IteratorType::kNormal);

      Register value = register_allocator()->NewRegister();
      FeedbackSlot next_value_load_slot = feedback_spec()->AddLoadICSlot();
      FeedbackSlot next_done_load_slot = feedback_spec()->AddLoadICSlot();
      FeedbackSlot real_index_slot = index_slot.Get();
      FeedbackSlot real_element_slot = element_slot.Get();
      BuildFillArrayWithIterator(iterator, array, index, value,
                                 next_value_load_slot, next_done_load_slot,
                                 real_index_slot, real_element_slot);
    } else if (!subexpr->IsTheHoleLiteral()) {
      // literal[index++] = subexpr
      VisitForAccumulatorValue(subexpr);
      builder()
          ->StoreInArrayLiteral(array, index,
                                feedback_index(element_slot.Get()))
          .LoadAccumulatorWithRegister(index);
      // Only increase the index if we are not the last element.
      if (current + 1 != end) {
        builder()
            ->UnaryOperation(Token::INC, feedback_index(index_slot.Get()))
            .StoreAccumulatorInRegister(index);
      }
    } else {
      // literal.length = ++index
      // length_slot is only used when there are holes.
      auto length = ast_string_constants()->length_string();
      builder()
          ->LoadAccumulatorWithRegister(index)
          .UnaryOperation(Token::INC, feedback_index(index_slot.Get()))
          .StoreAccumulatorInRegister(index)
          .StoreNamedProperty(array, length, feedback_index(length_slot.Get()),
                              LanguageMode::kStrict);
    }
  }

  builder()->LoadAccumulatorWithRegister(array);
}

void BytecodeGenerator::VisitArrayLiteral(ArrayLiteral* expr) {
  expr->InitDepthAndFlags();
  BuildCreateArrayLiteral(expr->values(), expr);
}

void BytecodeGenerator::VisitVariableProxy(VariableProxy* proxy) {
  builder()->SetExpressionPosition(proxy);
  BuildVariableLoad(proxy->var(), proxy->hole_check_mode());
}

void BytecodeGenerator::BuildVariableLoad(Variable* variable,
                                          HoleCheckMode hole_check_mode,
                                          TypeofMode typeof_mode) {
  switch (variable->location()) {
    case VariableLocation::LOCAL: {
      Register source(builder()->Local(variable->index()));
      // We need to load the variable into the accumulator, even when in a
      // VisitForRegisterScope, in order to avoid register aliasing if
      // subsequent expressions assign to the same variable.
      builder()->LoadAccumulatorWithRegister(source);
      if (hole_check_mode == HoleCheckMode::kRequired) {
        BuildThrowIfHole(variable);
      }
      break;
    }
    case VariableLocation::PARAMETER: {
      Register source;
      if (variable->IsReceiver()) {
        source = builder()->Receiver();
      } else {
        source = builder()->Parameter(variable->index());
      }
      // We need to load the variable into the accumulator, even when in a
      // VisitForRegisterScope, in order to avoid register aliasing if
      // subsequent expressions assign to the same variable.
      builder()->LoadAccumulatorWithRegister(source);
      if (hole_check_mode == HoleCheckMode::kRequired) {
        BuildThrowIfHole(variable);
      }
      break;
    }
    case VariableLocation::UNALLOCATED: {
      // The global identifier "undefined" is immutable. Everything
      // else could be reassigned. For performance, we do a pointer comparison
      // rather than checking if the raw_name is really "undefined".
      if (variable->raw_name() == ast_string_constants()->undefined_string()) {
        builder()->LoadUndefined();
      } else {
        FeedbackSlot slot = GetCachedLoadGlobalICSlot(typeof_mode, variable);
        builder()->LoadGlobal(variable->raw_name(), feedback_index(slot),
                              typeof_mode);
      }
      break;
    }
    case VariableLocation::CONTEXT: {
      int depth = execution_context()->ContextChainDepth(variable->scope());
      ContextScope* context = execution_context()->Previous(depth);
      Register context_reg;
      if (context) {
        context_reg = context->reg();
        depth = 0;
      } else {
        context_reg = execution_context()->reg();
      }

      BytecodeArrayBuilder::ContextSlotMutability immutable =
          (variable->maybe_assigned() == kNotAssigned)
              ? BytecodeArrayBuilder::kImmutableSlot
              : BytecodeArrayBuilder::kMutableSlot;

      builder()->LoadContextSlot(context_reg, variable->index(), depth,
                                 immutable);
      if (hole_check_mode == HoleCheckMode::kRequired) {
        BuildThrowIfHole(variable);
      }
      break;
    }
    case VariableLocation::LOOKUP: {
      switch (variable->mode()) {
        case VariableMode::kDynamicLocal: {
          Variable* local_variable = variable->local_if_not_shadowed();
          int depth =
              execution_context()->ContextChainDepth(local_variable->scope());
          builder()->LoadLookupContextSlot(variable->raw_name(), typeof_mode,
                                           local_variable->index(), depth);
          if (hole_check_mode == HoleCheckMode::kRequired) {
            BuildThrowIfHole(variable);
          }
          break;
        }
        case VariableMode::kDynamicGlobal: {
          int depth =
              current_scope()->ContextChainLengthUntilOutermostSloppyEval();
          // TODO(1008414): Add back caching here when bug is fixed properly.
          FeedbackSlot slot = feedback_spec()->AddLoadGlobalICSlot(typeof_mode);

          builder()->LoadLookupGlobalSlot(variable->raw_name(), typeof_mode,
                                          feedback_index(slot), depth);
          break;
        }
        default:
          builder()->LoadLookupSlot(variable->raw_name(), typeof_mode);
      }
      break;
    }
    case VariableLocation::MODULE: {
      int depth = execution_context()->ContextChainDepth(variable->scope());
      builder()->LoadModuleVariable(variable->index(), depth);
      if (hole_check_mode == HoleCheckMode::kRequired) {
        BuildThrowIfHole(variable);
      }
      break;
    }
    case VariableLocation::REPL_GLOBAL: {
      DCHECK(variable->IsReplGlobal());
      FeedbackSlot slot = GetCachedLoadGlobalICSlot(typeof_mode, variable);
      builder()->LoadGlobal(variable->raw_name(), feedback_index(slot),
                            typeof_mode);
      break;
    }
  }
}

void BytecodeGenerator::BuildVariableLoadForAccumulatorValue(
    Variable* variable, HoleCheckMode hole_check_mode, TypeofMode typeof_mode) {
  ValueResultScope accumulator_result(this);
  BuildVariableLoad(variable, hole_check_mode, typeof_mode);
}

void BytecodeGenerator::BuildReturn(int source_position) {
  if (FLAG_trace) {
    RegisterAllocationScope register_scope(this);
    Register result = register_allocator()->NewRegister();
    // Runtime returns {result} value, preserving accumulator.
    builder()->StoreAccumulatorInRegister(result).CallRuntime(
        Runtime::kTraceExit, result);
  }
  if (info()->flags().collect_type_profile()) {
    builder()->CollectTypeProfile(info()->literal()->return_position());
  }
  builder()->SetStatementPosition(source_position);
  builder()->Return();
}

void BytecodeGenerator::BuildAsyncReturn(int source_position) {
  RegisterAllocationScope register_scope(this);

  if (IsAsyncGeneratorFunction(info()->literal()->kind())) {
    RegisterList args = register_allocator()->NewRegisterList(3);
    builder()
        ->MoveRegister(generator_object(), args[0])  // generator
        .StoreAccumulatorInRegister(args[1])         // value
        .LoadTrue()
        .StoreAccumulatorInRegister(args[2])  // done
        .CallRuntime(Runtime::kInlineAsyncGeneratorResolve, args);
  } else {
    DCHECK(IsAsyncFunction(info()->literal()->kind()) ||
           IsAsyncModule(info()->literal()->kind()));
    RegisterList args = register_allocator()->NewRegisterList(3);
    builder()
        ->MoveRegister(generator_object(), args[0])  // generator
        .StoreAccumulatorInRegister(args[1])         // value
        .LoadBoolean(info()->literal()->CanSuspend())
        .StoreAccumulatorInRegister(args[2])  // can_suspend
        .CallRuntime(Runtime::kInlineAsyncFunctionResolve, args);
  }

  BuildReturn(source_position);
}

void BytecodeGenerator::BuildReThrow() { builder()->ReThrow(); }

void BytecodeGenerator::BuildThrowIfHole(Variable* variable) {
  if (variable->is_this()) {
    DCHECK(variable->mode() == VariableMode::kConst);
    builder()->ThrowSuperNotCalledIfHole();
  } else {
    builder()->ThrowReferenceErrorIfHole(variable->raw_name());
  }
}

void BytecodeGenerator::BuildHoleCheckForVariableAssignment(Variable* variable,
                                                            Token::Value op) {
  DCHECK(!IsPrivateMethodOrAccessorVariableMode(variable->mode()));
  if (variable->is_this() && variable->mode() == VariableMode::kConst &&
      op == Token::INIT) {
    // Perform an initialization check for 'this'. 'this' variable is the
    // only variable able to trigger bind operations outside the TDZ
    // via 'super' calls.
    builder()->ThrowSuperAlreadyCalledIfNotHole();
  } else {
    // Perform an initialization check for let/const declared variables.
    // E.g. let x = (x = 20); is not allowed.
    DCHECK(IsLexicalVariableMode(variable->mode()));
    BuildThrowIfHole(variable);
  }
}

void BytecodeGenerator::BuildVariableAssignment(
    Variable* variable, Token::Value op, HoleCheckMode hole_check_mode,
    LookupHoistingMode lookup_hoisting_mode) {
  VariableMode mode = variable->mode();
  RegisterAllocationScope assignment_register_scope(this);
  BytecodeLabel end_label;
  switch (variable->location()) {
    case VariableLocation::PARAMETER:
    case VariableLocation::LOCAL: {
      Register destination;
      if (VariableLocation::PARAMETER == variable->location()) {
        if (variable->IsReceiver()) {
          destination = builder()->Receiver();
        } else {
          destination = builder()->Parameter(variable->index());
        }
      } else {
        destination = builder()->Local(variable->index());
      }

      if (hole_check_mode == HoleCheckMode::kRequired) {
        // Load destination to check for hole.
        Register value_temp = register_allocator()->NewRegister();
        builder()
            ->StoreAccumulatorInRegister(value_temp)
            .LoadAccumulatorWithRegister(destination);

        BuildHoleCheckForVariableAssignment(variable, op);
        builder()->LoadAccumulatorWithRegister(value_temp);
      }

      if (mode != VariableMode::kConst || op == Token::INIT) {
        builder()->StoreAccumulatorInRegister(destination);
      } else if (variable->throw_on_const_assignment(language_mode())) {
        builder()->CallRuntime(Runtime::kThrowConstAssignError);
      }
      break;
    }
    case VariableLocation::UNALLOCATED: {
      FeedbackSlot slot = GetCachedStoreGlobalICSlot(language_mode(), variable);
      builder()->StoreGlobal(variable->raw_name(), feedback_index(slot));
      break;
    }
    case VariableLocation::CONTEXT: {
      int depth = execution_context()->ContextChainDepth(variable->scope());
      ContextScope* context = execution_context()->Previous(depth);
      Register context_reg;

      if (context) {
        context_reg = context->reg();
        depth = 0;
      } else {
        context_reg = execution_context()->reg();
      }

      if (hole_check_mode == HoleCheckMode::kRequired) {
        // Load destination to check for hole.
        Register value_temp = register_allocator()->NewRegister();
        builder()
            ->StoreAccumulatorInRegister(value_temp)
            .LoadContextSlot(context_reg, variable->index(), depth,
                             BytecodeArrayBuilder::kMutableSlot);

        BuildHoleCheckForVariableAssignment(variable, op);
        builder()->LoadAccumulatorWithRegister(value_temp);
      }

      if (mode != VariableMode::kConst || op == Token::INIT) {
        builder()->StoreContextSlot(context_reg, variable->index(), depth);
      } else if (variable->throw_on_const_assignment(language_mode())) {
        builder()->CallRuntime(Runtime::kThrowConstAssignError);
      }
      break;
    }
    case VariableLocation::LOOKUP: {
      builder()->StoreLookupSlot(variable->raw_name(), language_mode(),
                                 lookup_hoisting_mode);
      break;
    }
    case VariableLocation::MODULE: {
      DCHECK(IsDeclaredVariableMode(mode));

      if (mode == VariableMode::kConst && op != Token::INIT) {
        builder()->CallRuntime(Runtime::kThrowConstAssignError);
        break;
      }

      // If we don't throw above, we know that we're dealing with an
      // export because imports are const and we do not generate initializing
      // assignments for them.
      DCHECK(variable->IsExport());

      int depth = execution_context()->ContextChainDepth(variable->scope());
      if (hole_check_mode == HoleCheckMode::kRequired) {
        Register value_temp = register_allocator()->NewRegister();
        builder()
            ->StoreAccumulatorInRegister(value_temp)
            .LoadModuleVariable(variable->index(), depth);
        BuildHoleCheckForVariableAssignment(variable, op);
        builder()->LoadAccumulatorWithRegister(value_temp);
      }
      builder()->StoreModuleVariable(variable->index(), depth);
      break;
    }
    case VariableLocation::REPL_GLOBAL: {
      // A let or const declaration like 'let x = 7' is effectively translated
      // to:
      //   <top of the script>:
      //     ScriptContext.x = TheHole;
      //   ...
      //   <where the actual 'let' is>:
      //     ScriptContextTable.x = 7; // no hole check
      //
      // The ScriptContext slot for 'x' that we store to here is not
      // necessarily the ScriptContext of this script, but rather the
      // first ScriptContext that has a slot for name 'x'.
      DCHECK(variable->IsReplGlobal());
      if (op == Token::INIT) {
        RegisterList store_args = register_allocator()->NewRegisterList(2);
        builder()
            ->StoreAccumulatorInRegister(store_args[1])
            .LoadLiteral(variable->raw_name())
            .StoreAccumulatorInRegister(store_args[0]);
        builder()->CallRuntime(
            Runtime::kStoreGlobalNoHoleCheckForReplLetOrConst, store_args);
      } else {
        if (mode == VariableMode::kConst) {
          builder()->CallRuntime(Runtime::kThrowConstAssignError);
        } else {
          FeedbackSlot slot =
              GetCachedStoreGlobalICSlot(language_mode(), variable);
          builder()->StoreGlobal(variable->raw_name(), feedback_index(slot));
        }
      }
      break;
    }
  }
}

void BytecodeGenerator::BuildLoadNamedProperty(const Expression* object_expr,
                                               Register object,
                                               const AstRawString* name) {
  if (ShouldOptimizeAsOneShot()) {
    builder()->LoadNamedPropertyNoFeedback(object, name);
  } else {
    FeedbackSlot slot = GetCachedLoadICSlot(object_expr, name);
    builder()->LoadNamedProperty(object, name, feedback_index(slot));
  }
}

void BytecodeGenerator::BuildStoreNamedProperty(const Expression* object_expr,
                                                Register object,
                                                const AstRawString* name) {
  Register value;
  if (!execution_result()->IsEffect()) {
    value = register_allocator()->NewRegister();
    builder()->StoreAccumulatorInRegister(value);
  }

  if (ShouldOptimizeAsOneShot()) {
    builder()->StoreNamedPropertyNoFeedback(object, name, language_mode());
  } else {
    FeedbackSlot slot = GetCachedStoreICSlot(object_expr, name);
    builder()->StoreNamedProperty(object, name, feedback_index(slot),
                                  language_mode());
  }

  if (!execution_result()->IsEffect()) {
    builder()->LoadAccumulatorWithRegister(value);
  }
}

// static
BytecodeGenerator::AssignmentLhsData
BytecodeGenerator::AssignmentLhsData::NonProperty(Expression* expr) {
  return AssignmentLhsData(NON_PROPERTY, expr, RegisterList(), Register(),
                           Register(), nullptr, nullptr);
}
// static
BytecodeGenerator::AssignmentLhsData
BytecodeGenerator::AssignmentLhsData::NamedProperty(Expression* object_expr,
                                                    Register object,
                                                    const AstRawString* name) {
  return AssignmentLhsData(NAMED_PROPERTY, nullptr, RegisterList(), object,
                           Register(), object_expr, name);
}
// static
BytecodeGenerator::AssignmentLhsData
BytecodeGenerator::AssignmentLhsData::KeyedProperty(Register object,
                                                    Register key) {
  return AssignmentLhsData(KEYED_PROPERTY, nullptr, RegisterList(), object, key,
                           nullptr, nullptr);
}
// static
BytecodeGenerator::AssignmentLhsData
BytecodeGenerator::AssignmentLhsData::NamedSuperProperty(
    RegisterList super_property_args) {
  return AssignmentLhsData(NAMED_SUPER_PROPERTY, nullptr, super_property_args,
                           Register(), Register(), nullptr, nullptr);
}
// static
BytecodeGenerator::AssignmentLhsData
BytecodeGenerator::AssignmentLhsData::PrivateMethodOrAccessor(
    AssignType type, Property* property) {
  return AssignmentLhsData(type, property, RegisterList(), Register(),
                           Register(), nullptr, nullptr);
}
// static
BytecodeGenerator::AssignmentLhsData
BytecodeGenerator::AssignmentLhsData::KeyedSuperProperty(
    RegisterList super_property_args) {
  return AssignmentLhsData(KEYED_SUPER_PROPERTY, nullptr, super_property_args,
                           Register(), Register(), nullptr, nullptr);
}

BytecodeGenerator::AssignmentLhsData BytecodeGenerator::PrepareAssignmentLhs(
    Expression* lhs, AccumulatorPreservingMode accumulator_preserving_mode) {
  // Left-hand side can only be a property, a global or a variable slot.
  Property* property = lhs->AsProperty();
  AssignType assign_type = Property::GetAssignType(property);

  // Evaluate LHS expression.
  switch (assign_type) {
    case NON_PROPERTY:
      return AssignmentLhsData::NonProperty(lhs);
    case NAMED_PROPERTY: {
      AccumulatorPreservingScope scope(this, accumulator_preserving_mode);
      Register object = VisitForRegisterValue(property->obj());
      const AstRawString* name =
          property->key()->AsLiteral()->AsRawPropertyName();
      return AssignmentLhsData::NamedProperty(property->obj(), object, name);
    }
    case KEYED_PROPERTY: {
      AccumulatorPreservingScope scope(this, accumulator_preserving_mode);
      Register object = VisitForRegisterValue(property->obj());
      Register key = VisitForRegisterValue(property->key());
      return AssignmentLhsData::KeyedProperty(object, key);
    }
    case PRIVATE_METHOD:
    case PRIVATE_GETTER_ONLY:
    case PRIVATE_SETTER_ONLY:
    case PRIVATE_GETTER_AND_SETTER: {
      DCHECK(!property->IsSuperAccess());
      return AssignmentLhsData::PrivateMethodOrAccessor(assign_type, property);
    }
    case NAMED_SUPER_PROPERTY: {
      AccumulatorPreservingScope scope(this, accumulator_preserving_mode);
      RegisterList super_property_args =
          register_allocator()->NewRegisterList(4);
      BuildThisVariableLoad();
      builder()->StoreAccumulatorInRegister(super_property_args[0]);
      BuildVariableLoad(
          property->obj()->AsSuperPropertyReference()->home_object()->var(),
          HoleCheckMode::kElided);
      builder()->StoreAccumulatorInRegister(super_property_args[1]);
      builder()
          ->LoadLiteral(property->key()->AsLiteral()->AsRawPropertyName())
          .StoreAccumulatorInRegister(super_property_args[2]);
      return AssignmentLhsData::NamedSuperProperty(super_property_args);
    }
    case KEYED_SUPER_PROPERTY: {
      AccumulatorPreservingScope scope(this, accumulator_preserving_mode);
      RegisterList super_property_args =
          register_allocator()->NewRegisterList(4);
      BuildThisVariableLoad();
      builder()->StoreAccumulatorInRegister(super_property_args[0]);
      BuildVariableLoad(
          property->obj()->AsSuperPropertyReference()->home_object()->var(),
          HoleCheckMode::kElided);
      builder()->StoreAccumulatorInRegister(super_property_args[1]);
      VisitForRegisterValue(property->key(), super_property_args[2]);
      return AssignmentLhsData::KeyedSuperProperty(super_property_args);
    }
  }
  UNREACHABLE();
}

// Build the iteration finalizer called in the finally block of an iteration
// protocol execution. This closes the iterator if needed, and suppresses any
// exception it throws if necessary, including the exception when the return
// method is not callable.
//
// In pseudo-code, this builds:
//
// if (!done) {
//   try {
//     let method = iterator.return
//     if (method !== null && method !== undefined) {
//       let return_val = method.call(iterator)
//       if (!%IsObject(return_val)) throw TypeError
//     }
//   } catch (e) {
//     if (iteration_continuation != RETHROW)
//       rethrow e
//   }
// }
//
// For async iterators, iterator.close() becomes await iterator.close().
void BytecodeGenerator::BuildFinalizeIteration(
    IteratorRecord iterator, Register done,
    Register iteration_continuation_token) {
  RegisterAllocationScope register_scope(this);
  BytecodeLabels iterator_is_done(zone());

  // if (!done) {
  builder()->LoadAccumulatorWithRegister(done).JumpIfTrue(
      ToBooleanMode::kConvertToBoolean, iterator_is_done.New());

  {
    RegisterAllocationScope register_scope(this);
    BuildTryCatch(
        // try {
        //   let method = iterator.return
        //   if (method !== null && method !== undefined) {
        //     let return_val = method.call(iterator)
        //     if (!%IsObject(return_val)) throw TypeError
        //   }
        // }
        [&]() {
          Register method = register_allocator()->NewRegister();
          builder()
              ->LoadNamedProperty(
                  iterator.object(), ast_string_constants()->return_string(),
                  feedback_index(feedback_spec()->AddLoadICSlot()))
              .JumpIfUndefinedOrNull(iterator_is_done.New())
              .StoreAccumulatorInRegister(method);

          RegisterList args(iterator.object());
          builder()->CallProperty(
              method, args, feedback_index(feedback_spec()->AddCallICSlot()));
          if (iterator.type() == IteratorType::kAsync) {
            BuildAwait();
          }
          builder()->JumpIfJSReceiver(iterator_is_done.New());
          {
            // Throw this exception inside the try block so that it is
            // suppressed by the iteration continuation if necessary.
            RegisterAllocationScope register_scope(this);
            Register return_result = register_allocator()->NewRegister();
            builder()
                ->StoreAccumulatorInRegister(return_result)
                .CallRuntime(Runtime::kThrowIteratorResultNotAnObject,
                             return_result);
          }
        },

        // catch (e) {
        //   if (iteration_continuation != RETHROW)
        //     rethrow e
        // }
        [&](Register context) {
          // Reuse context register to store the exception.
          Register close_exception = context;
          builder()->StoreAccumulatorInRegister(close_exception);

          BytecodeLabel suppress_close_exception;
          builder()
              ->LoadLiteral(
                  Smi::FromInt(ControlScope::DeferredCommands::kRethrowToken))
              .CompareReference(iteration_continuation_token)
              .JumpIfTrue(ToBooleanMode::kAlreadyBoolean,
                          &suppress_close_exception)
              .LoadAccumulatorWithRegister(close_exception)
              .ReThrow()
              .Bind(&suppress_close_exception);
        },
        HandlerTable::UNCAUGHT);
  }

  iterator_is_done.Bind(builder());
}

// Get the default value of a destructuring target. Will mutate the
// destructuring target expression if there is a default value.
//
// For
//   a = b
// in
//   let {a = b} = c
// returns b and mutates the input into a.
Expression* BytecodeGenerator::GetDestructuringDefaultValue(
    Expression** target) {
  Expression* default_value = nullptr;
  if ((*target)->IsAssignment()) {
    Assignment* default_init = (*target)->AsAssignment();
    DCHECK_EQ(default_init->op(), Token::ASSIGN);
    default_value = default_init->value();
    *target = default_init->target();
    DCHECK((*target)->IsValidReferenceExpression() || (*target)->IsPattern());
  }
  return default_value;
}

// Convert a destructuring assignment to an array literal into a sequence of
// iterator accesses into the value being assigned (in the accumulator).
//
// [a().x, ...b] = accumulator
//
//   becomes
//
// iterator = %GetIterator(accumulator)
// try {
//
//   // Individual assignments read off the value from iterator.next() This gets
//   // repeated per destructuring element.
//   if (!done) {
//     // Make sure we are considered 'done' if .next(), .done or .value fail.
//     done = true
//     var next_result = iterator.next()
//     var tmp_done = next_result.done
//     if (!tmp_done) {
//       value = next_result.value
//       done = false
//     }
//   }
//   if (done)
//     value = undefined
//   a().x = value
//
//   // A spread receives the remaining items in the iterator.
//   var array = []
//   var index = 0
//   %FillArrayWithIterator(iterator, array, index, done)
//   done = true
//   b = array
//
// } catch(e) {
//   iteration_continuation = RETHROW
// } finally {
//   %FinalizeIteration(iterator, done, iteration_continuation)
// }
void BytecodeGenerator::BuildDestructuringArrayAssignment(
    ArrayLiteral* pattern, Token::Value op,
    LookupHoistingMode lookup_hoisting_mode) {
  RegisterAllocationScope scope(this);

  Register value = register_allocator()->NewRegister();
  builder()->StoreAccumulatorInRegister(value);

  // Store the iterator in a dedicated register so that it can be closed on
  // exit, and the 'done' value in a dedicated register so that it can be
  // changed and accessed independently of the iteration result.
  IteratorRecord iterator = BuildGetIteratorRecord(IteratorType::kNormal);
  Register done = register_allocator()->NewRegister();
  builder()->LoadFalse();
  builder()->StoreAccumulatorInRegister(done);

  BuildTryFinally(
      // Try block.
      [&]() {
        Register next_result = register_allocator()->NewRegister();
        FeedbackSlot next_value_load_slot = feedback_spec()->AddLoadICSlot();
        FeedbackSlot next_done_load_slot = feedback_spec()->AddLoadICSlot();

        Spread* spread = nullptr;
        for (Expression* target : *pattern->values()) {
          if (target->IsSpread()) {
            spread = target->AsSpread();
            break;
          }

          Expression* default_value = GetDestructuringDefaultValue(&target);
          if (!target->IsPattern()) {
            builder()->SetExpressionAsStatementPosition(target);
          }

          AssignmentLhsData lhs_data = PrepareAssignmentLhs(target);

          // if (!done) {
          //   // Make sure we are considered done if .next(), .done or .value
          //   // fail.
          //   done = true
          //   var next_result = iterator.next()
          //   var tmp_done = next_result.done
          //   if (!tmp_done) {
          //     value = next_result.value
          //     done = false
          //   }
          // }
          // if (done)
          //   value = undefined
          BytecodeLabels is_done(zone());

          builder()->LoadAccumulatorWithRegister(done);
          builder()->JumpIfTrue(ToBooleanMode::kConvertToBoolean,
                                is_done.New());

          builder()->LoadTrue().StoreAccumulatorInRegister(done);
          BuildIteratorNext(iterator, next_result);
          builder()
              ->LoadNamedProperty(next_result,
                                  ast_string_constants()->done_string(),
                                  feedback_index(next_done_load_slot))
              .JumpIfTrue(ToBooleanMode::kConvertToBoolean, is_done.New())
              .LoadNamedProperty(next_result,
                                 ast_string_constants()->value_string(),
                                 feedback_index(next_value_load_slot))
              .StoreAccumulatorInRegister(next_result)
              .LoadFalse()
              .StoreAccumulatorInRegister(done)
              .LoadAccumulatorWithRegister(next_result);

          // Only do the assignment if this is not a hole (i.e. 'elided').
          if (!target->IsTheHoleLiteral()) {
            // [<pattern> = <init>] = <value>
            //   becomes (roughly)
            // temp = <value>.next();
            // <pattern> = temp === undefined ? <init> : temp;
            BytecodeLabel do_assignment;
            if (default_value) {
              builder()->JumpIfNotUndefined(&do_assignment);
              // Since done == true => temp == undefined, jump directly to using
              // the default value for that case.
              is_done.Bind(builder());
              VisitForAccumulatorValue(default_value);
            } else {
              builder()->Jump(&do_assignment);
              is_done.Bind(builder());
              builder()->LoadUndefined();
            }
            builder()->Bind(&do_assignment);

            BuildAssignment(lhs_data, op, lookup_hoisting_mode);
          } else {
            DCHECK_EQ(lhs_data.assign_type(), NON_PROPERTY);
            is_done.Bind(builder());
          }
        }

        if (spread) {
          RegisterAllocationScope scope(this);
          BytecodeLabel is_done;

          // A spread is turned into a loop over the remainer of the iterator.
          Expression* target = spread->expression();

          if (!target->IsPattern()) {
            builder()->SetExpressionAsStatementPosition(spread);
          }

          AssignmentLhsData lhs_data = PrepareAssignmentLhs(target);

          // var array = [];
          Register array = register_allocator()->NewRegister();
          builder()->CreateEmptyArrayLiteral(
              feedback_index(feedback_spec()->AddLiteralSlot()));
          builder()->StoreAccumulatorInRegister(array);

          // If done, jump to assigning empty array
          builder()->LoadAccumulatorWithRegister(done);
          builder()->JumpIfTrue(ToBooleanMode::kConvertToBoolean, &is_done);

          // var index = 0;
          Register index = register_allocator()->NewRegister();
          builder()->LoadLiteral(Smi::zero());
          builder()->StoreAccumulatorInRegister(index);

          // Set done to true, since it's guaranteed to be true by the time the
          // array fill completes.
          builder()->LoadTrue().StoreAccumulatorInRegister(done);

          // Fill the array with the iterator.
          FeedbackSlot element_slot =
              feedback_spec()->AddStoreInArrayLiteralICSlot();
          FeedbackSlot index_slot = feedback_spec()->AddBinaryOpICSlot();
          BuildFillArrayWithIterator(iterator, array, index, next_result,
                                     next_value_load_slot, next_done_load_slot,
                                     index_slot, element_slot);

          builder()->Bind(&is_done);
          // Assign the array to the LHS.
          builder()->LoadAccumulatorWithRegister(array);
          BuildAssignment(lhs_data, op, lookup_hoisting_mode);
        }
      },
      // Finally block.
      [&](Register iteration_continuation_token) {
        // Finish the iteration in the finally block.
        BuildFinalizeIteration(iterator, done, iteration_continuation_token);
      },
      HandlerTable::UNCAUGHT);

  if (!execution_result()->IsEffect()) {
    builder()->LoadAccumulatorWithRegister(value);
  }
}

// Convert a destructuring assignment to an object literal into a sequence of
// property accesses into the value being assigned (in the accumulator).
//
// { y, [x++]: a(), ...b.c } = value
//
//   becomes
//
// var rest_runtime_callargs = new Array(3);
// rest_runtime_callargs[0] = value;
//
// rest_runtime_callargs[1] = value;
// y = value.y;
//
// var temp1 = %ToName(x++);
// rest_runtime_callargs[2] = temp1;
// a() = value[temp1];
//
// b.c = %CopyDataPropertiesWithExcludedProperties.call(rest_runtime_callargs);
void BytecodeGenerator::BuildDestructuringObjectAssignment(
    ObjectLiteral* pattern, Token::Value op,
    LookupHoistingMode lookup_hoisting_mode) {
  RegisterAllocationScope scope(this);

  // Store the assignment value in a register.
  Register value;
  RegisterList rest_runtime_callargs;
  if (pattern->has_rest_property()) {
    rest_runtime_callargs =
        register_allocator()->NewRegisterList(pattern->properties()->length());
    value = rest_runtime_callargs[0];
  } else {
    value = register_allocator()->NewRegister();
  }
  builder()->StoreAccumulatorInRegister(value);

  // if (value === null || value === undefined)
  //   throw new TypeError(kNonCoercible);
  //
  // Since the first property access on null/undefined will also trigger a
  // TypeError, we can elide this check. The exception is when there are no
  // properties and no rest property (this is an empty literal), or when the
  // first property is a computed name and accessing it can have side effects.
  //
  // TODO(leszeks): Also eliminate this check if the value is known to be
  // non-null (e.g. an object literal).
  if (pattern->properties()->is_empty() ||
      (pattern->properties()->at(0)->is_computed_name() &&
       pattern->properties()->at(0)->kind() != ObjectLiteralProperty::SPREAD)) {
    BytecodeLabel is_null_or_undefined, not_null_or_undefined;
    builder()
        ->JumpIfUndefinedOrNull(&is_null_or_undefined)
        .Jump(&not_null_or_undefined);

    {
      builder()->Bind(&is_null_or_undefined);
      builder()->SetExpressionPosition(pattern);
      builder()->CallRuntime(Runtime::kThrowPatternAssignmentNonCoercible,
                             value);
    }
    builder()->Bind(&not_null_or_undefined);
  }

  int i = 0;
  for (ObjectLiteralProperty* pattern_property : *pattern->properties()) {
    RegisterAllocationScope scope(this);

    // The key of the pattern becomes the key into the RHS value, and the value
    // of the pattern becomes the target of the assignment.
    //
    // e.g. { a: b } = o becomes b = o.a
    Expression* pattern_key = pattern_property->key();
    Expression* target = pattern_property->value();
    Expression* default_value = GetDestructuringDefaultValue(&target);

    if (!target->IsPattern()) {
      builder()->SetExpressionAsStatementPosition(target);
    }

    // Calculate this property's key into the assignment RHS value, additionally
    // storing the key for rest_runtime_callargs if needed.
    //
    // The RHS is accessed using the key either by LoadNamedProperty (if
    // value_name is valid) or by LoadKeyedProperty (otherwise).
    const AstRawString* value_name = nullptr;
    Register value_key;

    if (pattern_property->kind() != ObjectLiteralProperty::Kind::SPREAD) {
      if (pattern_key->IsPropertyName()) {
        value_name = pattern_key->AsLiteral()->AsRawPropertyName();
      }
      if (pattern->has_rest_property() || !value_name) {
        if (pattern->has_rest_property()) {
          value_key = rest_runtime_callargs[i + 1];
        } else {
          value_key = register_allocator()->NewRegister();
        }
        if (pattern_property->is_computed_name()) {
          // { [a()]: b().x } = c
          // becomes
          // var tmp = a()
          // b().x = c[tmp]
          DCHECK(!pattern_key->IsPropertyName() ||
                 !pattern_key->IsNumberLiteral());
          VisitForAccumulatorValue(pattern_key);
          builder()->ToName(value_key);
        } else {
          // We only need the key for non-computed properties when it is numeric
          // or is being saved for the rest_runtime_callargs.
          DCHECK(
              pattern_key->IsNumberLiteral() ||
              (pattern->has_rest_property() && pattern_key->IsPropertyName()));
          VisitForRegisterValue(pattern_key, value_key);
        }
      }
    }

    AssignmentLhsData lhs_data = PrepareAssignmentLhs(target);

    // Get the value from the RHS.
    if (pattern_property->kind() == ObjectLiteralProperty::Kind::SPREAD) {
      DCHECK_EQ(i, pattern->properties()->length() - 1);
      DCHECK(!value_key.is_valid());
      DCHECK_NULL(value_name);
      builder()->CallRuntime(Runtime::kCopyDataPropertiesWithExcludedProperties,
                             rest_runtime_callargs);
    } else if (value_name) {
      builder()->LoadNamedProperty(
          value, value_name, feedback_index(feedback_spec()->AddLoadICSlot()));
    } else {
      DCHECK(value_key.is_valid());
      builder()->LoadAccumulatorWithRegister(value_key).LoadKeyedProperty(
          value, feedback_index(feedback_spec()->AddKeyedLoadICSlot()));
    }

    // {<pattern> = <init>} = <value>
    //   becomes
    // temp = <value>;
    // <pattern> = temp === undefined ? <init> : temp;
    if (default_value) {
      BytecodeLabel value_not_undefined;
      builder()->JumpIfNotUndefined(&value_not_undefined);
      VisitForAccumulatorValue(default_value);
      builder()->Bind(&value_not_undefined);
    }

    BuildAssignment(lhs_data, op, lookup_hoisting_mode);

    i++;
  }

  if (!execution_result()->IsEffect()) {
    builder()->LoadAccumulatorWithRegister(value);
  }
}

void BytecodeGenerator::BuildAssignment(
    const AssignmentLhsData& lhs_data, Token::Value op,
    LookupHoistingMode lookup_hoisting_mode) {
  // Assign the value to the LHS.
  switch (lhs_data.assign_type()) {
    case NON_PROPERTY: {
      if (ObjectLiteral* pattern = lhs_data.expr()->AsObjectLiteral()) {
        // Split object literals into destructuring.
        BuildDestructuringObjectAssignment(pattern, op, lookup_hoisting_mode);
      } else if (ArrayLiteral* pattern = lhs_data.expr()->AsArrayLiteral()) {
        // Split object literals into destructuring.
        BuildDestructuringArrayAssignment(pattern, op, lookup_hoisting_mode);
      } else {
        DCHECK(lhs_data.expr()->IsVariableProxy());
        VariableProxy* proxy = lhs_data.expr()->AsVariableProxy();
        BuildVariableAssignment(proxy->var(), op, proxy->hole_check_mode(),
                                lookup_hoisting_mode);
      }
      break;
    }
    case NAMED_PROPERTY: {
      BuildStoreNamedProperty(lhs_data.object_expr(), lhs_data.object(),
                              lhs_data.name());
      break;
    }
    case KEYED_PROPERTY: {
      FeedbackSlot slot = feedback_spec()->AddKeyedStoreICSlot(language_mode());
      Register value;
      if (!execution_result()->IsEffect()) {
        value = register_allocator()->NewRegister();
        builder()->StoreAccumulatorInRegister(value);
      }
      builder()->StoreKeyedProperty(lhs_data.object(), lhs_data.key(),
                                    feedback_index(slot), language_mode());
      if (!execution_result()->IsEffect()) {
        builder()->LoadAccumulatorWithRegister(value);
      }
      break;
    }
    case NAMED_SUPER_PROPERTY: {
      builder()
          ->StoreAccumulatorInRegister(lhs_data.super_property_args()[3])
          .CallRuntime(Runtime::kStoreToSuper, lhs_data.super_property_args());
      break;
    }
    case KEYED_SUPER_PROPERTY: {
      builder()
          ->StoreAccumulatorInRegister(lhs_data.super_property_args()[3])
          .CallRuntime(Runtime::kStoreKeyedToSuper,
                       lhs_data.super_property_args());
      break;
    }
    case PRIVATE_METHOD: {
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateMethodWrite,
                                 lhs_data.expr()->AsProperty());
      break;
    }
    case PRIVATE_GETTER_ONLY: {
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateSetterAccess,
                                 lhs_data.expr()->AsProperty());
      break;
    }
    case PRIVATE_SETTER_ONLY:
    case PRIVATE_GETTER_AND_SETTER: {
      Register value = register_allocator()->NewRegister();
      builder()->StoreAccumulatorInRegister(value);
      Property* property = lhs_data.expr()->AsProperty();
      Register object = VisitForRegisterValue(property->obj());
      Register key = VisitForRegisterValue(property->key());
      BuildPrivateBrandCheck(property, object,
                             MessageTemplate::kInvalidPrivateMemberWrite);
      BuildPrivateSetterAccess(object, key, value);
      if (!execution_result()->IsEffect()) {
        builder()->LoadAccumulatorWithRegister(value);
      }
      break;
    }
  }
}

void BytecodeGenerator::VisitAssignment(Assignment* expr) {
  AssignmentLhsData lhs_data = PrepareAssignmentLhs(expr->target());

  VisitForAccumulatorValue(expr->value());

  builder()->SetExpressionPosition(expr);
  BuildAssignment(lhs_data, expr->op(), expr->lookup_hoisting_mode());
}

void BytecodeGenerator::VisitCompoundAssignment(CompoundAssignment* expr) {
  AssignmentLhsData lhs_data = PrepareAssignmentLhs(expr->target());

  // Evaluate the value and potentially handle compound assignments by loading
  // the left-hand side value and performing a binary operation.
  switch (lhs_data.assign_type()) {
    case NON_PROPERTY: {
      VariableProxy* proxy = expr->target()->AsVariableProxy();
      BuildVariableLoad(proxy->var(), proxy->hole_check_mode());
      break;
    }
    case NAMED_PROPERTY: {
      BuildLoadNamedProperty(lhs_data.object_expr(), lhs_data.object(),
                             lhs_data.name());
      break;
    }
    case KEYED_PROPERTY: {
      FeedbackSlot slot = feedback_spec()->AddKeyedLoadICSlot();
      builder()
          ->LoadAccumulatorWithRegister(lhs_data.key())
          .LoadKeyedProperty(lhs_data.object(), feedback_index(slot));
      break;
    }
    case NAMED_SUPER_PROPERTY: {
      builder()->CallRuntime(Runtime::kLoadFromSuper,
                             lhs_data.super_property_args().Truncate(3));
      break;
    }
    case KEYED_SUPER_PROPERTY: {
      builder()->CallRuntime(Runtime::kLoadKeyedFromSuper,
                             lhs_data.super_property_args().Truncate(3));
      break;
    }
    case PRIVATE_METHOD: {
      // The property access is invalid, but if the brand check fails too, we
      // need to return the error from the brand check.
      Property* property = lhs_data.expr()->AsProperty();
      Register object = VisitForRegisterValue(property->obj());
      BuildPrivateBrandCheck(property, object,
                             MessageTemplate::kInvalidPrivateMemberRead);
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateMethodWrite,
                                 lhs_data.expr()->AsProperty());
      break;
    }
    case PRIVATE_GETTER_ONLY: {
      // The property access is invalid, but if the brand check fails too, we
      // need to return the error from the brand check.
      Property* property = lhs_data.expr()->AsProperty();
      Register object = VisitForRegisterValue(property->obj());
      BuildPrivateBrandCheck(property, object,
                             MessageTemplate::kInvalidPrivateMemberRead);
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateSetterAccess,
                                 lhs_data.expr()->AsProperty());

      break;
    }
    case PRIVATE_SETTER_ONLY: {
      // The property access is invalid, but if the brand check fails too, we
      // need to return the error from the brand check.
      Property* property = lhs_data.expr()->AsProperty();
      Register object = VisitForRegisterValue(property->obj());
      BuildPrivateBrandCheck(property, object,
                             MessageTemplate::kInvalidPrivateMemberRead);
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateGetterAccess,
                                 lhs_data.expr()->AsProperty());
      break;
    }
    case PRIVATE_GETTER_AND_SETTER: {
      Property* property = lhs_data.expr()->AsProperty();
      Register object = VisitForRegisterValue(property->obj());
      Register key = VisitForRegisterValue(property->key());
      BuildPrivateBrandCheck(property, object,
                             MessageTemplate::kInvalidPrivateMemberRead);
      BuildPrivateGetterAccess(object, key);
      break;
    }
  }

  BinaryOperation* binop = expr->binary_operation();
  FeedbackSlot slot = feedback_spec()->AddBinaryOpICSlot();
  BytecodeLabel short_circuit;
  if (binop->op() == Token::NULLISH) {
    BytecodeLabel nullish;
    builder()
        ->JumpIfUndefinedOrNull(&nullish)
        .Jump(&short_circuit)
        .Bind(&nullish);
    VisitForAccumulatorValue(expr->value());
  } else if (binop->op() == Token::OR) {
    builder()->JumpIfTrue(ToBooleanMode::kConvertToBoolean, &short_circuit);
    VisitForAccumulatorValue(expr->value());
  } else if (binop->op() == Token::AND) {
    builder()->JumpIfFalse(ToBooleanMode::kConvertToBoolean, &short_circuit);
    VisitForAccumulatorValue(expr->value());
  } else if (expr->value()->IsSmiLiteral()) {
    builder()->BinaryOperationSmiLiteral(
        binop->op(), expr->value()->AsLiteral()->AsSmiLiteral(),
        feedback_index(slot));
  } else {
    Register old_value = register_allocator()->NewRegister();
    builder()->StoreAccumulatorInRegister(old_value);
    VisitForAccumulatorValue(expr->value());
    builder()->BinaryOperation(binop->op(), old_value, feedback_index(slot));
  }
  builder()->SetExpressionPosition(expr);
  BuildAssignment(lhs_data, expr->op(), expr->lookup_hoisting_mode());
  builder()->Bind(&short_circuit);
}

// Suspends the generator to resume at the next suspend_id, with output stored
// in the accumulator. When the generator is resumed, the sent value is loaded
// in the accumulator.
void BytecodeGenerator::BuildSuspendPoint(int position) {
  // Because we eliminate jump targets in dead code, we also eliminate resumes
  // when the suspend is not emitted because otherwise the below call to Bind
  // would start a new basic block and the code would be considered alive.
  if (builder()->RemainderOfBlockIsDead()) {
    return;
  }
  const int suspend_id = suspend_count_++;

  RegisterList registers = register_allocator()->AllLiveRegisters();

  // Save context, registers, and state. This bytecode then returns the value
  // in the accumulator.
  builder()->SetExpressionPosition(position);
  builder()->SuspendGenerator(generator_object(), registers, suspend_id);

  // Upon resume, we continue here.
  builder()->Bind(generator_jump_table_, suspend_id);

  // Clobbers all registers and sets the accumulator to the
  // [[input_or_debug_pos]] slot of the generator object.
  builder()->ResumeGenerator(generator_object(), registers);
}

void BytecodeGenerator::VisitYield(Yield* expr) {
  builder()->SetExpressionPosition(expr);
  VisitForAccumulatorValue(expr->expression());

  // If this is not the first yield
  if (suspend_count_ > 0) {
    if (IsAsyncGeneratorFunction(function_kind())) {
      // AsyncGenerator yields (with the exception of the initial yield)
      // delegate work to the AsyncGeneratorYield stub, which Awaits the operand
      // and on success, wraps the value in an IteratorResult.
      RegisterAllocationScope register_scope(this);
      RegisterList args = register_allocator()->NewRegisterList(3);
      builder()
          ->MoveRegister(generator_object(), args[0])  // generator
          .StoreAccumulatorInRegister(args[1])         // value
          .LoadBoolean(catch_prediction() != HandlerTable::ASYNC_AWAIT)
          .StoreAccumulatorInRegister(args[2])  // is_caught
          .CallRuntime(Runtime::kInlineAsyncGeneratorYield, args);
    } else {
      // Generator yields (with the exception of the initial yield) wrap the
      // value into IteratorResult.
      RegisterAllocationScope register_scope(this);
      RegisterList args = register_allocator()->NewRegisterList(2);
      builder()
          ->StoreAccumulatorInRegister(args[0])  // value
          .LoadFalse()
          .StoreAccumulatorInRegister(args[1])  // done
          .CallRuntime(Runtime::kInlineCreateIterResultObject, args);
    }
  }

  BuildSuspendPoint(expr->position());
  // At this point, the generator has been resumed, with the received value in
  // the accumulator.

  // TODO(caitp): remove once yield* desugaring for async generators is handled
  // in BytecodeGenerator.
  if (expr->on_abrupt_resume() == Yield::kNoControl) {
    DCHECK(IsAsyncGeneratorFunction(function_kind()));
    return;
  }

  Register input = register_allocator()->NewRegister();
  builder()->StoreAccumulatorInRegister(input).CallRuntime(
      Runtime::kInlineGeneratorGetResumeMode, generator_object());

  // Now dispatch on resume mode.
  STATIC_ASSERT(JSGeneratorObject::kNext + 1 == JSGeneratorObject::kReturn);
  BytecodeJumpTable* jump_table =
      builder()->AllocateJumpTable(2, JSGeneratorObject::kNext);

  builder()->SwitchOnSmiNoFeedback(jump_table);

  {
    // Resume with throw (switch fallthrough).
    // TODO(leszeks): Add a debug-only check that the accumulator is
    // JSGeneratorObject::kThrow.
    builder()->SetExpressionPosition(expr);
    builder()->LoadAccumulatorWithRegister(input);
    builder()->Throw();
  }

  {
    // Resume with return.
    builder()->Bind(jump_table, JSGeneratorObject::kReturn);
    builder()->LoadAccumulatorWithRegister(input);
    if (IsAsyncGeneratorFunction(function_kind())) {
      execution_control()->AsyncReturnAccumulator(kNoSourcePosition);
    } else {
      execution_control()->ReturnAccumulator(kNoSourcePosition);
    }
  }

  {
    // Resume with next.
    builder()->Bind(jump_table, JSGeneratorObject::kNext);
    BuildIncrementBlockCoverageCounterIfEnabled(expr,
                                                SourceRangeKind::kContinuation);
    builder()->LoadAccumulatorWithRegister(input);
  }
}

// Desugaring of (yield* iterable)
//
//   do {
//     const kNext = 0;
//     const kReturn = 1;
//     const kThrow = 2;
//
//     let output; // uninitialized
//
//     let iteratorRecord = GetIterator(iterable);
//     let iterator = iteratorRecord.[[Iterator]];
//     let next = iteratorRecord.[[NextMethod]];
//     let input = undefined;
//     let resumeMode = kNext;
//
//     while (true) {
//       // From the generator to the iterator:
//       // Forward input according to resumeMode and obtain output.
//       switch (resumeMode) {
//         case kNext:
//           output = next.[[Call]](iterator, « »);;
//           break;
//         case kReturn:
//           let iteratorReturn = iterator.return;
//           if (IS_NULL_OR_UNDEFINED(iteratorReturn)) {
//             if (IS_ASYNC_GENERATOR) input = await input;
//             return input;
//           }
//           output = iteratorReturn.[[Call]](iterator, «input»);
//           break;
//         case kThrow:
//           let iteratorThrow = iterator.throw;
//           if (IS_NULL_OR_UNDEFINED(iteratorThrow)) {
//             let iteratorReturn = iterator.return;
//             if (!IS_NULL_OR_UNDEFINED(iteratorReturn)) {
//               output = iteratorReturn.[[Call]](iterator, « »);
//               if (IS_ASYNC_GENERATOR) output = await output;
//               if (!IS_RECEIVER(output)) %ThrowIterResultNotAnObject(output);
//             }
//             throw MakeTypeError(kThrowMethodMissing);
//           }
//           output = iteratorThrow.[[Call]](iterator, «input»);
//           break;
//       }
//
//       if (IS_ASYNC_GENERATOR) output = await output;
//       if (!IS_RECEIVER(output)) %ThrowIterResultNotAnObject(output);
//       if (output.done) break;
//
//       // From the generator to its user:
//       // Forward output, receive new input, and determine resume mode.
//       if (IS_ASYNC_GENERATOR) {
//         // AsyncGeneratorYield abstract operation awaits the operand before
//         // resolving the promise for the current AsyncGeneratorRequest.
//         %_AsyncGeneratorYield(output.value)
//       }
//       input = Suspend(output);
//       resumeMode = %GeneratorGetResumeMode();
//     }
//
//     if (resumeMode === kReturn) {
//       return output.value;
//     }
//     output.value
//   }
void BytecodeGenerator::VisitYieldStar(YieldStar* expr) {
  Register output = register_allocator()->NewRegister();
  Register resume_mode = register_allocator()->NewRegister();
  IteratorType iterator_type = IsAsyncGeneratorFunction(function_kind())
                                   ? IteratorType::kAsync
                                   : IteratorType::kNormal;

  {
    RegisterAllocationScope register_scope(this);
    RegisterList iterator_and_input = register_allocator()->NewRegisterList(2);
    VisitForAccumulatorValue(expr->expression());
    IteratorRecord iterator = BuildGetIteratorRecord(
        register_allocator()->NewRegister() /* next method */,
        iterator_and_input[0], iterator_type);

    Register input = iterator_and_input[1];
    builder()->LoadUndefined().StoreAccumulatorInRegister(input);
    builder()
        ->LoadLiteral(Smi::FromInt(JSGeneratorObject::kNext))
        .StoreAccumulatorInRegister(resume_mode);

    {
      // This loop builder does not construct counters as the loop is not
      // visible to the user, and we therefore neither pass the block coverage
      // builder nor the expression.
      //
      // In addition to the normal suspend for yield*, a yield* in an async
      // generator has 2 additional suspends:
      //   - One for awaiting the iterator result of closing the generator when
      //     resumed with a "throw" completion, and a throw method is not
      //     present on the delegated iterator
      //   - One for awaiting the iterator result yielded by the delegated
      //     iterator

      LoopBuilder loop_builder(builder(), nullptr, nullptr);
      LoopScope loop_scope(this, &loop_builder);

      {
        BytecodeLabels after_switch(zone());
        BytecodeJumpTable* switch_jump_table =
            builder()->AllocateJumpTable(2, 1);

        builder()
            ->LoadAccumulatorWithRegister(resume_mode)
            .SwitchOnSmiNoFeedback(switch_jump_table);

        // Fallthrough to default case.
        // TODO(ignition): Add debug code to check that {resume_mode} really is
        // {JSGeneratorObject::kNext} in this case.
        STATIC_ASSERT(JSGeneratorObject::kNext == 0);
        {
          FeedbackSlot slot = feedback_spec()->AddCallICSlot();
          builder()->CallProperty(iterator.next(), iterator_and_input,
                                  feedback_index(slot));
          builder()->Jump(after_switch.New());
        }

        STATIC_ASSERT(JSGeneratorObject::kReturn == 1);
        builder()->Bind(switch_jump_table, JSGeneratorObject::kReturn);
        {
          const AstRawString* return_string =
              ast_string_constants()->return_string();
          BytecodeLabels no_return_method(zone());

          BuildCallIteratorMethod(iterator.object(), return_string,
                                  iterator_and_input, after_switch.New(),
                                  &no_return_method);
          no_return_method.Bind(builder());
          builder()->LoadAccumulatorWithRegister(input);
          if (iterator_type == IteratorType::kAsync) {
            // Await input.
            BuildAwait(expr->position());
            execution_control()->AsyncReturnAccumulator(kNoSourcePosition);
          } else {
            execution_control()->ReturnAccumulator(kNoSourcePosition);
          }
        }

        STATIC_ASSERT(JSGeneratorObject::kThrow == 2);
        builder()->Bind(switch_jump_table, JSGeneratorObject::kThrow);
        {
          const AstRawString* throw_string =
              ast_string_constants()->throw_string();
          BytecodeLabels no_throw_method(zone());
          BuildCallIteratorMethod(iterator.object(), throw_string,
                                  iterator_and_input, after_switch.New(),
                                  &no_throw_method);

          // If there is no "throw" method, perform IteratorClose, and finally
          // throw a TypeError.
          no_throw_method.Bind(builder());
          BuildIteratorClose(iterator, expr);
          builder()->CallRuntime(Runtime::kThrowThrowMethodMissing);
        }

        after_switch.Bind(builder());
      }

      if (iterator_type == IteratorType::kAsync) {
        // Await the result of the method invocation.
        BuildAwait(expr->position());
      }

      // Check that output is an object.
      BytecodeLabel check_if_done;
      builder()
          ->StoreAccumulatorInRegister(output)
          .JumpIfJSReceiver(&check_if_done)
          .CallRuntime(Runtime::kThrowIteratorResultNotAnObject, output);

      builder()->Bind(&check_if_done);
      // Break once output.done is true.
      builder()->LoadNamedProperty(
          output, ast_string_constants()->done_string(),
          feedback_index(feedback_spec()->AddLoadICSlot()));

      loop_builder.BreakIfTrue(ToBooleanMode::kConvertToBoolean);

      // Suspend the current generator.
      if (iterator_type == IteratorType::kNormal) {
        builder()->LoadAccumulatorWithRegister(output);
      } else {
        RegisterAllocationScope register_scope(this);
        DCHECK_EQ(iterator_type, IteratorType::kAsync);
        // If generatorKind is async, perform AsyncGeneratorYield(output.value),
        // which will await `output.value` before resolving the current
        // AsyncGeneratorRequest's promise.
        builder()->LoadNamedProperty(
            output, ast_string_constants()->value_string(),
            feedback_index(feedback_spec()->AddLoadICSlot()));

        RegisterList args = register_allocator()->NewRegisterList(3);
        builder()
            ->MoveRegister(generator_object(), args[0])  // generator
            .StoreAccumulatorInRegister(args[1])         // value
            .LoadBoolean(catch_prediction() != HandlerTable::ASYNC_AWAIT)
            .StoreAccumulatorInRegister(args[2])  // is_caught
            .CallRuntime(Runtime::kInlineAsyncGeneratorYield, args);
      }

      BuildSuspendPoint(expr->position());
      builder()->StoreAccumulatorInRegister(input);
      builder()
          ->CallRuntime(Runtime::kInlineGeneratorGetResumeMode,
                        generator_object())
          .StoreAccumulatorInRegister(resume_mode);

      loop_builder.BindContinueTarget();
    }
  }

  // Decide if we trigger a return or if the yield* expression should just
  // produce a value.
  BytecodeLabel completion_is_output_value;
  Register output_value = register_allocator()->NewRegister();
  builder()
      ->LoadNamedProperty(output, ast_string_constants()->value_string(),
                          feedback_index(feedback_spec()->AddLoadICSlot()))
      .StoreAccumulatorInRegister(output_value)
      .LoadLiteral(Smi::FromInt(JSGeneratorObject::kReturn))
      .CompareReference(resume_mode)
      .JumpIfFalse(ToBooleanMode::kAlreadyBoolean, &completion_is_output_value)
      .LoadAccumulatorWithRegister(output_value);
  if (iterator_type == IteratorType::kAsync) {
    execution_control()->AsyncReturnAccumulator(kNoSourcePosition);
  } else {
    execution_control()->ReturnAccumulator(kNoSourcePosition);
  }

  builder()->Bind(&completion_is_output_value);
  BuildIncrementBlockCoverageCounterIfEnabled(expr,
                                              SourceRangeKind::kContinuation);
  builder()->LoadAccumulatorWithRegister(output_value);
}

void BytecodeGenerator::BuildAwait(int position) {
  // Rather than HandlerTable::UNCAUGHT, async functions use
  // HandlerTable::ASYNC_AWAIT to communicate that top-level exceptions are
  // transformed into promise rejections. This is necessary to prevent emitting
  // multiple debug events for the same uncaught exception. There is no point
  // in the body of an async function where catch prediction is
  // HandlerTable::UNCAUGHT.
  DCHECK(catch_prediction() != HandlerTable::UNCAUGHT ||
         info()->scope()->is_repl_mode_scope());

  {
    // Await(operand) and suspend.
    RegisterAllocationScope register_scope(this);

    Runtime::FunctionId await_intrinsic_id;
    if (IsAsyncGeneratorFunction(function_kind())) {
      await_intrinsic_id = catch_prediction() == HandlerTable::ASYNC_AWAIT
                               ? Runtime::kInlineAsyncGeneratorAwaitUncaught
                               : Runtime::kInlineAsyncGeneratorAwaitCaught;
    } else {
      await_intrinsic_id = catch_prediction() == HandlerTable::ASYNC_AWAIT
                               ? Runtime::kInlineAsyncFunctionAwaitUncaught
                               : Runtime::kInlineAsyncFunctionAwaitCaught;
    }
    RegisterList args = register_allocator()->NewRegisterList(2);
    builder()
        ->MoveRegister(generator_object(), args[0])
        .StoreAccumulatorInRegister(args[1])
        .CallRuntime(await_intrinsic_id, args);
  }

  BuildSuspendPoint(position);

  Register input = register_allocator()->NewRegister();
  Register resume_mode = register_allocator()->NewRegister();

  // Now dispatch on resume mode.
  BytecodeLabel resume_next;
  builder()
      ->StoreAccumulatorInRegister(input)
      .CallRuntime(Runtime::kInlineGeneratorGetResumeMode, generator_object())
      .StoreAccumulatorInRegister(resume_mode)
      .LoadLiteral(Smi::FromInt(JSGeneratorObject::kNext))
      .CompareReference(resume_mode)
      .JumpIfTrue(ToBooleanMode::kAlreadyBoolean, &resume_next);

  // Resume with "throw" completion (rethrow the received value).
  // TODO(leszeks): Add a debug-only check that the accumulator is
  // JSGeneratorObject::kThrow.
  builder()->LoadAccumulatorWithRegister(input).ReThrow();

  // Resume with next.
  builder()->Bind(&resume_next);
  builder()->LoadAccumulatorWithRegister(input);
}

void BytecodeGenerator::VisitAwait(Await* expr) {
  builder()->SetExpressionPosition(expr);
  VisitForAccumulatorValue(expr->expression());
  BuildAwait(expr->position());
  BuildIncrementBlockCoverageCounterIfEnabled(expr,
                                              SourceRangeKind::kContinuation);
}

void BytecodeGenerator::VisitThrow(Throw* expr) {
  AllocateBlockCoverageSlotIfEnabled(expr, SourceRangeKind::kContinuation);
  VisitForAccumulatorValue(expr->exception());
  builder()->SetExpressionPosition(expr);
  builder()->Throw();
}

void BytecodeGenerator::VisitPropertyLoad(Register obj, Property* property) {
  if (property->is_optional_chain_link()) {
    DCHECK_NOT_NULL(optional_chaining_null_labels_);
    int right_range =
        AllocateBlockCoverageSlotIfEnabled(property, SourceRangeKind::kRight);
    builder()->LoadAccumulatorWithRegister(obj).JumpIfUndefinedOrNull(
        optional_chaining_null_labels_->New());
    BuildIncrementBlockCoverageCounterIfEnabled(right_range);
  }

  AssignType property_kind = Property::GetAssignType(property);

  switch (property_kind) {
    case NON_PROPERTY:
      UNREACHABLE();
    case NAMED_PROPERTY: {
      builder()->SetExpressionPosition(property);
      const AstRawString* name =
          property->key()->AsLiteral()->AsRawPropertyName();
      BuildLoadNamedProperty(property->obj(), obj, name);
      break;
    }
    case KEYED_PROPERTY: {
      VisitForAccumulatorValue(property->key());
      builder()->SetExpressionPosition(property);
      builder()->LoadKeyedProperty(
          obj, feedback_index(feedback_spec()->AddKeyedLoadICSlot()));
      break;
    }
    case NAMED_SUPER_PROPERTY:
      VisitNamedSuperPropertyLoad(property, Register::invalid_value());
      break;
    case KEYED_SUPER_PROPERTY:
      VisitKeyedSuperPropertyLoad(property, Register::invalid_value());
      break;
    case PRIVATE_SETTER_ONLY: {
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateGetterAccess,
                                 property);
      break;
    }
    case PRIVATE_GETTER_ONLY:
    case PRIVATE_GETTER_AND_SETTER: {
      Register key = VisitForRegisterValue(property->key());
      BuildPrivateBrandCheck(property, obj,
                             MessageTemplate::kInvalidPrivateMemberRead);
      BuildPrivateGetterAccess(obj, key);
      break;
    }
    case PRIVATE_METHOD: {
      BuildPrivateBrandCheck(property, obj,
                             MessageTemplate::kInvalidPrivateMemberRead);
      // In the case of private methods, property->key() is the function to be
      // loaded (stored in a context slot), so load this directly.
      VisitForAccumulatorValue(property->key());
      break;
    }
  }
}

void BytecodeGenerator::BuildPrivateGetterAccess(Register object,
                                                 Register accessor_pair) {
  RegisterAllocationScope scope(this);
  Register accessor = register_allocator()->NewRegister();
  RegisterList args = register_allocator()->NewRegisterList(1);

  builder()
      ->CallRuntime(Runtime::kLoadPrivateGetter, accessor_pair)
      .StoreAccumulatorInRegister(accessor)
      .MoveRegister(object, args[0])
      .CallProperty(accessor, args,
                    feedback_index(feedback_spec()->AddCallICSlot()));
}

void BytecodeGenerator::BuildPrivateSetterAccess(Register object,
                                                 Register accessor_pair,
                                                 Register value) {
  RegisterAllocationScope scope(this);
  Register accessor = register_allocator()->NewRegister();
  RegisterList args = register_allocator()->NewRegisterList(2);

  builder()
      ->CallRuntime(Runtime::kLoadPrivateSetter, accessor_pair)
      .StoreAccumulatorInRegister(accessor)
      .MoveRegister(object, args[0])
      .MoveRegister(value, args[1])
      .CallProperty(accessor, args,
                    feedback_index(feedback_spec()->AddCallICSlot()));
}

void BytecodeGenerator::BuildPrivateMethodIn(Variable* private_name,
                                             Expression* object_expression) {
  DCHECK(IsPrivateMethodOrAccessorVariableMode(private_name->mode()));
  ClassScope* scope = private_name->scope()->AsClassScope();
  if (private_name->is_static()) {
    // For static private methods, "#privatemethod in ..." only returns true for
    // the class constructor.
    if (scope->class_variable() == nullptr) {
      // Can only happen via the debugger. See comment in
      // BuildPrivateBrandCheck.
      RegisterAllocationScope register_scope(this);
      RegisterList args = register_allocator()->NewRegisterList(2);
      builder()
          ->LoadLiteral(Smi::FromEnum(
              MessageTemplate::
                  kInvalidUnusedPrivateStaticMethodAccessedByDebugger))
          .StoreAccumulatorInRegister(args[0])
          .LoadLiteral(private_name->raw_name())
          .StoreAccumulatorInRegister(args[1])
          .CallRuntime(Runtime::kNewError, args)
          .Throw();
    } else {
      VisitForAccumulatorValue(object_expression);
      Register object = register_allocator()->NewRegister();
      builder()->StoreAccumulatorInRegister(object);

      BytecodeLabel is_object;
      builder()->JumpIfJSReceiver(&is_object);

      RegisterList args = register_allocator()->NewRegisterList(3);
      builder()
          ->StoreAccumulatorInRegister(args[2])
          .LoadLiteral(Smi::FromEnum(MessageTemplate::kInvalidInOperatorUse))
          .StoreAccumulatorInRegister(args[0])
          .LoadLiteral(private_name->raw_name())
          .StoreAccumulatorInRegister(args[1])
          .CallRuntime(Runtime::kNewTypeError, args)
          .Throw();

      builder()->Bind(&is_object);
      BuildVariableLoadForAccumulatorValue(scope->class_variable(),
                                           HoleCheckMode::kElided);
      builder()->CompareReference(object);
    }
  } else {
    BuildVariableLoadForAccumulatorValue(scope->brand(),
                                         HoleCheckMode::kElided);
    Register brand = register_allocator()->NewRegister();
    builder()->StoreAccumulatorInRegister(brand);

    VisitForAccumulatorValue(object_expression);
    builder()->SetExpressionPosition(object_expression);

    FeedbackSlot slot = feedback_spec()->AddKeyedHasICSlot();
    builder()->CompareOperation(Token::IN, brand, feedback_index(slot));
    execution_result()->SetResultIsBoolean();
  }
}

void BytecodeGenerator::BuildPrivateBrandCheck(Property* property,
                                               Register object,
                                               MessageTemplate tmpl) {
  Variable* private_name = property->key()->AsVariableProxy()->var();
  DCHECK(IsPrivateMethodOrAccessorVariableMode(private_name->mode()));
  ClassScope* scope = private_name->scope()->AsClassScope();
  if (private_name->is_static()) {
    // For static private methods, the only valid receiver is the class.
    // Load the class constructor.
    if (scope->class_variable() == nullptr) {
      // If the static private method has not been used used in source
      // code (either explicitly or through the presence of eval), but is
      // accessed by the debugger at runtime, reference to the class variable
      // is not available since it was not be context-allocated. Therefore we
      // can't build a branch check, and throw an ReferenceError as if the
      // method was optimized away.
      // TODO(joyee): get a reference to the class constructor through
      // something other than scope->class_variable() in this scenario.
      RegisterAllocationScope register_scope(this);
      RegisterList args = register_allocator()->NewRegisterList(2);
      builder()
          ->LoadLiteral(Smi::FromEnum(
              MessageTemplate::
                  kInvalidUnusedPrivateStaticMethodAccessedByDebugger))
          .StoreAccumulatorInRegister(args[0])
          .LoadLiteral(private_name->raw_name())
          .StoreAccumulatorInRegister(args[1])
          .CallRuntime(Runtime::kNewError, args)
          .Throw();
    } else {
      BuildVariableLoadForAccumulatorValue(scope->class_variable(),
                                           HoleCheckMode::kElided);
      BytecodeLabel return_check;
      builder()->CompareReference(object).JumpIfTrue(
          ToBooleanMode::kAlreadyBoolean, &return_check);
      BuildInvalidPropertyAccess(tmpl, property);
      builder()->Bind(&return_check);
    }
  } else {
    BuildVariableLoadForAccumulatorValue(scope->brand(),
                                         HoleCheckMode::kElided);
    builder()->SetExpressionPosition(property);
    builder()->LoadKeyedProperty(
        object, feedback_index(feedback_spec()->AddKeyedLoadICSlot()));
  }
}

void BytecodeGenerator::VisitPropertyLoadForRegister(Register obj,
                                                     Property* expr,
                                                     Register destination) {
  ValueResultScope result_scope(this);
  VisitPropertyLoad(obj, expr);
  builder()->StoreAccumulatorInRegister(destination);
}

void BytecodeGenerator::VisitNamedSuperPropertyLoad(Property* property,
                                                    Register opt_receiver_out) {
  RegisterAllocationScope register_scope(this);
  if (FLAG_super_ic) {
    Register receiver = register_allocator()->NewRegister();
    BuildThisVariableLoad();
    builder()->StoreAccumulatorInRegister(receiver);
    BuildVariableLoad(
        property->obj()->AsSuperPropertyReference()->home_object()->var(),
        HoleCheckMode::kElided);
    builder()->SetExpressionPosition(property);
    auto name = property->key()->AsLiteral()->AsRawPropertyName();
    FeedbackSlot slot = GetCachedLoadSuperICSlot(name);
    builder()->LoadNamedPropertyFromSuper(receiver, name, feedback_index(slot));
    if (opt_receiver_out.is_valid()) {
      builder()->MoveRegister(receiver, opt_receiver_out);
    }
  } else {
    RegisterList args = register_allocator()->NewRegisterList(3);
    BuildThisVariableLoad();
    builder()->StoreAccumulatorInRegister(args[0]);
    BuildVariableLoad(
        property->obj()->AsSuperPropertyReference()->home_object()->var(),
        HoleCheckMode::kElided);
    builder()->StoreAccumulatorInRegister(args[1]);
    builder()->SetExpressionPosition(property);
    builder()
        ->LoadLiteral(property->key()->AsLiteral()->AsRawPropertyName())
        .StoreAccumulatorInRegister(args[2])
        .CallRuntime(Runtime::kLoadFromSuper, args);

    if (opt_receiver_out.is_valid()) {
      builder()->MoveRegister(args[0], opt_receiver_out);
    }
  }
}

void BytecodeGenerator::VisitKeyedSuperPropertyLoad(Property* property,
                                                    Register opt_receiver_out) {
  RegisterAllocationScope register_scope(this);
  RegisterList args = register_allocator()->NewRegisterList(3);
  BuildThisVariableLoad();
  builder()->StoreAccumulatorInRegister(args[0]);
  BuildVariableLoad(
      property->obj()->AsSuperPropertyReference()->home_object()->var(),
      HoleCheckMode::kElided);
  builder()->StoreAccumulatorInRegister(args[1]);
  VisitForRegisterValue(property->key(), args[2]);

  builder()->SetExpressionPosition(property);
  builder()->CallRuntime(Runtime::kLoadKeyedFromSuper, args);

  if (opt_receiver_out.is_valid()) {
    builder()->MoveRegister(args[0], opt_receiver_out);
  }
}

template <typename ExpressionFunc>
void BytecodeGenerator::BuildOptionalChain(ExpressionFunc expression_func) {
  BytecodeLabel done;
  OptionalChainNullLabelScope label_scope(this);
  expression_func();
  builder()->Jump(&done);
  label_scope.labels()->Bind(builder());
  builder()->LoadUndefined();
  builder()->Bind(&done);
}

void BytecodeGenerator::VisitOptionalChain(OptionalChain* expr) {
  BuildOptionalChain([&]() { VisitForAccumulatorValue(expr->expression()); });
}

void BytecodeGenerator::VisitProperty(Property* expr) {
  AssignType property_kind = Property::GetAssignType(expr);
  if (property_kind != NAMED_SUPER_PROPERTY &&
      property_kind != KEYED_SUPER_PROPERTY) {
    Register obj = VisitForRegisterValue(expr->obj());
    VisitPropertyLoad(obj, expr);
  } else {
    VisitPropertyLoad(Register::invalid_value(), expr);
  }
}

void BytecodeGenerator::VisitArguments(const ZonePtrList<Expression>* args,
                                       RegisterList* arg_regs) {
  // Visit arguments.
  for (int i = 0; i < static_cast<int>(args->length()); i++) {
    VisitAndPushIntoRegisterList(args->at(i), arg_regs);
  }
}

void BytecodeGenerator::VisitCall(Call* expr) {
  Expression* callee_expr = expr->expression();
  Call::CallType call_type = expr->GetCallType();

  if (call_type == Call::SUPER_CALL) {
    return VisitCallSuper(expr);
  }

  // We compile the call differently depending on the presence of spreads and
  // their positions.
  //
  // If there is only one spread and it is the final argument, there is a
  // special CallWithSpread bytecode.
  //
  // If there is a non-final spread, we rewrite calls like
  //     callee(1, ...x, 2)
  // to
  //     %reflect_apply(callee, receiver, [1, ...x, 2])
  const Call::SpreadPosition spread_position = expr->spread_position();

  // Grow the args list as we visit receiver / arguments to avoid allocating all
  // the registers up-front. Otherwise these registers are unavailable during
  // receiver / argument visiting and we can end up with memory leaks due to
  // registers keeping objects alive.
  RegisterList args = register_allocator()->NewGrowableRegisterList();

  // The callee is the first register in args for ease of calling %reflect_apply
  // if we have a non-final spread. For all other cases it is popped from args
  // before emitting the call below.
  Register callee = register_allocator()->GrowRegisterList(&args);

  bool implicit_undefined_receiver = false;
  bool optimize_as_one_shot = ShouldOptimizeAsOneShot();

  // TODO(petermarshall): We have a lot of call bytecodes that are very similar,
  // see if we can reduce the number by adding a separate argument which
  // specifies the call type (e.g., property, spread, tailcall, etc.).

  // Prepare the callee and the receiver to the function call. This depends on
  // the semantics of the underlying call type.
  switch (call_type) {
    case Call::NAMED_PROPERTY_CALL:
    case Call::KEYED_PROPERTY_CALL:
    case Call::PRIVATE_CALL: {
      Property* property = callee_expr->AsProperty();
      VisitAndPushIntoRegisterList(property->obj(), &args);
      VisitPropertyLoadForRegister(args.last_register(), property, callee);
      break;
    }
    case Call::GLOBAL_CALL: {
      // Receiver is undefined for global calls.
      if (spread_position == Call::kNoSpread && !optimize_as_one_shot) {
        implicit_undefined_receiver = true;
      } else {
        // TODO(leszeks): There's no special bytecode for tail calls or spread
        // calls with an undefined receiver, so just push undefined ourselves.
        BuildPushUndefinedIntoRegisterList(&args);
      }
      // Load callee as a global variable.
      VariableProxy* proxy = callee_expr->AsVariableProxy();
      BuildVariableLoadForAccumulatorValue(proxy->var(),
                                           proxy->hole_check_mode());
      builder()->StoreAccumulatorInRegister(callee);
      break;
    }
    case Call::WITH_CALL: {
      Register receiver = register_allocator()->GrowRegisterList(&args);
      DCHECK(callee_expr->AsVariableProxy()->var()->IsLookupSlot());
      {
        RegisterAllocationScope inner_register_scope(this);
        Register name = register_allocator()->NewRegister();

        // Call %LoadLookupSlotForCall to get the callee and receiver.
        RegisterList result_pair = register_allocator()->NewRegisterList(2);
        Variable* variable = callee_expr->AsVariableProxy()->var();
        builder()
            ->LoadLiteral(variable->raw_name())
            .StoreAccumulatorInRegister(name)
            .CallRuntimeForPair(Runtime::kLoadLookupSlotForCall, name,
                                result_pair)
            .MoveRegister(result_pair[0], callee)
            .MoveRegister(result_pair[1], receiver);
      }
      break;
    }
    case Call::OTHER_CALL: {
      // Receiver is undefined for other calls.
      if (spread_position == Call::kNoSpread && !optimize_as_one_shot) {
        implicit_undefined_receiver = true;
      } else {
        // TODO(leszeks): There's no special bytecode for tail calls or spread
        // calls with an undefined receiver, so just push undefined ourselves.
        BuildPushUndefinedIntoRegisterList(&args);
      }
      VisitForRegisterValue(callee_expr, callee);
      break;
    }
    case Call::NAMED_SUPER_PROPERTY_CALL: {
      Register receiver = register_allocator()->GrowRegisterList(&args);
      Property* property = callee_expr->AsProperty();
      VisitNamedSuperPropertyLoad(property, receiver);
      builder()->StoreAccumulatorInRegister(callee);
      break;
    }
    case Call::KEYED_SUPER_PROPERTY_CALL: {
      Register receiver = register_allocator()->GrowRegisterList(&args);
      Property* property = callee_expr->AsProperty();
      VisitKeyedSuperPropertyLoad(property, receiver);
      builder()->StoreAccumulatorInRegister(callee);
      break;
    }
    case Call::NAMED_OPTIONAL_CHAIN_PROPERTY_CALL:
    case Call::KEYED_OPTIONAL_CHAIN_PROPERTY_CALL:
    case Call::PRIVATE_OPTIONAL_CHAIN_CALL: {
      OptionalChain* chain = callee_expr->AsOptionalChain();
      Property* property = chain->expression()->AsProperty();
      BuildOptionalChain([&]() {
        VisitAndPushIntoRegisterList(property->obj(), &args);
        VisitPropertyLoad(args.last_register(), property);
      });
      builder()->StoreAccumulatorInRegister(callee);
      break;
    }
    case Call::SUPER_CALL:
      UNREACHABLE();
  }

  if (expr->is_optional_chain_link()) {
    DCHECK_NOT_NULL(optional_chaining_null_labels_);
    int right_range =
        AllocateBlockCoverageSlotIfEnabled(expr, SourceRangeKind::kRight);
    builder()->LoadAccumulatorWithRegister(callee).JumpIfUndefinedOrNull(
        optional_chaining_null_labels_->New());
    BuildIncrementBlockCoverageCounterIfEnabled(right_range);
  }

  int receiver_arg_count = -1;
  if (spread_position == Call::kHasNonFinalSpread) {
    // If we're building %reflect_apply, build the array literal and put it in
    // the 3rd argument.
    DCHECK(!implicit_undefined_receiver);
    DCHECK_EQ(args.register_count(), 2);
    BuildCreateArrayLiteral(expr->arguments(), nullptr);
    builder()->StoreAccumulatorInRegister(
        register_allocator()->GrowRegisterList(&args));
  } else {
    // If we're not building %reflect_apply and don't need to build an array
    // literal, pop the callee and evaluate all arguments to the function call
    // and store in sequential args registers.
    args = args.PopLeft();
    VisitArguments(expr->arguments(), &args);
    receiver_arg_count = implicit_undefined_receiver ? 0 : 1;
    CHECK_EQ(receiver_arg_count + expr->arguments()->length(),
             args.register_count());
  }

  // Resolve callee for a potential direct eval call. This block will mutate the
  // callee value.
  if (expr->is_possibly_eval() && expr->arguments()->length() > 0) {
    RegisterAllocationScope inner_register_scope(this);
    RegisterList runtime_call_args = register_allocator()->NewRegisterList(6);
    // Set up arguments for ResolvePossiblyDirectEval by copying callee, source
    // strings and function closure, and loading language and
    // position.

    // Move the first arg.
    if (spread_position == Call::kHasNonFinalSpread) {
      int feedback_slot_index =
          feedback_index(feedback_spec()->AddKeyedLoadICSlot());
      Register args_array = args[2];
      builder()
          ->LoadLiteral(Smi::FromInt(0))
          .LoadKeyedProperty(args_array, feedback_slot_index)
          .StoreAccumulatorInRegister(runtime_call_args[1]);
    } else {
      // FIXME(v8:5690): Support final spreads for eval.
      DCHECK_GE(receiver_arg_count, 0);
      builder()->MoveRegister(args[receiver_arg_count], runtime_call_args[1]);
    }
    builder()
        ->MoveRegister(callee, runtime_call_args[0])
        .MoveRegister(Register::function_closure(), runtime_call_args[2])
        .LoadLiteral(Smi::FromEnum(language_mode()))
        .StoreAccumulatorInRegister(runtime_call_args[3])
        .LoadLiteral(Smi::FromInt(current_scope()->start_position()))
        .StoreAccumulatorInRegister(runtime_call_args[4])
        .LoadLiteral(Smi::FromInt(expr->position()))
        .StoreAccumulatorInRegister(runtime_call_args[5]);

    // Call ResolvePossiblyDirectEval and modify the callee.
    builder()
        ->CallRuntime(Runtime::kResolvePossiblyDirectEval, runtime_call_args)
        .StoreAccumulatorInRegister(callee);
  }

  builder()->SetExpressionPosition(expr);

  if (spread_position == Call::kHasFinalSpread) {
    DCHECK(!implicit_undefined_receiver);
    builder()->CallWithSpread(callee, args,
                              feedback_index(feedback_spec()->AddCallICSlot()));
  } else if (spread_position == Call::kHasNonFinalSpread) {
    builder()->CallJSRuntime(Context::REFLECT_APPLY_INDEX, args);
  } else if (optimize_as_one_shot) {
    DCHECK(!implicit_undefined_receiver);
    builder()->CallNoFeedback(callee, args);
  } else if (call_type == Call::NAMED_PROPERTY_CALL ||
             call_type == Call::KEYED_PROPERTY_CALL) {
    DCHECK(!implicit_undefined_receiver);
    builder()->CallProperty(callee, args,
                            feedback_index(feedback_spec()->AddCallICSlot()));
  } else if (implicit_undefined_receiver) {
    builder()->CallUndefinedReceiver(
        callee, args, feedback_index(feedback_spec()->AddCallICSlot()));
  } else {
    builder()->CallAnyReceiver(
        callee, args, feedback_index(feedback_spec()->AddCallICSlot()));
  }
}

void BytecodeGenerator::VisitCallSuper(Call* expr) {
  RegisterAllocationScope register_scope(this);
  SuperCallReference* super = expr->expression()->AsSuperCallReference();
  const ZonePtrList<Expression>* args = expr->arguments();

  // We compile the super call differently depending on the presence of spreads
  // and their positions.
  //
  // If there is only one spread and it is the final argument, there is a
  // special ConstructWithSpread bytecode.
  //
  // It there is a non-final spread, we rewrite something like
  //    super(1, ...x, 2)
  // to
  //    %reflect_construct(constructor, [1, ...x, 2], new_target)
  //
  // That is, we implement (non-last-arg) spreads in super calls via our
  // mechanism for spreads in array literals.
  const Call::SpreadPosition spread_position = expr->spread_position();

  // Prepare the constructor to the super call.
  Register this_function = VisitForRegisterValue(super->this_function_var());
  Register constructor = register_allocator()->NewRegister();
  builder()
      ->LoadAccumulatorWithRegister(this_function)
      .GetSuperConstructor(constructor);

  if (spread_position == Call::kHasNonFinalSpread) {
    // First generate the array containing all arguments.
    BuildCreateArrayLiteral(args, nullptr);

    // Check if the constructor is in fact a constructor.
    builder()->ThrowIfNotSuperConstructor(constructor);

    // Now pass that array to %reflect_construct.
    RegisterList construct_args = register_allocator()->NewRegisterList(3);
    builder()->StoreAccumulatorInRegister(construct_args[1]);
    builder()->MoveRegister(constructor, construct_args[0]);
    VisitForRegisterValue(super->new_target_var(), construct_args[2]);
    builder()->CallJSRuntime(Context::REFLECT_CONSTRUCT_INDEX, construct_args);
  } else {
    RegisterList args_regs = register_allocator()->NewGrowableRegisterList();
    VisitArguments(args, &args_regs);

    // Check if the constructor is in fact a constructor.
    builder()->ThrowIfNotSuperConstructor(constructor);

    // The new target is loaded into the accumulator from the
    // {new.target} variable.
    VisitForAccumulatorValue(super->new_target_var());
    builder()->SetExpressionPosition(expr);

    int feedback_slot_index = feedback_index(feedback_spec()->AddCallICSlot());

    if (spread_position == Call::kHasFinalSpread) {
      builder()->ConstructWithSpread(constructor, args_regs,
                                     feedback_slot_index);
    } else {
      DCHECK_EQ(spread_position, Call::kNoSpread);
      // Call construct.
      // TODO(turbofan): For now we do gather feedback on super constructor
      // calls, utilizing the existing machinery to inline the actual call
      // target and the JSCreate for the implicit receiver allocation. This
      // is not an ideal solution for super constructor calls, but it gets
      // the job done for now. In the long run we might want to revisit this
      // and come up with a better way.
      builder()->Construct(constructor, args_regs, feedback_slot_index);
    }
  }

  // Explicit calls to the super constructor using super() perform an
  // implicit binding assignment to the 'this' variable.
  //
  // Default constructors don't need have to do the assignment because
  // 'this' isn't accessed in default constructors.
  if (!IsDefaultConstructor(info()->literal()->kind())) {
    Variable* var = closure_scope()->GetReceiverScope()->receiver();
    BuildVariableAssignment(var, Token::INIT, HoleCheckMode::kRequired);
  }

  Register instance = register_allocator()->NewRegister();
  builder()->StoreAccumulatorInRegister(instance);

  if (info()->literal()->class_scope_has_private_brand()) {
    BuildPrivateBrandInitialization(instance);
  }

  // The derived constructor has the correct bit set always, so we
  // don't emit code to load and call the initializer if not
  // required.
  //
  // For the arrow function or eval case, we always emit code to load
  // and call the initializer.
  //
  // TODO(gsathya): In the future, we could tag nested arrow functions
  // or eval with the correct bit so that we do the load conditionally
  // if required.
  if (info()->literal()->requires_instance_members_initializer() ||
      !IsDerivedConstructor(info()->literal()->kind())) {
    BuildInstanceMemberInitialization(this_function, instance);
  }

  builder()->LoadAccumulatorWithRegister(instance);
}

void BytecodeGenerator::VisitCallNew(CallNew* expr) {
  RegisterList args = register_allocator()->NewGrowableRegisterList();

  // Load the constructor. It's in the first register in args for ease of
  // calling %reflect_construct if we have a non-final spread. For all other
  // cases it is popped before emitting the construct below.
  VisitAndPushIntoRegisterList(expr->expression(), &args);

  // We compile the new differently depending on the presence of spreads and
  // their positions.
  //
  // If there is only one spread and it is the final argument, there is a
  // special ConstructWithSpread bytecode.
  //
  // If there is a non-final spread, we rewrite calls like
  //     new ctor(1, ...x, 2)
  // to
  //     %reflect_construct(ctor, [1, ...x, 2])
  const CallNew::SpreadPosition spread_position = expr->spread_position();

  if (spread_position == CallNew::kHasNonFinalSpread) {
    BuildCreateArrayLiteral(expr->arguments(), nullptr);
    builder()->SetExpressionPosition(expr);
    builder()
        ->StoreAccumulatorInRegister(
            register_allocator()->GrowRegisterList(&args))
        .CallJSRuntime(Context::REFLECT_CONSTRUCT_INDEX, args);
    return;
  }

  Register constructor = args.first_register();
  args = args.PopLeft();
  VisitArguments(expr->arguments(), &args);

  // The accumulator holds new target which is the same as the
  // constructor for CallNew.
  builder()->SetExpressionPosition(expr);
  builder()->LoadAccumulatorWithRegister(constructor);

  int feedback_slot_index = feedback_index(feedback_spec()->AddCallICSlot());
  if (spread_position == CallNew::kHasFinalSpread) {
    builder()->ConstructWithSpread(constructor, args, feedback_slot_index);
  } else {
    DCHECK_EQ(spread_position, CallNew::kNoSpread);
    builder()->Construct(constructor, args, feedback_slot_index);
  }
}

void BytecodeGenerator::VisitCallRuntime(CallRuntime* expr) {
  if (expr->is_jsruntime()) {
    RegisterList args = register_allocator()->NewGrowableRegisterList();
    VisitArguments(expr->arguments(), &args);
    builder()->CallJSRuntime(expr->context_index(), args);
  } else {
    // Evaluate all arguments to the runtime call.
    RegisterList args = register_allocator()->NewGrowableRegisterList();
    VisitArguments(expr->arguments(), &args);
    Runtime::FunctionId function_id = expr->function()->function_id;
    builder()->CallRuntime(function_id, args);
  }
}

void BytecodeGenerator::VisitVoid(UnaryOperation* expr) {
  VisitForEffect(expr->expression());
  builder()->LoadUndefined();
}

void BytecodeGenerator::VisitForTypeOfValue(Expression* expr) {
  if (expr->IsVariableProxy()) {
    // Typeof does not throw a reference error on global variables, hence we
    // perform a non-contextual load in case the operand is a variable proxy.
    VariableProxy* proxy = expr->AsVariableProxy();
    BuildVariableLoadForAccumulatorValue(proxy->var(), proxy->hole_check_mode(),
                                         TypeofMode::kInside);
  } else {
    VisitForAccumulatorValue(expr);
  }
}

void BytecodeGenerator::VisitTypeOf(UnaryOperation* expr) {
  VisitForTypeOfValue(expr->expression());
  builder()->TypeOf();
}

void BytecodeGenerator::VisitNot(UnaryOperation* expr) {
  if (execution_result()->IsEffect()) {
    VisitForEffect(expr->expression());
  } else if (execution_result()->IsTest()) {
    // No actual logical negation happening, we just swap the control flow, by
    // swapping the target labels and the fallthrough branch, and visit in the
    // same test result context.
    TestResultScope* test_result = execution_result()->AsTest();
    test_result->InvertControlFlow();
    VisitInSameTestExecutionScope(expr->expression());
  } else {
    TypeHint type_hint = VisitForAccumulatorValue(expr->expression());
    builder()->LogicalNot(ToBooleanModeFromTypeHint(type_hint));
    // Always returns a boolean value.
    execution_result()->SetResultIsBoolean();
  }
}

void BytecodeGenerator::VisitUnaryOperation(UnaryOperation* expr) {
  switch (expr->op()) {
    case Token::Value::NOT:
      VisitNot(expr);
      break;
    case Token::Value::TYPEOF:
      VisitTypeOf(expr);
      break;
    case Token::Value::VOID:
      VisitVoid(expr);
      break;
    case Token::Value::DELETE:
      VisitDelete(expr);
      break;
    case Token::Value::ADD:
    case Token::Value::SUB:
    case Token::Value::BIT_NOT:
      VisitForAccumulatorValue(expr->expression());
      builder()->SetExpressionPosition(expr);
      builder()->UnaryOperation(
          expr->op(), feedback_index(feedback_spec()->AddBinaryOpICSlot()));
      break;
    default:
      UNREACHABLE();
  }
}

void BytecodeGenerator::VisitDelete(UnaryOperation* unary) {
  Expression* expr = unary->expression();
  if (expr->IsProperty()) {
    // Delete of an object property is allowed both in sloppy
    // and strict modes.
    Property* property = expr->AsProperty();
    DCHECK(!property->IsPrivateReference());
    Register object = VisitForRegisterValue(property->obj());
    VisitForAccumulatorValue(property->key());
    builder()->Delete(object, language_mode());
  } else if (expr->IsOptionalChain()) {
    Expression* expr_inner = expr->AsOptionalChain()->expression();
    if (expr_inner->IsProperty()) {
      Property* property = expr_inner->AsProperty();
      DCHECK(!property->IsPrivateReference());
      BytecodeLabel done;
      OptionalChainNullLabelScope label_scope(this);
      VisitForAccumulatorValue(property->obj());
      if (property->is_optional_chain_link()) {
        int right_range = AllocateBlockCoverageSlotIfEnabled(
            property, SourceRangeKind::kRight);
        builder()->JumpIfUndefinedOrNull(label_scope.labels()->New());
        BuildIncrementBlockCoverageCounterIfEnabled(right_range);
      }
      Register object = register_allocator()->NewRegister();
      builder()->StoreAccumulatorInRegister(object);
      VisitForAccumulatorValue(property->key());
      builder()->Delete(object, language_mode());
      builder()->Jump(&done);
      label_scope.labels()->Bind(builder());
      builder()->LoadTrue();
      builder()->Bind(&done);
    } else {
      VisitForEffect(expr);
      builder()->LoadTrue();
    }
  } else if (expr->IsVariableProxy() &&
             !expr->AsVariableProxy()->is_new_target()) {
    // Delete of an unqualified identifier is allowed in sloppy mode but is
    // not allowed in strict mode.
    DCHECK(is_sloppy(language_mode()));
    Variable* variable = expr->AsVariableProxy()->var();
    switch (variable->location()) {
      case VariableLocation::PARAMETER:
      case VariableLocation::LOCAL:
      case VariableLocation::CONTEXT:
      case VariableLocation::REPL_GLOBAL: {
        // Deleting local var/let/const, context variables, and arguments
        // does not have any effect.
        builder()->LoadFalse();
        break;
      }
      case VariableLocation::UNALLOCATED:
      // TODO(adamk): Falling through to the runtime results in correct
      // behavior, but does unnecessary context-walking (since scope
      // analysis has already proven that the variable doesn't exist in
      // any non-global scope). Consider adding a DeleteGlobal bytecode
      // that knows how to deal with ScriptContexts as well as global
      // object properties.
      case VariableLocation::LOOKUP: {
        Register name_reg = register_allocator()->NewRegister();
        builder()
            ->LoadLiteral(variable->raw_name())
            .StoreAccumulatorInRegister(name_reg)
            .CallRuntime(Runtime::kDeleteLookupSlot, name_reg);
        break;
      }
      case VariableLocation::MODULE:
        // Modules are always in strict mode and unqualified identifers are not
        // allowed in strict mode.
        UNREACHABLE();
    }
  } else {
    // Delete of an unresolvable reference, new.target, and this returns true.
    VisitForEffect(expr);
    builder()->LoadTrue();
  }
}

void BytecodeGenerator::VisitCountOperation(CountOperation* expr) {
  DCHECK(expr->expression()->IsValidReferenceExpression());

  // Left-hand side can only be a property, a global or a variable slot.
  Property* property = expr->expression()->AsProperty();
  AssignType assign_type = Property::GetAssignType(property);

  bool is_postfix = expr->is_postfix() && !execution_result()->IsEffect();

  // Evaluate LHS expression and get old value.
  Register object, key, old_value;
  RegisterList super_property_args;
  const AstRawString* name;
  switch (assign_type) {
    case NON_PROPERTY: {
      VariableProxy* proxy = expr->expression()->AsVariableProxy();
      BuildVariableLoadForAccumulatorValue(proxy->var(),
                                           proxy->hole_check_mode());
      break;
    }
    case NAMED_PROPERTY: {
      object = VisitForRegisterValue(property->obj());
      name = property->key()->AsLiteral()->AsRawPropertyName();
      builder()->LoadNamedProperty(
          object, name,
          feedback_index(GetCachedLoadICSlot(property->obj(), name)));
      break;
    }
    case KEYED_PROPERTY: {
      object = VisitForRegisterValue(property->obj());
      // Use visit for accumulator here since we need the key in the accumulator
      // for the LoadKeyedProperty.
      key = register_allocator()->NewRegister();
      VisitForAccumulatorValue(property->key());
      builder()->StoreAccumulatorInRegister(key).LoadKeyedProperty(
          object, feedback_index(feedback_spec()->AddKeyedLoadICSlot()));
      break;
    }
    case NAMED_SUPER_PROPERTY: {
      super_property_args = register_allocator()->NewRegisterList(4);
      RegisterList load_super_args = super_property_args.Truncate(3);
      BuildThisVariableLoad();
      builder()->StoreAccumulatorInRegister(load_super_args[0]);
      BuildVariableLoad(
          property->obj()->AsSuperPropertyReference()->home_object()->var(),
          HoleCheckMode::kElided);
      builder()->StoreAccumulatorInRegister(load_super_args[1]);
      builder()
          ->LoadLiteral(property->key()->AsLiteral()->AsRawPropertyName())
          .StoreAccumulatorInRegister(load_super_args[2])
          .CallRuntime(Runtime::kLoadFromSuper, load_super_args);
      break;
    }
    case KEYED_SUPER_PROPERTY: {
      super_property_args = register_allocator()->NewRegisterList(4);
      RegisterList load_super_args = super_property_args.Truncate(3);
      BuildThisVariableLoad();
      builder()->StoreAccumulatorInRegister(load_super_args[0]);
      BuildVariableLoad(
          property->obj()->AsSuperPropertyReference()->home_object()->var(),
          HoleCheckMode::kElided);
      builder()->StoreAccumulatorInRegister(load_super_args[1]);
      VisitForRegisterValue(property->key(), load_super_args[2]);
      builder()->CallRuntime(Runtime::kLoadKeyedFromSuper, load_super_args);
      break;
    }
    case PRIVATE_METHOD: {
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateMethodWrite,
                                 property);
      return;
    }
    case PRIVATE_GETTER_ONLY: {
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateSetterAccess,
                                 property);
      return;
    }
    case PRIVATE_SETTER_ONLY: {
      BuildInvalidPropertyAccess(MessageTemplate::kInvalidPrivateGetterAccess,
                                 property);
      return;
    }
    case PRIVATE_GETTER_AND_SETTER: {
      object = VisitForRegisterValue(property->obj());
      key = VisitForRegisterValue(property->key());
      BuildPrivateBrandCheck(property, object,
                             MessageTemplate::kInvalidPrivateMemberRead);
      BuildPrivateGetterAccess(object, key);
      break;
    }
  }

  // Save result for postfix expressions.
  FeedbackSlot count_slot = feedback_spec()->AddBinaryOpICSlot();
  if (is_postfix) {
    old_value = register_allocator()->NewRegister();
    // Convert old value into a number before saving it.
    // TODO(ignition): Think about adding proper PostInc/PostDec bytecodes
    // instead of this ToNumeric + Inc/Dec dance.
    builder()
        ->ToNumeric(feedback_index(count_slot))
        .StoreAccumulatorInRegister(old_value);
  }

  // Perform +1/-1 operation.
  builder()->UnaryOperation(expr->op(), feedback_index(count_slot));

  // Store the value.
  builder()->SetExpressionPosition(expr);
  switch (assign_type) {
    case NON_PROPERTY: {
      VariableProxy* proxy = expr->expression()->AsVariableProxy();
      BuildVariableAssignment(proxy->var(), expr->op(),
                              proxy->hole_check_mode());
      break;
    }
    case NAMED_PROPERTY: {
      FeedbackSlot slot = GetCachedStoreICSlot(property->obj(), name);
      Register value;
      if (!execution_result()->IsEffect()) {
        value = register_allocator()->NewRegister();
        builder()->StoreAccumulatorInRegister(value);
      }
      builder()->StoreNamedProperty(object, name, feedback_index(slot),
                                    language_mode());
      if (!execution_result()->IsEffect()) {
        builder()->LoadAccumulatorWithRegister(value);
      }
      break;
    }
    case KEYED_PROPERTY: {
      FeedbackSlot slot = feedback_spec()->AddKeyedStoreICSlot(language_mode());
      Register value;
      if (!execution_result()->IsEffect()) {
        value = register_allocator()->NewRegister();
        builder()->StoreAccumulatorInRegister(value);
      }
      builder()->StoreKeyedProperty(object, key, feedback_index(slot),
                                    language_mode());
      if (!execution_result()->IsEffect()) {
        builder()->LoadAccumulatorWithRegister(value);
      }
      break;
    }
    case NAMED_SUPER_PROPERTY: {
      builder()
          ->StoreAccumulatorInRegister(super_property_args[3])
          .CallRuntime(Runtime::kStoreToSuper, super_property_args);
      break;
    }
    case KEYED_SUPER_PROPERTY: {
      builder()
          ->StoreAccumulatorInRegister(super_property_args[3])
          .CallRuntime(Runtime::kStoreKeyedToSuper, super_property_args);
      break;
    }
    case PRIVATE_SETTER_ONLY:
    case PRIVATE_GETTER_ONLY:
    case PRIVATE_METHOD: {
      UNREACHABLE();
    }
    case PRIVATE_GETTER_AND_SETTER: {
      Register value = register_allocator()->NewRegister();
      builder()->StoreAccumulatorInRegister(value);
      BuildPrivateSetterAccess(object, key, value);
      if (!execution_result()->IsEffect()) {
        builder()->LoadAccumulatorWithRegister(value);
      }
      break;
    }
  }

  // Restore old value for postfix expressions.
  if (is_postfix) {
    builder()->LoadAccumulatorWithRegister(old_value);
  }
}

void BytecodeGenerator::VisitBinaryOperation(BinaryOperation* binop) {
  switch (binop->op()) {
    case Token::COMMA:
      VisitCommaExpression(binop);
      break;
    case Token::OR:
      VisitLogicalOrExpression(binop);
      break;
    case Token::AND:
      VisitLogicalAndExpression(binop);
      break;
    case Token::NULLISH:
      VisitNullishExpression(binop);
      break;
    default:
      VisitArithmeticExpression(binop);
      break;
  }
}

void BytecodeGenerator::VisitNaryOperation(NaryOperation* expr) {
  switch (expr->op()) {
    case Token::COMMA:
      VisitNaryCommaExpression(expr);
      break;
    case Token::OR:
      VisitNaryLogicalOrExpression(expr);
      break;
    case Token::AND:
      VisitNaryLogicalAndExpression(expr);
      break;
    case Token::NULLISH:
      VisitNaryNullishExpression(expr);
      break;
    default:
      VisitNaryArithmeticExpression(expr);
      break;
  }
}

void BytecodeGenerator::BuildLiteralCompareNil(
    Token::Value op, BytecodeArrayBuilder::NilValue nil) {
  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    switch (test_result->fallthrough()) {
      case TestFallthrough::kThen:
        builder()->JumpIfNotNil(test_result->NewElseLabel(), op, nil);
        break;
      case TestFallthrough::kElse:
        builder()->JumpIfNil(test_result->NewThenLabel(), op, nil);
        break;
      case TestFallthrough::kNone:
        builder()
            ->JumpIfNil(test_result->NewThenLabel(), op, nil)
            .Jump(test_result->NewElseLabel());
    }
    test_result->SetResultConsumedByTest();
  } else {
    builder()->CompareNil(op, nil);
  }
}

void BytecodeGenerator::VisitCompareOperation(CompareOperation* expr) {
  Expression* sub_expr;
  Literal* literal;
  if (expr->IsLiteralCompareTypeof(&sub_expr, &literal)) {
    // Emit a fast literal comparion for expressions of the form:
    // typeof(x) === 'string'.
    VisitForTypeOfValue(sub_expr);
    builder()->SetExpressionPosition(expr);
    TestTypeOfFlags::LiteralFlag literal_flag =
        TestTypeOfFlags::GetFlagForLiteral(ast_string_constants(), literal);
    if (literal_flag == TestTypeOfFlags::LiteralFlag::kOther) {
      builder()->LoadFalse();
    } else {
      builder()->CompareTypeOf(literal_flag);
    }
  } else if (expr->IsLiteralCompareUndefined(&sub_expr)) {
    VisitForAccumulatorValue(sub_expr);
    builder()->SetExpressionPosition(expr);
    BuildLiteralCompareNil(expr->op(), BytecodeArrayBuilder::kUndefinedValue);
  } else if (expr->IsLiteralCompareNull(&sub_expr)) {
    VisitForAccumulatorValue(sub_expr);
    builder()->SetExpressionPosition(expr);
    BuildLiteralCompareNil(expr->op(), BytecodeArrayBuilder::kNullValue);
  } else {
    if (expr->op() == Token::IN && expr->left()->IsPrivateName()) {
      DCHECK(FLAG_harmony_private_brand_checks);
      Variable* var = expr->left()->AsVariableProxy()->var();
      if (IsPrivateMethodOrAccessorVariableMode(var->mode())) {
        BuildPrivateMethodIn(var, expr->right());
        return;
      }
      // For private fields, the code below does the right thing.
    }

    Register lhs = VisitForRegisterValue(expr->left());
    VisitForAccumulatorValue(expr->right());
    builder()->SetExpressionPosition(expr);
    FeedbackSlot slot;
    if (expr->op() == Token::IN) {
      slot = feedback_spec()->AddKeyedHasICSlot();
    } else if (expr->op() == Token::INSTANCEOF) {
      slot = feedback_spec()->AddInstanceOfSlot();
    } else {
      slot = feedback_spec()->AddCompareICSlot();
    }
    builder()->CompareOperation(expr->op(), lhs, feedback_index(slot));
  }
  // Always returns a boolean value.
  execution_result()->SetResultIsBoolean();
}

void BytecodeGenerator::VisitArithmeticExpression(BinaryOperation* expr) {
  FeedbackSlot slot = feedback_spec()->AddBinaryOpICSlot();
  Expression* subexpr;
  Smi literal;
  if (expr->IsSmiLiteralOperation(&subexpr, &literal)) {
    TypeHint type_hint = VisitForAccumulatorValue(subexpr);
    builder()->SetExpressionPosition(expr);
    builder()->BinaryOperationSmiLiteral(expr->op(), literal,
                                         feedback_index(slot));
    if (expr->op() == Token::ADD && type_hint == TypeHint::kString) {
      execution_result()->SetResultIsString();
    }
  } else {
    TypeHint lhs_type = VisitForAccumulatorValue(expr->left());
    Register lhs = register_allocator()->NewRegister();
    builder()->StoreAccumulatorInRegister(lhs);
    TypeHint rhs_type = VisitForAccumulatorValue(expr->right());
    if (expr->op() == Token::ADD &&
        (lhs_type == TypeHint::kString || rhs_type == TypeHint::kString)) {
      execution_result()->SetResultIsString();
    }

    builder()->SetExpressionPosition(expr);
    builder()->BinaryOperation(expr->op(), lhs, feedback_index(slot));
  }
}

void BytecodeGenerator::VisitNaryArithmeticExpression(NaryOperation* expr) {
  // TODO(leszeks): Add support for lhs smi in commutative ops.
  TypeHint type_hint = VisitForAccumulatorValue(expr->first());

  for (size_t i = 0; i < expr->subsequent_length(); ++i) {
    RegisterAllocationScope register_scope(this);
    if (expr->subsequent(i)->IsSmiLiteral()) {
      builder()->SetExpressionPosition(expr->subsequent_op_position(i));
      builder()->BinaryOperationSmiLiteral(
          expr->op(), expr->subsequent(i)->AsLiteral()->AsSmiLiteral(),
          feedback_index(feedback_spec()->AddBinaryOpICSlot()));
    } else {
      Register lhs = register_allocator()->NewRegister();
      builder()->StoreAccumulatorInRegister(lhs);
      TypeHint rhs_hint = VisitForAccumulatorValue(expr->subsequent(i));
      if (rhs_hint == TypeHint::kString) type_hint = TypeHint::kString;
      builder()->SetExpressionPosition(expr->subsequent_op_position(i));
      builder()->BinaryOperation(
          expr->op(), lhs,
          feedback_index(feedback_spec()->AddBinaryOpICSlot()));
    }
  }

  if (type_hint == TypeHint::kString && expr->op() == Token::ADD) {
    // If any operand of an ADD is a String, a String is produced.
    execution_result()->SetResultIsString();
  }
}

// Note: the actual spreading is performed by the surrounding expression's
// visitor.
void BytecodeGenerator::VisitSpread(Spread* expr) { Visit(expr->expression()); }

void BytecodeGenerator::VisitEmptyParentheses(EmptyParentheses* expr) {
  UNREACHABLE();
}

void BytecodeGenerator::VisitImportCallExpression(ImportCallExpression* expr) {
  const int register_count = expr->import_assertions() ? 3 : 2;
  RegisterList args = register_allocator()->NewRegisterList(register_count);
  VisitForRegisterValue(expr->specifier(), args[1]);
  if (expr->import_assertions()) {
    VisitForRegisterValue(expr->import_assertions(), args[2]);
  }
  builder()
      ->MoveRegister(Register::function_closure(), args[0])
      .CallRuntime(Runtime::kDynamicImportCall, args);
}

void BytecodeGenerator::BuildGetIterator(IteratorType hint) {
  if (hint == IteratorType::kAsync) {
    RegisterAllocationScope scope(this);

    Register obj = register_allocator()->NewRegister();
    Register method = register_allocator()->NewRegister();

    // Set method to GetMethod(obj, @@asyncIterator)
    builder()->StoreAccumulatorInRegister(obj).LoadAsyncIteratorProperty(
        obj, feedback_index(feedback_spec()->AddLoadICSlot()));

    BytecodeLabel async_iterator_undefined, done;
    builder()->JumpIfUndefinedOrNull(&async_iterator_undefined);

    // Let iterator be Call(method, obj)
    builder()->StoreAccumulatorInRegister(method).CallProperty(
        method, RegisterList(obj),
        feedback_index(feedback_spec()->AddCallICSlot()));

    // If Type(iterator) is not Object, throw a TypeError exception.
    builder()->JumpIfJSReceiver(&done);
    builder()->CallRuntime(Runtime::kThrowSymbolAsyncIteratorInvalid);

    builder()->Bind(&async_iterator_undefined);
    // If method is undefined,
    //     Let syncMethod be GetMethod(obj, @@iterator)
    builder()
        ->LoadIteratorProperty(obj,
                               feedback_index(feedback_spec()->AddLoadICSlot()))
        .StoreAccumulatorInRegister(method);

    //     Let syncIterator be Call(syncMethod, obj)
    builder()->CallProperty(method, RegisterList(obj),
                            feedback_index(feedback_spec()->AddCallICSlot()));

    // Return CreateAsyncFromSyncIterator(syncIterator)
    // alias `method` register as it's no longer used
    Register sync_iter = method;
    builder()->StoreAccumulatorInRegister(sync_iter).CallRuntime(
        Runtime::kInlineCreateAsyncFromSyncIterator, sync_iter);

    builder()->Bind(&done);
  } else {
    {
      RegisterAllocationScope scope(this);

      Register obj = register_allocator()->NewRegister();
      int load_feedback_index =
          feedback_index(feedback_spec()->AddLoadICSlot());
      int call_feedback_index =
          feedback_index(feedback_spec()->AddCallICSlot());

      // Let method be GetMethod(obj, @@iterator) and
      // iterator be Call(method, obj).
      builder()->StoreAccumulatorInRegister(obj).GetIterator(
          obj, load_feedback_index, call_feedback_index);
    }

    // If Type(iterator) is not Object, throw a TypeError exception.
    BytecodeLabel no_type_error;
    builder()->JumpIfJSReceiver(&no_type_error);
    builder()->CallRuntime(Runtime::kThrowSymbolIteratorInvalid);
    builder()->Bind(&no_type_error);
  }
}

// Returns an IteratorRecord which is valid for the lifetime of the current
// register_allocation_scope.
BytecodeGenerator::IteratorRecord BytecodeGenerator::BuildGetIteratorRecord(
    Register next, Register object, IteratorType hint) {
  DCHECK(next.is_valid() && object.is_valid());
  BuildGetIterator(hint);

  builder()
      ->StoreAccumulatorInRegister(object)
      .LoadNamedProperty(object, ast_string_constants()->next_string(),
                         feedback_index(feedback_spec()->AddLoadICSlot()))
      .StoreAccumulatorInRegister(next);
  return IteratorRecord(object, next, hint);
}

BytecodeGenerator::IteratorRecord BytecodeGenerator::BuildGetIteratorRecord(
    IteratorType hint) {
  Register next = register_allocator()->NewRegister();
  Register object = register_allocator()->NewRegister();
  return BuildGetIteratorRecord(next, object, hint);
}

void BytecodeGenerator::BuildIteratorNext(const IteratorRecord& iterator,
                                          Register next_result) {
  DCHECK(next_result.is_valid());
  builder()->CallProperty(iterator.next(), RegisterList(iterator.object()),
                          feedback_index(feedback_spec()->AddCallICSlot()));

  if (iterator.type() == IteratorType::kAsync) {
    BuildAwait();
  }

  BytecodeLabel is_object;
  builder()
      ->StoreAccumulatorInRegister(next_result)
      .JumpIfJSReceiver(&is_object)
      .CallRuntime(Runtime::kThrowIteratorResultNotAnObject, next_result)
      .Bind(&is_object);
}

void BytecodeGenerator::BuildCallIteratorMethod(Register iterator,
                                                const AstRawString* method_name,
                                                RegisterList receiver_and_args,
                                                BytecodeLabel* if_called,
                                                BytecodeLabels* if_notcalled) {
  RegisterAllocationScope register_scope(this);

  Register method = register_allocator()->NewRegister();
  FeedbackSlot slot = feedback_spec()->AddLoadICSlot();
  builder()
      ->LoadNamedProperty(iterator, method_name, feedback_index(slot))
      .JumpIfUndefinedOrNull(if_notcalled->New())
      .StoreAccumulatorInRegister(method)
      .CallProperty(method, receiver_and_args,
                    feedback_index(feedback_spec()->AddCallICSlot()))
      .Jump(if_called);
}

void BytecodeGenerator::BuildIteratorClose(const IteratorRecord& iterator,
                                           Expression* expr) {
  RegisterAllocationScope register_scope(this);
  BytecodeLabels done(zone());
  BytecodeLabel if_called;
  RegisterList args = RegisterList(iterator.object());
  BuildCallIteratorMethod(iterator.object(),
                          ast_string_constants()->return_string(), args,
                          &if_called, &done);
  builder()->Bind(&if_called);

  if (iterator.type() == IteratorType::kAsync) {
    DCHECK_NOT_NULL(expr);
    BuildAwait(expr->position());
  }

  builder()->JumpIfJSReceiver(done.New());
  {
    RegisterAllocationScope register_scope(this);
    Register return_result = register_allocator()->NewRegister();
    builder()
        ->StoreAccumulatorInRegister(return_result)
        .CallRuntime(Runtime::kThrowIteratorResultNotAnObject, return_result);
  }

  done.Bind(builder());
}

void BytecodeGenerator::VisitGetTemplateObject(GetTemplateObject* expr) {
  builder()->SetExpressionPosition(expr);
  size_t entry = builder()->AllocateDeferredConstantPoolEntry();
  template_objects_.push_back(std::make_pair(expr, entry));
  FeedbackSlot literal_slot = feedback_spec()->AddLiteralSlot();
  builder()->GetTemplateObject(entry, feedback_index(literal_slot));
}

void BytecodeGenerator::VisitTemplateLiteral(TemplateLiteral* expr) {
  const ZonePtrList<const AstRawString>& parts = *expr->string_parts();
  const ZonePtrList<Expression>& substitutions = *expr->substitutions();
  // Template strings with no substitutions are turned into StringLiterals.
  DCHECK_GT(substitutions.length(), 0);
  DCHECK_EQ(parts.length(), substitutions.length() + 1);

  // Generate string concatenation
  // TODO(caitp): Don't generate feedback slot if it's not used --- introduce
  // a simple, concise, reusable mechanism to lazily create reusable slots.
  FeedbackSlot slot = feedback_spec()->AddBinaryOpICSlot();
  Register last_part = register_allocator()->NewRegister();
  bool last_part_valid = false;

  builder()->SetExpressionPosition(expr);
  for (int i = 0; i < substitutions.length(); ++i) {
    if (i != 0) {
      builder()->StoreAccumulatorInRegister(last_part);
      last_part_valid = true;
    }

    if (!parts[i]->IsEmpty()) {
      builder()->LoadLiteral(parts[i]);
      if (last_part_valid) {
        builder()->BinaryOperation(Token::ADD, last_part, feedback_index(slot));
      }
      builder()->StoreAccumulatorInRegister(last_part);
      last_part_valid = true;
    }

    TypeHint type_hint = VisitForAccumulatorValue(substitutions[i]);
    if (type_hint != TypeHint::kString) {
      builder()->ToString();
    }
    if (last_part_valid) {
      builder()->BinaryOperation(Token::ADD, last_part, feedback_index(slot));
    }
    last_part_valid = false;
  }

  if (!parts.last()->IsEmpty()) {
    builder()->StoreAccumulatorInRegister(last_part);
    builder()->LoadLiteral(parts.last());
    builder()->BinaryOperation(Token::ADD, last_part, feedback_index(slot));
  }
}

void BytecodeGenerator::BuildThisVariableLoad() {
  DeclarationScope* receiver_scope = closure_scope()->GetReceiverScope();
  Variable* var = receiver_scope->receiver();
  // TODO(littledan): implement 'this' hole check elimination.
  HoleCheckMode hole_check_mode =
      IsDerivedConstructor(receiver_scope->function_kind())
          ? HoleCheckMode::kRequired
          : HoleCheckMode::kElided;
  BuildVariableLoad(var, hole_check_mode);
}

void BytecodeGenerator::VisitThisExpression(ThisExpression* expr) {
  BuildThisVariableLoad();
}

void BytecodeGenerator::VisitSuperCallReference(SuperCallReference* expr) {
  // Handled by VisitCall().
  UNREACHABLE();
}

void BytecodeGenerator::VisitSuperPropertyReference(
    SuperPropertyReference* expr) {
  builder()->CallRuntime(Runtime::kThrowUnsupportedSuperError);
}

void BytecodeGenerator::VisitCommaExpression(BinaryOperation* binop) {
  VisitForEffect(binop->left());
  Visit(binop->right());
}

void BytecodeGenerator::VisitNaryCommaExpression(NaryOperation* expr) {
  DCHECK_GT(expr->subsequent_length(), 0);

  VisitForEffect(expr->first());
  for (size_t i = 0; i < expr->subsequent_length() - 1; ++i) {
    VisitForEffect(expr->subsequent(i));
  }
  Visit(expr->subsequent(expr->subsequent_length() - 1));
}

void BytecodeGenerator::VisitLogicalTestSubExpression(
    Token::Value token, Expression* expr, BytecodeLabels* then_labels,
    BytecodeLabels* else_labels, int coverage_slot) {
  DCHECK(token == Token::OR || token == Token::AND || token == Token::NULLISH);

  BytecodeLabels test_next(zone());
  if (token == Token::OR) {
    VisitForTest(expr, then_labels, &test_next, TestFallthrough::kElse);
  } else if (token == Token::AND) {
    VisitForTest(expr, &test_next, else_labels, TestFallthrough::kThen);
  } else {
    DCHECK_EQ(Token::NULLISH, token);
    VisitForNullishTest(expr, then_labels, &test_next, else_labels);
  }
  test_next.Bind(builder());

  BuildIncrementBlockCoverageCounterIfEnabled(coverage_slot);
}

void BytecodeGenerator::VisitLogicalTest(Token::Value token, Expression* left,
                                         Expression* right,
                                         int right_coverage_slot) {
  DCHECK(token == Token::OR || token == Token::AND || token == Token::NULLISH);
  TestResultScope* test_result = execution_result()->AsTest();
  BytecodeLabels* then_labels = test_result->then_labels();
  BytecodeLabels* else_labels = test_result->else_labels();
  TestFallthrough fallthrough = test_result->fallthrough();

  VisitLogicalTestSubExpression(token, left, then_labels, else_labels,
                                right_coverage_slot);
  // The last test has the same then, else and fallthrough as the parent test.
  VisitForTest(right, then_labels, else_labels, fallthrough);
}

void BytecodeGenerator::VisitNaryLogicalTest(
    Token::Value token, NaryOperation* expr,
    const NaryCodeCoverageSlots* coverage_slots) {
  DCHECK(token == Token::OR || token == Token::AND || token == Token::NULLISH);
  DCHECK_GT(expr->subsequent_length(), 0);

  TestResultScope* test_result = execution_result()->AsTest();
  BytecodeLabels* then_labels = test_result->then_labels();
  BytecodeLabels* else_labels = test_result->else_labels();
  TestFallthrough fallthrough = test_result->fallthrough();

  VisitLogicalTestSubExpression(token, expr->first(), then_labels, else_labels,
                                coverage_slots->GetSlotFor(0));
  for (size_t i = 0; i < expr->subsequent_length() - 1; ++i) {
    VisitLogicalTestSubExpression(token, expr->subsequent(i), then_labels,
                                  else_labels,
                                  coverage_slots->GetSlotFor(i + 1));
  }
  // The last test has the same then, else and fallthrough as the parent test.
  VisitForTest(expr->subsequent(expr->subsequent_length() - 1), then_labels,
               else_labels, fallthrough);
}

bool BytecodeGenerator::VisitLogicalOrSubExpression(Expression* expr,
                                                    BytecodeLabels* end_labels,
                                                    int coverage_slot) {
  if (expr->ToBooleanIsTrue()) {
    VisitForAccumulatorValue(expr);
    end_labels->Bind(builder());
    return true;
  } else if (!expr->ToBooleanIsFalse()) {
    TypeHint type_hint = VisitForAccumulatorValue(expr);
    builder()->JumpIfTrue(ToBooleanModeFromTypeHint(type_hint),
                          end_labels->New());
  }

  BuildIncrementBlockCoverageCounterIfEnabled(coverage_slot);

  return false;
}

bool BytecodeGenerator::VisitLogicalAndSubExpression(Expression* expr,
                                                     BytecodeLabels* end_labels,
                                                     int coverage_slot) {
  if (expr->ToBooleanIsFalse()) {
    VisitForAccumulatorValue(expr);
    end_labels->Bind(builder());
    return true;
  } else if (!expr->ToBooleanIsTrue()) {
    TypeHint type_hint = VisitForAccumulatorValue(expr);
    builder()->JumpIfFalse(ToBooleanModeFromTypeHint(type_hint),
                           end_labels->New());
  }

  BuildIncrementBlockCoverageCounterIfEnabled(coverage_slot);

  return false;
}

bool BytecodeGenerator::VisitNullishSubExpression(Expression* expr,
                                                  BytecodeLabels* end_labels,
                                                  int coverage_slot) {
  if (expr->IsLiteralButNotNullOrUndefined()) {
    VisitForAccumulatorValue(expr);
    end_labels->Bind(builder());
    return true;
  } else if (!expr->IsNullOrUndefinedLiteral()) {
    VisitForAccumulatorValue(expr);
    BytecodeLabel is_null_or_undefined;
    builder()
        ->JumpIfUndefinedOrNull(&is_null_or_undefined)
        .Jump(end_labels->New());
    builder()->Bind(&is_null_or_undefined);
  }

  BuildIncrementBlockCoverageCounterIfEnabled(coverage_slot);

  return false;
}

void BytecodeGenerator::VisitLogicalOrExpression(BinaryOperation* binop) {
  Expression* left = binop->left();
  Expression* right = binop->right();

  int right_coverage_slot =
      AllocateBlockCoverageSlotIfEnabled(binop, SourceRangeKind::kRight);

  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    if (left->ToBooleanIsTrue()) {
      builder()->Jump(test_result->NewThenLabel());
    } else if (left->ToBooleanIsFalse() && right->ToBooleanIsFalse()) {
      BuildIncrementBlockCoverageCounterIfEnabled(right_coverage_slot);
      builder()->Jump(test_result->NewElseLabel());
    } else {
      VisitLogicalTest(Token::OR, left, right, right_coverage_slot);
    }
    test_result->SetResultConsumedByTest();
  } else {
    BytecodeLabels end_labels(zone());
    if (VisitLogicalOrSubExpression(left, &end_labels, right_coverage_slot)) {
      return;
    }
    VisitForAccumulatorValue(right);
    end_labels.Bind(builder());
  }
}

void BytecodeGenerator::VisitNaryLogicalOrExpression(NaryOperation* expr) {
  Expression* first = expr->first();
  DCHECK_GT(expr->subsequent_length(), 0);

  NaryCodeCoverageSlots coverage_slots(this, expr);

  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    if (first->ToBooleanIsTrue()) {
      builder()->Jump(test_result->NewThenLabel());
    } else {
      VisitNaryLogicalTest(Token::OR, expr, &coverage_slots);
    }
    test_result->SetResultConsumedByTest();
  } else {
    BytecodeLabels end_labels(zone());
    if (VisitLogicalOrSubExpression(first, &end_labels,
                                    coverage_slots.GetSlotFor(0))) {
      return;
    }
    for (size_t i = 0; i < expr->subsequent_length() - 1; ++i) {
      if (VisitLogicalOrSubExpression(expr->subsequent(i), &end_labels,
                                      coverage_slots.GetSlotFor(i + 1))) {
        return;
      }
    }
    // We have to visit the last value even if it's true, because we need its
    // actual value.
    VisitForAccumulatorValue(expr->subsequent(expr->subsequent_length() - 1));
    end_labels.Bind(builder());
  }
}

void BytecodeGenerator::VisitLogicalAndExpression(BinaryOperation* binop) {
  Expression* left = binop->left();
  Expression* right = binop->right();

  int right_coverage_slot =
      AllocateBlockCoverageSlotIfEnabled(binop, SourceRangeKind::kRight);

  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    if (left->ToBooleanIsFalse()) {
      builder()->Jump(test_result->NewElseLabel());
    } else if (left->ToBooleanIsTrue() && right->ToBooleanIsTrue()) {
      BuildIncrementBlockCoverageCounterIfEnabled(right_coverage_slot);
      builder()->Jump(test_result->NewThenLabel());
    } else {
      VisitLogicalTest(Token::AND, left, right, right_coverage_slot);
    }
    test_result->SetResultConsumedByTest();
  } else {
    BytecodeLabels end_labels(zone());
    if (VisitLogicalAndSubExpression(left, &end_labels, right_coverage_slot)) {
      return;
    }
    VisitForAccumulatorValue(right);
    end_labels.Bind(builder());
  }
}

void BytecodeGenerator::VisitNaryLogicalAndExpression(NaryOperation* expr) {
  Expression* first = expr->first();
  DCHECK_GT(expr->subsequent_length(), 0);

  NaryCodeCoverageSlots coverage_slots(this, expr);

  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    if (first->ToBooleanIsFalse()) {
      builder()->Jump(test_result->NewElseLabel());
    } else {
      VisitNaryLogicalTest(Token::AND, expr, &coverage_slots);
    }
    test_result->SetResultConsumedByTest();
  } else {
    BytecodeLabels end_labels(zone());
    if (VisitLogicalAndSubExpression(first, &end_labels,
                                     coverage_slots.GetSlotFor(0))) {
      return;
    }
    for (size_t i = 0; i < expr->subsequent_length() - 1; ++i) {
      if (VisitLogicalAndSubExpression(expr->subsequent(i), &end_labels,
                                       coverage_slots.GetSlotFor(i + 1))) {
        return;
      }
    }
    // We have to visit the last value even if it's false, because we need its
    // actual value.
    VisitForAccumulatorValue(expr->subsequent(expr->subsequent_length() - 1));
    end_labels.Bind(builder());
  }
}

void BytecodeGenerator::VisitNullishExpression(BinaryOperation* binop) {
  Expression* left = binop->left();
  Expression* right = binop->right();

  int right_coverage_slot =
      AllocateBlockCoverageSlotIfEnabled(binop, SourceRangeKind::kRight);

  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    if (left->IsLiteralButNotNullOrUndefined() && left->ToBooleanIsTrue()) {
      builder()->Jump(test_result->NewThenLabel());
    } else if (left->IsNullOrUndefinedLiteral() &&
               right->IsNullOrUndefinedLiteral()) {
      BuildIncrementBlockCoverageCounterIfEnabled(right_coverage_slot);
      builder()->Jump(test_result->NewElseLabel());
    } else {
      VisitLogicalTest(Token::NULLISH, left, right, right_coverage_slot);
    }
    test_result->SetResultConsumedByTest();
  } else {
    BytecodeLabels end_labels(zone());
    if (VisitNullishSubExpression(left, &end_labels, right_coverage_slot)) {
      return;
    }
    VisitForAccumulatorValue(right);
    end_labels.Bind(builder());
  }
}

void BytecodeGenerator::VisitNaryNullishExpression(NaryOperation* expr) {
  Expression* first = expr->first();
  DCHECK_GT(expr->subsequent_length(), 0);

  NaryCodeCoverageSlots coverage_slots(this, expr);

  if (execution_result()->IsTest()) {
    TestResultScope* test_result = execution_result()->AsTest();
    if (first->IsLiteralButNotNullOrUndefined() && first->ToBooleanIsTrue()) {
      builder()->Jump(test_result->NewThenLabel());
    } else {
      VisitNaryLogicalTest(Token::NULLISH, expr, &coverage_slots);
    }
    test_result->SetResultConsumedByTest();
  } else {
    BytecodeLabels end_labels(zone());
    if (VisitNullishSubExpression(first, &end_labels,
                                  coverage_slots.GetSlotFor(0))) {
      return;
    }
    for (size_t i = 0; i < expr->subsequent_length() - 1; ++i) {
      if (VisitNullishSubExpression(expr->subsequent(i), &end_labels,
                                    coverage_slots.GetSlotFor(i + 1))) {
        return;
      }
    }
    // We have to visit the last value even if it's nullish, because we need its
    // actual value.
    VisitForAccumulatorValue(expr->subsequent(expr->subsequent_length() - 1));
    end_labels.Bind(builder());
  }
}

void BytecodeGenerator::BuildNewLocalActivationContext() {
  ValueResultScope value_execution_result(this);
  Scope* scope = closure_scope();
  DCHECK_EQ(current_scope(), closure_scope());

  // Create the appropriate context.
  DCHECK(scope->is_function_scope() || scope->is_eval_scope());
  int slot_count = scope->num_heap_slots() - Context::MIN_CONTEXT_SLOTS;
  if (slot_count <= ConstructorBuiltins::MaximumFunctionContextSlots()) {
    switch (scope->scope_type()) {
      case EVAL_SCOPE:
        builder()->CreateEvalContext(scope, slot_count);
        break;
      case FUNCTION_SCOPE:
        builder()->CreateFunctionContext(scope, slot_count);
        break;
      default:
        UNREACHABLE();
    }
  } else {
    Register arg = register_allocator()->NewRegister();
    builder()->LoadLiteral(scope).StoreAccumulatorInRegister(arg).CallRuntime(
        Runtime::kNewFunctionContext, arg);
  }
}

void BytecodeGenerator::BuildLocalActivationContextInitialization() {
  DeclarationScope* scope = closure_scope();

  if (scope->has_this_declaration() && scope->receiver()->IsContextSlot()) {
    Variable* variable = scope->receiver();
    Register receiver(builder()->Receiver());
    // Context variable (at bottom of the context chain).
    DCHECK_EQ(0, scope->ContextChainLength(variable->scope()));
    builder()->LoadAccumulatorWithRegister(receiver).StoreContextSlot(
        execution_context()->reg(), variable->index(), 0);
  }

  // Copy parameters into context if necessary.
  int num_parameters = scope->num_parameters();
  for (int i = 0; i < num_parameters; i++) {
    Variable* variable = scope->parameter(i);
    if (!variable->IsContextSlot()) continue;

    Register parameter(builder()->Parameter(i));
    // Context variable (at bottom of the context chain).
    DCHECK_EQ(0, scope->ContextChainLength(variable->scope()));
    builder()->LoadAccumulatorWithRegister(parameter).StoreContextSlot(
        execution_context()->reg(), variable->index(), 0);
  }
}

void BytecodeGenerator::BuildNewLocalBlockContext(Scope* scope) {
  ValueResultScope value_execution_result(this);
  DCHECK(scope->is_block_scope());

  builder()->CreateBlockContext(scope);
}

void BytecodeGenerator::BuildNewLocalWithContext(Scope* scope) {
  ValueResultScope value_execution_result(this);

  Register extension_object = register_allocator()->NewRegister();

  builder()->ToObject(extension_object);
  builder()->CreateWithContext(extension_object, scope);
}

void BytecodeGenerator::BuildNewLocalCatchContext(Scope* scope) {
  ValueResultScope value_execution_result(this);
  DCHECK(scope->catch_variable()->IsContextSlot());

  Register exception = register_allocator()->NewRegister();
  builder()->StoreAccumulatorInRegister(exception);
  builder()->CreateCatchContext(exception, scope);
}

void BytecodeGenerator::VisitLiteralAccessor(LiteralProperty* property,
                                             Register value_out) {
  if (property == nullptr) {
    builder()->LoadNull().StoreAccumulatorInRegister(value_out);
  } else {
    VisitForRegisterValue(property->value(), value_out);
  }
}

void BytecodeGenerator::VisitArgumentsObject(Variable* variable) {
  if (variable == nullptr) return;

  DCHECK(variable->IsContextSlot() || variable->IsStackAllocated());

  // Allocate and initialize a new arguments object and assign to the
  // {arguments} variable.
  builder()->CreateArguments(closure_scope()->GetArgumentsType());
  BuildVariableAssignment(variable, Token::ASSIGN, HoleCheckMode::kElided);
}

void BytecodeGenerator::VisitRestArgumentsArray(Variable* rest) {
  if (rest == nullptr) return;

  // Allocate and initialize a new rest parameter and assign to the {rest}
  // variable.
  builder()->CreateArguments(CreateArgumentsType::kRestParameter);
  DCHECK(rest->IsContextSlot() || rest->IsStackAllocated());
  BuildVariableAssignment(rest, Token::ASSIGN, HoleCheckMode::kElided);
}

void BytecodeGenerator::VisitThisFunctionVariable(Variable* variable) {
  if (variable == nullptr) return;

  // Store the closure we were called with in the given variable.
  builder()->LoadAccumulatorWithRegister(Register::function_closure());
  BuildVariableAssignment(variable, Token::INIT, HoleCheckMode::kElided);
}

void BytecodeGenerator::VisitNewTargetVariable(Variable* variable) {
  if (variable == nullptr) return;

  // The generator resume trampoline abuses the new.target register
  // to pass in the generator object.  In ordinary calls, new.target is always
  // undefined because generator functions are non-constructible, so don't
  // assign anything to the new.target variable.
  if (IsResumableFunction(info()->literal()->kind())) return;

  if (variable->location() == VariableLocation::LOCAL) {
    // The new.target register was already assigned by entry trampoline.
    DCHECK_EQ(incoming_new_target_or_generator_.index(),
              GetRegisterForLocalVariable(variable).index());
    return;
  }

  // Store the new target we were called with in the given variable.
  builder()->LoadAccumulatorWithRegister(incoming_new_target_or_generator_);
  BuildVariableAssignment(variable, Token::INIT, HoleCheckMode::kElided);
}

void BytecodeGenerator::BuildGeneratorObjectVariableInitialization() {
  DCHECK(IsResumableFunction(info()->literal()->kind()));

  Variable* generator_object_var = closure_scope()->generator_object_var();
  RegisterAllocationScope register_scope(this);
  RegisterList args = register_allocator()->NewRegisterList(2);
  Runtime::FunctionId function_id =
      ((IsAsyncFunction(info()->literal()->kind()) &&
        !IsAsyncGeneratorFunction(info()->literal()->kind())) ||
       IsAsyncModule(info()->literal()->kind()))
          ? Runtime::kInlineAsyncFunctionEnter
          : Runtime::kInlineCreateJSGeneratorObject;
  builder()
      ->MoveRegister(Register::function_closure(), args[0])
      .MoveRegister(builder()->Receiver(), args[1])
      .CallRuntime(function_id, args)
      .StoreAccumulatorInRegister(generator_object());

  if (generator_object_var->location() == VariableLocation::LOCAL) {
    // The generator object register is already set to the variable's local
    // register.
    DCHECK_EQ(generator_object().index(),
              GetRegisterForLocalVariable(generator_object_var).index());
  } else {
    BuildVariableAssignment(generator_object_var, Token::INIT,
                            HoleCheckMode::kElided);
  }
}

void BytecodeGenerator::BuildPushUndefinedIntoRegisterList(
    RegisterList* reg_list) {
  Register reg = register_allocator()->GrowRegisterList(reg_list);
  builder()->LoadUndefined().StoreAccumulatorInRegister(reg);
}

void BytecodeGenerator::BuildLoadPropertyKey(LiteralProperty* property,
                                             Register out_reg) {
  if (property->key()->IsStringLiteral()) {
    builder()
        ->LoadLiteral(property->key()->AsLiteral()->AsRawString())
        .StoreAccumulatorInRegister(out_reg);
  } else {
    VisitForAccumulatorValue(property->key());
    builder()->ToName(out_reg);
  }
}

int BytecodeGenerator::AllocateBlockCoverageSlotIfEnabled(
    AstNode* node, SourceRangeKind kind) {
  return (block_coverage_builder_ == nullptr)
             ? BlockCoverageBuilder::kNoCoverageArraySlot
             : block_coverage_builder_->AllocateBlockCoverageSlot(node, kind);
}

int BytecodeGenerator::AllocateNaryBlockCoverageSlotIfEnabled(
    NaryOperation* node, size_t index) {
  return (block_coverage_builder_ == nullptr)
             ? BlockCoverageBuilder::kNoCoverageArraySlot
             : block_coverage_builder_->AllocateNaryBlockCoverageSlot(node,
                                                                      index);
}

void BytecodeGenerator::BuildIncrementBlockCoverageCounterIfEnabled(
    AstNode* node, SourceRangeKind kind) {
  if (block_coverage_builder_ == nullptr) return;
  block_coverage_builder_->IncrementBlockCounter(node, kind);
}

void BytecodeGenerator::BuildIncrementBlockCoverageCounterIfEnabled(
    int coverage_array_slot) {
  if (block_coverage_builder_ != nullptr) {
    block_coverage_builder_->IncrementBlockCounter(coverage_array_slot);
  }
}

// Visits the expression |expr| and places the result in the accumulator.
BytecodeGenerator::TypeHint BytecodeGenerator::VisitForAccumulatorValue(
    Expression* expr) {
  ValueResultScope accumulator_scope(this);
  Visit(expr);
  return accumulator_scope.type_hint();
}

void BytecodeGenerator::VisitForAccumulatorValueOrTheHole(Expression* expr) {
  if (expr == nullptr) {
    builder()->LoadTheHole();
  } else {
    VisitForAccumulatorValue(expr);
  }
}

// Visits the expression |expr| and discards the result.
void BytecodeGenerator::VisitForEffect(Expression* expr) {
  EffectResultScope effect_scope(this);
  Visit(expr);
}

// Visits the expression |expr| and returns the register containing
// the expression result.
Register BytecodeGenerator::VisitForRegisterValue(Expression* expr) {
  VisitForAccumulatorValue(expr);
  Register result = register_allocator()->NewRegister();
  builder()->StoreAccumulatorInRegister(result);
  return result;
}

// Visits the expression |expr| and stores the expression result in
// |destination|.
void BytecodeGenerator::VisitForRegisterValue(Expression* expr,
                                              Register destination) {
  ValueResultScope register_scope(this);
  Visit(expr);
  builder()->StoreAccumulatorInRegister(destination);
}

// Visits the expression |expr| and pushes the result into a new register
// added to the end of |reg_list|.
void BytecodeGenerator::VisitAndPushIntoRegisterList(Expression* expr,
                                                     RegisterList* reg_list) {
  {
    ValueResultScope register_scope(this);
    Visit(expr);
  }
  // Grow the register list after visiting the expression to avoid reserving
  // the register across the expression evaluation, which could cause memory
  // leaks for deep expressions due to dead objects being kept alive by pointers
  // in registers.
  Register destination = register_allocator()->GrowRegisterList(reg_list);
  builder()->StoreAccumulatorInRegister(destination);
}

void BytecodeGenerator::BuildTest(ToBooleanMode mode,
                                  BytecodeLabels* then_labels,
                                  BytecodeLabels* else_labels,
                                  TestFallthrough fallthrough) {
  switch (fallthrough) {
    case TestFallthrough::kThen:
      builder()->JumpIfFalse(mode, else_labels->New());
      break;
    case TestFallthrough::kElse:
      builder()->JumpIfTrue(mode, then_labels->New());
      break;
    case TestFallthrough::kNone:
      builder()->JumpIfTrue(mode, then_labels->New());
      builder()->Jump(else_labels->New());
      break;
  }
}

// Visits the expression |expr| for testing its boolean value and jumping to the
// |then| or |other| label depending on value and short-circuit semantics
void BytecodeGenerator::VisitForTest(Expression* expr,
                                     BytecodeLabels* then_labels,
                                     BytecodeLabels* else_labels,
                                     TestFallthrough fallthrough) {
  bool result_consumed;
  TypeHint type_hint;
  {
    // To make sure that all temporary registers are returned before generating
    // jumps below, we ensure that the result scope is deleted before doing so.
    // Dead registers might be materialized otherwise.
    TestResultScope test_result(this, then_labels, else_labels, fallthrough);
    Visit(expr);
    result_consumed = test_result.result_consumed_by_test();
    type_hint = test_result.type_hint();
    // Labels and fallthrough might have been mutated, so update based on
    // TestResultScope.
    then_labels = test_result.then_labels();
    else_labels = test_result.else_labels();
    fallthrough = test_result.fallthrough();
  }
  if (!result_consumed) {
    BuildTest(ToBooleanModeFromTypeHint(type_hint), then_labels, else_labels,
              fallthrough);
  }
}

// Visits the expression |expr| for testing its nullish value and jumping to the
// |then| or |other| label depending on value and short-circuit semantics
void BytecodeGenerator::VisitForNullishTest(Expression* expr,
                                            BytecodeLabels* then_labels,
                                            BytecodeLabels* test_next_labels,
                                            BytecodeLabels* else_labels) {
  // Nullish short circuits on undefined or null, otherwise we fall back to
  // BuildTest with no fallthrough.
  // TODO(joshualitt): We should do this in a TestResultScope.
  TypeHint type_hint = VisitForAccumulatorValue(expr);
  ToBooleanMode mode = ToBooleanModeFromTypeHint(type_hint);

  // Skip the nullish shortcircuit if we already have a boolean.
  if (mode != ToBooleanMode::kAlreadyBoolean) {
    builder()->JumpIfUndefinedOrNull(test_next_labels->New());
  }
  BuildTest(mode, then_labels, else_labels, TestFallthrough::kNone);
}

void BytecodeGenerator::VisitInSameTestExecutionScope(Expression* expr) {
  DCHECK(execution_result()->IsTest());
  {
    RegisterAllocationScope reg_scope(this);
    Visit(expr);
  }
  if (!execution_result()->AsTest()->result_consumed_by_test()) {
    TestResultScope* result_scope = execution_result()->AsTest();
    BuildTest(ToBooleanModeFromTypeHint(result_scope->type_hint()),
              result_scope->then_labels(), result_scope->else_labels(),
              result_scope->fallthrough());
    result_scope->SetResultConsumedByTest();
  }
}

void BytecodeGenerator::VisitInScope(Statement* stmt, Scope* scope) {
  DCHECK(scope->declarations()->is_empty());
  CurrentScope current_scope(this, scope);
  ContextScope context_scope(this, scope);
  Visit(stmt);
}

Register BytecodeGenerator::GetRegisterForLocalVariable(Variable* variable) {
  DCHECK_EQ(VariableLocation::LOCAL, variable->location());
  return builder()->Local(variable->index());
}

FunctionKind BytecodeGenerator::function_kind() const {
  return info()->literal()->kind();
}

LanguageMode BytecodeGenerator::language_mode() const {
  return current_scope()->language_mode();
}

Register BytecodeGenerator::generator_object() const {
  DCHECK(IsResumableFunction(info()->literal()->kind()));
  return incoming_new_target_or_generator_;
}

FeedbackVectorSpec* BytecodeGenerator::feedback_spec() {
  return info()->feedback_vector_spec();
}

int BytecodeGenerator::feedback_index(FeedbackSlot slot) const {
  DCHECK(!slot.IsInvalid());
  return FeedbackVector::GetIndex(slot);
}

FeedbackSlot BytecodeGenerator::GetCachedLoadGlobalICSlot(
    TypeofMode typeof_mode, Variable* variable) {
  FeedbackSlotCache::SlotKind slot_kind =
      typeof_mode == TypeofMode::kInside
          ? FeedbackSlotCache::SlotKind::kLoadGlobalInsideTypeof
          : FeedbackSlotCache::SlotKind::kLoadGlobalNotInsideTypeof;
  FeedbackSlot slot(feedback_slot_cache()->Get(slot_kind, variable));
  if (!slot.IsInvalid()) {
    return slot;
  }
  slot = feedback_spec()->AddLoadGlobalICSlot(typeof_mode);
  feedback_slot_cache()->Put(slot_kind, variable, feedback_index(slot));
  return slot;
}

FeedbackSlot BytecodeGenerator::GetCachedStoreGlobalICSlot(
    LanguageMode language_mode, Variable* variable) {
  FeedbackSlotCache::SlotKind slot_kind =
      is_strict(language_mode)
          ? FeedbackSlotCache::SlotKind::kStoreGlobalStrict
          : FeedbackSlotCache::SlotKind::kStoreGlobalSloppy;
  FeedbackSlot slot(feedback_slot_cache()->Get(slot_kind, variable));
  if (!slot.IsInvalid()) {
    return slot;
  }
  slot = feedback_spec()->AddStoreGlobalICSlot(language_mode);
  feedback_slot_cache()->Put(slot_kind, variable, feedback_index(slot));
  return slot;
}

FeedbackSlot BytecodeGenerator::GetCachedLoadICSlot(const Expression* expr,
                                                    const AstRawString* name) {
  DCHECK(!expr->IsSuperPropertyReference());
  if (!FLAG_ignition_share_named_property_feedback) {
    return feedback_spec()->AddLoadICSlot();
  }
  FeedbackSlotCache::SlotKind slot_kind =
      FeedbackSlotCache::SlotKind::kLoadProperty;
  if (!expr->IsVariableProxy()) {
    return feedback_spec()->AddLoadICSlot();
  }
  const VariableProxy* proxy = expr->AsVariableProxy();
  FeedbackSlot slot(
      feedback_slot_cache()->Get(slot_kind, proxy->var()->index(), name));
  if (!slot.IsInvalid()) {
    return slot;
  }
  slot = feedback_spec()->AddLoadICSlot();
  feedback_slot_cache()->Put(slot_kind, proxy->var()->index(), name,
                             feedback_index(slot));
  return slot;
}

FeedbackSlot BytecodeGenerator::GetCachedLoadSuperICSlot(
    const AstRawString* name) {
  if (!FLAG_ignition_share_named_property_feedback) {
    return feedback_spec()->AddLoadICSlot();
  }
  FeedbackSlotCache::SlotKind slot_kind =
      FeedbackSlotCache::SlotKind::kLoadSuperProperty;

  FeedbackSlot slot(feedback_slot_cache()->Get(slot_kind, name));
  if (!slot.IsInvalid()) {
    return slot;
  }
  slot = feedback_spec()->AddLoadICSlot();
  feedback_slot_cache()->Put(slot_kind, name, feedback_index(slot));
  return slot;
}

FeedbackSlot BytecodeGenerator::GetCachedStoreICSlot(const Expression* expr,
                                                     const AstRawString* name) {
  if (!FLAG_ignition_share_named_property_feedback) {
    return feedback_spec()->AddStoreICSlot(language_mode());
  }
  FeedbackSlotCache::SlotKind slot_kind =
      is_strict(language_mode())
          ? FeedbackSlotCache::SlotKind::kStoreNamedStrict
          : FeedbackSlotCache::SlotKind::kStoreNamedSloppy;
  if (!expr->IsVariableProxy()) {
    return feedback_spec()->AddStoreICSlot(language_mode());
  }
  const VariableProxy* proxy = expr->AsVariableProxy();
  FeedbackSlot slot(
      feedback_slot_cache()->Get(slot_kind, proxy->var()->index(), name));
  if (!slot.IsInvalid()) {
    return slot;
  }
  slot = feedback_spec()->AddStoreICSlot(language_mode());
  feedback_slot_cache()->Put(slot_kind, proxy->var()->index(), name,
                             feedback_index(slot));
  return slot;
}

int BytecodeGenerator::GetCachedCreateClosureSlot(FunctionLiteral* literal) {
  FeedbackSlotCache::SlotKind slot_kind =
      FeedbackSlotCache::SlotKind::kClosureFeedbackCell;
  int index = feedback_slot_cache()->Get(slot_kind, literal);
  if (index != -1) {
    return index;
  }
  index = feedback_spec()->AddCreateClosureSlot();
  feedback_slot_cache()->Put(slot_kind, literal, index);
  return index;
}

FeedbackSlot BytecodeGenerator::GetDummyCompareICSlot() {
  return dummy_feedback_slot_.Get();
}

}  // namespace interpreter
}  // namespace internal
}  // namespace v8
