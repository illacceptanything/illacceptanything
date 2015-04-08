import EmberView from "ember-views/views/view";
import run from "ember-metal/run_loop";
import Registry from "container/registry";
import makeBoundHelper from "ember-htmlbars/system/make_bound_helper";
import compile from "ember-template-compiler/system/compile";
import { runAppend, runDestroy } from "ember-runtime/tests/utils";
import {
  dasherize
} from 'ember-runtime/system/string';
import SimpleBoundView from "ember-views/views/simple_bound_view";
import EmberObject from "ember-runtime/system/object";

var view, registry, container;

function registerRepeatHelper() {
  registry.register('helper:x-repeat', makeBoundHelper(function(params, hash, options, env) {
    var times = hash.times || 1;
    return new Array(times + 1).join(params[0]);
  }));
}

QUnit.module("ember-htmlbars: makeBoundHelper", {
  setup() {
    registry = new Registry();
    container = registry.container();
    registry.optionsForType('helper', { instantiate: false });
  },

  teardown() {
    runDestroy(view);
    runDestroy(container);
    registry = container = view = null;
  }
});

QUnit.test("should update bound helpers in a subexpression when properties change", function() {
  registry.register('helper:x-dasherize', makeBoundHelper(function(params, hash, options, env) {
    return dasherize(params[0]);
  }));

  view = EmberView.create({
    container: container,
    controller: { prop: "isThing" },
    template: compile("<div {{bind-attr data-foo=(x-dasherize prop)}}>{{prop}}</div>")
  });

  runAppend(view);

  equal(view.$('div[data-foo="is-thing"]').text(), 'isThing', "helper output is correct");

  run(view, 'set', 'controller.prop', 'notThing');

  equal(view.$('div[data-foo="not-thing"]').text(), 'notThing', "helper output is correct");
});

QUnit.test("should update bound helpers when properties change", function() {
  registry.register('helper:x-capitalize', makeBoundHelper(function(params, hash, options, env) {
    return params[0].toUpperCase();
  }));

  view = EmberView.create({
    container: container,
    controller: { name: "Brogrammer" },
    template: compile("{{x-capitalize name}}")
  });

  runAppend(view);

  equal(view.$().text(), 'BROGRAMMER', "helper output is correct");

  run(view, 'set', 'controller.name', 'wes');

  equal(view.$().text(), 'WES', "helper output updated");
});

QUnit.test("should update bound helpers when hash properties change", function() {
  registerRepeatHelper();

  view = EmberView.create({
    container: container,
    controller: {
      phrase: "Yo",
      repeatCount: 1
    },
    template: compile("{{x-repeat phrase times=repeatCount}}")
  });

  runAppend(view);

  equal(view.$().text(), 'Yo', "initial helper output is correct");

  run(view, 'set', 'controller.repeatCount', 5);

  equal(view.$().text(), 'YoYoYoYoYo', "helper output updated");
});

QUnit.test("bound helpers should support keywords", function() {
  registry.register('helper:x-capitalize', makeBoundHelper(function(params, hash, options, env) {
    return params[0].toUpperCase();
  }));

  view = EmberView.create({
    container: container,
    text: 'ab',
    template: compile("{{x-capitalize view.text}}")
  });

  runAppend(view);

  equal(view.$().text(), 'AB', "helper output is correct");
});

QUnit.test("bound helpers should not process `fooBinding` style hash properties", function() {
  registry.register('helper:x-repeat', makeBoundHelper(function(params, hash, options, env) {
    equal(hash.timesBinding, "numRepeats");
  }));

  view = EmberView.create({
    container: container,
    controller: {
      text: 'ab',
      numRepeats: 3
    },
    template: compile('{{x-repeat text timesBinding="numRepeats"}}')
  });

  runAppend(view);
});

