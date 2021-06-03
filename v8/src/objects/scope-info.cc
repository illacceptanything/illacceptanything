// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/objects/scope-info.h"

#include <stdlib.h>

#include "src/ast/scopes.h"
#include "src/ast/variables.h"
#include "src/init/bootstrapper.h"
#include "src/objects/module-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/scope-info-inl.h"
#include "src/objects/string-set-inl.h"
#include "src/roots/roots.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#ifdef DEBUG
bool ScopeInfo::Equals(ScopeInfo other) const {
  if (length() != other.length()) return false;
  for (int index = 0; index < length(); ++index) {
    Object entry = get(index);
    Object other_entry = other.get(index);
    if (entry.IsSmi()) {
      if (entry != other_entry) return false;
    } else {
      if (HeapObject::cast(entry).map().instance_type() !=
          HeapObject::cast(other_entry).map().instance_type()) {
        return false;
      }
      if (entry.IsString()) {
        if (!String::cast(entry).Equals(String::cast(other_entry))) {
          return false;
        }
      } else if (entry.IsScopeInfo()) {
        if (!ScopeInfo::cast(entry).Equals(ScopeInfo::cast(other_entry))) {
          return false;
        }
      } else if (entry.IsSourceTextModuleInfo()) {
        if (!SourceTextModuleInfo::cast(entry).Equals(
                SourceTextModuleInfo::cast(other_entry))) {
          return false;
        }
      } else {
        UNREACHABLE();
      }
    }
  }
  return true;
}
#endif

