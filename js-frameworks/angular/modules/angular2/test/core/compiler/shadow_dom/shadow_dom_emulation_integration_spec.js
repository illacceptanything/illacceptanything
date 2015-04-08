import {
  AsyncTestCompleter,
  beforeEach,
  ddescribe,
  describe,
  el,
  expect,
  iit,
  inject,
  it,
  xit,
} from 'angular2/test_lib';

import {StringMapWrapper, List} from 'angular2/src/facade/collection';
import {Type} from 'angular2/src/facade/lang';
import {DOM} from 'angular2/src/dom/dom_adapter';

import {Injector} from 'angular2/di';
import {Lexer, Parser, ChangeDetector, dynamicChangeDetection} from 'angular2/change_detection';
import {ExceptionHandler} from 'angular2/src/core/exception_handler';

import {Compiler, CompilerCache} from 'angular2/src/core/compiler/compiler';
import {LifeCycle} from 'angular2/src/core/life_cycle/life_cycle';
import {DirectiveMetadataReader} from 'angular2/src/core/compiler/directive_metadata_reader';
import {ShadowDomStrategy,
        NativeShadowDomStrategy,
        EmulatedScopedShadowDomStrategy,
        EmulatedUnscopedShadowDomStrategy,
} from 'angular2/src/core/compiler/shadow_dom_strategy';
import {TemplateLoader} from 'angular2/src/render/dom/compiler/template_loader';
import {ComponentUrlMapper} from 'angular2/src/core/compiler/component_url_mapper';
import {UrlResolver} from 'angular2/src/services/url_resolver';
import {StyleUrlResolver} from 'angular2/src/render/dom/shadow_dom/style_url_resolver';
import {StyleInliner} from 'angular2/src/render/dom/shadow_dom/style_inliner';

import {MockTemplateResolver} from 'angular2/src/mock/template_resolver_mock';

import {Decorator, Component, Viewport} from 'angular2/src/core/annotations/annotations';
import {Template} from 'angular2/src/core/annotations/template';

import {ViewContainer} from 'angular2/src/core/compiler/view_container';

import {BrowserDomAdapter} from 'angular2/src/dom/browser_adapter';

