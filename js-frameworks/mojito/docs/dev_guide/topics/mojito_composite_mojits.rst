================
Composite Mojits
================

.. _mojito_composite-intro:

Introduction
============

A composite mojit is a parent mojit that has child mojits. This parent mojit is 
responsible for the execution and layout of its children. The child mojits as 
subordinates create content and provide functionality for the parent mojit. 
See `Using Multiple Mojits <../code_exs/multiple_mojits.html>`_ for a working 
example of composite mojits.

.. _mojito_composite-parent_child:

Creating Parent and Child Mojit Instances
=========================================

As with any mojit, you need to define a mojit instances in ``application.json``. 
The parent mojit instance defines its child mojits in the ``children`` object. 
In the example ``application.json`` below, the parent mojit instance is ``foo``, 
which has the child mojit instances ``nav``, ``news``, and ``footer``. Each 
mojit instance has a ``type`` that specifies the mojits that are instantiated. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "foo": {
           "type": "MyComp",
           "config": {
             "children": {
               "nav": {
                 "type": "Nav"
               },
               "news": {
                 "type": "News"
               },
               "footer": {
                  "type": "Footer"
               }
             }
           }
         }
       }
     }
   ]


.. _mojito_composite-execute_child:

Executing Child Mojits
======================

The parent mojit instance defined in ``application.json`` can access the 
``config`` object and execute the child mojits from the controller. 
The controller methods of the parent mojit can use the ``Config`` addon
to get the application configuration with the method ``getAppConfig``.

In the example controller of ``parent`` mojit below, the ``index`` function saves 
and displays the ``children`` object that lists the child mojits.

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var app_config = ac.config.getAppConfig();
         // The app config contains the 'children' object that is
         // defined in application.json
         Y.log(app_config);
         ac.done(app_config);
       },
       ...
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-config-addon']});


When the controller of the parent mojit calls ``ac.composite.done`` from the ``index`` 
function, the controllers of the mojit children execute ``ac.done`` from their 
``index`` functions. The rendered views from the child mojits are then available 
as Handlebars expressions in the ``index`` template of the parent mojit.

For example, in the example controller of the parent mojit below, the ``index`` 
function calls ``ac.composite.done``, which executes ``ac.done`` in the 
``index`` functions of the child mojits. The rendered ``index`` templates for 
each of the child mojits is then available as a Handlebars expression, such 
as ``{{{child_mojit}}}`` in the parent template. 

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.composite.done();
       }
     };
   }, '0.1.0', {requires: ['mojito', 'mojito-composite-addon']});

If ``parent`` above is the parent of ``child``, the controller of 
``child`` shown below will execute ``ac.done`` in the ``index`` function.

.. code-block:: javascript

   YUI.add('child', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.done({ title: 'Child Mojit'});
       }
     };
   }, '0.1.0', {requires: []});


.. _mojito_composite-pass_data_parent:

Passing Data to the Parent Template
===================================

The parent mojit can pass data to its templates by passing an object as the
first argument to ``ac.composite.done``.

The example parent controller below passes ``parent_data`` to its template, so that
the Handlebars expression ``{{parent_data}}`` in the parent template can be replaced with 
the value ``'Welcome'`` when the template is rendered.

.. code-block:: javascript

   
   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.composite.done({ parent_data: 'Welcome'});
       }
     };
   }, '0.1.0', {requires: ['mojito', 'mojito-composite-addon']});


.. _mojito_composite-specify_view:

Specifying the View for a Parent Mojit
======================================

In addition to passing data to the parent template, you can 
specify what parent template to use by passing an object containing the property
``view`` object as a second argument to ``ac.composite.done``.  The ``name`` property
specifies the name of the view to render.

The example controller of parent mojit passes data and selects the template 
``page`` (e.g., ``page.hb.html``):

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.composite.done({ parent_data: 'Welcome'}, { "view": { "name": "page" }});
       }
     };
   }, '0.1.0', {requires: ['mojito', 'mojito-composite-addon']});


