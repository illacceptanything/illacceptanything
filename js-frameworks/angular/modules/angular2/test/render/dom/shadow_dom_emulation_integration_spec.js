import {
  AsyncTestCompleter,
  beforeEach,
  ddescribe,
  describe,
  el,
  elementText,
  expect,
  iit,
  inject,
  it,
  xit,
  SpyObject,
} from 'angular2/test_lib';

import {MapWrapper, ListWrapper, StringMapWrapper} from 'angular2/src/facade/collection';
import {DOM} from 'angular2/src/dom/dom_adapter';

import {
  ProtoView, Template, ViewContainerRef, DirectiveMetadata
} from 'angular2/src/render/api';

import {EmulatedScopedShadowDomStrategy} from 'angular2/src/render/dom/shadow_dom/emulated_scoped_shadow_dom_strategy';
import {EmulatedUnscopedShadowDomStrategy} from 'angular2/src/render/dom/shadow_dom/emulated_unscoped_shadow_dom_strategy';
import {NativeShadowDomStrategy} from 'angular2/src/render/dom/shadow_dom/native_shadow_dom_strategy';
import {UrlResolver} from 'angular2/src/services/url_resolver';
import {StyleUrlResolver} from 'angular2/src/render/dom/shadow_dom/style_url_resolver';
import {StyleInliner} from 'angular2/src/render/dom/shadow_dom/style_inliner';

import {IntegrationTestbed} from './integration_testbed';