export function main() {
  BrowserDomAdapter.makeCurrent();
  describe('integration tests', function() {
    var urlResolver;
    var styleUrlResolver;
    var styleInliner;
    var strategies = {
      "scoped" : () => new EmulatedScopedShadowDomStrategy(styleInliner, styleUrlResolver, DOM.createElement('div')),
      "unscoped" : () => new EmulatedUnscopedShadowDomStrategy(styleUrlResolver, DOM.createElement('div'))
    }
    if (DOM.supportsNativeShadowDOM()) {
      StringMapWrapper.set(strategies, "native", () => new NativeShadowDomStrategy(styleUrlResolver));
    }

    StringMapWrapper.forEach(strategies,
      (strategyFactory, name) => {

      describe(`${name} shadow dom strategy`, () => {
        var compiler, tplResolver;

        beforeEach(() => {
          urlResolver = new UrlResolver();
          styleUrlResolver = new StyleUrlResolver(urlResolver);
          styleInliner = new StyleInliner(null, styleUrlResolver, urlResolver);
          tplResolver = new MockTemplateResolver();
          compiler = new Compiler(dynamicChangeDetection,
            new TemplateLoader(null, null),
            new DirectiveMetadataReader(),
            new Parser(new Lexer()),
            new CompilerCache(),
            strategyFactory(),
            tplResolver,
            new ComponentUrlMapper(),
            urlResolver
          );
        });

        function compile(template, directives: List<Type>, assertions) {
          tplResolver.setTemplate(MyComp, new Template({
            inline: template,
            directives: directives
          }));
          compiler.compile(MyComp)
            .then(createView)
            .then((view) => {
              var lc = new LifeCycle(new ExceptionHandler(), view.changeDetector, false);
              assertions(view, lc);
            });
        }

        it('should support simple components', inject([AsyncTestCompleter], (async) => {
          var temp = '<simple>' +
            '<div>A</div>' +
            '</simple>';

          compile(temp, [Simple], (view, lc) => {
            expect(view.nodes).toHaveText('SIMPLE(A)');

            async.done();
          });
        }));

        it('should support multiple content tags', inject([AsyncTestCompleter], (async) => {
          var temp = '<multiple-content-tags>' +
            '<div>B</div>' +
            '<div>C</div>' +
            '<div class="left">A</div>' +
          '</multiple-content-tags>';

          compile(temp, [MultipleContentTagsComponent], (view, lc) => {
            expect(view.nodes).toHaveText('(A, BC)');
            async.done();
          });
        }));

        it('should redistribute only direct children', inject([AsyncTestCompleter], (async) => {
          var temp = '<multiple-content-tags>' +
            '<div>B<div class="left">A</div></div>' +
            '<div>C</div>' +
            '</multiple-content-tags>';

          compile(temp, [MultipleContentTagsComponent], (view, lc) => {
            expect(view.nodes).toHaveText('(, BAC)');
            async.done();
          });
        }));

        it("should redistribute direct child viewcontainers when the light dom changes", inject([AsyncTestCompleter], (async) => {
          var temp = '<multiple-content-tags>' +
            '<div><div template="manual" class="left">A</div></div>' +
            '<div>B</div>' +
            '</multiple-content-tags>';

          compile(temp, [MultipleContentTagsComponent, ManualViewportDirective], (view, lc) => {
            var dir = view.elementInjectors[1].get(ManualViewportDirective);

            expect(view.nodes).toHaveText('(, B)');

            dir.show();
            lc.tick();

            expect(view.nodes).toHaveText('(, AB)');

            dir.hide();
            lc.tick();

            expect(view.nodes).toHaveText('(, B)');

            async.done();
          });
        }));

        it("should redistribute when the light dom changes", inject([AsyncTestCompleter], (async) => {
          var temp = '<multiple-content-tags>' +
            '<div template="manual" class="left">A</div>' +
            '<div>B</div>' +
            '</multiple-content-tags>';

          compile(temp, [MultipleContentTagsComponent, ManualViewportDirective], (view, lc) => {
            var dir = view.elementInjectors[1].get(ManualViewportDirective);

            expect(view.nodes).toHaveText('(, B)');

            dir.show();
            lc.tick();

            expect(view.nodes).toHaveText('(A, B)');

            dir.hide();
            lc.tick();

            expect(view.nodes).toHaveText('(, B)');

            async.done();
          });
        }));

        it("should support nested components", inject([AsyncTestCompleter], (async) => {
          var temp = '<outer-with-indirect-nested>' +
            '<div>A</div>' +
            '<div>B</div>' +
            '</outer-with-indirect-nested>';

          compile(temp, [OuterWithIndirectNestedComponent], (view, lc) => {
            expect(view.nodes).toHaveText('OUTER(SIMPLE(AB))');

            async.done();
          });
        }));

        it("should support nesting with content being direct child of a nested component", inject([AsyncTestCompleter], (async) => {
          var temp = '<outer>' +
            '<div template="manual" class="left">A</div>' +
            '<div>B</div>' +
            '<div>C</div>' +
            '</outer>';

          compile(temp, [OuterComponent, ManualViewportDirective], (view, lc) => {
            var dir = view.elementInjectors[1].get(ManualViewportDirective);

            expect(view.nodes).toHaveText('OUTER(INNER(INNERINNER(,BC)))');

            dir.show();
            lc.tick();

            expect(view.nodes).toHaveText('OUTER(INNER(INNERINNER(A,BC)))');
            async.done();
          });
        }));

        it('should redistribute when the shadow dom changes', inject([AsyncTestCompleter], (async) => {
          var temp = '<conditional-content>' +
            '<div class="left">A</div>' +
            '<div>B</div>' +
            '<div>C</div>' +
            '</conditional-content>';


          compile(temp, [ConditionalContentComponent, AutoViewportDirective], (view, lc) => {
            var cmp = view.elementInjectors[0].get(ConditionalContentComponent);

            expect(view.nodes).toHaveText('(, ABC)');

            cmp.showLeft();
            lc.tick();

            expect(view.nodes).toHaveText('(A, BC)');

            cmp.hideLeft();
            lc.tick();

            expect(view.nodes).toHaveText('(, ABC)');

            async.done();
          });
        }));

        //Implement once NgElement support changing a class
        //it("should redistribute when a class has been added or removed");
        //it('should not lose focus', () => {
        //  var temp = `<simple>aaa<input type="text" id="focused-input" ng-class="{'aClass' : showClass}"> bbb</simple>`;
        //
        //  compile(temp, (view, lc) => {
        //    var input = view.nodes[1];
        //    input.focus();
        //
        //    expect(document.activeElement.id).toEqual("focused-input");
        //
        //    // update class of input
        //
        //    expect(document.activeElement.id).toEqual("focused-input");
        //  });
        //});
      });
    });

  });
}

