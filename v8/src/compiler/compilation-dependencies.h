// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_COMPILATION_DEPENDENCIES_H_
#define V8_COMPILER_COMPILATION_DEPENDENCIES_H_

#include "src/compiler/js-heap-broker.h"
#include "src/objects/objects.h"
#include "src/zone/zone-containers.h"

namespace v8 {
namespace internal {
namespace compiler {

class SlackTrackingPrediction {
 public:
  SlackTrackingPrediction(MapRef initial_map, int instance_size);

  int inobject_property_count() const { return inobject_property_count_; }
  int instance_size() const { return instance_size_; }

 private:
  int instance_size_;
  int inobject_property_count_;
};

class CompilationDependency;

// Collects and installs dependencies of the code that is being generated.
class V8_EXPORT_PRIVATE CompilationDependencies : public ZoneObject {
 public:
  CompilationDependencies(JSHeapBroker* broker, Zone* zone);

  V8_WARN_UNUSED_RESULT bool Commit(Handle<Code> code);

  // TODO(jgruber): Remove this method once GetPropertyAccessInfo no longer
  // uses the two-phase approach between serialization and compilation.
  void ClearForConcurrentGetPropertyAccessInfo() { dependencies_.clear(); }

  // Return the initial map of {function} and record the assumption that it
  // stays the initial map.
  MapRef DependOnInitialMap(const JSFunctionRef& function);

  // Return the "prototype" property of the given function and record the
  // assumption that it doesn't change.
  ObjectRef DependOnPrototypeProperty(const JSFunctionRef& function);

  // Record the assumption that {map} stays stable.
  void DependOnStableMap(const MapRef& map);

  // Depend on the fact that accessing property |property_name| from
  // |receiver_map| yields the constant value |constant|, which is held by
  // |holder|. Therefore, must be invalidated if |property_name| is added to any
  // of the objects between receiver and |holder| on the prototype chain, b) any
  // of the objects on the prototype chain up to |holder| change prototypes, or
  // c) the value of |property_name| in |holder| changes.
  // If PropertyKind is kData, |constant| is the value of the property in
  // question. In case of PropertyKind::kAccessor, |constant| is the accessor
  // function (i.e., getter or setter) itself, not the overall AccessorPair.
  void DependOnConstantInDictionaryPrototypeChain(const MapRef& receiver_map,
                                                  const NameRef& property_name,
                                                  const ObjectRef& constant,
                                                  PropertyKind kind);

  // Return the pretenure mode of {site} and record the assumption that it does
  // not change.
  AllocationType DependOnPretenureMode(const AllocationSiteRef& site);

  // Return a field's constness and, if kConst, record the assumption that it
  // remains kConst. The field is identified by the arguments.
  //
  // For arrays, arguments objects and value wrappers, only consider the field
  // kConst if the map is stable (and register stability dependency in that
  // case).  This is to ensure that fast elements kind transitions cannot be
  // used to mutate fields without deoptimization of the dependent code.
  PropertyConstness DependOnFieldConstness(const MapRef& map,
                                           InternalIndex descriptor);

  // Record the assumption that neither {cell}'s {CellType} changes, nor the
  // {IsReadOnly()} flag of {cell}'s {PropertyDetails}.
  void DependOnGlobalProperty(const PropertyCellRef& cell);

  // Return the validity of the given protector and, if true, record the
  // assumption that the protector remains valid.
  bool DependOnProtector(const PropertyCellRef& cell);

  // Convenience wrappers around {DependOnProtector}.
  bool DependOnArrayBufferDetachingProtector();
  bool DependOnArrayIteratorProtector();
  bool DependOnArraySpeciesProtector();
  bool DependOnNoElementsProtector();
  bool DependOnPromiseHookProtector();
  bool DependOnPromiseSpeciesProtector();
  bool DependOnPromiseThenProtector();

  // Record the assumption that {site}'s {ElementsKind} doesn't change.
  void DependOnElementsKind(const AllocationSiteRef& site);

  // For each given map, depend on the stability of (the maps of) all prototypes
  // up to (and including) the {last_prototype}.
  template <class MapContainer>
  void DependOnStablePrototypeChains(
      MapContainer const& receiver_maps, WhereToStart start,
      base::Optional<JSObjectRef> last_prototype =
          base::Optional<JSObjectRef>());

  // Like DependOnElementsKind but also applies to all nested allocation sites.
  void DependOnElementsKinds(const AllocationSiteRef& site);

  // Predict the final instance size for {function}'s initial map and record
  // the assumption that this prediction is correct. In addition, register
  // the initial map dependency. This method returns the {function}'s the
  // predicted minimum slack instance size count (wrapped together with
  // the corresponding in-object property count for convenience).
  SlackTrackingPrediction DependOnInitialMapInstanceSizePrediction(
      const JSFunctionRef& function);

  // Records {dependency} if not null.
  void RecordDependency(CompilationDependency const* dependency);

  // The methods below allow for gathering dependencies without actually
  // recording them. They can be recorded at a later time via RecordDependency
  // (or they can be ignored).

  // Gather the assumption that {target_map} can be transitioned to, i.e., that
  // it does not become deprecated.
  CompilationDependency const* TransitionDependencyOffTheRecord(
      const MapRef& target_map) const;

  // Gather the assumption that the field representation of a field does not
  // change. The field is identified by the arguments.
  CompilationDependency const* FieldRepresentationDependencyOffTheRecord(
      const MapRef& map, InternalIndex descriptor) const;

  // Gather the assumption that the field type of a field does not change. The
  // field is identified by the arguments.
  CompilationDependency const* FieldTypeDependencyOffTheRecord(
      const MapRef& map, InternalIndex descriptor) const;

 private:
  Zone* const zone_;
  JSHeapBroker* const broker_;
  ZoneForwardList<CompilationDependency const*> dependencies_;
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_COMPILATION_DEPENDENCIES_H_
