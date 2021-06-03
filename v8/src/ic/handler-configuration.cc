// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/ic/handler-configuration.h"

#include "src/codegen/code-factory.h"
#include "src/ic/handler-configuration-inl.h"
#include "src/objects/data-handler-inl.h"
#include "src/objects/maybe-object.h"
#include "src/objects/transitions.h"

namespace v8 {
namespace internal {

namespace {

template <typename BitField>
Handle<Smi> SetBitFieldValue(Isolate* isolate, Handle<Smi> smi_handler,
                             typename BitField::FieldType value) {
  int config = smi_handler->value();
  config = BitField::update(config, true);
  return handle(Smi::FromInt(config), isolate);
}

// TODO(ishell): Remove templatezation once we move common bits from
// Load/StoreHandler to the base class.
template <typename ICHandler, bool fill_handler = true>
int InitPrototypeChecksImpl(Isolate* isolate, Handle<ICHandler> handler,
                            Handle<Smi>* smi_handler,
                            Handle<Map> lookup_start_object_map,
                            MaybeObjectHandle data1,
                            MaybeObjectHandle maybe_data2) {
  int data_size = 1;
  // Holder-is-receiver case itself does not add entries unless there is an
  // optional data2 value provided.

  DCHECK_IMPLIES(lookup_start_object_map->IsJSGlobalObjectMap(),
                 lookup_start_object_map->is_prototype_map());

  if (lookup_start_object_map->IsPrimitiveMap() ||
      lookup_start_object_map->is_access_check_needed()) {
    DCHECK(!lookup_start_object_map->IsJSGlobalObjectMap());
    // The validity cell check for primitive and global proxy receivers does
    // not guarantee that certain native context ever had access to other
    // native context. However, a handler created for one native context could
    // be used in other native context through the megamorphic stub cache.
    // So we record the original native context to which this handler
    // corresponds.
    if (fill_handler) {
      Handle<Context> native_context = isolate->native_context();
      handler->set_data2(HeapObjectReference::Weak(*native_context));
    } else {
      // Enable access checks on the lookup start object.
      *smi_handler = SetBitFieldValue<
          typename ICHandler::DoAccessCheckOnLookupStartObjectBits>(
          isolate, *smi_handler, true);
    }
    data_size++;
  } else if (lookup_start_object_map->is_dictionary_map() &&
             !lookup_start_object_map->IsJSGlobalObjectMap()) {
    if (!fill_handler) {
      // Enable lookup on lookup start object.
      *smi_handler =
          SetBitFieldValue<typename ICHandler::LookupOnLookupStartObjectBits>(
              isolate, *smi_handler, true);
    }
  }
  if (fill_handler) {
    handler->set_data1(*data1);
  }
  if (!maybe_data2.is_null()) {
    if (fill_handler) {
      // This value will go either to data2 or data3 slot depending on whether
      // data2 slot is already occupied by native context.
      if (data_size == 1) {
        handler->set_data2(*maybe_data2);
      } else {
        DCHECK_EQ(2, data_size);
        handler->set_data3(*maybe_data2);
      }
    }
    data_size++;
  }
  return data_size;
}

// Returns 0 if the validity cell check is enough to ensure that the
// prototype chain from |lookup_start_object_map| till |holder| did not change.
// If the |holder| is an empty handle then the full prototype chain is
// checked.
template <typename ICHandler>
int GetHandlerDataSize(Isolate* isolate, Handle<Smi>* smi_handler,
                       Handle<Map> lookup_start_object_map,
                       MaybeObjectHandle data1,
                       MaybeObjectHandle maybe_data2 = MaybeObjectHandle()) {
  DCHECK_NOT_NULL(smi_handler);
  return InitPrototypeChecksImpl<ICHandler, false>(
      isolate, Handle<ICHandler>(), smi_handler, lookup_start_object_map, data1,
      maybe_data2);
}

template <typename ICHandler>
void InitPrototypeChecks(Isolate* isolate, Handle<ICHandler> handler,
                         Handle<Map> lookup_start_object_map,
                         MaybeObjectHandle data1,
                         MaybeObjectHandle maybe_data2 = MaybeObjectHandle()) {
  InitPrototypeChecksImpl<ICHandler, true>(
      isolate, handler, nullptr, lookup_start_object_map, data1, maybe_data2);
}

}  // namespace

// static
Handle<Object> LoadHandler::LoadFromPrototype(
    Isolate* isolate, Handle<Map> lookup_start_object_map,
    Handle<JSReceiver> holder, Handle<Smi> smi_handler,
    MaybeObjectHandle maybe_data1, MaybeObjectHandle maybe_data2) {
  MaybeObjectHandle data1;
  if (maybe_data1.is_null()) {
    data1 = MaybeObjectHandle::Weak(holder);
  } else {
    data1 = maybe_data1;
  }

  int data_size = GetHandlerDataSize<LoadHandler>(
      isolate, &smi_handler, lookup_start_object_map, data1, maybe_data2);

  Handle<Object> validity_cell = Map::GetOrCreatePrototypeChainValidityCell(
      lookup_start_object_map, isolate);

  Handle<LoadHandler> handler = isolate->factory()->NewLoadHandler(data_size);

  handler->set_smi_handler(*smi_handler);
  handler->set_validity_cell(*validity_cell);
  InitPrototypeChecks(isolate, handler, lookup_start_object_map, data1,
                      maybe_data2);
  return handler;
}

// static
Handle<Object> LoadHandler::LoadFullChain(Isolate* isolate,
                                          Handle<Map> lookup_start_object_map,
                                          const MaybeObjectHandle& holder,
                                          Handle<Smi> smi_handler) {
  MaybeObjectHandle data1 = holder;
  int data_size = GetHandlerDataSize<LoadHandler>(
      isolate, &smi_handler, lookup_start_object_map, data1);

  Handle<Object> validity_cell = Map::GetOrCreatePrototypeChainValidityCell(
      lookup_start_object_map, isolate);
  if (validity_cell->IsSmi()) {
    DCHECK_EQ(1, data_size);
    // Lookup on lookup start object isn't supported in case of a simple smi
    // handler.
    if (!LookupOnLookupStartObjectBits::decode(smi_handler->value())) {
      return smi_handler;
    }
  }

  Handle<LoadHandler> handler = isolate->factory()->NewLoadHandler(data_size);

  handler->set_smi_handler(*smi_handler);
  handler->set_validity_cell(*validity_cell);
  InitPrototypeChecks(isolate, handler, lookup_start_object_map, data1);
  return handler;
}

// static
KeyedAccessLoadMode LoadHandler::GetKeyedAccessLoadMode(MaybeObject handler) {
  DisallowGarbageCollection no_gc;
  if (handler->IsSmi()) {
    int const raw_handler = handler.ToSmi().value();
    Kind const kind = KindBits::decode(raw_handler);
    if ((kind == kElement || kind == kIndexedString) &&
        AllowOutOfBoundsBits::decode(raw_handler)) {
      return LOAD_IGNORE_OUT_OF_BOUNDS;
    }
  }
  return STANDARD_LOAD;
}

// static
KeyedAccessStoreMode StoreHandler::GetKeyedAccessStoreMode(
    MaybeObject handler) {
  DisallowGarbageCollection no_gc;
  if (handler->IsSmi()) {
    int const raw_handler = handler.ToSmi().value();
    Kind const kind = KindBits::decode(raw_handler);
    // All the handlers except the Slow Handler that use the
    // KeyedAccessStoreMode, compute it using KeyedAccessStoreModeForBuiltin
    // method. Hence if any other Handler get to this path, just return
    // STANDARD_STORE.
    if (kind != kSlow) {
      return STANDARD_STORE;
    }
    KeyedAccessStoreMode store_mode =
        KeyedAccessStoreModeBits::decode(raw_handler);
    return store_mode;
  }
  return STANDARD_STORE;
}

// static
Handle<Object> StoreHandler::StoreElementTransition(
    Isolate* isolate, Handle<Map> receiver_map, Handle<Map> transition,
    KeyedAccessStoreMode store_mode, MaybeHandle<Object> prev_validity_cell) {
  Handle<Code> stub =
      CodeFactory::ElementsTransitionAndStore(isolate, store_mode).code();
  Handle<Object> validity_cell;
  if (!prev_validity_cell.ToHandle(&validity_cell)) {
    validity_cell =
        Map::GetOrCreatePrototypeChainValidityCell(receiver_map, isolate);
  }
  Handle<StoreHandler> handler = isolate->factory()->NewStoreHandler(1);
  handler->set_smi_handler(*stub);
  handler->set_validity_cell(*validity_cell);
  handler->set_data1(HeapObjectReference::Weak(*transition));
  return handler;
}

MaybeObjectHandle StoreHandler::StoreTransition(Isolate* isolate,
                                                Handle<Map> transition_map) {
  bool is_dictionary_map = transition_map->is_dictionary_map();
#ifdef DEBUG
  if (!is_dictionary_map) {
    InternalIndex descriptor = transition_map->LastAdded();
    Handle<DescriptorArray> descriptors(
        transition_map->instance_descriptors(isolate), isolate);
    PropertyDetails details = descriptors->GetDetails(descriptor);
    if (descriptors->GetKey(descriptor).IsPrivate()) {
      DCHECK_EQ(DONT_ENUM, details.attributes());
    } else {
      DCHECK_EQ(NONE, details.attributes());
    }
    Representation representation = details.representation();
    DCHECK(!representation.IsNone());
  }
#endif
  // Declarative handlers don't support access checks.
  DCHECK(!transition_map->is_access_check_needed());

  // Get validity cell value if it is necessary for the handler.
  Handle<Object> validity_cell;
  if (is_dictionary_map || !transition_map->IsPrototypeValidityCellValid()) {
    validity_cell =
        Map::GetOrCreatePrototypeChainValidityCell(transition_map, isolate);
  }

  if (is_dictionary_map) {
    DCHECK(!transition_map->IsJSGlobalObjectMap());
    Handle<StoreHandler> handler = isolate->factory()->NewStoreHandler(0);
    // Store normal with enabled lookup on receiver.
    int config =
        KindBits::encode(kNormal) | LookupOnLookupStartObjectBits::encode(true);
    handler->set_smi_handler(Smi::FromInt(config));
    handler->set_validity_cell(*validity_cell);
    return MaybeObjectHandle(handler);

  } else {
    // Ensure the transition map contains a valid prototype validity cell.
    if (!validity_cell.is_null()) {
      transition_map->set_prototype_validity_cell(*validity_cell);
    }
    return MaybeObjectHandle::Weak(transition_map);
  }
}

// static
Handle<Object> StoreHandler::StoreThroughPrototype(
    Isolate* isolate, Handle<Map> receiver_map, Handle<JSReceiver> holder,
    Handle<Smi> smi_handler, MaybeObjectHandle maybe_data1,
    MaybeObjectHandle maybe_data2) {
  MaybeObjectHandle data1;
  if (maybe_data1.is_null()) {
    data1 = MaybeObjectHandle::Weak(holder);
  } else {
    data1 = maybe_data1;
  }

  int data_size = GetHandlerDataSize<StoreHandler>(
      isolate, &smi_handler, receiver_map, data1, maybe_data2);

  Handle<Object> validity_cell =
      Map::GetOrCreatePrototypeChainValidityCell(receiver_map, isolate);

  Handle<StoreHandler> handler = isolate->factory()->NewStoreHandler(data_size);

  handler->set_smi_handler(*smi_handler);
  handler->set_validity_cell(*validity_cell);
  InitPrototypeChecks(isolate, handler, receiver_map, data1, maybe_data2);
  return handler;
}

// static
MaybeObjectHandle StoreHandler::StoreGlobal(Handle<PropertyCell> cell) {
  return MaybeObjectHandle::Weak(cell);
}

// static
Handle<Object> StoreHandler::StoreProxy(Isolate* isolate,
                                        Handle<Map> receiver_map,
                                        Handle<JSProxy> proxy,
                                        Handle<JSReceiver> receiver) {
  Handle<Smi> smi_handler = StoreProxy(isolate);
  if (receiver.is_identical_to(proxy)) return smi_handler;
  return StoreThroughPrototype(isolate, receiver_map, proxy, smi_handler,
                               MaybeObjectHandle::Weak(proxy));
}

#if defined(OBJECT_PRINT)
namespace {
void PrintSmiLoadHandler(int raw_handler, std::ostream& os) {
  LoadHandler::Kind kind = LoadHandler::KindBits::decode(raw_handler);
  os << "kind = ";
  switch (kind) {
    case LoadHandler::Kind::kElement:
      os << "kElement, allow out of bounds = "
         << LoadHandler::AllowOutOfBoundsBits::decode(raw_handler)
         << ", is JSArray = " << LoadHandler::IsJsArrayBits::decode(raw_handler)
         << ", convert hole = "
         << LoadHandler::ConvertHoleBits::decode(raw_handler)
         << ", elements kind = "
         << ElementsKindToString(
                LoadHandler::ElementsKindBits::decode(raw_handler));
      break;
    case LoadHandler::Kind::kIndexedString:
      os << "kIndexedString, allow out of bounds = "
         << LoadHandler::AllowOutOfBoundsBits::decode(raw_handler);
      break;
    case LoadHandler::Kind::kNormal:
      os << "kNormal";
      break;
    case LoadHandler::Kind::kGlobal:
      os << "kGlobal";
      break;
    case LoadHandler::Kind::kField: {
      os << "kField, is in object = "
         << LoadHandler::IsInobjectBits::decode(raw_handler)
         << ", is double = " << LoadHandler::IsDoubleBits::decode(raw_handler)
         << ", field index = "
         << LoadHandler::FieldIndexBits::decode(raw_handler);
      break;
    }
    case LoadHandler::Kind::kConstantFromPrototype: {
      os << "kConstantFromPrototype ";
      break;
    }
    case LoadHandler::Kind::kAccessor:
      os << "kAccessor, descriptor = "
         << LoadHandler::DescriptorBits::decode(raw_handler);
      break;
    case LoadHandler::Kind::kNativeDataProperty:
      os << "kNativeDataProperty, descriptor = "
         << LoadHandler::DescriptorBits::decode(raw_handler);
      break;
    case LoadHandler::Kind::kApiGetter:
      os << "kApiGetter";
      break;
    case LoadHandler::Kind::kApiGetterHolderIsPrototype:
      os << "kApiGetterHolderIsPrototype";
      break;
    case LoadHandler::Kind::kInterceptor:
      os << "kInterceptor";
      break;
    case LoadHandler::Kind::kSlow:
      os << "kSlow";
      break;
    case LoadHandler::Kind::kProxy:
      os << "kProxy";
      break;
    case LoadHandler::Kind::kNonExistent:
      os << "kNonExistent";
      break;
    case LoadHandler::Kind::kModuleExport:
      os << "kModuleExport, exports index = "
         << LoadHandler::ExportsIndexBits::decode(raw_handler);
      break;
    default:
      UNREACHABLE();
  }
}

const char* KeyedAccessStoreModeToString(KeyedAccessStoreMode mode) {
  switch (mode) {
    case STANDARD_STORE:
      return "STANDARD_STORE";
    case STORE_AND_GROW_HANDLE_COW:
      return "STORE_AND_GROW_HANDLE_COW";
    case STORE_IGNORE_OUT_OF_BOUNDS:
      return "STORE_IGNORE_OUT_OF_BOUNDS";
    case STORE_HANDLE_COW:
      return "STORE_HANDLE_COW";
  }
  UNREACHABLE();
}

void PrintSmiStoreHandler(int raw_handler, std::ostream& os) {
  StoreHandler::Kind kind = StoreHandler::KindBits::decode(raw_handler);
  os << "kind = ";
  switch (kind) {
    case StoreHandler::Kind::kField:
    case StoreHandler::Kind::kConstField: {
      os << "k";
      if (kind == StoreHandler::Kind::kConstField) {
        os << "Const";
      }
      Representation representation = Representation::FromKind(
          StoreHandler::RepresentationBits::decode(raw_handler));
      os << "Field, descriptor = "
         << StoreHandler::DescriptorBits::decode(raw_handler)
         << ", is in object = "
         << StoreHandler::IsInobjectBits::decode(raw_handler)
         << ", representation = " << representation.Mnemonic()
         << ", field index = "
         << StoreHandler::FieldIndexBits::decode(raw_handler);
      break;
    }
    case StoreHandler::Kind::kAccessor:
      os << "kAccessor, descriptor = "
         << StoreHandler::DescriptorBits::decode(raw_handler);
      break;
    case StoreHandler::Kind::kNativeDataProperty:
      os << "kNativeDataProperty, descriptor = "
         << StoreHandler::DescriptorBits::decode(raw_handler);
      break;
    case StoreHandler::Kind::kApiSetter:
      os << "kApiSetter";
      break;
    case StoreHandler::Kind::kApiSetterHolderIsPrototype:
      os << "kApiSetterHolderIsPrototype";
      break;
    case StoreHandler::Kind::kGlobalProxy:
      os << "kGlobalProxy";
      break;
    case StoreHandler::Kind::kNormal:
      os << "kNormal";
      break;
    case StoreHandler::Kind::kInterceptor:
      os << "kInterceptor";
      break;
    case StoreHandler::Kind::kSlow: {
      KeyedAccessStoreMode keyed_access_store_mode =
          StoreHandler::KeyedAccessStoreModeBits::decode(raw_handler);
      os << "kSlow, keyed access store mode = "
         << KeyedAccessStoreModeToString(keyed_access_store_mode);
      break;
    }
    case StoreHandler::Kind::kProxy:
      os << "kProxy";
      break;
    default:
      UNREACHABLE();
  }
}

}  // namespace

// static
void LoadHandler::PrintHandler(Object handler, std::ostream& os) {
  DisallowGarbageCollection no_gc;
  if (handler.IsSmi()) {
    int raw_handler = handler.ToSmi().value();
    os << "LoadHandler(Smi)(";
    PrintSmiLoadHandler(raw_handler, os);
    os << ")" << std::endl;
  } else {
    LoadHandler load_handler = LoadHandler::cast(handler);
    int raw_handler = load_handler.smi_handler().ToSmi().value();
    os << "LoadHandler(do access check on lookup start object = "
       << DoAccessCheckOnLookupStartObjectBits::decode(raw_handler)
       << ", lookup on lookup start object = "
       << LookupOnLookupStartObjectBits::decode(raw_handler) << ", ";
    PrintSmiLoadHandler(raw_handler, os);
    DCHECK_GE(load_handler.data_field_count(), 1);
    os << ", data1 = ";
    load_handler.data1().ShortPrint(os);
    if (load_handler.data_field_count() >= 2) {
      os << ", data2 = ";
      load_handler.data2().ShortPrint(os);
    }
    if (load_handler.data_field_count() >= 3) {
      os << ", data3 = ";
      load_handler.data3().ShortPrint(os);
    }
    os << ", validity cell = ";
    load_handler.validity_cell().ShortPrint(os);
    os << ")" << std::endl;
  }
}

void StoreHandler::PrintHandler(Object handler, std::ostream& os) {
  DisallowGarbageCollection no_gc;
  if (handler.IsSmi()) {
    int raw_handler = handler.ToSmi().value();
    os << "StoreHandler(Smi)(";
    PrintSmiStoreHandler(raw_handler, os);
    os << ")" << std::endl;
  } else {
    os << "StoreHandler(";
    StoreHandler store_handler = StoreHandler::cast(handler);
    if (store_handler.smi_handler().IsCode()) {
      Code code = Code::cast(store_handler.smi_handler());
      os << "builtin = ";
      code.ShortPrint(os);
    } else {
      int raw_handler = store_handler.smi_handler().ToSmi().value();
      os << "do access check on lookup start object = "
         << DoAccessCheckOnLookupStartObjectBits::decode(raw_handler)
         << ", lookup on lookup start object = "
         << LookupOnLookupStartObjectBits::decode(raw_handler) << ", ";
      PrintSmiStoreHandler(raw_handler, os);
    }
    DCHECK_GE(store_handler.data_field_count(), 1);
    os << ", data1 = ";
    store_handler.data1().ShortPrint(os);
    if (store_handler.data_field_count() >= 2) {
      os << ", data2 = ";
      store_handler.data2().ShortPrint(os);
    }
    if (store_handler.data_field_count() >= 3) {
      os << ", data3 = ";
      store_handler.data3().ShortPrint(os);
    }
    os << ", validity cell = ";
    store_handler.validity_cell().ShortPrint(os);
    os << ")" << std::endl;
  }
}
#endif  // defined(OBJECT_PRINT)

}  // namespace internal
}  // namespace v8