class TestDirectiveMetadataReader extends DirectiveMetadataReader {
  shadowDomStrategy;

  constructor(shadowDomStrategy) {
    super();
    this.shadowDomStrategy = shadowDomStrategy;
  }

  parseShadowDomStrategy(annotation:Component):ShadowDomStrategy{
    return this.shadowDomStrategy;
  }
}

@Viewport({
  selector: '[manual]'
})
class ManualViewportDirective {
  viewContainer;
  constructor(viewContainer:ViewContainer) {
    this.viewContainer = viewContainer;
  }

  show() { this.viewContainer.create(); }
  hide() { this.viewContainer.remove(0); }
}

@Viewport({
  selector: '[auto]',
  bind: {
    'auto': 'auto'
  }
})
class AutoViewportDirective {
  viewContainer;
  constructor(viewContainer:ViewContainer) {
    this.viewContainer = viewContainer;
  }

  set auto(newValue:boolean) {
    if (newValue) {
      this.viewContainer.create();
    } else {
      this.viewContainer.remove(0);
    }
  }
}

@Component({selector: 'simple'})
@Template({inline: 'SIMPLE(<content></content>)'})
class Simple {
}

@Component({selector: 'multiple-content-tags'})
@Template({
  inline: '(<content select=".left"></content>, <content></content>)'
})
class MultipleContentTagsComponent {
}


@Component({selector: 'conditional-content'})
@Template({
  inline: '<div>(<div *auto="cond"><content select=".left"></content></div>, <content></content>)</div>',
  directives: [AutoViewportDirective]
})
class ConditionalContentComponent  {
  cond:boolean;

  constructor() {
    this.cond = false;
  }

  showLeft() { this.cond = true; }
  hideLeft() { this.cond = false; }
}

@Component({selector: 'outer-with-indirect-nested'})
@Template({
  inline: 'OUTER(<simple><div><content></content></div></simple>)',
  directives: [Simple]
})
class OuterWithIndirectNestedComponent  {
}

@Component({selector: 'outer'})
@Template({
  inline: 'OUTER(<inner><content></content></inner>)',
  directives: [InnerComponent]
})
class OuterComponent {
}

@Component({selector: 'inner'})
@Template({
  inline: 'INNER(<innerinner><content></content></innerinner>)',
  directives: [InnerInnerComponent]
})
class InnerComponent {
}

@Component({selector: 'innerinner'})
@Template({
  inline: 'INNERINNER(<content select=".left"></content>,<content></content>)'
})
class InnerInnerComponent {
}


@Component({selector: 'my-comp'})
@Template({
  directives: [MultipleContentTagsComponent, ManualViewportDirective,
    ConditionalContentComponent, OuterWithIndirectNestedComponent, OuterComponent]
})
class MyComp {
}

function createView(pv) {
  var view = pv.instantiate(null, null);
  view.hydrate(new Injector([]), null, null, {}, null);
  return view;
}
