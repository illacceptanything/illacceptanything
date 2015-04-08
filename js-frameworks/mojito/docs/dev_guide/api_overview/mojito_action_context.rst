==============
Action Context
==============

The Action Context is an essential element of the Mojito framework that gives you access 
to the frameworks features from within a controller function. To use the Action Context, 
you create an instance of the ``ActionContext`` class, which we will call ``ac`` for 
short. From ``ac``, you can call methods to execute mojit actions within either a server 
or client context. See the `ActionContext Class <../../api/classes/ActionContext.html>`_ 
for the methods available from ``ac``.

.. _ac-done:

done Method
===========

One of the most common methods used from an instance of the ``ActionContext`` class is 
``done``, which lets you pass data from the controller to a view. You can also optionally
pass meta data that contains the view name, mojit definitions, binders, assets, HTTP information, 
and a callback function. We'll look at the ``meta`` object more closely in the next
section.

See the `done method <https://developer.yahoo.com/cocktails/mojito/api/classes/ActionContext.html#method_done>`_
in the `Mojito API Documentation <https://developer.yahoo.com/cocktails/mojito/api/>`_ for 
more information about the parameters and to see the source code.


.. _done-meta:

meta Object
###########

By default, Mojito builds the metadata that is used when the method ``done`` executes and 
renders a template. You can, however, pass an object to override the defaults, such as the 
view name, mojit definitions, the assets, the view engine, HTTP information, etc. 

Generally, the only time you would need to pass the ``meta`` object is in the following
cases:

- you want to render a different template than the default
- a parent mojit is executing child mojits with the methods 
  `ac.composite.done <https://developer.yahoo.com/cocktails/mojito/api/classes/Composite.common.html#method_done>`_ 
  or `ac._dispatch <https://developer.yahoo.com/cocktails/mojito/api/classes/ActionContext.html#method__dispatch>`_. 

In the first case, suppose you want the ``index`` function of your controller to use the template 
``foo.hb.html`` instead of the default ``index.hb.html``. You would call ``ac.done`` with the 
following ``meta`` object (second argument) as shown below:

.. code-block:: javascript

   ...
     index: function(actionContext) {
       ac.done({ data: "Example of how to specify the templates" }, { view: { name: "foo" }});
     }
   ...

In the more complicated second case where metadata needs to be passed to either ``ac.composite.done``
or ``ac._dispatch`` so a parent mojit can execute children, 
you may need to merge the metadata of both the parent and children. Below is
an example ``meta`` object containing mojit definitions and metadata about binders, assets,
and the HTTP response.

.. code-block:: javascript

   { 
     children: 
     { 
       child: 
       { 
         type: 'Frame',
         config: [Object],
         action: 'index',
         instanceId: 'yui_3_7_3_1_1360791588420_57' 
       },
       header: 
       { 
         type: 'Header',
         instanceId: 'yui_3_7_3_1_1360791588420_58',
         viewId: 'yui_3_7_3_1_1360791588865_8',
         params: undefined 
       },
       body: 
       { 
         type: 'Body',
         instanceId: 'yui_3_7_3_1_1360791588420_59',
         viewId: 'yui_3_7_3_1_1360791588865_9',
         params: undefined 
       },
       footer: 
       { 
         type: 'Footer',
         instanceId: 'yui_3_7_3_1_1360791588420_60',
         viewId: 'yui_3_7_3_1_1360791588865_10',
         params: undefined } 
       },
       assets: { bottom: { js: [] } },
       http: { code: 200, headers: { 'content-type': [Object] } },
       binders: 
       { 
         yui_3_7_3_1_1360791588865_8: 
         { 
           base: undefined,
           name: 'header-binder-index',
           action: 'index',
           config: [Object],
           type: 'Header',
           viewId: 'yui_3_7_3_1_1360791588865_8',
           instanceId: 'yui_3_7_3_1_1360791588420_58',
           children: undefined 
         },
         yui_3_7_3_1_1360791588865_9: 
         { 
           base: undefined,
           name: 'body-binder-index',
           action: 'index',
           config: [Object],
           type: 'Body',
           viewId: 'yui_3_7_3_1_1360791588865_9',
           instanceId: 'yui_3_7_3_1_1360791588420_59',
           children: undefined 
         },
         yui_3_7_3_1_1360791588865_10: 
         { 
           base: undefined,
           name: 'footer-binder-index',
           action: 'index',
           config: [Object],
           type: 'Footer',
           viewId: 'yui_3_7_3_1_1360791588865_10',
           instanceId: 'yui_3_7_3_1_1360791588420_60',
           children: undefined 
         },
         yui_3_7_3_1_1360791588865_11: 
         { 
           base: undefined,
           name: 'frame-binder-index',
           action: 'index',
           config: [Object],
           type: 'Frame',
           viewId: 'yui_3_7_3_1_1360791588865_11',
           instanceId: 'yui_3_7_3_1_1360791588420_57',
           children: [Object] 
         } 
       } 
     }
   }

.. _done-ex:

Example
-------

In the example 
``controller.server.js`` below, the ``done`` method sends the ``data`` object to the 
``index`` template and specifies that the template ``foo.hb.html`` should be used instead
of the default ``index.hb.html``.

.. code-block:: javascript

   YUI.add('Hello', function(Y, NAME) {
     /**
     * The Hello module.
     *
     * @module Hello
     */
     /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
     Y.namespace('mojito.controllers')[NAME] = { 
       /**
       * Method corresponding to the 'index' action.
       *
       * @param ac {Object} The action context that
       * provides access to the Mojito API.
       */
       index: function(ac) {
         var data = { "data":"data passed to the index template" };
         ac.done(data, { view: { "name": "foo" }});
       }
     };
   }, '0.0.1', {requires: []});


