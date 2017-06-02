import {
  AsyncTestCompleter,
  beforeEach,
  ddescribe,
  describe,
  el,
  expect,
  iit,
  inject,
  IS_DARTIUM,
  it,
} from 'angular2/test_lib';

import {DOM} from 'angular2/src/dom/dom_adapter';
import {List, ListWrapper, Map, MapWrapper, StringMapWrapper} from 'angular2/src/facade/collection';
import {Type, isBlank, stringify, isPresent} from 'angular2/src/facade/lang';
import {PromiseWrapper, Promise} from 'angular2/src/facade/async';

import {Compiler, CompilerCache} from 'angular2/src/render/dom/compiler/compiler';
import {ProtoView, Template} from 'angular2/src/render/api';
import {CompileElement} from 'angular2/src/render/dom/compiler/compile_element';
import {CompileStep} from 'angular2/src/render/dom/compiler/compile_step'
import {CompileStepFactory} from 'angular2/src/render/dom/compiler/compile_step_factory';
import {CompileControl} from 'angular2/src/render/dom/compiler/compile_control';
import {TemplateLoader} from 'angular2/src/render/dom/compiler/template_loader';

import {UrlResolver} from 'angular2/src/services/url_resolver';

export function runCompilerCommonTests() {
  describe('compiler', function() {
    var mockStepFactory;

    function createCompiler(processClosure, urlData = null) {
      if (isBlank(urlData)) {
        urlData = MapWrapper.create();
      }
      var tplLoader =  new FakeTemplateLoader(urlData);
      mockStepFactory = new MockStepFactory([new MockStep(processClosure)]);
      return new Compiler(mockStepFactory, tplLoader);
    }

    it('should run the steps and build the ProtoView of the root element', inject([AsyncTestCompleter], (async) => {
      var compiler = createCompiler((parent, current, control) => {
        current.inheritedProtoView.bindVariable('b', 'a');
      });
      compiler.compile(new Template({
        componentId: 'someComponent',
        inline: '<div></div>'
      })).then( (protoView) => {
        expect(protoView.variableBindings).toEqual(MapWrapper.createFromStringMap({
          'a': 'b'
        }));
        async.done();
      });
    }));

    it('should use the inline template and compile in sync', inject([AsyncTestCompleter], (async) => {
      var compiler = createCompiler(EMPTY_STEP);
      compiler.compile(new Template({
        componentId: 'someId',
        inline: 'inline component'
      })).then( (protoView) => {
        expect(DOM.getInnerHTML(protoView.render.delegate.element)).toEqual('inline component');
        async.done();
      });
    }));

    it('should load url templates', inject([AsyncTestCompleter], (async) => {
      var urlData = MapWrapper.createFromStringMap({
        'someUrl': 'url component'
      });
      var compiler = createCompiler(EMPTY_STEP, urlData);
      compiler.compile(new Template({
        componentId: 'someId',
        absUrl: 'someUrl'
      })).then( (protoView) => {
        expect(DOM.getInnerHTML(protoView.render.delegate.element)).toEqual('url component');
        async.done();
      });
    }));

    it('should report loading errors', inject([AsyncTestCompleter], (async) => {
      var compiler = createCompiler(EMPTY_STEP, MapWrapper.create());
      PromiseWrapper.catchError(compiler.compile(new Template({
        componentId: 'someId',
        absUrl: 'someUrl'
      })), (e) => {
        expect(e.message).toContain(`Failed to load the template "someId"`);
        async.done();
      });
    }));

    it('should wait for async subtasks to be resolved', inject([AsyncTestCompleter], (async) => {
      var subTasksCompleted = false;

      var completer = PromiseWrapper.completer();

      var compiler = createCompiler( (parent, current, control) => {
        ListWrapper.push(mockStepFactory.subTaskPromises, completer.promise.then((_) => {
          subTasksCompleted = true;
        }));
      });

      // It should always return a Promise because the subtask is async
      var pvPromise = compiler.compile(new Template({
        componentId: 'someId',
        inline: 'some component'
      }));
      expect(pvPromise).toBePromise();
      expect(subTasksCompleted).toEqual(false);

      // The Promise should resolve after the subtask is ready
      completer.resolve(null);
      pvPromise.then((protoView) => {
        expect(subTasksCompleted).toEqual(true);
        async.done();
      });
    }));

  });

}

class MockStepFactory extends CompileStepFactory {
  steps:List<CompileStep>;
  subTaskPromises:List<Promise>;
  constructor(steps) {
    super();
    this.steps = steps;
  }
  createSteps(template, subTaskPromises) {
    this.subTaskPromises = subTaskPromises;
    ListWrapper.forEach(this.subTaskPromises, (p) => ListWrapper.push(subTaskPromises, p) );
    return this.steps;
  }
}

class MockStep extends CompileStep {
  processClosure:Function;
  constructor(process) {
    super();
    this.processClosure = process;
  }
  process(parent:CompileElement, current:CompileElement, control:CompileControl) {
    this.processClosure(parent, current, control);
  }
}

var EMPTY_STEP = (parent, current, control) => {
  if (isPresent(parent)) {
    current.inheritedProtoView = parent.inheritedProtoView;
  }
};

class FakeTemplateLoader extends TemplateLoader {
  _urlData: Map<string, string>;
  constructor(urlData) {
    super(null, new UrlResolver());
    this._urlData = urlData;
  }

  load(template: Template) {
    if (isPresent(template.inline)) {
      return PromiseWrapper.resolve(DOM.createTemplate(template.inline));
    }

    if (isPresent(template.absUrl)) {
      var content = MapWrapper.get(this._urlData, template.absUrl);
      if (isPresent(content)) {
        return PromiseWrapper.resolve(DOM.createTemplate(content));
      }
    }

    return PromiseWrapper.reject('Load failed');
  }
}
