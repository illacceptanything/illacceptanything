import {bind} from 'angular2/di';

import {Compiler, CompilerCache} from 'angular2/src/core/compiler/compiler';
import {Reflector, reflector} from 'angular2/src/reflection/reflection';
import {Parser, Lexer, ChangeDetection, dynamicChangeDetection} from 'angular2/change_detection';
import {ExceptionHandler} from 'angular2/src/core/exception_handler';
import {TemplateLoader} from 'angular2/src/render/dom/compiler/template_loader';
import {TemplateResolver} from 'angular2/src/core/compiler/template_resolver';
import {DirectiveMetadataReader} from 'angular2/src/core/compiler/directive_metadata_reader';
import {ShadowDomStrategy, EmulatedUnscopedShadowDomStrategy} from 'angular2/src/core/compiler/shadow_dom_strategy';
import {XHR} from 'angular2/src/services/xhr';
import {ComponentUrlMapper} from 'angular2/src/core/compiler/component_url_mapper';
import {UrlResolver} from 'angular2/src/services/url_resolver';
import {StyleUrlResolver} from 'angular2/src/render/dom/shadow_dom/style_url_resolver';
import {StyleInliner} from 'angular2/src/render/dom/shadow_dom/style_inliner';
import {VmTurnZone} from 'angular2/src/core/zone/vm_turn_zone';

import {DOM} from 'angular2/src/dom/dom_adapter';

import {appDocumentToken} from 'angular2/src/core/application_tokens';

import {EventManager, DomEventsPlugin} from 'angular2/src/render/dom/events/event_manager';

import {MockTemplateResolver} from 'angular2/src/mock/template_resolver_mock';
import {MockXHR} from 'angular2/src/mock/xhr_mock';
import {MockVmTurnZone} from 'angular2/src/mock/vm_turn_zone_mock';

import {TestBed} from './test_bed';

import {Injector} from 'angular2/di';

import {List, ListWrapper} from 'angular2/src/facade/collection';
import {FunctionWrapper} from 'angular2/src/facade/lang';

/**
 * Returns the root injector bindings.
 *
 * This must be kept in sync with the _rootBindings in application.js
 *
 * @returns {*[]}
 */
function _getRootBindings() {
  return [
    bind(Reflector).toValue(reflector),
  ];
}

/**
 * Returns the application injector bindings.
 *
 * This must be kept in sync with _injectorBindings() in application.js
 *
 * @returns {*[]}
 */
function _getAppBindings() {
  var appDoc;

  // The document is only available in browser environment
  try {
    appDoc = DOM.defaultDoc();
  } catch(e) {
    appDoc = null;
  }

  return [
    bind(appDocumentToken).toValue(appDoc),
    bind(ShadowDomStrategy).toFactory(
        (styleUrlResolver, doc) => new EmulatedUnscopedShadowDomStrategy(styleUrlResolver, doc.head),
        [StyleUrlResolver, appDocumentToken]),
    Compiler,
    CompilerCache,
    bind(TemplateResolver).toClass(MockTemplateResolver),
    bind(ChangeDetection).toValue(dynamicChangeDetection),
    TemplateLoader,
    DirectiveMetadataReader,
    Parser,
    Lexer,
    ExceptionHandler,
    bind(XHR).toClass(MockXHR),
    ComponentUrlMapper,
    UrlResolver,
    StyleUrlResolver,
    StyleInliner,
    TestBed,
    bind(VmTurnZone).toClass(MockVmTurnZone),
    bind(EventManager).toFactory((zone) => {
      var plugins = [
        new DomEventsPlugin(),
      ];
      return new EventManager(plugins, zone);
    }, [VmTurnZone]),
  ];
}

export function createTestInjector(bindings: List) {
  var rootInjector = new Injector(_getRootBindings());
  return rootInjector.createChild(ListWrapper.concat(_getAppBindings(), bindings));
}

/**
 * Allows injecting dependencies in `beforeEach()` and `it()`.
 *
 * Example:
 *
 * ```
 * beforeEach(inject([Dependency, AClass], (dep, object) => {
 *   // some code that uses `dep` and `object`
 *   // ...
 * }));
 *
 * it('...', inject([AClass, AsyncTestCompleter], (object, async) => {
 *   object.doSomething().then(() => {
 *     expect(...);
 *     async.done();
 *   });
 * })
 * ```
 *
 * Notes:
 * - injecting an `AsyncTestCompleter` allow completing async tests - this is the equivalent of
 *   adding a `done` parameter in Jasmine,
 * - inject is currently a function because of some Traceur limitation the syntax should eventually
 *   becomes `it('...', @Inject (object: AClass, async: AsyncTestCompleter) => { ... });`
 *
 * @param {Array} tokens
 * @param {Function} fn
 * @return {FunctionWithParamTokens}
 */
export function inject(tokens: List, fn: Function) {
  return new FunctionWithParamTokens(tokens, fn);
}

export class FunctionWithParamTokens {
  _tokens: List;
  _fn: Function;

  constructor(tokens: List, fn: Function) {
    this._tokens = tokens;
    this._fn = fn;
  }

  execute(injector: Injector) {
    var params = ListWrapper.map(this._tokens, (t) => injector.get(t));
    FunctionWrapper.apply(this._fn, params);
  }
}

