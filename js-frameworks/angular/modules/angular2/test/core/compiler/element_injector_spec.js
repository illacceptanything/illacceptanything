import {describe, ddescribe, it, iit, xit, xdescribe, expect, beforeEach, SpyObject, proxy, el} from 'angular2/test_lib';
import {isBlank, isPresent, IMPLEMENTS} from 'angular2/src/facade/lang';
import {ListWrapper, MapWrapper, List, StringMapWrapper} from 'angular2/src/facade/collection';
import {DOM} from 'angular2/src/dom/dom_adapter';
import {ProtoElementInjector, PreBuiltObjects, DirectiveBinding} from 'angular2/src/core/compiler/element_injector';
import {Parent, Ancestor} from 'angular2/src/core/annotations/visibility';
import {EventEmitter, PropertySetter, Attribute} from 'angular2/src/core/annotations/di';
import {onDestroy} from 'angular2/src/core/annotations/annotations';
import {Optional, Injector, Inject, bind} from 'angular2/di';
import {ProtoView, View} from 'angular2/src/core/compiler/view';
import {ViewContainer} from 'angular2/src/core/compiler/view_container';
import {NgElement} from 'angular2/src/core/dom/element';
import {LightDom, DestinationLightDom} from 'angular2/src/core/compiler/shadow_dom_emulation/light_dom';
import {Directive} from 'angular2/src/core/annotations/annotations';
import {BindingPropagationConfig} from 'angular2/change_detection';

class DummyDirective extends Directive {
  constructor({lifecycle} = {}) { super({lifecycle: lifecycle}); }
}

@proxy
@IMPLEMENTS(View)
class DummyView extends SpyObject {noSuchMethod(m){super.noSuchMethod(m)}}

@proxy
@IMPLEMENTS(LightDom)
class DummyLightDom extends SpyObject {noSuchMethod(m){super.noSuchMethod(m)}}


class SimpleDirective {
}


class SomeOtherDirective {
}

class NeedsDirective {
  dependency:SimpleDirective;
  constructor(dependency:SimpleDirective){
    this.dependency = dependency;
  }
}

class OptionallyNeedsDirective {
  dependency:SimpleDirective;
  constructor(@Optional() dependency:SimpleDirective){
    this.dependency = dependency;
  }
}

class NeedDirectiveFromParent {
  dependency:SimpleDirective;
  constructor(@Parent() dependency:SimpleDirective){
    this.dependency = dependency;
  }
}

class NeedDirectiveFromAncestor {
  dependency:SimpleDirective;
  constructor(@Ancestor() dependency:SimpleDirective){
    this.dependency = dependency;
  }
}

class NeedsService {
  service:any;
  constructor(@Inject("service") service) {
    this.service = service;
  }
}

class NeedsEventEmitter {
  clickEmitter;
  constructor(@EventEmitter('click') clickEmitter:Function) {
    this.clickEmitter = clickEmitter;
  }
  click() {
    this.clickEmitter(null);
  }
}

class NeedsEventEmitterNoType {
  clickEmitter;
  constructor(@EventEmitter('click') clickEmitter) {
    this.clickEmitter = clickEmitter;
  }
  click() {
    this.clickEmitter(null);
  }
}

class NeedsPropertySetter {
  propSetter;
  roleSetter;
  classSetter;
  classWithDashSetter;
  styleSetter;
  unitSetter;
  constructor(@PropertySetter('title') propSetter: Function, @PropertySetter('attr.role') roleSetter: Function,
    @PropertySetter('class.active') classSetter: Function, @PropertySetter('class.foo-bar') classWithDashSetter: Function,
    @PropertySetter('style.width') styleSetter: Function, @PropertySetter('style.height.px') unitSetter: Function) {
    this.propSetter = propSetter;
    this.roleSetter = roleSetter;
    this.classSetter = classSetter;
    this.classWithDashSetter = classWithDashSetter;
    this.styleSetter = styleSetter;
    this.unitSetter = unitSetter;
  }
  setProp(value) {
    this.propSetter(value);
  }
  setRole(value) {
    this.roleSetter(value);
  }
  setClass(value) {
    this.classSetter(value);
  }
  setStyle(value) {
    this.styleSetter(value);
  }
  setStyleWithUnit(value) {
    this.unitSetter(value);
  }
}

