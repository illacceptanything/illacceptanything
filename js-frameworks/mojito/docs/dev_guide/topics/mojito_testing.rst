=======
Testing
=======

Mojito provides a testing framework based on `YUI Test`_ that 
allows you to run unit tests for modules, applications, mojit controllers, 
mojit models, and mojit binders.

The next few sections show you how to run specific tests with the ``mojito`` 
command.

.. _mojito_testing-conventions:

Conventions
===========

- Tests should be in the following directories:

     - ``{app_name}/tests`` - application tests
     - ``{app_name}/mojits/{mojit_name}/tests`` - mojit tests
     - ``{app_name}/yui_modules/{yui_module}/tests`` - tests for 
       application-level YUI modules
     - ``{app_name}/mojits/{mojit_name}/yui_modules/{yui_module}/tests`` - tests for 
       mojit-level YUI modules
- Syntax for the name of the test file: ``{yui_module}.{affinity}-tests.js``

  For example, the name of the unit test for the ``Hello`` mojit (``hello`` YUI module) 
  with the ``server`` affinity would be ``hello.server-tests.js``.

- The unit test YUI module should include the target module and the ``mojito-test`` 
  module in the ``requires`` array. The requires array includes the ``mojito-test`` 
  module and the target module ``hello``:

  .. code-block:: javascript

     { requires: [ 'mojito-test', 'hello' ] }

.. note:: Test files that are **not** in a ``tests`` directory may be found by 
          Mojito as long as the file name has the suffix ``-tests``. The 
          suggested practice though is to place all test files in the ``tests`` 
          directories shown above.

.. _mojito_testing-application:

Application Tests
=================

The following command runs tests for all of the mojits of a Mojito application.

``$ mojito test app {path-to-app}/{application-name}``

To run one specific test in your application, use the following where ``[test-name]`` is 
either the YUI module or the module to be tested.

``$ mojito test app {path-to-app}/{application-name} [test-name]``

.. _mojito_testing-mojit:

Mojit Tests
===========

You create unit tests for your mojits and execute them also using the ``mojito`` 
command. Mojit tests must require (included in the YUI ``require`` array) the 
module undergoing testing and the Mojito Test module ``mojito-test``. For 
example, if the ``foo`` module was being tested, the ``requires`` array would 
include the ``foo`` and ``mojit-test`` modules as seen here: 
``requires: [ 'foo', 'mojit-test']``

By default, Mojito uses the `YUI Test <http://yuilibrary.com/yuitest/>`_ 
framework for the `test harness <http://en.wikipedia.org/wiki/Test_harness>`_ 
and assertion functions. Each mojit test will be executed within a YUI 
instance along with its required dependencies, so you can be assured to only 
have properly scoped values.

.. _mojit_testing-types:

Types of Mojit Tests
--------------------

The following two types of mojit tests exist:

- controller tests
- model tests

.. _mojito_testing-standards:

Testing Standards
=================

To use the Mojito test harness, you are required to name files and testing 
modules according to certain rules. The name of the test file must have the 
same `affinity <../reference/glossary.html>`_ as the file being tested and 
have the string ``-tests`` appended to the affinity. For example, the mojit 
controller with the ``common`` affinity would be ``controller.common.js``, 
so the name of the test file must be ``controller.common-tests.js``.

The ``controller.common.js`` below registers the ``foo`` module.

.. code-block:: javascript

   YUI.add('foo', function(Y) {
     ...
   });

To test the ``foo``, module, the the test file ``controller.common-tests.js`` would 
require the ``foo`` and 'mojito-test' modules as seen below.

.. code-block:: javascript

   YUI.add('foo-tests', function(Y) {
     ...
   }, 'VERSION', {requires: ['mojito-test', 'foo']});


.. _mojito_testing-controller:

Controller Tests
================

A mojit can have one or more controllers that have different affinities. For each 
controller, you can create create a test controller with the same affinity or use 
``controller.common-tests.js``, which tests controllers with any affinity. For example, 
``controller.server.js`` can be tested with ``controller.server-tests.js`` or 
``controller.common-tests.js``.

.. _controller_tests-ex:

Example
-------

The ``controller.server.js`` below requires the ``foo`` module.

.. code-block:: javascript

   YUI.add('foo', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.done();
       }
     };
   }, '0.0.1', {requires: []});

To test the controller of the ``Foo`` mojit (``foo`` YUI module), create a file in the tests 
directory called ``controller.common-tests.js`` that includes the ``foo-tests`` 
module as seen below. Note that the reference to the controller is gotten 
using ``Y.mojito.controllers[NAME]``.

