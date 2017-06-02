import {isPresent, isBlank, BaseException} from 'angular2/src/facade/lang';
import {ListWrapper, MapWrapper, Set, SetWrapper} from 'angular2/src/facade/collection';
import {DOM} from 'angular2/src/dom/dom_adapter';

import {
  ASTWithSource, AST, AstTransformer, AccessMember, LiteralArray, ImplicitReceiver
} from 'angular2/change_detection';
import {SetterFn} from 'angular2/src/reflection/types';

import {ProtoView} from './proto_view';
import {ElementBinder} from './element_binder';
import {setterFactory} from './property_setter_factory';

import * as api from '../../api';
import * as directDomRenderer from '../direct_dom_renderer';

import {NG_BINDING_CLASS} from '../util';

export class ProtoViewBuilder {
  rootElement;
  variableBindings: Map<string, string>;
  elements:List<ElementBinderBuilder>;
  isRootView:boolean;

  constructor(rootElement) {
    this.rootElement = rootElement;
    this.elements = [];
    this.isRootView = false;
    this.variableBindings = MapWrapper.create();
  }

  bindElement(element, description = null):ElementBinderBuilder {
    var builder = new ElementBinderBuilder(this.elements.length, element, description);
    ListWrapper.push(this.elements, builder);
    DOM.addClass(element, NG_BINDING_CLASS);

    return builder;
  }

  bindVariable(name, value) {
    // Store the variable map from value to variable, reflecting how it will be used later by
    // View. When a local is set to the view, a lookup for the variable name will take place keyed
    // by the "value", or exported identifier. For example, ng-repeat sets a view local of "index".
    // When this occurs, a lookup keyed by "index" must occur to find if there is a var referencing
    // it.
    MapWrapper.set(this.variableBindings, value, name);
  }

  setIsRootView(value) {
    this.isRootView = value;
  }

  build():api.ProtoView {
    var renderElementBinders = [];

    var apiElementBinders = [];
    ListWrapper.forEach(this.elements, (ebb) => {
      var propertySetters = MapWrapper.create();
      var eventLocalsAstSplitter = new EventLocalsAstSplitter();
      var apiDirectiveBinders = ListWrapper.map(ebb.directives, (db) => {
        return new api.DirectiveBinder({
          directiveIndex: db.directiveIndex,
          propertyBindings: db.propertyBindings,
          eventBindings: eventLocalsAstSplitter.splitEventAstIntoLocals(db.eventBindings)
        });
      });
      MapWrapper.forEach(ebb.propertySetters, (setter, propertyName) => {
        MapWrapper.set(propertySetters, propertyName, setter);
      });
      var nestedProtoView =
          isPresent(ebb.nestedProtoView) ? ebb.nestedProtoView.build() : null;
      var parentIndex = isPresent(ebb.parent) ? ebb.parent.index : -1;
      ListWrapper.push(apiElementBinders, new api.ElementBinder({
        index: ebb.index, parentIndex:parentIndex, distanceToParent:ebb.distanceToParent,
        directives: apiDirectiveBinders,
        nestedProtoView: nestedProtoView,
        propertyBindings: ebb.propertyBindings, variableBindings: ebb.variableBindings,
        eventBindings: eventLocalsAstSplitter.splitEventAstIntoLocals(ebb.eventBindings),
        textBindings: ebb.textBindings,
        readAttributes: ebb.readAttributes
      }));
      ListWrapper.push(renderElementBinders, new ElementBinder({
        textNodeIndices: ebb.textBindingIndices,
        contentTagSelector: ebb.contentTagSelector,
        parentIndex: parentIndex,
        distanceToParent: ebb.distanceToParent,
        nestedProtoView: isPresent(nestedProtoView) ? nestedProtoView.render.delegate : null,
        componentId: ebb.componentId,
        eventLocals: eventLocalsAstSplitter.buildEventLocals(),
        eventNames: eventLocalsAstSplitter.buildEventNames(),
        propertySetters: propertySetters
      }));
    });
    return new api.ProtoView({
      render: new directDomRenderer.DirectDomProtoViewRef(new ProtoView({
        element: this.rootElement,
        elementBinders: renderElementBinders,
        isRootView: this.isRootView
      })),
      elementBinders: apiElementBinders,
      variableBindings: this.variableBindings
    });
  }
}

export class ElementBinderBuilder {
  element;
  index:number;
  parent:ElementBinderBuilder;
  distanceToParent:number;
  directives:List<DirectiveBuilder>;
  nestedProtoView:ProtoViewBuilder;
  propertyBindings: Map<string, ASTWithSource>;
  variableBindings: Map<string, string>;
  eventBindings: Map<string, ASTWithSource>;
  textBindingIndices: List<number>;
  textBindings: List<ASTWithSource>;
  contentTagSelector:string;
  propertySetters: Map<string, SetterFn>;
  readAttributes: Map<string, string>;
  componentId: string;