QUnit.test("bound helpers should support multiple bound properties", function() {

  registry.register('helper:x-combine', makeBoundHelper(function(params, hash, options, env) {
    return params.join('');
  }));

  view = EmberView.create({
    container: container,
    controller: {
      thing1: 'ZOID',
      thing2: 'BERG'
    },
    template: compile('{{x-combine thing1 thing2}}')
  });

  runAppend(view);

  equal(view.$().text(), 'ZOIDBERG', "helper output is correct");

  run(view, 'set', 'controller.thing2', "NERD");

  equal(view.$().text(), 'ZOIDNERD', "helper correctly re-rendered after second bound helper property changed");

  run(function() {
    view.set('controller.thing1', 'WOOT');
    view.set('controller.thing2', 'YEAH');
  });

  equal(view.$().text(), 'WOOTYEAH', "helper correctly re-rendered after both bound helper properties changed");
});

QUnit.test("bound helpers can be invoked with zero args", function() {
  registry.register('helper:x-troll', makeBoundHelper(function(params, hash) {
    return hash.text || "TROLOLOL";
  }));

  view = EmberView.create({
    container: container,
    controller: {
      trollText: "yumad"
    },
    template: compile('{{x-troll}} and {{x-troll text="bork"}}')
  });

  runAppend(view);

  equal(view.$().text(), 'TROLOLOL and bork', "helper output is correct");
});

QUnit.test("bound helpers should not be invoked with blocks", function() {
  registerRepeatHelper();
  view = EmberView.create({
    container: container,
    controller: {},
    template: compile("{{#x-repeat}}Sorry, Charlie{{/x-repeat}}")
  });

  expectAssertion(function() {
    runAppend(view);
  }, /makeBoundHelper generated helpers do not support use with blocks/i);
});

QUnit.test("shouldn't treat raw numbers as bound paths", function() {
  registry.register('helper:x-sum', makeBoundHelper(function(params) {
    return params[0] + params[1];
  }));

  view = EmberView.create({
    container: container,
    controller: { aNumber: 1 },
    template: compile("{{x-sum aNumber 1}} {{x-sum 0 aNumber}} {{x-sum 5 6}}")
  });

  runAppend(view);

  equal(view.$().text(), '2 1 11', "helper output is correct");

  run(view, 'set', 'controller.aNumber', 5);

  equal(view.$().text(), '6 5 11', "helper still updates as expected");
});

QUnit.test("should have correct argument types", function() {
  registry.register('helper:get-type', makeBoundHelper(function(params) {
    return typeof params[0];
  }));

  view = EmberView.create({
    container: container,
    controller: {},
    template: compile('{{get-type null}}, {{get-type undefProp}}, {{get-type "string"}}, {{get-type 1}}, {{get-type this}}')
  });

  runAppend(view);

  equal(view.$().text(), 'undefined, undefined, string, number, object', "helper output is correct");
});

QUnit.test("when no parameters are bound, no new views are created", function() {
  registerRepeatHelper();
  var originalRender = SimpleBoundView.prototype.render;
  var renderWasCalled = false;
  SimpleBoundView.prototype.render = function() {
    renderWasCalled = true;
    return originalRender.apply(this, arguments);
  };

  try {
    view = EmberView.create({
      template: compile('{{x-repeat "a"}}'),
      controller: EmberObject.create(),
      container: container
    });
    runAppend(view);
  } finally {
    SimpleBoundView.prototype.render = originalRender;
  }

  ok(!renderWasCalled, 'simple bound view should not have been created and rendered');
  equal(view.$().text(), 'a');
});


QUnit.test('when no hash parameters are bound, no new views are created', function() {
  registerRepeatHelper();
  var originalRender = SimpleBoundView.prototype.render;
  var renderWasCalled = false;
  SimpleBoundView.prototype.render = function() {
    renderWasCalled = true;
    return originalRender.apply(this, arguments);
  };

  try {
    view = EmberView.create({
      template: compile('{{x-repeat "a" times=3}}'),
      controller: EmberObject.create(),
      container: container
    });
    runAppend(view);
  } finally {
    SimpleBoundView.prototype.render = originalRender;
  }

  ok(!renderWasCalled, 'simple bound view should not have been created and rendered');
  equal(view.$().text(), 'aaa');
});