.. code-block:: javascript

   YUI.add('foo-tests', function(Y, NAME) {
     var suite = new YUITest.TestSuite(NAME),
     controller = null,
     A = YUITest.Assert;
     suite.add(new YUITest.TestCase({
       name: 'Foo tests',
       setUp: function() {
         controller = Y.mojito.controllers["foo"];
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
         A.isTrue(doneCalled);
       }
     }));
     YUITest.TestRunner.add(suite);
   }, '0.0.1', {requires: ['mojito-test', 'foo']});

.. _mojito_testing-mockactioncontext:

Testing with the MockActionContext Object
=========================================

The ``mojito-test`` YUI module allows you to create the mock object
``MockActionContext`` to test without dependencies. Using the 
``MockActionContext`` object, you can easily build an ``ActionContext`` 
for your controller, addon, and model tests. To learn more information 
about using YUI to create mock objects, see 
`YUI Test Standalone Library: Mock Objects <http://yuilibrary.com/yuitest/#mockobjects>`_.

.. _mockactioncontext_testing-using:

Using the Mock ActionContext
----------------------------

The following sections will explain the below example code that creates a 
simple ``MockActionContext`` that tests the ``done`` function and verifies 
it was called correctly.

.. code-block:: javascript

   var ac = new Y.mojito.MockActionContext();
   ac.expect(
     {
       method: 'done',
       args: [YUITest.Mock.Value.Object],
       run: function(data) {
         YUITest.ObjectAssert.areEqual({ just: 'testing' });
       }
     }
   );
   Y.mojito.controller.actionUnderTest(ac);
   ac.verify();


.. _mockactioncontext_testing-creating:

Creating the MockActionContext Object
#####################################

To mock the ``ActionContext``, the ``mojito-test`` YUI module provides the 
``MockActionContext`` constructor that returns a mocked ``ActionContext`` 
as shown below:

.. code-block:: javascript

   var ac = new Y.mojito.MockActionContext();

.. _mockactioncontext_testing-expectations:

Setting Test Expectations
#########################

To test with the ``MockActionContext`` object, you use the ``expect`` method 
and pass it an ``expectation`` object containing the properties ``method``, 
``args``, and ``run``. These properties, in turn, contain the controller 
method to test, the function parameters, and the test function.

In the code snippet below, the ``expect`` method creates a test for the 
controller method ``done``, using the ``YUITest`` module to perform an 
assertion on the function's return value.

.. code-block:: javascript

   ac.expect({
     method: 'done',
     args: [YUITest.Mock.Value.Object],
     run: function(data) {
       YUITest.ObjectAssert.areEqual({ just: 'testing' });
     }
   });

.. _mockactioncontext_testing-configure:

Configuring Mojito to Test MockActionContext Object
###################################################

To configure Mojito to use your ``MockActionContext`` object to run test, 
use the following, where ``{actionUnderTest}`` is the action you are testing.

.. code-block:: javascript

   Y.mojito.controller.{actionUnderTest}(ac);

If the ``{actionUnderTest}`` function fails to call the ``done`` function, calls 
it more than one time, or calls it with the wrong parameters, the test will 
fail.

.. _mockactioncontext_testing-run:

Running the Test
****************

Finally, run the expectation by call the ``verify`` method from the 
``MockActionContext`` object as seen here:

.. code-block:: javascript

   ac.verify();


.. note:: Expectations for addons, models, and extras will be be verified 
          automatically when you call the main ``verify`` function from the 
          ``MockActionContext`` object.

.. _mockac_testing_expectations-ex:

Example Expectations
--------------------

.. _testing_expectations_ex-pass_objs:

Passing Multiple expectation Objects
####################################

You can pass many ``expectation`` objects to the ``expect`` method:

.. code-block:: javascript

   ac.assets.expect({
     method: 'preLoadImages',
     args: [YUITest.Mock.Value.Object],
     run: function(arr) {
       OA.areEqual(['thepath','thepath'], arr);
     },
     callCount: 1
     },
     {
       method: 'getUrl',
       args: [YUITest.Mock.Value.String],
       returns: 'thepath',
       callCount: 3
     },
     {
       method: 'addCss',
       args: ['thepath']
     }
   );

.. _testing_expectations_ex-chain_methods:

Chaining expect Methods
#######################

You can also chain ``expect`` methods:

.. code-block:: javascript

   ac.assets.expect(
     {
       method: 'preLoadImages',
       args: [YUITest.Mock.Value.Object],
       run: function(arr) {
         OA.areEqual(['thepath','thepath'], arr);
       },
       callCount: 1
     }).expect({
       method: 'getUrl',
       args: [YUITest.Mock.Value.String],
       returns: 'thepath',
       callCount: 3
     }).expect({
       method: 'addCss',
       args: ['thepath']
     });

.. _mock_addons:

Mocking Addons
--------------

To use the MockActionContext object to test different addons, you specify 
the namespaces of the addons within the ``MockActionContext`` constructor:

