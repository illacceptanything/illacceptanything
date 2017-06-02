import {isBlank, isPresent, BaseException} from 'angular2/src/facade/lang';
import {MapWrapper, ListWrapper, List} from 'angular2/src/facade/collection';
import {PromiseWrapper, Promise} from 'angular2/src/facade/async';
import {DOM} from 'angular2/src/dom/dom_adapter';

import {Parser, Lexer} from 'angular2/change_detection';
import {DirectDomRenderer} from 'angular2/src/render/dom/direct_dom_renderer';
import {Compiler} from 'angular2/src/render/dom/compiler/compiler';
import {ProtoViewRef, ProtoView, Template, ViewContainerRef, EventDispatcher, DirectiveMetadata} from 'angular2/src/render/api';
import {DefaultStepFactory} from 'angular2/src/render/dom/compiler/compile_step_factory';
import {TemplateLoader} from 'angular2/src/render/dom/compiler/template_loader';
import {UrlResolver} from 'angular2/src/services/url_resolver';
import {EmulatedUnscopedShadowDomStrategy} from 'angular2/src/render/dom/shadow_dom/emulated_unscoped_shadow_dom_strategy';
import {EventManager, EventManagerPlugin} from 'angular2/src/render/dom/events/event_manager';
import {VmTurnZone} from 'angular2/src/core/zone/vm_turn_zone';
import {StyleUrlResolver} from 'angular2/src/render/dom/shadow_dom/style_url_resolver';
import {ViewFactory} from 'angular2/src/render/dom/view/view_factory';

export class IntegrationTestbed {
  renderer;
  parser;
  eventPlugin;
  _templates:Map<string, Template>;

  constructor({urlData, viewCacheCapacity, shadowDomStrategy, templates}) {
    this._templates = MapWrapper.create();
    if (isPresent(templates)) {
      ListWrapper.forEach(templates, (template) => {
        MapWrapper.set(this._templates, template.componentId, template);
      });
    }
    var parser = new Parser(new Lexer());
    var urlResolver = new UrlResolver();
    if (isBlank(shadowDomStrategy)) {
      shadowDomStrategy = new EmulatedUnscopedShadowDomStrategy(new StyleUrlResolver(urlResolver), null);
    }
    var compiler = new Compiler(new DefaultStepFactory(parser, shadowDomStrategy), new FakeTemplateLoader(urlResolver, urlData));

    if (isBlank(viewCacheCapacity)) {
      viewCacheCapacity = 1;
    }
    if (isBlank(urlData)) {
      urlData = MapWrapper.create();
    }
    this.eventPlugin = new FakeEventManagerPlugin();
    var eventManager = new EventManager([this.eventPlugin], new FakeVmTurnZone());
    var viewFactory = new ViewFactory(viewCacheCapacity, eventManager, shadowDomStrategy);
    this.renderer = new DirectDomRenderer(compiler, viewFactory, shadowDomStrategy);
  }

  compile(rootEl, componentId):Promise<ProtoView> {
    return this.renderer.createRootProtoView(rootEl, componentId).then( (rootProtoView) => {
      return this._compileNestedProtoViews(rootProtoView, [
        new DirectiveMetadata({
          type: DirectiveMetadata.COMPONENT_TYPE,
          id: componentId
        })
      ]);
    });
  }

  _compile(template):Promise<ProtoView> {
    return this.renderer.compile(template).then( (protoView) => {
      return this._compileNestedProtoViews(protoView, template.directives);
    });
  }

  _compileNestedProtoViews(protoView, directives):Promise<ProtoView> {
    var childComponentRenderPvRefs = [];
    var nestedPVPromises = [];
    ListWrapper.forEach(protoView.elementBinders, (elementBinder) => {
      var nestedComponentId = null;
      ListWrapper.forEach(elementBinder.directives, (db) => {
        var directiveMeta = directives[db.directiveIndex];
        if (directiveMeta.type === DirectiveMetadata.COMPONENT_TYPE) {
          nestedComponentId = directiveMeta.id;
        }
      });
      var nestedCall;
      if (isPresent(nestedComponentId)) {
        var childTemplate = MapWrapper.get(this._templates, nestedComponentId);
        if (isBlank(childTemplate)) {
          throw new BaseException(`Could not find template for ${nestedComponentId}!`);
        }
        nestedCall = this._compile(childTemplate);
      } else if (isPresent(elementBinder.nestedProtoView)) {
        nestedCall = this._compileNestedProtoViews(elementBinder.nestedProtoView, directives);
      }
      if (isPresent(nestedCall)) {
        ListWrapper.push(
          nestedPVPromises,
          nestedCall.then( (nestedPv) => {
            elementBinder.nestedProtoView = nestedPv;
            if (isPresent(nestedComponentId)) {
              ListWrapper.push(childComponentRenderPvRefs, nestedPv.render);
            }
          })
        );
      }
    });
    if (nestedPVPromises.length > 0) {
      return PromiseWrapper.all(nestedPVPromises).then((_) => {
        this.renderer.mergeChildComponentProtoViews(protoView.render, childComponentRenderPvRefs);
        return protoView;
      });
    } else {
      return PromiseWrapper.resolve(protoView);
    }
  }

}


class FakeTemplateLoader extends TemplateLoader {
  _urlData: Map<string, string>;

  constructor(urlResolver, urlData) {
    super(null, urlResolver);
    this._urlData = urlData;
  }

  load(template: Template) {
    if (isPresent(template.inline)) {
      return PromiseWrapper.resolve(DOM.createTemplate(template.inline));
    }

    if (isPresent(template.absUrl)) {
      var content = this._urlData[template.absUrl];
      if (isPresent(content)) {
        return PromiseWrapper.resolve(DOM.createTemplate(content));
      }
    }

    return PromiseWrapper.reject('Load failed');
  }
}

export class FakeVmTurnZone extends VmTurnZone {
  constructor() {
    super({enableLongStackTrace: false});
  }

  run(fn) {
    fn();
  }

  runOutsideAngular(fn) {
    fn();
  }
}

export class FakeEventManagerPlugin extends EventManagerPlugin {
  _eventHandlers: Map;

  constructor() {
    super();
    this._eventHandlers = MapWrapper.create();
  }

  dispatchEvent(eventName, event) {
    MapWrapper.get(this._eventHandlers, eventName)(event);
  }

  supports(eventName: string): boolean {
    return true;
  }

  addEventListener(element, eventName: string, handler: Function, shouldSupportBubble: boolean) {
    MapWrapper.set(this._eventHandlers, eventName, handler);
  }
}

export class LoggingEventDispatcher extends EventDispatcher {
  log:List;
  constructor() {
    super();
    this.log = [];
  }
  dispatchEvent(
    elementIndex:number, eventName:string, locals:List<any>
  ) {
    ListWrapper.push(this.log, [elementIndex, eventName, locals]);
  }
}

export class FakeEvent {
  target;
  constructor(target) {
    this.target = target;
  }
}