export function main() {
  describe('ShadowDom integration tests', function() {
    var urlResolver, styleUrlResolver, styleInliner;
    var strategies = {
      "scoped" : () => new EmulatedScopedShadowDomStrategy(styleInliner, styleUrlResolver, null),
      "unscoped" : () => new EmulatedUnscopedShadowDomStrategy(styleUrlResolver, null)
    }
    if (DOM.supportsNativeShadowDOM()) {
      StringMapWrapper.set(strategies, "native", () => new NativeShadowDomStrategy(styleUrlResolver));
    }

    beforeEach( () => {
      urlResolver = new UrlResolver();
      styleUrlResolver = new StyleUrlResolver(urlResolver);
      styleInliner = new StyleInliner(null, styleUrlResolver, urlResolver);
    });


    StringMapWrapper.forEach(strategies,
      (strategyFactory, name) => {

      describe(`${name} shadow dom strategy`, () => {

        var testbed, renderer, rootEl, compile;

        function createRenderer({templates}) {
          testbed = new IntegrationTestbed({
            shadowDomStrategy: strategyFactory(),
            templates: ListWrapper.concat(templates, componentTemplates)
          });
          renderer = testbed.renderer;
          compile = (rootEl, componentId) => testbed.compile(rootEl, componentId);
        }

        beforeEach( () => {
          rootEl = el('<div></div>');
        });

        it('should support simple components', inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<simple>' +
                '<div>A</div>' +
                '</simple>',
              directives: [simple]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            renderer.createView(pv.render);

            expect(rootEl).toHaveText('SIMPLE(A)');

            async.done();
          });
        }));

        it('should support multiple content tags', inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<multiple-content-tags>' +
                  '<div>B</div>' +
                  '<div>C</div>' +
                  '<div class="left">A</div>' +
                '</multiple-content-tags>',
              directives: [multipleContentTagsComponent]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            renderer.createView(pv.render);

            expect(rootEl).toHaveText('(A, BC)');

            async.done();
          });
        }));

        it('should redistribute only direct children', inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<multiple-content-tags>' +
                '<div>B<div class="left">A</div></div>' +
                '<div>C</div>' +
                '</multiple-content-tags>',
              directives: [multipleContentTagsComponent]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            renderer.createView(pv.render);

            expect(rootEl).toHaveText('(, BAC)');

            async.done();
          });
        }));

        it("should redistribute direct child viewcontainers when the light dom changes", inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<multiple-content-tags>' +
                '<div><div template="manual" class="left">A</div></div>' +
                '<div>B</div>' +
                '</multiple-content-tags>',
              directives: [multipleContentTagsComponent, manualViewportDirective]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            var viewRefs = renderer.createView(pv.render);
            var vcRef = new ViewContainerRef(viewRefs[1], 1);
            var vcProtoViewRef = pv.elementBinders[0].nestedProtoView
              .elementBinders[1].nestedProtoView.render;
            var childViewRef = renderer.createView(vcProtoViewRef)[0];

            expect(rootEl).toHaveText('(, B)');

            renderer.insertViewIntoContainer(vcRef, childViewRef);

            expect(rootEl).toHaveText('(, AB)');

            renderer.detachViewFromContainer(vcRef, 0);

            expect(rootEl).toHaveText('(, B)');

            async.done();
          });
        }));

        it("should redistribute when the light dom changes", inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<multiple-content-tags>' +
                '<div template="manual" class="left">A</div>' +
                '<div>B</div>' +
                '</multiple-content-tags>',
              directives: [multipleContentTagsComponent, manualViewportDirective]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            var viewRefs = renderer.createView(pv.render);
            var vcRef = new ViewContainerRef(viewRefs[1], 1);
            var vcProtoViewRef = pv.elementBinders[0].nestedProtoView
              .elementBinders[1].nestedProtoView.render;
            var childViewRef = renderer.createView(vcProtoViewRef)[0];

            expect(rootEl).toHaveText('(, B)');

            renderer.insertViewIntoContainer(vcRef, childViewRef);

            expect(rootEl).toHaveText('(A, B)');

            renderer.detachViewFromContainer(vcRef, 0);

            expect(rootEl).toHaveText('(, B)');

            async.done();
          });
        }));

        it("should support nested components", inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<outer-with-indirect-nested>' +
                '<div>A</div>' +
                '<div>B</div>' +
                '</outer-with-indirect-nested>',
              directives: [outerWithIndirectNestedComponent]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            renderer.createView(pv.render);

            expect(rootEl).toHaveText('OUTER(SIMPLE(AB))');

            async.done();
          });
        }));

        it("should support nesting with content being direct child of a nested component", inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<outer>' +
                '<div template="manual" class="left">A</div>' +
                '<div>B</div>' +
                '<div>C</div>' +
                '</outer>',
              directives: [outerComponent, manualViewportDirective]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            var viewRefs = renderer.createView(pv.render);
            var vcRef = new ViewContainerRef(viewRefs[1], 1);
            var vcProtoViewRef = pv.elementBinders[0].nestedProtoView
              .elementBinders[1].nestedProtoView.render;
            var childViewRef = renderer.createView(vcProtoViewRef)[0];

            expect(rootEl).toHaveText('OUTER(INNER(INNERINNER(,BC)))');

            renderer.insertViewIntoContainer(vcRef, childViewRef);

            expect(rootEl).toHaveText('OUTER(INNER(INNERINNER(A,BC)))');
            async.done();
          });
        }));

        it('should redistribute when the shadow dom changes', inject([AsyncTestCompleter], (async) => {
          createRenderer({
            templates: [new Template({
              componentId: 'main',
              inline: '<conditional-content>' +
                '<div class="left">A</div>' +
                '<div>B</div>' +
                '<div>C</div>' +
                '</conditional-content>',
              directives: [conditionalContentComponent]
            })]
          });
          compile(rootEl, 'main').then( (pv) => {
            var viewRefs = renderer.createView(pv.render);
            var vcRef = new ViewContainerRef(viewRefs[2], 0);
            var vcProtoViewRef = pv.elementBinders[0].nestedProtoView
              .elementBinders[0].nestedProtoView
              .elementBinders[0].nestedProtoView.render;
            var childViewRef = renderer.createView(vcProtoViewRef)[0];

            expect(rootEl).toHaveText('(, ABC)');

            renderer.insertViewIntoContainer(vcRef, childViewRef);

            expect(rootEl).toHaveText('(A, BC)');

            renderer.detachViewFromContainer(vcRef, 0);

            expect(rootEl).toHaveText('(, ABC)');

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


var simple = new DirectiveMetadata({
  selector: 'simple',
  id: 'simple',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var multipleContentTagsComponent = new DirectiveMetadata({
  selector: 'multiple-content-tags',
  id: 'multiple-content-tags',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var manualViewportDirective = new DirectiveMetadata({
  selector: '[manual]',
  id: 'manual',
  type: DirectiveMetadata.VIEWPORT_TYPE
});

var outerWithIndirectNestedComponent = new DirectiveMetadata({
  selector: 'outer-with-indirect-nested',
  id: 'outer-with-indirect-nested',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var outerComponent = new DirectiveMetadata({
  selector: 'outer',
  id: 'outer',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var innerComponent = new DirectiveMetadata({
  selector: 'inner',
  id: 'inner',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var innerInnerComponent = new DirectiveMetadata({
  selector: 'innerinner',
  id: 'innerinner',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var conditionalContentComponent = new DirectiveMetadata({
  selector: 'conditional-content',
  id: 'conditional-content',
  type: DirectiveMetadata.COMPONENT_TYPE
});

var autoViewportDirective = new DirectiveMetadata({
  selector: '[auto]',
  id: '[auto]',
  type: DirectiveMetadata.VIEWPORT_TYPE
});

var componentTemplates = [
  new Template({
    componentId: 'simple',
    inline: 'SIMPLE(<content></content>)',
    directives: []
  }),
  new Template({
    componentId: 'multiple-content-tags',
    inline: '(<content select=".left"></content>, <content></content>)',
    directives: []
  }),
  new Template({
    componentId: 'outer-with-indirect-nested',
    inline: 'OUTER(<simple><div><content></content></div></simple>)',
    directives: [simple]
  }),
  new Template({
    componentId: 'outer',
    inline: 'OUTER(<inner><content></content></inner>)',
    directives: [innerComponent]
  }),
  new Template({
    componentId: 'inner',
    inline: 'INNER(<innerinner><content></content></innerinner>)',
    directives: [innerInnerComponent]
  }),
  new Template({
    componentId: 'innerinner',
    inline: 'INNERINNER(<content select=".left"></content>,<content></content>)',
    directives: []
  }),
  new Template({
    componentId: 'conditional-content',
    inline: '<div>(<div *auto="cond"><content select=".left"></content></div>, <content></content>)</div>',
    directives: [autoViewportDirective]
  })
];