.. code-block:: javascript

   var ac = new Y.mojito.MockActionContext({
     addons: ['intl', 'assets']
   });
   ac.intl.expect({
     method: 'lang',
     args: ['UPDATING'],
     returns: 'updating, yo'
   });




.. _mock_custom_addons:

Mocking Custom Addons
#####################

To create a custom addon that contains functions within a property, you might 
have an addon that is used in the following way:

.. code-block:: javascript

   ac.customAddon.params.get('key');

To test the addon, you pass the ``addons`` array with a list of the addons 
you want to test to the ``MockActionContext`` constructor as seen below:

.. code-block:: javascript

   var ac = new Y.mojito.MockActionContext(
     {
       addons: ['customAddon'],
       extras: { customAddon: 'params'}
     }
   );

This will give you a mock object at ``ac.customAddon.params`` from which you can 
call ``expect``.

.. _mock_models:

Mocking Models
##############

To test models with the ``MockActionContext`` object, you pass the ``models`` 
array with the model YUI modules as is done with addons:

.. code-block:: javascript

   var ac = new Y.mojito.MockActionContext(
     {
       addons: ['intl', 'params'],
       models: ['foo']
     }
   );
   ac.models.foo.expect(
     {
       method: 'getData',
       args: [YUITest.Mock.Value.Object,
       YUITest.Mock.Value.Function],
       run: function(prms, cb) {
         cb(null, {my: 'data'});
       }
     }
   );

.. _mock_addons-ex:

Example MockAction Test
-----------------------

.. _mock_addons_ex-controller:

controller.server.js
####################


.. code-block:: javascript

   YUI.add('mymojit', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {
       index: function(ac) {
           ac.done({
              status: 'Mojito is working.',
           });
         }
       };
   }, '0.0.1', {requires: ['mojito', 'mymojit-model-foo']});

.. _mock_addons_ex-controller_test:

controller.server-tests.js
##########################

.. code-block:: javascript

   YUI.add('tester-tests', function(Y) {
     var suite = new YUITest.TestSuite('tester-tests'),
         controller = null,
         A = YUITest.Assert;

     suite.add(new YUITest.TestCase({
       name: 'tester user tests',
       setUp: function() {
         controller = Y.mojito.controllers["tester"];
       },
       tearDown: function() {
         controller = null;
       },
       'test mojit': function() {
         var ac = new Y.mojito.MockActionContext({});
         A.isNotNull(controller);
         A.isFunction(controller.index);
         ac.expect({
           method: 'done',
           args: [YUITest.Mock.Value.Object],
           callCount: 1,
           run: function(data){
             YUITest.ObjectAssert.areEqual({ status: 'Mojito is working.' },data);
           }
         });
         controller.index(ac);
         ac.verify();
       }
     }));
     YUITest.TestRunner.add(suite);
   }, '0.0.1', {requires: ['mojito-test', 'mymojit']});


.. _mojito_testing-models:

Model Tests
===========

Model tests are largely the same as controller tests, except there can be 
many of them. The model tests are placed in the ``tests/models`` directory. 
You can create multiple model tests or use ``models.common-tests.js`` to test 
both server and client models.

.. _mojito_testing_models-ex:

Example
-------

The ``model.server.js`` below includes the ``foo-model`` module.

.. code-block:: javascript

   YUI.add('foo-model', function(Y, NAME) {
     Y.namespace('mojito.models')[NAME] = {      
       getData: function(callback) {
         callback({some:'data'});
       }
     };
   }, '0.0.1', {requires: []});

The ``tests/models/models.common-tests.js`` test below includes the 
``foo-model-tests`` module and the ``requires`` array contains the ``foo-model`` 
module.

.. code-block:: javascript

   YUI.add('foo-model-tests', function(Y, NAME) {
     var suite = new YUITest.TestSuite(NAME),
     model = null,
     A = YUITest.Assert;
     suite.add(new YUITest.TestCase({
       name: 'Foo model tests',
       setUp: function() {
         model = Y.mojito.models["foo-model"];
       },
       tearDown: function() {
         model = null;
       },
       'test mojit model': function() {
         A.isNotNull(model);
         A.isFunction(model.getData);
       }
     }));
     YUITest.TestRunner.add(suite);
   }, '0.0.1', {requires: ['mojito-test', 'foo-model']});


.. _moj_tests-func_unit:

Mojito Built-In Functional/Unit Tests
=====================================

Mojito's built-in functional and unit tests are intended for use by Mojito contributors.
Because this developer guide contains documentation intended for Mojito application developers,
we have moved the instructions for running the built-in tests to the wiki 
`Mojito Framework's Unit and Functional Tests <https://github.com/yahoo/mojito/wiki/Mojito-Framework's-Unit-and-Functional-Tests>`_.

   
.. _YUI Test: http://yuilibrary.com/yuitest/