  constructor(index, element, description) {
    this.element = element;
    this.index = index;
    this.parent = null;
    this.distanceToParent = 0;
    this.directives = [];
    this.nestedProtoView = null;
    this.propertyBindings = MapWrapper.create();
    this.variableBindings = MapWrapper.create();
    this.eventBindings = MapWrapper.create();
    this.textBindings = [];
    this.textBindingIndices = [];
    this.contentTagSelector = null;
    this.propertySetters = MapWrapper.create();
    this.componentId = null;
    this.readAttributes = MapWrapper.create();
  }

  setParent(parent:ElementBinderBuilder, distanceToParent):ElementBinderBuilder {
    this.parent = parent;
    if (isPresent(parent)) {
      this.distanceToParent = distanceToParent;
    }
    return this;
  }

  readAttribute(attrName:string) {
    if (isBlank(MapWrapper.get(this.readAttributes, attrName))) {
      MapWrapper.set(this.readAttributes, attrName, DOM.getAttribute(this.element, attrName));
    }
  }

  bindDirective(directiveIndex:number):DirectiveBuilder {
    var directive = new DirectiveBuilder(directiveIndex);
    ListWrapper.push(this.directives, directive);
    return directive;
  }

  bindNestedProtoView(rootElement):ProtoViewBuilder {
    if (isPresent(this.nestedProtoView)) {
      throw new BaseException('Only one nested view per element is allowed');
    }
    this.nestedProtoView = new ProtoViewBuilder(rootElement);
    return this.nestedProtoView;
  }

  bindProperty(name, expression) {
    MapWrapper.set(this.propertyBindings, name, expression);
    this.bindPropertySetter(name);
  }

  bindPropertySetter(name) {
    MapWrapper.set(this.propertySetters, name, setterFactory(name));
  }

  bindVariable(name, value) {
    // When current is a view root, the variable bindings are set to the *nested* proto view.
    // The root view conceptually signifies a new "block scope" (the nested view), to which
    // the variables are bound.
    if (isPresent(this.nestedProtoView)) {
      this.nestedProtoView.bindVariable(name, value);
    } else {
      // Store the variable map from value to variable, reflecting how it will be used later by
      // View. When a local is set to the view, a lookup for the variable name will take place keyed
      // by the "value", or exported identifier. For example, ng-repeat sets a view local of "index".
      // When this occurs, a lookup keyed by "index" must occur to find if there is a var referencing
      // it.
      MapWrapper.set(this.variableBindings, value, name);
    }
  }

  bindEvent(name, expression) {
    MapWrapper.set(this.eventBindings, name, expression);
  }

  bindText(index, expression) {
    ListWrapper.push(this.textBindingIndices, index);
    ListWrapper.push(this.textBindings, expression);
  }

  setContentTagSelector(value:string) {
    this.contentTagSelector = value;
  }

  setComponentId(componentId:string) {
    this.componentId = componentId;
  }
}

export class DirectiveBuilder {
  directiveIndex:number;
  propertyBindings: Map<string, ASTWithSource>;
  eventBindings: Map<string, ASTWithSource>;

  constructor(directiveIndex) {
    this.directiveIndex = directiveIndex;
    this.propertyBindings = MapWrapper.create();
    this.eventBindings = MapWrapper.create();
  }

  bindProperty(name, expression) {
    MapWrapper.set(this.propertyBindings, name, expression);
  }

  bindEvent(name, expression) {
    MapWrapper.set(this.eventBindings, name, expression);
  }
}

export class EventLocalsAstSplitter extends AstTransformer {
  locals:List<AST>;
  eventNames:List<string>;
  _implicitReceiver:AST;

  constructor() {
    super();
    this.locals = [];
    this.eventNames = [];
    this._implicitReceiver = new ImplicitReceiver();
  }

  splitEventAstIntoLocals(eventBindings:Map<string, ASTWithSource>):Map<string, ASTWithSource> {
    // TODO(tbosch): reenable this when we are using
    // the render views
    return eventBindings;
    // if (isPresent(eventBindings)) {
    //   var result = MapWrapper.create();
    //   MapWrapper.forEach(eventBindings, (astWithSource, eventName) => {
    //     var adjustedAst = astWithSource.ast.visit(this);
    //     MapWrapper.set(result, eventName, new ASTWithSource(adjustedAst, astWithSource.source, ''));
    //     ListWrapper.push(this.eventNames, eventName);
    //   });
    //   return result;
    // }
    // return null;
  }

  visitAccessMember(ast:AccessMember) {
    var isEventAccess = false;
    var current = ast;
    while (!isEventAccess && (current instanceof AccessMember)) {
      if (current.name == '$event') {
        isEventAccess = true;
      }
      current = current.receiver;
    }

    if (isEventAccess) {
      ListWrapper.push(this.locals, ast);
      var index = this.locals.length - 1;
      return new AccessMember(this._implicitReceiver, `${index}`, (arr) => arr[index], null);
    } else {
      return ast;
    }
  }

  buildEventLocals() {
    return new LiteralArray(this.locals);
  }

  buildEventNames() {
    return this.eventNames;
  }
}