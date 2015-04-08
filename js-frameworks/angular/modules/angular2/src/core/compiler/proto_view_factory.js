import {List, ListWrapper, MapWrapper} from 'angular2/src/facade/collection';
import {isPresent, isBlank} from 'angular2/src/facade/lang';
import {reflector} from 'angular2/src/reflection/reflection';

import {ChangeDetection} from 'angular2/change_detection';
import {ShadowDomStrategy} from './shadow_dom_strategy';
import {Component, Viewport, DynamicComponent} from '../annotations/annotations';

import * as renderApi from 'angular2/src/render/api';
import {DirectDomProtoViewRef} from 'angular2/src/render/dom/direct_dom_renderer';
import {ProtoView} from './view';
import {ProtoElementInjector, DirectiveBinding} from './element_injector';

export class ProtoViewFactory {
  _changeDetection:ChangeDetection;
  _shadowDomStrategy:ShadowDomStrategy;

  constructor(changeDetection, shadowDomStrategy) {
    this._changeDetection = changeDetection;
    this._shadowDomStrategy = shadowDomStrategy;
  }

  createProtoView(componentBinding:DirectiveBinding, renderProtoView: renderApi.ProtoView, directives:List<DirectiveBinding>):ProtoView {
    var protoChangeDetector;
    if (isBlank(componentBinding)) {
      protoChangeDetector = this._changeDetection.createProtoChangeDetector('root', null);
    } else {
      var componentAnnotation:Component = componentBinding.annotation;
      protoChangeDetector = this._changeDetection.createProtoChangeDetector(
        'dummy', componentAnnotation.changeDetection
      );
    }
    var domProtoView = this._getDomProtoView(renderProtoView.render);
    var protoView = new ProtoView(renderProtoView.render, domProtoView.element, protoChangeDetector,
        this._shadowDomStrategy, null);

    for (var i=0; i<renderProtoView.elementBinders.length; i++) {
      var renderElementBinder = renderProtoView.elementBinders[i];
      var domElementBinder = domProtoView.elementBinders[i];
      var sortedDirectives = new SortedDirectives(renderElementBinder.directives, directives);
      var parentPeiWithDistance = this._findParentProtoElementInjectorWithDistance(
        i, protoView.elementBinders, renderProtoView.elementBinders
      );
      var protoElementInjector = this._createProtoElementInjector(
        i, parentPeiWithDistance,
        sortedDirectives, renderElementBinder
      );
      this._createElementBinder(
        protoView, renderElementBinder, domElementBinder, protoElementInjector, sortedDirectives
      );
      this._createDirectiveBinders(protoView, sortedDirectives);
    }
    MapWrapper.forEach(renderProtoView.variableBindings, (mappedName, varName) => {
      protoView.bindVariable(varName, mappedName);
    });
    return protoView;
  }

  // This method is needed to make DartAnalyzer happy
  _getDomProtoView(protoViewRef: DirectDomProtoViewRef) {
    return protoViewRef.delegate;
  }

  _findParentProtoElementInjectorWithDistance(binderIndex, elementBinders, renderElementBinders) {
    var distance = 0;
    do {
      var renderElementBinder = renderElementBinders[binderIndex];
      binderIndex = renderElementBinder.parentIndex;
      if (binderIndex !== -1) {
        distance += renderElementBinder.distanceToParent;
        var elementBinder = elementBinders[binderIndex];
        if (isPresent(elementBinder.protoElementInjector)) {
          return new ParentProtoElementInjectorWithDistance(elementBinder.protoElementInjector, distance);
        }
      }
    } while (binderIndex !== -1);
    return new ParentProtoElementInjectorWithDistance(null, -1);
  }

  _createProtoElementInjector(binderIndex, parentPeiWithDistance, sortedDirectives, renderElementBinder) {
    var protoElementInjector = null;
    // Create a protoElementInjector for any element that either has bindings *or* has one
    // or more var- defined. Elements with a var- defined need a their own element injector
    // so that, when hydrating, $implicit can be set to the element.
    var hasVariables = MapWrapper.size(renderElementBinder.variableBindings) > 0;
    if (sortedDirectives.directives.length > 0 || hasVariables) {
      protoElementInjector = new ProtoElementInjector(
          parentPeiWithDistance.protoElementInjector, binderIndex,
          sortedDirectives.directives,
          isPresent(sortedDirectives.componentDirective), parentPeiWithDistance.distance
      );
      protoElementInjector.attributes = renderElementBinder.readAttributes;
      // Viewport directives are treated differently than other element with var- definitions.
      if (hasVariables && !isPresent(sortedDirectives.viewportDirective)) {
        protoElementInjector.exportComponent = isPresent(sortedDirectives.componentDirective);
        protoElementInjector.exportElement = isBlank(sortedDirectives.componentDirective);

        // experiment
        var exportImplicitName = MapWrapper.get(renderElementBinder.variableBindings, '\$implicit');
        if (isPresent(exportImplicitName)) {
          protoElementInjector.exportImplicitName = exportImplicitName;
        }
      }
    }
    return protoElementInjector;
  }