You can also specify the binder to use with the template. By default, Mojito chooses
the binder with the same name as the template. Using the same example controller from
above, we add the property ``binder`` to the ``view`` object to specify the ``bar``
binder to use with the ``page`` template.

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.composite.done({ parent_data: 'Welcome'}, 
           { "view": { "name": "page", "binder": "bar" }}
         );
       }
     };
   }, '0.1.0', {requires: ['mojito', 'mojito-composite-addon']});


.. _mojito_composite-include_assets:

Attaching Assets to a Parent Template
=====================================

If the parent mojit is a child of ``HTMLFrameMojit``, assets can be attached
to the parent template by passing an object containing the ``assets`` property.

In this example, the controller of the parent mojit is passing data,
specifying a template, and attaching JavaScript assets to the ``head`` element
of the rendered page.

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.composite.done({ parent_data: 'Welcome'}, {
           "view": {
             "name": "page"
           },
           "assets": {
             "top": {
               "js": ["/static/app_ex/assets/js/something.js"]
             }
           }
         );
       }
     };
   }, '0.1.0', {requires: ['mojito', 'mojito-composite-addon']});


.. _mojito_composite-child_view:

Displaying Child Mojits in View
===============================

After the controller of the parent mojit calls ``ac.composite.done``, its 
template then has access to the content created by the child mojits. The 
template of the parent mojit can use Handlebars expressions to embed the 
output from the child mojits. For example, if the child mojit instance 
``footer`` was defined in ``application.json``, the template of the parent 
mojit could use  ``{{{footer}}}`` to embed the content created 
by ``footer``.

In the example ``index`` template of the parent mojit below, the rendered 
``index`` templates of the child mojits  ``nav``,  ``body``, ``footer`` 
are embedded using Handlebars expressions.


.. code-block:: html

   <div id="{{mojit_view_id}}" class="mojit" style="border: dashed black 1px;">
     <h1>{{title}}</h1>
     <div class="nav" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">{{{nav}}}</div>
     <div class="body" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">{{{body}}}</div>
     <div class="footer" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">{{{footer}}}</div>
   </div>

.. _mojito_composite-child_errors:

Propagating Child Mojit Errors to Parent Mojits
===============================================

By default, when a child mojit calls the method ``ac.error`, an error message is
logged (depending on the logging configurations) and an empty string is passed to the
parent. The parent continues to execute its children in parallel, and finally, the 
parent template is rendered with the content from the successfully executed
children.

To propagate the error from the child mojit to its parent so that the parent
halts execution of child mojits, you set the property ``propagateFailure`` to ``true``
in ``application.json``. The ``propagateFailure`` property is part of the
child configuration, not the parent configuration, so you can configure critical
child mojits to propagate errors while making sure that the parent mojit **does not**
fail because a nonessential child mojit calls ``ac.error``.

Based on the example ``application.json`` below, when the ``real_content`` mojit 
instance calls the method ``ac.error``, the error will be propagated to the ``parent`` 
mojit instance, which will then halt the execution of child mojits.
If the ``fluff`` mojit instance calls ``ac.error``, the error
can be logged, but will not be propagated to the ``parent`` mojit, so the parent
will resume executing the other child mojits.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "parent": {
           "type": "Parent",
           "config" : {
             "children": {
               "fluff": {
                 "type": "Fluff",
                 "propagateFailure": false
               },
               "real_content": {
                 "type": "Content",
                 "propagateFailure": true
               }
             }
           }
         }
       }
     }
   ]

   
.. _mojito_composite-dyn_define:

Dynamically Defining Child Mojits
=================================

In some cases, the parent mojit won't know the children specs until runtime. For 
example, the specs of the children might depend on the results of a 
Web service call. In such cases, your controller can generate the equivalent 
of the ``config`` object and a callback, which are then passed to 
``ac.composite.execute``. Using ``ac.composite.execute`` lets you run 
dynamically defined child mojits. See 
`Running Dynamically Defined Mojit Instances <./mojito_run_dyn_defined_mojits.html>`_ 
for more information.