// static
template <typename IsolateT>
Handle<ScopeInfo> ScopeInfo::Create(IsolateT* isolate, Zone* zone, Scope* scope,
                                    MaybeHandle<ScopeInfo> outer_scope) {
  // Collect variables.
  int context_local_count = 0;
  int module_vars_count = 0;
  // Stack allocated block scope variables are allocated in the parent
  // declaration scope, but are recorded in the block scope's scope info. First
  // slot index indicates at which offset a particular scope starts in the
  // parent declaration scope.
  for (Variable* var : *scope->locals()) {
    switch (var->location()) {
      case VariableLocation::CONTEXT:
      case VariableLocation::REPL_GLOBAL:
        context_local_count++;
        break;
      case VariableLocation::MODULE:
        module_vars_count++;
        break;
      default:
        break;
    }
  }
  // Determine use and location of the "this" binding if it is present.
  VariableAllocationInfo receiver_info;
  if (scope->is_declaration_scope() &&
      scope->AsDeclarationScope()->has_this_declaration()) {
    Variable* var = scope->AsDeclarationScope()->receiver();
    if (!var->is_used()) {
      receiver_info = VariableAllocationInfo::UNUSED;
    } else if (var->IsContextSlot()) {
      receiver_info = VariableAllocationInfo::CONTEXT;
      context_local_count++;
    } else {
      DCHECK(var->IsParameter());
      receiver_info = VariableAllocationInfo::STACK;
    }
  } else {
    receiver_info = VariableAllocationInfo::NONE;
  }

  DCHECK(module_vars_count == 0 || scope->is_module_scope());

  // Make sure we allocate the correct amount.
  DCHECK_EQ(scope->ContextLocalCount(), context_local_count);

  const bool has_new_target =
      scope->is_declaration_scope() &&
      scope->AsDeclarationScope()->new_target_var() != nullptr;
  // TODO(cbruni): Don't always waste a field for the inferred name.
  const bool has_inferred_function_name = scope->is_function_scope();

  // Determine use and location of the function variable if it is present.
  VariableAllocationInfo function_name_info;
  if (scope->is_function_scope()) {
    if (scope->AsDeclarationScope()->function_var() != nullptr) {
      Variable* var = scope->AsDeclarationScope()->function_var();
      if (!var->is_used()) {
        function_name_info = VariableAllocationInfo::UNUSED;
      } else if (var->IsContextSlot()) {
        function_name_info = VariableAllocationInfo::CONTEXT;
      } else {
        DCHECK(var->IsStackLocal());
        function_name_info = VariableAllocationInfo::STACK;
      }
    } else {
      // Always reserve space for the debug name in the scope info.
      function_name_info = VariableAllocationInfo::UNUSED;
    }
  } else if (scope->is_module_scope() || scope->is_script_scope() ||
             scope->is_eval_scope()) {
    // Always reserve space for the debug name in the scope info.
    function_name_info = VariableAllocationInfo::UNUSED;
  } else {
    function_name_info = VariableAllocationInfo::NONE;
  }

  const bool has_brand = scope->is_class_scope()
                             ? scope->AsClassScope()->brand() != nullptr
                             : false;
  const bool should_save_class_variable_index =
      scope->is_class_scope()
          ? scope->AsClassScope()->should_save_class_variable_index()
          : false;
  const bool has_function_name =
      function_name_info != VariableAllocationInfo::NONE;
  const bool has_position_info = NeedsPositionInfo(scope->scope_type());
  const bool has_receiver = receiver_info == VariableAllocationInfo::STACK ||
                            receiver_info == VariableAllocationInfo::CONTEXT;
  const int parameter_count =
      scope->is_declaration_scope()
          ? scope->AsDeclarationScope()->num_parameters()
          : 0;
  const bool has_outer_scope_info = !outer_scope.is_null();

  Handle<SourceTextModuleInfo> module_info;
  if (scope->is_module_scope()) {
    module_info = SourceTextModuleInfo::New(isolate, zone,
                                            scope->AsModuleScope()->module());
  }

// Make sure the Fields enum agrees with Torque-generated offsets.
#define ASSERT_MATCHED_FIELD(name) \
  STATIC_ASSERT(OffsetOfElementAt(k##name) == k##name##Offset);
  FOR_EACH_SCOPE_INFO_NUMERIC_FIELD(ASSERT_MATCHED_FIELD)
#undef ASSERT_MATCHED_FIELD

  const int length = kVariablePartIndex + 2 * context_local_count +
                     (should_save_class_variable_index ? 1 : 0) +
                     (has_receiver ? 1 : 0) +
                     (has_function_name ? kFunctionNameEntries : 0) +
                     (has_inferred_function_name ? 1 : 0) +
                     (has_position_info ? kPositionInfoEntries : 0) +
                     (has_outer_scope_info ? 1 : 0) +
                     (scope->is_module_scope()
                          ? 2 + kModuleVariableEntryLength * module_vars_count
                          : 0);

  Handle<ScopeInfo> scope_info_handle =
      isolate->factory()->NewScopeInfo(length);
  int index = kVariablePartIndex;
  {
    DisallowGarbageCollection no_gc;
    ScopeInfo scope_info = *scope_info_handle;
    WriteBarrierMode mode = scope_info.GetWriteBarrierMode(no_gc);

    bool has_simple_parameters = false;
    bool is_asm_module = false;
    bool sloppy_eval_can_extend_vars = false;
    if (scope->is_function_scope()) {
      DeclarationScope* function_scope = scope->AsDeclarationScope();
      has_simple_parameters = function_scope->has_simple_parameters();
#if V8_ENABLE_WEBASSEMBLY
      is_asm_module = function_scope->is_asm_module();
#endif  // V8_ENABLE_WEBASSEMBLY
    }
    FunctionKind function_kind = kNormalFunction;
    if (scope->is_declaration_scope()) {
      function_kind = scope->AsDeclarationScope()->function_kind();
      sloppy_eval_can_extend_vars =
          scope->AsDeclarationScope()->sloppy_eval_can_extend_vars();
    }

    // Encode the flags.
    int flags =
        ScopeTypeBits::encode(scope->scope_type()) |
        SloppyEvalCanExtendVarsBit::encode(sloppy_eval_can_extend_vars) |
        LanguageModeBit::encode(scope->language_mode()) |
        DeclarationScopeBit::encode(scope->is_declaration_scope()) |
        ReceiverVariableBits::encode(receiver_info) |
        HasClassBrandBit::encode(has_brand) |
        HasSavedClassVariableIndexBit::encode(
            should_save_class_variable_index) |
        HasNewTargetBit::encode(has_new_target) |
        FunctionVariableBits::encode(function_name_info) |
        HasInferredFunctionNameBit::encode(has_inferred_function_name) |
        IsAsmModuleBit::encode(is_asm_module) |
        HasSimpleParametersBit::encode(has_simple_parameters) |
        FunctionKindBits::encode(function_kind) |
        HasOuterScopeInfoBit::encode(has_outer_scope_info) |
        IsDebugEvaluateScopeBit::encode(scope->is_debug_evaluate_scope()) |
        ForceContextAllocationBit::encode(
            scope->ForceContextForLanguageMode()) |
        PrivateNameLookupSkipsOuterClassBit::encode(
            scope->private_name_lookup_skips_outer_class()) |
        HasContextExtensionSlotBit::encode(scope->HasContextExtensionSlot()) |
        IsReplModeScopeBit::encode(scope->is_repl_mode_scope()) |
        HasLocalsBlockListBit::encode(false);
    scope_info.set_flags(flags);

    scope_info.set_parameter_count(parameter_count);
    scope_info.set_context_local_count(context_local_count);

    // Jump ahead to set the number of module variables so that we can use range
    // DCHECKs in future steps.
    if (scope->is_module_scope()) {
      scope_info.set_module_variable_count(module_vars_count);
    }

    // Add context locals' names and info, module variables' names and info.
    // Context locals are added using their index.
    int context_local_base = index;
    int context_local_info_base = context_local_base + context_local_count;
    int module_var_entry = scope_info.ModuleVariableCountIndex() + 1;

    for (Variable* var : *scope->locals()) {
      switch (var->location()) {
        case VariableLocation::CONTEXT:
        case VariableLocation::REPL_GLOBAL: {
          // Due to duplicate parameters, context locals aren't guaranteed to
          // come in order.
          int local_index = var->index() - scope->ContextHeaderLength();
          DCHECK_LE(0, local_index);
          DCHECK_LT(local_index, context_local_count);
          uint32_t info =
              VariableModeBits::encode(var->mode()) |
              InitFlagBit::encode(var->initialization_flag()) |
              MaybeAssignedFlagBit::encode(var->maybe_assigned()) |
              ParameterNumberBits::encode(ParameterNumberBits::kMax) |
              IsStaticFlagBit::encode(var->is_static_flag());
          scope_info.set(context_local_base + local_index, *var->name(), mode);
          scope_info.set(context_local_info_base + local_index,
                         Smi::FromInt(info));
          break;
        }
        case VariableLocation::MODULE: {
          scope_info.set(module_var_entry +
                             TorqueGeneratedModuleVariableOffsets::kNameOffset /
                                 kTaggedSize,
                         *var->name(), mode);
          scope_info.set(
              module_var_entry +
                  TorqueGeneratedModuleVariableOffsets::kIndexOffset /
                      kTaggedSize,
              Smi::FromInt(var->index()));
          uint32_t properties =
              VariableModeBits::encode(var->mode()) |
              InitFlagBit::encode(var->initialization_flag()) |
              MaybeAssignedFlagBit::encode(var->maybe_assigned()) |
              ParameterNumberBits::encode(ParameterNumberBits::kMax) |
              IsStaticFlagBit::encode(var->is_static_flag());
          scope_info.set(
              module_var_entry +
                  TorqueGeneratedModuleVariableOffsets::kPropertiesOffset /
                      kTaggedSize,
              Smi::FromInt(properties));
          module_var_entry += kModuleVariableEntryLength;
          break;
        }
        default:
          break;
      }
    }

    if (scope->is_declaration_scope()) {
      // Mark contexts slots with the parameter number they represent. We walk
      // the list of parameters. That can include duplicate entries if a
      // parameter name is repeated. By walking upwards, we'll automatically
      // mark the context slot with the highest parameter number that uses this
      // variable. That will be the parameter number that is represented by the
      // context slot. All lower parameters will only be available on the stack
      // through the arguments object.
      for (int i = 0; i < parameter_count; i++) {
        Variable* parameter = scope->AsDeclarationScope()->parameter(i);
        if (parameter->location() != VariableLocation::CONTEXT) continue;
        int index = parameter->index() - scope->ContextHeaderLength();
        int info_index = context_local_info_base + index;
        int info = Smi::ToInt(scope_info.get(info_index));
        info = ParameterNumberBits::update(info, i);
        scope_info.set(info_index, Smi::FromInt(info));
      }

      // TODO(verwaest): Remove this unnecessary entry.
      if (scope->AsDeclarationScope()->has_this_declaration()) {
        Variable* var = scope->AsDeclarationScope()->receiver();
        if (var->location() == VariableLocation::CONTEXT) {
          int local_index = var->index() - scope->ContextHeaderLength();
          uint32_t info =
              VariableModeBits::encode(var->mode()) |
              InitFlagBit::encode(var->initialization_flag()) |
              MaybeAssignedFlagBit::encode(var->maybe_assigned()) |
              ParameterNumberBits::encode(ParameterNumberBits::kMax) |
              IsStaticFlagBit::encode(var->is_static_flag());
          scope_info.set(context_local_base + local_index, *var->name(), mode);
          scope_info.set(context_local_info_base + local_index,
                         Smi::FromInt(info));
        }
      }
    }

    index += 2 * context_local_count;

    DCHECK_EQ(index, scope_info.SavedClassVariableInfoIndex());
    // If the scope is a class scope and has used static private methods, save
    // the context slot index of the class variable.
    // Store the class variable index.
    if (should_save_class_variable_index) {
      Variable* class_variable = scope->AsClassScope()->class_variable();
      DCHECK_EQ(class_variable->location(), VariableLocation::CONTEXT);
      scope_info.set(index++, Smi::FromInt(class_variable->index()));
    }

    // If the receiver is allocated, add its index.
    DCHECK_EQ(index, scope_info.ReceiverInfoIndex());
    if (has_receiver) {
      int var_index = scope->AsDeclarationScope()->receiver()->index();
      scope_info.set(index++, Smi::FromInt(var_index));
      // ?? DCHECK(receiver_info != CONTEXT || var_index ==
      // scope_info->ContextLength() - 1);
    }

    // If present, add the function variable name and its index.
    DCHECK_EQ(index, scope_info.FunctionVariableInfoIndex());
    if (has_function_name) {
      Variable* var = scope->AsDeclarationScope()->function_var();
      int var_index = -1;
      Object name = Smi::zero();
      if (var != nullptr) {
        var_index = var->index();
        name = *var->name();
      }
      scope_info.set(index++, name, mode);
      scope_info.set(index++, Smi::FromInt(var_index));
      DCHECK(function_name_info != VariableAllocationInfo::CONTEXT ||
             var_index == scope_info.ContextLength() - 1);
    }

    DCHECK_EQ(index, scope_info.InferredFunctionNameIndex());
    if (has_inferred_function_name) {
      // The inferred function name is taken from the SFI.
      index++;
    }

    DCHECK_EQ(index, scope_info.PositionInfoIndex());
    if (has_position_info) {
      scope_info.set(index++, Smi::FromInt(scope->start_position()));
      scope_info.set(index++, Smi::FromInt(scope->end_position()));
    }

    // If present, add the outer scope info.
    DCHECK(index == scope_info.OuterScopeInfoIndex());
    if (has_outer_scope_info) {
      scope_info.set(index++, *outer_scope.ToHandleChecked(), mode);
    }

    // Module-specific information (only for module scopes).
    if (scope->is_module_scope()) {
      DCHECK_EQ(index, scope_info.ModuleInfoIndex());
      scope_info.set(index++, *module_info);
      DCHECK_EQ(index, scope_info.ModuleVariableCountIndex());
      // Module variable count was already written above.
      index++;
      DCHECK_EQ(index, scope_info.ModuleVariablesIndex());
      // The variable entries themselves have already been written above.
      index += kModuleVariableEntryLength * module_vars_count;
    }
  }

  DCHECK_EQ(index, scope_info_handle->length());
  DCHECK_EQ(parameter_count, scope_info_handle->ParameterCount());
  DCHECK_EQ(scope->num_heap_slots(), scope_info_handle->ContextLength());
  return scope_info_handle;
}

template EXPORT_TEMPLATE_DEFINE(V8_EXPORT_PRIVATE)
    Handle<ScopeInfo> ScopeInfo::Create(Isolate* isolate, Zone* zone,
                                        Scope* scope,
                                        MaybeHandle<ScopeInfo> outer_scope);
template EXPORT_TEMPLATE_DEFINE(V8_EXPORT_PRIVATE)
    Handle<ScopeInfo> ScopeInfo::Create(LocalIsolate* isolate, Zone* zone,
                                        Scope* scope,
                                        MaybeHandle<ScopeInfo> outer_scope);

// static
Handle<ScopeInfo> ScopeInfo::CreateForWithScope(
    Isolate* isolate, MaybeHandle<ScopeInfo> outer_scope) {
  const bool has_outer_scope_info = !outer_scope.is_null();
  const int length = kVariablePartIndex + (has_outer_scope_info ? 1 : 0);

  Factory* factory = isolate->factory();
  Handle<ScopeInfo> scope_info = factory->NewScopeInfo(length);

  // Encode the flags.
  int flags =
      ScopeTypeBits::encode(WITH_SCOPE) |
      SloppyEvalCanExtendVarsBit::encode(false) |
      LanguageModeBit::encode(LanguageMode::kSloppy) |
      DeclarationScopeBit::encode(false) |
      ReceiverVariableBits::encode(VariableAllocationInfo::NONE) |
      HasClassBrandBit::encode(false) |
      HasSavedClassVariableIndexBit::encode(false) |
      HasNewTargetBit::encode(false) |
      FunctionVariableBits::encode(VariableAllocationInfo::NONE) |
      IsAsmModuleBit::encode(false) | HasSimpleParametersBit::encode(true) |
      FunctionKindBits::encode(kNormalFunction) |
      HasOuterScopeInfoBit::encode(has_outer_scope_info) |
      IsDebugEvaluateScopeBit::encode(false) |
      ForceContextAllocationBit::encode(false) |
      PrivateNameLookupSkipsOuterClassBit::encode(false) |
      HasContextExtensionSlotBit::encode(true) |
      IsReplModeScopeBit::encode(false) | HasLocalsBlockListBit::encode(false);
  scope_info->set_flags(flags);

  scope_info->set_parameter_count(0);
  scope_info->set_context_local_count(0);

  int index = kVariablePartIndex;
  DCHECK_EQ(index, scope_info->ReceiverInfoIndex());
  DCHECK_EQ(index, scope_info->FunctionVariableInfoIndex());
  DCHECK_EQ(index, scope_info->InferredFunctionNameIndex());
  DCHECK_EQ(index, scope_info->PositionInfoIndex());
  DCHECK(index == scope_info->OuterScopeInfoIndex());
  if (has_outer_scope_info) {
    scope_info->set(index++, *outer_scope.ToHandleChecked());
  }
  DCHECK_EQ(index, scope_info->length());
  DCHECK_EQ(0, scope_info->ParameterCount());
  DCHECK_EQ(scope_info->ContextHeaderLength(), scope_info->ContextLength());
  return scope_info;
}

// static
Handle<ScopeInfo> ScopeInfo::CreateGlobalThisBinding(Isolate* isolate) {
  return CreateForBootstrapping(isolate, BootstrappingType::kScript);
}

// static
Handle<ScopeInfo> ScopeInfo::CreateForEmptyFunction(Isolate* isolate) {
  return CreateForBootstrapping(isolate, BootstrappingType::kFunction);
}

// static
Handle<ScopeInfo> ScopeInfo::CreateForNativeContext(Isolate* isolate) {
  return CreateForBootstrapping(isolate, BootstrappingType::kNative);
}

// static
Handle<ScopeInfo> ScopeInfo::CreateForBootstrapping(Isolate* isolate,
                                                    BootstrappingType type) {
  const int parameter_count = 0;
  const bool is_empty_function = type == BootstrappingType::kFunction;
  const bool is_native_context = type == BootstrappingType::kNative;
  const bool is_script = type == BootstrappingType::kScript;
  const int context_local_count =
      is_empty_function || is_native_context ? 0 : 1;
  const bool has_receiver = is_script;
  const bool has_inferred_function_name = is_empty_function;
  const bool has_position_info = true;
  const int length = kVariablePartIndex + 2 * context_local_count +
                     (has_receiver ? 1 : 0) +
                     (is_empty_function ? kFunctionNameEntries : 0) +
                     (has_inferred_function_name ? 1 : 0) +
                     (has_position_info ? kPositionInfoEntries : 0);

  Factory* factory = isolate->factory();
  Handle<ScopeInfo> scope_info =
      factory->NewScopeInfo(length, AllocationType::kReadOnly);

  // Encode the flags.
  int flags =
      ScopeTypeBits::encode(is_empty_function ? FUNCTION_SCOPE : SCRIPT_SCOPE) |
      SloppyEvalCanExtendVarsBit::encode(false) |
      LanguageModeBit::encode(LanguageMode::kSloppy) |
      DeclarationScopeBit::encode(true) |
      ReceiverVariableBits::encode(is_script ? VariableAllocationInfo::CONTEXT
                                             : VariableAllocationInfo::UNUSED) |
      HasClassBrandBit::encode(false) |
      HasSavedClassVariableIndexBit::encode(false) |
      HasNewTargetBit::encode(false) |
      FunctionVariableBits::encode(is_empty_function
                                       ? VariableAllocationInfo::UNUSED
                                       : VariableAllocationInfo::NONE) |
      HasInferredFunctionNameBit::encode(has_inferred_function_name) |
      IsAsmModuleBit::encode(false) | HasSimpleParametersBit::encode(true) |
      FunctionKindBits::encode(FunctionKind::kNormalFunction) |
      HasOuterScopeInfoBit::encode(false) |
      IsDebugEvaluateScopeBit::encode(false) |
      ForceContextAllocationBit::encode(false) |
      PrivateNameLookupSkipsOuterClassBit::encode(false) |
      HasContextExtensionSlotBit::encode(is_native_context) |
      IsReplModeScopeBit::encode(false) | HasLocalsBlockListBit::encode(false);
  scope_info->set_flags(flags);
  scope_info->set_parameter_count(parameter_count);
  scope_info->set_context_local_count(context_local_count);

  int index = kVariablePartIndex;

  // Here we add info for context-allocated "this".
  DCHECK_EQ(index, scope_info->ContextLocalNamesIndex());
  if (context_local_count) {
    scope_info->set(index++, ReadOnlyRoots(isolate).this_string());
  }
  DCHECK_EQ(index, scope_info->ContextLocalInfosIndex());
  if (context_local_count > 0) {
    const uint32_t value =
        VariableModeBits::encode(VariableMode::kConst) |
        InitFlagBit::encode(kCreatedInitialized) |
        MaybeAssignedFlagBit::encode(kNotAssigned) |
        ParameterNumberBits::encode(ParameterNumberBits::kMax) |
        IsStaticFlagBit::encode(IsStaticFlag::kNotStatic);
    scope_info->set(index++, Smi::FromInt(value));
  }

  // And here we record that this scopeinfo binds a receiver.
  DCHECK_EQ(index, scope_info->ReceiverInfoIndex());
  if (has_receiver) {
    const int receiver_index = scope_info->ContextHeaderLength();
    scope_info->set(index++, Smi::FromInt(receiver_index));
  }

  DCHECK_EQ(index, scope_info->FunctionVariableInfoIndex());
  if (is_empty_function) {
    scope_info->set(index++, *isolate->factory()->empty_string());
    scope_info->set(index++, Smi::zero());
  }
  DCHECK_EQ(index, scope_info->InferredFunctionNameIndex());
  if (has_inferred_function_name) {
    scope_info->set(index++, *isolate->factory()->empty_string());
  }
  DCHECK_EQ(index, scope_info->PositionInfoIndex());
  // Store dummy position to be in sync with the {scope_type}.
  scope_info->set(index++, Smi::zero());
  scope_info->set(index++, Smi::zero());
  DCHECK_EQ(index, scope_info->OuterScopeInfoIndex());
  DCHECK_EQ(index, scope_info->length());
  DCHECK_EQ(scope_info->ParameterCount(), parameter_count);
  if (is_empty_function || is_native_context) {
    DCHECK_EQ(scope_info->ContextLength(), 0);
  } else {
    DCHECK_EQ(scope_info->ContextLength(),
              scope_info->ContextHeaderLength() + 1);
  }

  return scope_info;
}

Object ScopeInfo::get(int index) const {
  PtrComprCageBase cage_base = GetPtrComprCageBase(*this);
  return get(cage_base, index);
}

Object ScopeInfo::get(PtrComprCageBase cage_base, int index) const {
  DCHECK_LT(static_cast<unsigned>(index), static_cast<unsigned>(length()));
  return TaggedField<Object>::Relaxed_Load(cage_base, *this,
                                           OffsetOfElementAt(index));
}

void ScopeInfo::set(int index, Smi value) {
  DCHECK_LT(static_cast<unsigned>(index), static_cast<unsigned>(length()));
  DCHECK(Object(value).IsSmi());
  int offset = OffsetOfElementAt(index);
  RELAXED_WRITE_FIELD(*this, offset, value);
}

void ScopeInfo::set(int index, Object value, WriteBarrierMode mode) {
  DCHECK_LT(static_cast<unsigned>(index), static_cast<unsigned>(length()));
  int offset = OffsetOfElementAt(index);
  RELAXED_WRITE_FIELD(*this, offset, value);
  CONDITIONAL_WRITE_BARRIER(*this, offset, value, mode);
}

void ScopeInfo::CopyElements(Isolate* isolate, int dst_index, ScopeInfo src,
                             int src_index, int len, WriteBarrierMode mode) {
  if (len == 0) return;
  DCHECK_LE(src_index + len, src.length());
  DisallowGarbageCollection no_gc;

  ObjectSlot dst_slot(RawFieldOfElementAt(dst_index));
  ObjectSlot src_slot(src.RawFieldOfElementAt(src_index));
  isolate->heap()->CopyRange(*this, dst_slot, src_slot, len, mode);
}

ObjectSlot ScopeInfo::RawFieldOfElementAt(int index) {
  return RawField(OffsetOfElementAt(index));
}

int ScopeInfo::length() const {
  // AllocatedSize() is generated by Torque and represents the size in bytes of
  // the object, as computed from flags, context_local_count, and possibly
  // module_variable_count. Convert that size into a number of slots.
  return (AllocatedSize() - HeapObject::kHeaderSize) / kTaggedSize;
}

// static
Handle<ScopeInfo> ScopeInfo::RecreateWithBlockList(
    Isolate* isolate, Handle<ScopeInfo> original, Handle<StringSet> blocklist) {
  DCHECK(!original.is_null());
  if (original->HasLocalsBlockList()) return original;

  Handle<ScopeInfo> scope_info =
      isolate->factory()->NewScopeInfo(original->length() + 1);

  // Copy the static part first and update the flags to include the
  // blocklist field, so {LocalsBlockListIndex} returns the correct value.
  scope_info->CopyElements(isolate, 0, *original, 0, kVariablePartIndex,
                           WriteBarrierMode::UPDATE_WRITE_BARRIER);
  scope_info->set_flags(
      HasLocalsBlockListBit::update(scope_info->Flags(), true));

  // Copy the dynamic part including the provided blocklist:
  //   1) copy all the fields up to the blocklist index
  //   2) add the blocklist
  //   3) copy the remaining fields
  scope_info->CopyElements(
      isolate, kVariablePartIndex, *original, kVariablePartIndex,
      scope_info->LocalsBlockListIndex() - kVariablePartIndex,
      WriteBarrierMode::UPDATE_WRITE_BARRIER);
  scope_info->set_locals_block_list(*blocklist);
  scope_info->CopyElements(
      isolate, scope_info->LocalsBlockListIndex() + 1, *original,
      scope_info->LocalsBlockListIndex(),
      scope_info->length() - scope_info->LocalsBlockListIndex() - 1,
      WriteBarrierMode::UPDATE_WRITE_BARRIER);
  return scope_info;
}

ScopeInfo ScopeInfo::Empty(Isolate* isolate) {
  return ReadOnlyRoots(isolate).empty_scope_info();
}

bool ScopeInfo::IsEmpty() const { return IsEmptyBit::decode(Flags()); }

ScopeType ScopeInfo::scope_type() const {
  DCHECK(!IsEmpty());
  return ScopeTypeBits::decode(Flags());
}

bool ScopeInfo::is_script_scope() const {
  return !IsEmpty() && scope_type() == SCRIPT_SCOPE;
}

bool ScopeInfo::SloppyEvalCanExtendVars() const {
  bool sloppy_eval_can_extend_vars =
      SloppyEvalCanExtendVarsBit::decode(Flags());
  DCHECK_IMPLIES(sloppy_eval_can_extend_vars, is_sloppy(language_mode()));
  DCHECK_IMPLIES(sloppy_eval_can_extend_vars, is_declaration_scope());
  return sloppy_eval_can_extend_vars;
}

LanguageMode ScopeInfo::language_mode() const {
  return LanguageModeBit::decode(Flags());
}

bool ScopeInfo::is_declaration_scope() const {
  return DeclarationScopeBit::decode(Flags());
}

int ScopeInfo::ContextLength() const {
  if (!IsEmpty()) {
    int context_locals = ContextLocalCount();
    bool function_name_context_slot = FunctionVariableBits::decode(Flags()) ==
                                      VariableAllocationInfo::CONTEXT;
    bool force_context = ForceContextAllocationBit::decode(Flags());
    bool has_context =
        context_locals > 0 || force_context || function_name_context_slot ||
        scope_type() == WITH_SCOPE || scope_type() == CLASS_SCOPE ||
        (scope_type() == BLOCK_SCOPE && SloppyEvalCanExtendVars() &&
         is_declaration_scope()) ||
        (scope_type() == FUNCTION_SCOPE && SloppyEvalCanExtendVars()) ||
        (scope_type() == FUNCTION_SCOPE && IsAsmModule()) ||
        scope_type() == MODULE_SCOPE;

    if (has_context) {
      return ContextHeaderLength() + context_locals +
             (function_name_context_slot ? 1 : 0);
    }
  }
  return 0;
}

bool ScopeInfo::HasContextExtensionSlot() const {
  return HasContextExtensionSlotBit::decode(Flags());
}

int ScopeInfo::ContextHeaderLength() const {
  return HasContextExtensionSlot() ? Context::MIN_CONTEXT_EXTENDED_SLOTS
                                   : Context::MIN_CONTEXT_SLOTS;
}

bool ScopeInfo::HasReceiver() const {
  return VariableAllocationInfo::NONE != ReceiverVariableBits::decode(Flags());
}

bool ScopeInfo::HasAllocatedReceiver() const {
  VariableAllocationInfo allocation = ReceiverVariableBits::decode(Flags());
  return allocation == VariableAllocationInfo::STACK ||
         allocation == VariableAllocationInfo::CONTEXT;
}

bool ScopeInfo::HasClassBrand() const {
  return HasClassBrandBit::decode(Flags());
}

bool ScopeInfo::HasSavedClassVariableIndex() const {
  return HasSavedClassVariableIndexBit::decode(Flags());
}

bool ScopeInfo::HasNewTarget() const {
  return HasNewTargetBit::decode(Flags());
}

bool ScopeInfo::HasFunctionName() const {
  return VariableAllocationInfo::NONE != FunctionVariableBits::decode(Flags());
}

bool ScopeInfo::HasInferredFunctionName() const {
  return HasInferredFunctionNameBit::decode(Flags());
}

bool ScopeInfo::HasPositionInfo() const {
  if (IsEmpty()) return false;
  return NeedsPositionInfo(scope_type());
}

// static
bool ScopeInfo::NeedsPositionInfo(ScopeType type) {
  return type == FUNCTION_SCOPE || type == SCRIPT_SCOPE || type == EVAL_SCOPE ||
         type == MODULE_SCOPE;
}

bool ScopeInfo::HasSharedFunctionName() const {
  return FunctionName() != SharedFunctionInfo::kNoSharedNameSentinel;
}

void ScopeInfo::SetFunctionName(Object name) {
  DCHECK(HasFunctionName());
  DCHECK(name.IsString() || name == SharedFunctionInfo::kNoSharedNameSentinel);
  set_function_variable_info_name(name);
}

void ScopeInfo::SetInferredFunctionName(String name) {
  DCHECK(HasInferredFunctionName());
  set_inferred_function_name(name);
}

bool ScopeInfo::HasOuterScopeInfo() const {
  return HasOuterScopeInfoBit::decode(Flags());
}

bool ScopeInfo::IsDebugEvaluateScope() const {
  return IsDebugEvaluateScopeBit::decode(Flags());
}

void ScopeInfo::SetIsDebugEvaluateScope() {
  CHECK(!IsEmpty());
  DCHECK_EQ(scope_type(), WITH_SCOPE);
  set_flags(Flags() | IsDebugEvaluateScopeBit::encode(true));
}

bool ScopeInfo::PrivateNameLookupSkipsOuterClass() const {
  return PrivateNameLookupSkipsOuterClassBit::decode(Flags());
}

bool ScopeInfo::IsReplModeScope() const {
  return IsReplModeScopeBit::decode(Flags());
}

bool ScopeInfo::HasLocalsBlockList() const {
  return HasLocalsBlockListBit::decode(Flags());
}

StringSet ScopeInfo::LocalsBlockList() const {
  DCHECK(HasLocalsBlockList());
  return StringSet::cast(locals_block_list());
}

bool ScopeInfo::HasContext() const { return ContextLength() > 0; }

Object ScopeInfo::FunctionName() const {
  DCHECK(HasFunctionName());
  return function_variable_info_name();
}

Object ScopeInfo::InferredFunctionName() const {
  DCHECK(HasInferredFunctionName());
  return inferred_function_name();
}

String ScopeInfo::FunctionDebugName() const {
  if (!HasFunctionName()) return GetReadOnlyRoots().empty_string();
  Object name = FunctionName();
  if (name.IsString() && String::cast(name).length() > 0) {
    return String::cast(name);
  }
  if (HasInferredFunctionName()) {
    name = InferredFunctionName();
    if (name.IsString()) return String::cast(name);
  }
  return GetReadOnlyRoots().empty_string();
}

int ScopeInfo::StartPosition() const {
  DCHECK(HasPositionInfo());
  return position_info_start();
}

int ScopeInfo::EndPosition() const {
  DCHECK(HasPositionInfo());
  return position_info_end();
}

void ScopeInfo::SetPositionInfo(int start, int end) {
  DCHECK(HasPositionInfo());
  DCHECK_LE(start, end);
  set_position_info_start(start);
  set_position_info_end(end);
}

ScopeInfo ScopeInfo::OuterScopeInfo() const {
  DCHECK(HasOuterScopeInfo());
  return ScopeInfo::cast(outer_scope_info());
}

SourceTextModuleInfo ScopeInfo::ModuleDescriptorInfo() const {
  DCHECK(scope_type() == MODULE_SCOPE);
  return SourceTextModuleInfo::cast(module_info());
}

String ScopeInfo::ContextLocalName(int var) const {
  return context_local_names(var);
}

VariableMode ScopeInfo::ContextLocalMode(int var) const {
  int value = context_local_infos(var);
  return VariableModeBits::decode(value);
}

IsStaticFlag ScopeInfo::ContextLocalIsStaticFlag(int var) const {
  int value = context_local_infos(var);
  return IsStaticFlagBit::decode(value);
}

InitializationFlag ScopeInfo::ContextLocalInitFlag(int var) const {
  int value = context_local_infos(var);
  return InitFlagBit::decode(value);
}

bool ScopeInfo::ContextLocalIsParameter(int var) const {
  int value = context_local_infos(var);
  return ParameterNumberBits::decode(value) != ParameterNumberBits::kMax;
}

uint32_t ScopeInfo::ContextLocalParameterNumber(int var) const {
  DCHECK(ContextLocalIsParameter(var));
  int value = context_local_infos(var);
  return ParameterNumberBits::decode(value);
}

MaybeAssignedFlag ScopeInfo::ContextLocalMaybeAssignedFlag(int var) const {
  int value = context_local_infos(var);
  return MaybeAssignedFlagBit::decode(value);
}

// static
bool ScopeInfo::VariableIsSynthetic(String name) {
  // There's currently no flag stored on the ScopeInfo to indicate that a
  // variable is a compiler-introduced temporary. However, to avoid conflict
  // with user declarations, the current temporaries like .generator_object and
  // .result start with a dot, so we can use that as a flag. It's a hack!
  return name.length() == 0 || name.Get(0) == '.' || name.Get(0) == '#' ||
         name.Equals(name.GetReadOnlyRoots().this_string());
}

int ScopeInfo::ModuleVariableCount() const {
  DCHECK_EQ(scope_type(), MODULE_SCOPE);
  return module_variable_count();
}

int ScopeInfo::ModuleIndex(String name, VariableMode* mode,
                           InitializationFlag* init_flag,
                           MaybeAssignedFlag* maybe_assigned_flag) {
  DisallowGarbageCollection no_gc;
  DCHECK(name.IsInternalizedString());
  DCHECK_EQ(scope_type(), MODULE_SCOPE);
  DCHECK_NOT_NULL(mode);
  DCHECK_NOT_NULL(init_flag);
  DCHECK_NOT_NULL(maybe_assigned_flag);

  int module_vars_count = module_variable_count();
  for (int i = 0; i < module_vars_count; ++i) {
    String var_name = module_variables_name(i);
    if (name.Equals(var_name)) {
      int index;
      ModuleVariable(i, nullptr, &index, mode, init_flag, maybe_assigned_flag);
      return index;
    }
  }

  return 0;
}

// static
int ScopeInfo::ContextSlotIndex(ScopeInfo scope_info, String name,
                                VariableMode* mode,
                                InitializationFlag* init_flag,
                                MaybeAssignedFlag* maybe_assigned_flag,
                                IsStaticFlag* is_static_flag) {
  DisallowGarbageCollection no_gc;
  DCHECK(name.IsInternalizedString());
  DCHECK_NOT_NULL(mode);
  DCHECK_NOT_NULL(init_flag);
  DCHECK_NOT_NULL(maybe_assigned_flag);

  if (scope_info.IsEmpty()) return -1;

  int context_local_count = scope_info.context_local_count();
  for (int var = 0; var < context_local_count; ++var) {
    if (name != scope_info.context_local_names(var)) {
      continue;
    }
    *mode = scope_info.ContextLocalMode(var);
    *is_static_flag = scope_info.ContextLocalIsStaticFlag(var);
    *init_flag = scope_info.ContextLocalInitFlag(var);
    *maybe_assigned_flag = scope_info.ContextLocalMaybeAssignedFlag(var);
    int result = scope_info.ContextHeaderLength() + var;

    DCHECK_LT(result, scope_info.ContextLength());
    return result;
  }

  return -1;
}

int ScopeInfo::SavedClassVariableContextLocalIndex() const {
  if (HasSavedClassVariableIndexBit::decode(Flags())) {
    int index = saved_class_variable_info();
    return index - Context::MIN_CONTEXT_SLOTS;
  }
  return -1;
}

int ScopeInfo::ReceiverContextSlotIndex() const {
  if (ReceiverVariableBits::decode(Flags()) ==
      VariableAllocationInfo::CONTEXT) {
    return receiver_info();
  }
  return -1;
}

int ScopeInfo::FunctionContextSlotIndex(String name) const {
  DCHECK(name.IsInternalizedString());
  if (FunctionVariableBits::decode(Flags()) ==
          VariableAllocationInfo::CONTEXT &&
      FunctionName() == name) {
    return function_variable_info_context_or_stack_slot_index();
  }
  return -1;
}

FunctionKind ScopeInfo::function_kind() const {
  return FunctionKindBits::decode(Flags());
}

int ScopeInfo::ContextLocalNamesIndex() const {
  return ConvertOffsetToIndex(kContextLocalNamesOffset);
}

int ScopeInfo::ContextLocalInfosIndex() const {
  return ConvertOffsetToIndex(ContextLocalInfosOffset());
}

int ScopeInfo::SavedClassVariableInfoIndex() const {
  return ConvertOffsetToIndex(SavedClassVariableInfoOffset());
}

int ScopeInfo::ReceiverInfoIndex() const {
  return ConvertOffsetToIndex(ReceiverInfoOffset());
}

int ScopeInfo::FunctionVariableInfoIndex() const {
  return ConvertOffsetToIndex(FunctionVariableInfoOffset());
}

int ScopeInfo::InferredFunctionNameIndex() const {
  return ConvertOffsetToIndex(InferredFunctionNameOffset());
}

int ScopeInfo::PositionInfoIndex() const {
  return ConvertOffsetToIndex(PositionInfoOffset());
}

int ScopeInfo::OuterScopeInfoIndex() const {
  return ConvertOffsetToIndex(OuterScopeInfoOffset());
}

int ScopeInfo::LocalsBlockListIndex() const {
  return ConvertOffsetToIndex(LocalsBlockListOffset());
}

int ScopeInfo::ModuleInfoIndex() const {
  return ConvertOffsetToIndex(ModuleInfoOffset());
}

int ScopeInfo::ModuleVariableCountIndex() const {
  return ConvertOffsetToIndex(ModuleVariableCountOffset());
}

int ScopeInfo::ModuleVariablesIndex() const {
  return ConvertOffsetToIndex(ModuleVariablesOffset());
}

void ScopeInfo::ModuleVariable(int i, String* name, int* index,
                               VariableMode* mode,
                               InitializationFlag* init_flag,
                               MaybeAssignedFlag* maybe_assigned_flag) {
  int properties = module_variables_properties(i);

  if (name != nullptr) {
    *name = module_variables_name(i);
  }
  if (index != nullptr) {
    *index = module_variables_index(i);
    DCHECK_NE(*index, 0);
  }
  if (mode != nullptr) {
    *mode = VariableModeBits::decode(properties);
  }
  if (init_flag != nullptr) {
    *init_flag = InitFlagBit::decode(properties);
  }
  if (maybe_assigned_flag != nullptr) {
    *maybe_assigned_flag = MaybeAssignedFlagBit::decode(properties);
  }
}

std::ostream& operator<<(std::ostream& os, VariableAllocationInfo var_info) {
  switch (var_info) {
    case VariableAllocationInfo::NONE:
      return os << "NONE";
    case VariableAllocationInfo::STACK:
      return os << "STACK";
    case VariableAllocationInfo::CONTEXT:
      return os << "CONTEXT";
    case VariableAllocationInfo::UNUSED:
      return os << "UNUSED";
  }
  UNREACHABLE();
  return os;
}

template <typename IsolateT>
Handle<ModuleRequest> ModuleRequest::New(IsolateT* isolate,
                                         Handle<String> specifier,
                                         Handle<FixedArray> import_assertions,
                                         int position) {
  Handle<ModuleRequest> result = Handle<ModuleRequest>::cast(
      isolate->factory()->NewStruct(MODULE_REQUEST_TYPE, AllocationType::kOld));
  result->set_specifier(*specifier);
  result->set_import_assertions(*import_assertions);
  result->set_position(position);
  return result;
}

template Handle<ModuleRequest> ModuleRequest::New(
    Isolate* isolate, Handle<String> specifier,
    Handle<FixedArray> import_assertions, int position);
template Handle<ModuleRequest> ModuleRequest::New(
    LocalIsolate* isolate, Handle<String> specifier,
    Handle<FixedArray> import_assertions, int position);

template <typename IsolateT>
Handle<SourceTextModuleInfoEntry> SourceTextModuleInfoEntry::New(
    IsolateT* isolate, Handle<PrimitiveHeapObject> export_name,
    Handle<PrimitiveHeapObject> local_name,
    Handle<PrimitiveHeapObject> import_name, int module_request, int cell_index,
    int beg_pos, int end_pos) {
  Handle<SourceTextModuleInfoEntry> result =
      Handle<SourceTextModuleInfoEntry>::cast(isolate->factory()->NewStruct(
          SOURCE_TEXT_MODULE_INFO_ENTRY_TYPE, AllocationType::kOld));
  result->set_export_name(*export_name);
  result->set_local_name(*local_name);
  result->set_import_name(*import_name);
  result->set_module_request(module_request);
  result->set_cell_index(cell_index);
  result->set_beg_pos(beg_pos);
  result->set_end_pos(end_pos);
  return result;
}

template Handle<SourceTextModuleInfoEntry> SourceTextModuleInfoEntry::New(
    Isolate* isolate, Handle<PrimitiveHeapObject> export_name,
    Handle<PrimitiveHeapObject> local_name,
    Handle<PrimitiveHeapObject> import_name, int module_request, int cell_index,
    int beg_pos, int end_pos);
template Handle<SourceTextModuleInfoEntry> SourceTextModuleInfoEntry::New(
    LocalIsolate* isolate, Handle<PrimitiveHeapObject> export_name,
    Handle<PrimitiveHeapObject> local_name,
    Handle<PrimitiveHeapObject> import_name, int module_request, int cell_index,
    int beg_pos, int end_pos);

template <typename IsolateT>
Handle<SourceTextModuleInfo> SourceTextModuleInfo::New(
    IsolateT* isolate, Zone* zone, SourceTextModuleDescriptor* descr) {
  // Serialize module requests.
  int size = static_cast<int>(descr->module_requests().size());
  Handle<FixedArray> module_requests = isolate->factory()->NewFixedArray(size);
  for (const auto& elem : descr->module_requests()) {
    Handle<ModuleRequest> serialized_module_request = elem->Serialize(isolate);
    module_requests->set(elem->index(), *serialized_module_request);
  }

  // Serialize special exports.
  Handle<FixedArray> special_exports = isolate->factory()->NewFixedArray(
      static_cast<int>(descr->special_exports().size()));
  {
    int i = 0;
    for (auto entry : descr->special_exports()) {
      Handle<SourceTextModuleInfoEntry> serialized_entry =
          entry->Serialize(isolate);
      special_exports->set(i++, *serialized_entry);
    }
  }

  // Serialize namespace imports.
  Handle<FixedArray> namespace_imports = isolate->factory()->NewFixedArray(
      static_cast<int>(descr->namespace_imports().size()));
  {
    int i = 0;
    for (auto entry : descr->namespace_imports()) {
      Handle<SourceTextModuleInfoEntry> serialized_entry =
          entry->Serialize(isolate);
      namespace_imports->set(i++, *serialized_entry);
    }
  }

  // Serialize regular exports.
  Handle<FixedArray> regular_exports =
      descr->SerializeRegularExports(isolate, zone);

  // Serialize regular imports.
  Handle<FixedArray> regular_imports = isolate->factory()->NewFixedArray(
      static_cast<int>(descr->regular_imports().size()));
  {
    int i = 0;
    for (const auto& elem : descr->regular_imports()) {
      Handle<SourceTextModuleInfoEntry> serialized_entry =
          elem.second->Serialize(isolate);
      regular_imports->set(i++, *serialized_entry);
    }
  }

  Handle<SourceTextModuleInfo> result =
      isolate->factory()->NewSourceTextModuleInfo();
  result->set(kModuleRequestsIndex, *module_requests);
  result->set(kSpecialExportsIndex, *special_exports);
  result->set(kRegularExportsIndex, *regular_exports);
  result->set(kNamespaceImportsIndex, *namespace_imports);
  result->set(kRegularImportsIndex, *regular_imports);
  return result;
}
template Handle<SourceTextModuleInfo> SourceTextModuleInfo::New(
    Isolate* isolate, Zone* zone, SourceTextModuleDescriptor* descr);
template Handle<SourceTextModuleInfo> SourceTextModuleInfo::New(
    LocalIsolate* isolate, Zone* zone, SourceTextModuleDescriptor* descr);

int SourceTextModuleInfo::RegularExportCount() const {
  DCHECK_EQ(regular_exports().length() % kRegularExportLength, 0);
  return regular_exports().length() / kRegularExportLength;
}

String SourceTextModuleInfo::RegularExportLocalName(int i) const {
  return String::cast(regular_exports().get(i * kRegularExportLength +
                                            kRegularExportLocalNameOffset));
}

int SourceTextModuleInfo::RegularExportCellIndex(int i) const {
  return Smi::ToInt(regular_exports().get(i * kRegularExportLength +
                                          kRegularExportCellIndexOffset));
}

FixedArray SourceTextModuleInfo::RegularExportExportNames(int i) const {
  return FixedArray::cast(regular_exports().get(
      i * kRegularExportLength + kRegularExportExportNamesOffset));
}

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"
