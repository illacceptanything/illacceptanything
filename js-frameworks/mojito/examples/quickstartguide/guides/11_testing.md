# 11. Testing in Mojito #

## Intro ##

Mojito provides a testing framework based on [YUI Test](http://yuilibrary.com/yuitest/) 
that allows you to run unit tests for modules, applications, mojit controllers, 
mojit models, and mojit binders.

The next few sections show you how to run specific tests with the `mojito` 
command.

## Conventions ##

* Tests should be in the following directories:
  * `{app_name}/tests` - application tests
  * `{app_name}/mojits/{mojit_name}/tests` - mojit tests
  * `{app_name}/autoload/{yui_module}/tests` - tests for application-level YUI modules
  * `{app_name}/mojits/{mojit_name}/autoload/{yui_module}/tests` - tests for 
    mojit-level YUI modules
* Syntax for the name of the test file: `{yui_module}.{affinity}-tests.js`
  For example, the name of the unit test YUI module for the `HelloMojit` mojit 
  with the `server`   affinity would be `HelloMojit-tests.server.js`.

* The unit test YUI module should include the target module and the `mojito-test` 
  module in the `requires` array. The requires array includes the `mojito-test` 
  module and the target module `HelloMojit`:

    { requires: [ 'mojito-test', 'HelloMojit' ] }

> **Note:** Test files that are **not** in a `tests` directory may be found by 
>          Mojito as long as the file name has the suffix `-tests`. The 
>          suggested practice though is to place all test files in the `tests` 
>          directories shown above.


## Application Tests ##


The following command runs tests for all of the mojits of a Mojito application.

`$ mojito test app {path-to-app}/{application-name}`

To run one specific test in your application, use the following where `[test-name]` is 
either the YUI module or the module to be tested.

`$ mojito test app {path-to-app}/{application-name} [test-name]`

## Mojit Tests ##

You create unit tests for your mojits and execute them also using the `mojito` 
command. Mojit tests must require (included in the YUI `require` array) the 
module undergoing testing and the Mojito Test module `mojito-test`. For 
example, if the `Foo` module was being tested, the `requires` array would 
include the `Foo` and `mojit-test` modules as seen here: 
`requires: [ 'Foo', 'mojit-test']`

By default, Mojito uses the [YUI Test](http://yuilibrary.com/yuitest/)
framework for the [test harness](http://en.wikipedia.org/wiki/Test_harness) 
and assertion functions. Each mojit test will be executed within a YUI 
instance along with its required dependencies, so you can be assured to only 
have properly scoped values.


### Types of Mojit Tests ###

The following two types of mojit tests exist:

- controller tests
- model tests


## Controller Tests ##

A mojit can have one or more controllers that have different affinities. For each 
controller, you can create create a test controller with the same affinity or use 
`controller.common-tests.js`, which tests controllers with any affinity. For example, 
`controller.server.js` can be tested with `controller.server-tests.js` or 
`controller.common-tests.js`.


### Example ###

The `controller.server.js` below requires the `Foo` module. :

    YUI.add('Foo', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = { 
            index: function(ac) {
                ac.done();
            }
        };
    }, '0.0.1', {requires: []});

To test the controller of the `Foo` mojit, create a file in the tests 
directory called `controller.common-tests.js` that includes the `Foo-tests` 
module as seen below. Note that the reference to the controller is gotten 
using `Y.mojito.controllers[NAME]`.

    YUI.add('Foo-tests', function(Y, NAME) {
        var suite = new YUITest.TestSuite(NAME),
            controller = null,
           A = YUITest.Assert;
        suite.add(new YUITest.TestCase({
            name: 'Foo tests',
            setUp: function() {
                controller = Y.mojito.controllers.Foo;
            },
            tearDown: function() {
                controller = null;
            },
            'test mojit': function() {
                var ac, doneCalled = false;
                A.isNotNull(controller);
                A.isFunction(controller.index);
                ac = {
                    done: function(data) {
                        doneCalled = true;
                        A.isUndefined(data);
                    }
                };
                controller.index(ac);
                A.isTrue(compCalled);
            }
        }));
        YUITest.TestRunner.add(suite);
    }, '0.0.1', {requires: ['mojito-test', 'Foo']});


## Learn More ##

* [Testing with the MockActionContext Object](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html#testing-with-the-mockactioncontext-object)
* [Model Tests](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html#model-tests)
* [Mojito Built-In Functional/Unit Tests](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html#mojito-built-in-functional-unit-tests)