class NeedsPropertySetterNoType {
  propSetter;
  constructor(@PropertySetter('title') propSetter) {
    this.propSetter = propSetter;
  }

  setProp(value) {
    this.propSetter(value);
  }
}

class NeedsAttribute {
  typeAttribute;
  titleAttribute;
  fooAttribute;
  constructor(@Attribute('type') typeAttribute: string, @Attribute('title') titleAttribute: string, @Attribute('foo') fooAttribute: string) {
    this.typeAttribute = typeAttribute;
    this.titleAttribute = titleAttribute;
    this.fooAttribute = fooAttribute;
  }
}

class NeedsAttributeNoType {
  fooAttribute;
  constructor(@Attribute('foo') fooAttribute) {
    this.fooAttribute = fooAttribute;
  }
}

class A_Needs_B {
  constructor(dep){}
}

class B_Needs_A {
  constructor(dep){}
}

class NeedsView {
  view:any;
  constructor(@Inject(View) view) {
    this.view = view;
  }
}

class DirectiveWithDestroy {
  onDestroyCounter:number;

  constructor(){
    this.onDestroyCounter = 0;
  }

  onDestroy() {
    this.onDestroyCounter ++;
  }
}

export function main() {
  var defaultPreBuiltObjects = new PreBuiltObjects(null, null, null, null);
  var appInjector = new Injector([]);

  function humanize(tree, names:List) {
    var lookupName = (item) =>
      ListWrapper.last(
        ListWrapper.find(names, (pair) => pair[0] === item));

    if (tree.children.length == 0) return lookupName(tree);
    var children = tree.children.map(m => humanize(m, names));
    return [lookupName(tree), children];
  }

  function injector(bindings, lightDomAppInjector = null, shadowDomAppInjector = null, preBuiltObjects = null, attributes = null) {
    if (isBlank(lightDomAppInjector)) lightDomAppInjector = appInjector;

    var proto = new ProtoElementInjector(null, 0, bindings, isPresent(shadowDomAppInjector));
    proto.attributes = attributes;
    var inj = proto.instantiate(null, null);
    var preBuilt = isPresent(preBuiltObjects) ? preBuiltObjects : defaultPreBuiltObjects;

    inj.instantiateDirectives(lightDomAppInjector, shadowDomAppInjector, preBuilt);
    return inj;
  }

  function parentChildInjectors(parentBindings, childBindings, parentPreBuildObjects = null) {
    if (isBlank(parentPreBuildObjects)) parentPreBuildObjects = defaultPreBuiltObjects;

    var inj = new Injector([]);

    var protoParent = new ProtoElementInjector(null, 0, parentBindings);
    var parent = protoParent.instantiate(null, null);

    parent.instantiateDirectives(inj, null, parentPreBuildObjects);

    var protoChild = new ProtoElementInjector(protoParent, 1, childBindings, false, 1);
    var child = protoChild.instantiate(parent, null);
    child.instantiateDirectives(inj, null, defaultPreBuiltObjects);

    return child;
  }

  function hostShadowInjectors(hostBindings, shadowBindings, hostPreBuildObjects = null) {
    if (isBlank(hostPreBuildObjects)) hostPreBuildObjects = defaultPreBuiltObjects;

    var inj = new Injector([]);
    var shadowInj = inj.createChild([]);

    var protoParent = new ProtoElementInjector(null, 0, hostBindings, true);
    var host = protoParent.instantiate(null, null);
    host.instantiateDirectives(inj, shadowInj, hostPreBuildObjects);

    var protoChild = new ProtoElementInjector(protoParent, 0, shadowBindings, false, 1);
    var shadow = protoChild.instantiate(null, host);
    shadow.instantiateDirectives(shadowInj, null, null);

    return shadow;
  }

  describe("ProtoElementInjector", () => {
    describe("direct parent", () => {
      it("should return parent proto injector when distance is 1", () => {
        var distance = 1;
        var protoParent = new ProtoElementInjector(null, 0, []);
        var protoChild = new ProtoElementInjector(protoParent, 1, [], false, distance);

        expect(protoChild.directParent()).toEqual(protoParent);
      });

      it("should return null otherwise", () => {
        var distance = 2;
        var protoParent = new ProtoElementInjector(null, 0, []);
        var protoChild = new ProtoElementInjector(protoParent, 1, [], false, distance);

        expect(protoChild.directParent()).toEqual(null);
      });

      it("should allow for direct access using getDirectiveBindingAtIndex", function () {
        var binding = DirectiveBinding.createFromBinding(
          bind(SimpleDirective).toClass(SimpleDirective), null);
        var proto = new ProtoElementInjector(null, 0, [binding]);

        expect(proto.getDirectiveBindingAtIndex(0)).toBeAnInstanceOf(DirectiveBinding);
        expect(() => proto.getDirectiveBindingAtIndex(-1)).toThrowError(
          'Index -1 is out-of-bounds.');
        expect(() => proto.getDirectiveBindingAtIndex(10)).toThrowError(
          'Index 10 is out-of-bounds.');
      });
    });
  });

  describe("ElementInjector", function () {
    describe("instantiate", function () {
      it("should create an element injector", function () {
        var protoParent = new ProtoElementInjector(null, 0, []);
        var protoChild1 = new ProtoElementInjector(protoParent, 1, []);
        var protoChild2 = new ProtoElementInjector(protoParent, 2, []);

        var p = protoParent.instantiate(null, null);
        var c1 = protoChild1.instantiate(p, null);
        var c2 = protoChild2.instantiate(p, null);

        expect(humanize(p, [
          [p, 'parent'],
          [c1, 'child1'],
          [c2, 'child2']
        ])).toEqual(["parent", ["child1", "child2"]]);
      });

      describe("direct parent", () => {
        it("should return parent injector when distance is 1", () => {
          var distance = 1;
          var protoParent = new ProtoElementInjector(null, 0, []);
          var protoChild = new ProtoElementInjector(protoParent, 1, [], false, distance);

          var p = protoParent.instantiate(null, null);
          var c = protoChild.instantiate(p, null);

          expect(c.directParent()).toEqual(p);
        });

        it("should return null otherwise", () => {
          var distance = 2;
          var protoParent = new ProtoElementInjector(null, 0, []);
          var protoChild = new ProtoElementInjector(protoParent, 1, [], false, distance);

          var p = protoParent.instantiate(null, null);
          var c = protoChild.instantiate(p, null);

          expect(c.directParent()).toEqual(null);
        });
      });
    });

    describe("hasBindings", function () {
      it("should be true when there are bindings", function () {
        var p = new ProtoElementInjector(null, 0, [SimpleDirective]);
        expect(p.hasBindings).toBeTruthy();
      });

      it("should be false otherwise", function () {
        var p = new ProtoElementInjector(null, 0, []);
        expect(p.hasBindings).toBeFalsy();
      });
    });

    describe("hasInstances", function () {
      it("should be false when no directives are instantiated", function () {
        expect(injector([]).hasInstances()).toBe(false);
      });

      it("should be true when directives are instantiated", function () {
        expect(injector([SimpleDirective]).hasInstances()).toBe(true);
      });
    });

    describe("instantiateDirectives", function () {
      it("should instantiate directives that have no dependencies", function () {
        var inj = injector([SimpleDirective]);
        expect(inj.get(SimpleDirective)).toBeAnInstanceOf(SimpleDirective);
      });

      it("should instantiate directives that depend on other directives", function () {
        var inj = injector([SimpleDirective, NeedsDirective]);

        var d = inj.get(NeedsDirective);

        expect(d).toBeAnInstanceOf(NeedsDirective);
        expect(d.dependency).toBeAnInstanceOf(SimpleDirective);
      });

      it("should instantiate directives that depend on app services", function () {
        var appInjector = new Injector([
          bind("service").toValue("service")
        ]);
        var inj = injector([NeedsService], appInjector);

        var d = inj.get(NeedsService);
        expect(d).toBeAnInstanceOf(NeedsService);
        expect(d.service).toEqual("service");
      });

      it("should instantiate directives that depend on pre built objects", function () {
        var view = new DummyView();
        var inj = injector([NeedsView], null, null, new PreBuiltObjects(view, null, null, null));

        expect(inj.get(NeedsView).view).toBe(view);
      });

      it("should instantiate directives that depend on the containing component", function () {
        var shadow = hostShadowInjectors([SimpleDirective], [NeedsDirective]);

        var d = shadow.get(NeedsDirective);
        expect(d).toBeAnInstanceOf(NeedsDirective);
        expect(d.dependency).toBeAnInstanceOf(SimpleDirective);
      });

      it("should not instantiate directives that depend on other directives in the containing component's ElementInjector", () => {
        expect( () => {
          hostShadowInjectors([SomeOtherDirective, SimpleDirective], [NeedsDirective]);
        }).toThrowError('No provider for SimpleDirective! (NeedsDirective -> SimpleDirective)')
      });

      it("should instantiate component directives that depend on app services in the shadow app injector", () => {
        var shadowAppInjector = new Injector([
          bind("service").toValue("service")
        ]);
        var inj = injector([NeedsService], null, shadowAppInjector);

        var d = inj.get(NeedsService);
        expect(d).toBeAnInstanceOf(NeedsService);
        expect(d.service).toEqual("service");
      });

      it("should not instantiate other directives that depend on app services in the shadow app injector", () => {
        var shadowAppInjector = new Injector([
          bind("service").toValue("service")
        ]);
        expect( () => {
          injector([SomeOtherDirective, NeedsService], null, shadowAppInjector);
        }).toThrowError('No provider for service! (NeedsService -> service)');
      });

      it("should return app services", function () {
        var appInjector = new Injector([
          bind("service").toValue("service")
        ]);
        var inj = injector([], appInjector);

        expect(inj.get('service')).toEqual('service');
      });

      it("should get directives from parent", function () {
        var child = parentChildInjectors([SimpleDirective], [NeedDirectiveFromParent]);

        var d = child.get(NeedDirectiveFromParent);

        expect(d).toBeAnInstanceOf(NeedDirectiveFromParent);
        expect(d.dependency).toBeAnInstanceOf(SimpleDirective);
      });

      it("should not return parent's directives on self", function () {
        expect(() => {
          injector([SimpleDirective, NeedDirectiveFromParent]);
        }).toThrowError();
      });

      it("should get directives from ancestor", function () {
        var child = parentChildInjectors([SimpleDirective], [NeedDirectiveFromAncestor]);

        var d = child.get(NeedDirectiveFromAncestor);

        expect(d).toBeAnInstanceOf(NeedDirectiveFromAncestor);
        expect(d.dependency).toBeAnInstanceOf(SimpleDirective);
      });

      it("should throw when no SimpleDirective found", function () {
        expect(() => injector([NeedDirectiveFromParent])).
            toThrowError('No provider for SimpleDirective! (NeedDirectiveFromParent -> SimpleDirective)');
      });

      it("should inject null when no directive found", function () {
        var inj = injector([OptionallyNeedsDirective]);
        var d = inj.get(OptionallyNeedsDirective);
        expect(d.dependency).toEqual(null);
      });

      it("should accept SimpleDirective bindings instead of SimpleDirective types", function () {
        var inj = injector([
          DirectiveBinding.createFromBinding(bind(SimpleDirective).toClass(SimpleDirective), null)
        ]);
        expect(inj.get(SimpleDirective)).toBeAnInstanceOf(SimpleDirective);
      });

      it("should allow for direct access using getDirectiveAtIndex", function () {
        var inj = injector([
          DirectiveBinding.createFromBinding(bind(SimpleDirective).toClass(SimpleDirective), null)
        ]);
        expect(inj.getDirectiveAtIndex(0)).toBeAnInstanceOf(SimpleDirective);
        expect(() => inj.getDirectiveAtIndex(-1)).toThrowError(
          'Index -1 is out-of-bounds.');
        expect(() => inj.getDirectiveAtIndex(10)).toThrowError(
          'Index 10 is out-of-bounds.');
      });

      it("should handle cyclic dependencies", function () {
        expect(() => {
          var bAneedsB = bind(A_Needs_B).toFactory((a) => new A_Needs_B(a), [B_Needs_A]);
          var bBneedsA = bind(B_Needs_A).toFactory((a) => new B_Needs_A(a), [A_Needs_B]);
          injector([
            DirectiveBinding.createFromBinding(bAneedsB, null),
            DirectiveBinding.createFromBinding(bBneedsA, null)
          ]);
        }).toThrowError('Cannot instantiate cyclic dependency! ' +
          '(A_Needs_B -> B_Needs_A -> A_Needs_B)');
      });

      it("should call onDestroy on directives subscribed to this event", function() {
        var inj = injector([DirectiveBinding.createFromType(
            DirectiveWithDestroy,
            new DummyDirective({lifecycle: [onDestroy]}))]);
        var destroy = inj.get(DirectiveWithDestroy);
        inj.clearDirectives();
        expect(destroy.onDestroyCounter).toBe(1);
      });
    });

    describe("pre built objects", function () {
      it("should return view", function () {
        var view = new DummyView();
        var inj = injector([], null, null, new PreBuiltObjects(view, null, null, null));

        expect(inj.get(View)).toEqual(view);
      });

      it("should return element", function () {
        var element = new NgElement(null);
        var inj = injector([], null, null, new PreBuiltObjects(null, element, null, null));

        expect(inj.get(NgElement)).toEqual(element);
      });

      it('should return viewContainer', function () {
        var viewContainer = new ViewContainer(null, null, null, null, null);
        var inj = injector([], null, null, new PreBuiltObjects(null, null, viewContainer, null));

        expect(inj.get(ViewContainer)).toEqual(viewContainer);
      });

      it('should return bindingPropagationConfig', function () {
        var config = new BindingPropagationConfig(null);
        var inj = injector([], null, null, new PreBuiltObjects(null, null, null, config));

        expect(inj.get(BindingPropagationConfig)).toEqual(config);
      });
    });

    describe("createPrivateComponent", () => {
      it("should create a private component", () => {
        var inj = injector([]);
        inj.createPrivateComponent(SimpleDirective, null);
        expect(inj.getPrivateComponent()).toBeAnInstanceOf(SimpleDirective);
      });

      it("should inject parent dependencies into the private component", () => {
        var inj = parentChildInjectors([SimpleDirective], []);
        inj.createPrivateComponent(NeedDirectiveFromAncestor, null);
        expect(inj.getPrivateComponent()).toBeAnInstanceOf(NeedDirectiveFromAncestor);
        expect(inj.getPrivateComponent().dependency).toBeAnInstanceOf(SimpleDirective);
      });

      it("should not inject the proxy component into the children of the private component", () => {
        var injWithPrivateComponent = injector([SimpleDirective]);
        injWithPrivateComponent.createPrivateComponent(SomeOtherDirective, null);

        var shadowDomProtoInjector = new ProtoElementInjector(null, 0, [NeedDirectiveFromAncestor], false);
        var shadowDomInj = shadowDomProtoInjector.instantiate(null, injWithPrivateComponent);

        expect(() => shadowDomInj.instantiateDirectives(appInjector, null, defaultPreBuiltObjects)).
          toThrowError(new RegExp("No provider for SimpleDirective"));
      });

      it("should inject the private component into the children of the private component", () => {
        var injWithPrivateComponent = injector([]);
        injWithPrivateComponent.createPrivateComponent(SimpleDirective, null);

        var shadowDomProtoInjector = new ProtoElementInjector(null, 0, [NeedDirectiveFromAncestor], false);
        var shadowDomInjector = shadowDomProtoInjector.instantiate(null, injWithPrivateComponent);
        shadowDomInjector.instantiateDirectives(appInjector, null, defaultPreBuiltObjects);

        expect(shadowDomInjector.get(NeedDirectiveFromAncestor)).toBeAnInstanceOf(NeedDirectiveFromAncestor);
        expect(shadowDomInjector.get(NeedDirectiveFromAncestor).dependency).toBeAnInstanceOf(SimpleDirective);
      });

      it("should support rehydrating the private component", () => {
        var inj = injector([]);
        inj.createPrivateComponent(
            DirectiveWithDestroy,
            new DummyDirective({lifecycle: [onDestroy]}));
        var dir = inj.getPrivateComponent();

        inj.clearDirectives();

        expect(inj.getPrivateComponent()).toBe(null);
        expect(dir.onDestroyCounter).toBe(1);

        inj.instantiateDirectives(null, null, null);

        expect(inj.getPrivateComponent()).not.toBe(null);
      });
    });

    describe('event emitters', () => {

      function createpreBuildObject(eventName, eventHandler) {
        var handlers = StringMapWrapper.create();
        StringMapWrapper.set(handlers, eventName, eventHandler);
        var pv = new ProtoView(null, null, null, null);
        pv.eventHandlers = [handlers];
        var view = new View(pv, null, MapWrapper.create());
        return new PreBuiltObjects(view, null, null, null);
      }

      it('should be injectable and callable', () => {
        var called = false;
        var preBuildObject = createpreBuildObject('click', (e, view) => { called = true;});
        var inj = injector([NeedsEventEmitter], null, null, preBuildObject);
        inj.get(NeedsEventEmitter).click();
        expect(called).toEqual(true);
      });

      it('should be injectable and callable without specifying param type annotation', () => {
        var called = false;
        var preBuildObject = createpreBuildObject('click', (e, view) => { called = true;});
        var inj = injector([NeedsEventEmitterNoType], null, null, preBuildObject);
        inj.get(NeedsEventEmitterNoType).click();
        expect(called).toEqual(true);
      });

      it('should be queryable through hasEventEmitter', () => {
        var inj = injector([NeedsEventEmitter]);
        expect(inj.hasEventEmitter('click')).toBe(true);
        expect(inj.hasEventEmitter('move')).toBe(false);
      });

      it('should be queryable through hasEventEmitter without specifying param type annotation', () => {
        var inj = injector([NeedsEventEmitterNoType]);
        expect(inj.hasEventEmitter('click')).toBe(true);
        expect(inj.hasEventEmitter('move')).toBe(false);
      });
    });

    describe('property setter', () => {
      it('should be injectable and callable', () => {
        var div = el('<div></div>');
        var ngElement = new NgElement(div);

        var preBuildObject = new PreBuiltObjects(null, ngElement, null, null);
        var inj = injector([NeedsPropertySetter], null, null, preBuildObject);
        var component = inj.get(NeedsPropertySetter);
        component.setProp('foobar');
        component.setRole('button');
        component.setClass(true);
        component.classWithDashSetter(true);
        component.setStyle('40px');
        component.setStyleWithUnit(50);

        expect(div.title).toEqual('foobar');
        expect(DOM.getAttribute(div, 'role')).toEqual('button');
        expect(DOM.hasClass(div, 'active')).toEqual(true);
        expect(DOM.hasClass(div, 'foo-bar')).toEqual(true);
        expect(DOM.getStyle(div, 'width')).toEqual('40px');
        expect(DOM.getStyle(div, 'height')).toEqual('50px');
      });

      it('should be injectable and callable without specifying param type annotation', () => {
        var div = el('<div></div>');
        var preBuildObject = new PreBuiltObjects(null, new NgElement(div), null, null);
        var inj = injector([NeedsPropertySetterNoType], null, null, preBuildObject);
        var component = inj.get(NeedsPropertySetterNoType);
        component.setProp('foobar');

        expect(div.title).toEqual('foobar');
      });
    });

    describe('static attributes', () => {
      it('should be injectable', () => {
        var attributes = MapWrapper.create();
        MapWrapper.set(attributes, 'type', 'text');
        MapWrapper.set(attributes, 'title', '');

        var inj = injector([NeedsAttribute], null, null, null, attributes);
        var needsAttribute = inj.get(NeedsAttribute);

        expect(needsAttribute.typeAttribute).toEqual('text');
        expect(needsAttribute.titleAttribute).toEqual('');
        expect(needsAttribute.fooAttribute).toEqual(null);
      });

      it('should be injectable without type annotation', () => {
        var attributes = MapWrapper.create();
        MapWrapper.set(attributes, 'foo', 'bar');

        var inj = injector([NeedsAttributeNoType], null, null, null, attributes);
        var needsAttribute = inj.get(NeedsAttributeNoType);

        expect(needsAttribute.fooAttribute).toEqual('bar');
      });
    });

  });
}
