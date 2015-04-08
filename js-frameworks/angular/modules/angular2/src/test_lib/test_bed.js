import {Injector} from 'angular2/di';

import {Type, isPresent, BaseException} from 'angular2/src/facade/lang';
import {Promise} from 'angular2/src/facade/async';
import {isBlank} from 'angular2/src/facade/lang';
import {List} from 'angular2/src/facade/collection';

import {Template} from 'angular2/src/core/annotations/template';

import {TemplateResolver} from 'angular2/src/core/compiler/template_resolver';
import {Compiler} from 'angular2/src/core/compiler/compiler';
import {View} from 'angular2/src/core/compiler/view';

import {EventManager} from 'angular2/src/render/dom/events/event_manager';

import {queryView} from './utils';
import {instantiateType, getTypeOf} from './lang_utils';

export class TestBed {
  _injector: Injector;

  constructor(injector: Injector) {
    this._injector = injector;
  }

  /**
   * Overrides the [Template] of a [Component].
   *
   * @see setInlineTemplate() to only override the html
   *
   * @param {Type} component
   * @param {Template} template
   */
  overrideTemplate(component: Type, template: Template): void {
    this._injector.get(TemplateResolver).setTemplate(component, template);
  }

  /**
   * Overrides only the html of a [Component].
   * All the other propoerties of the component's [Template] are preserved.
   *
   * @param {Type} component
   * @param {string} html
   */
  setInlineTemplate(component: Type, html: string): void {
    this._injector.get(TemplateResolver).setInlineTemplate(component, html);
  }

  /**
   * Overrides the directives from the component [Template].
   *
   * @param {Type} component
   * @param {Type} from
   * @param {Type} to
   */
  overrideDirective(component: Type, from: Type, to: Type): void {
    this._injector.get(TemplateResolver).overrideTemplateDirective(component, from, to);
  }

  /**
   * Creates a [View] for the given component.
   *
   * Only either a component or a context needs to be specified but both can be provided for
   * advanced use cases (ie subclassing the context).
   *
   * @param {Type} component
   * @param {*} context
   * @param {string} html Use as the component template when specified (shortcut for setInlineTemplate)
   * @return {Promise<ViewProxy>}
   */
  createView(component: Type,
             {context = null, html = null}: {context:any, html: string} = {}): Promise<View> {

    if (isBlank(component) && isBlank(context)) {
      throw new BaseException('You must specified at least a component or a context');
    }

    if (isBlank(component)) {
      component = getTypeOf(context);
    } else if (isBlank(context)) {
      context = instantiateType(component);
    }

    if (isPresent(html)) {
      this.setInlineTemplate(component, html);
    }

    return this._injector.get(Compiler).compile(component).then((pv) => {
      var eventManager = this._injector.get(EventManager);
      var view = pv.instantiate(null, eventManager);
      view.hydrate(this._injector, null, null, context, null);
      return new ViewProxy(view);
    });
  }
}

/**
 * Proxy to [View] return by [TestBed.createView] which offers a high level API for tests.
 */
export class ViewProxy {
  _view: View;

  constructor(view: View) {
    this._view = view;
  }

  get context(): any {
    return this._view.context;
  }

  get nodes(): List {
    return this._view.nodes;
  }

  detectChanges(): void {
    this._view.changeDetector.detectChanges();
  }

  querySelector(selector) {
    return queryView(this._view, selector);
  }

  /**
   * @returns {View} return the underlying [View].
   *
   * Prefer using the other methods which hide implementation details.
   */
  get rawView(): View {
    return this._view;
  }
}