  _createElementBinder(protoView, renderElementBinder, domElementBinder, protoElementInjector, sortedDirectives) {
    var parent = null;
    if (renderElementBinder.parentIndex !== -1) {
      parent = protoView.elementBinders[renderElementBinder.parentIndex];
    }
    var elBinder = protoView.bindElement(
      parent,
      renderElementBinder.distanceToParent,
      protoElementInjector,
      sortedDirectives.componentDirective,
      sortedDirectives.viewportDirective
    );
    elBinder.contentTagSelector = domElementBinder.contentTagSelector;
    // text nodes
    for (var i=0; i<renderElementBinder.textBindings.length; i++) {
      protoView.bindTextNode(domElementBinder.textNodeIndices[i], renderElementBinder.textBindings[i].ast);
    }
    // element properties
    MapWrapper.forEach(renderElementBinder.propertyBindings, (astWithSource, propertyName) => {
      protoView.bindElementProperty(astWithSource.ast, propertyName, MapWrapper.get(domElementBinder.propertySetters, propertyName));
    });
    // events
    MapWrapper.forEach(renderElementBinder.eventBindings, (astWithSource, eventName) => {
      protoView.bindEvent(eventName, astWithSource.ast, -1);
    });
    // variables
    // The view's locals needs to have a full set of variable names at construction time
    // in order to prevent new variables from being set later in the lifecycle. Since we don't want
    // to actually create variable bindings for the $implicit bindings, add to the
    // protoLocals manually.
    MapWrapper.forEach(renderElementBinder.variableBindings, (mappedName, varName) => {
      MapWrapper.set(protoView.protoLocals, mappedName, null);
    });
    return elBinder;
  }

  _createDirectiveBinders(protoView, sortedDirectives) {
    for (var i=0; i<sortedDirectives.renderDirectives.length; i++) {
      var renderDirectiveMetadata = sortedDirectives.renderDirectives[i];
      // directive properties
      MapWrapper.forEach(renderDirectiveMetadata.propertyBindings, (astWithSource, propertyName) => {
        // TODO: these setters should eventually be created by change detection, to make
        // it monomorphic!
        var setter = reflector.setter(propertyName);
        protoView.bindDirectiveProperty(i, astWithSource.ast, propertyName, setter);
      });
      // directive events
      MapWrapper.forEach(renderDirectiveMetadata.eventBindings, (astWithSource, eventName) => {
        protoView.bindEvent(eventName, astWithSource.ast, i);
      });
    }
  }

}

class SortedDirectives {
  componentDirective: DirectiveBinding;
  viewportDirective: DirectiveBinding;
  renderDirectives: List<renderApi.DirectiveMetadata>;
  directives: List<DirectiveBinding>;

  constructor(renderDirectives, allDirectives) {
    this.renderDirectives = [];
    this.directives = [];
    this.viewportDirective = null;
    this.componentDirective = null;
    ListWrapper.forEach(renderDirectives, (renderDirectiveMetadata) => {
      var directiveBinding = allDirectives[renderDirectiveMetadata.directiveIndex];
      if ((directiveBinding.annotation instanceof Component) || (directiveBinding.annotation instanceof DynamicComponent)) {
        // component directives need to be the first binding in ElementInjectors!
        this.componentDirective = directiveBinding;
        ListWrapper.insert(this.renderDirectives, 0, renderDirectiveMetadata);
        ListWrapper.insert(this.directives, 0, directiveBinding);
      } else {
        if (directiveBinding.annotation instanceof Viewport) {
          this.viewportDirective = directiveBinding;
        }
        ListWrapper.push(this.renderDirectives, renderDirectiveMetadata);
        ListWrapper.push(this.directives, directiveBinding);
      }
    });

  }
}

class ParentProtoElementInjectorWithDistance {
  protoElementInjector:ProtoElementInjector;
  distance:number;
  constructor(protoElementInjector:ProtoElementInjector, distance:number) {
    this.protoElementInjector = protoElementInjector;
    this.distance = distance;
  }
}
