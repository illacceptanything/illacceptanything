===========================================
Running Dynamically Defined Mojit Instances
===========================================

.. _dyn_defined_mojits-intro:

Introduction
============

Mojito allows developer to statically or dynamically define child mojit 
instances. In the simplest case, your parent and its child mojit instances 
will be statically defined in ``application.json``. The parent mojit will 
run its child mojits and then attach their rendered output to its own 
template. In more complex cases, your application may need to run a mojit 
instance or pass data to another mojit instance because of some condition, 
such as a user event or an environment variable. Being self-contained units 
of execution, mojits can only pass data or run mojits that have been defined 
as children in configuration. If you have not statically defined the child 
instances that you want to run or receive data, you can still dynamically 
define those child instances in configuration objects at runtime.

The dynamically defined child instances, however, are only useful if the 
parent mojit can run them. Thus, the Mojito API provides the two methods 
``ac.composite.execute`` or ``ac._dispatch`` that parent mojits can use to 
run dynamically defined child mojit instances. The parent mojit passes 
configuration objects that define the child mojit instances and any data 
you want to pass to one of the two methods. Although both the 
``ac.composite.execute`` and ``ac._dispatch`` methods allow a parent mojit 
to run a dynamically defined child instance and pass data to that child 
instance, the two methods do have some distinct differences, which are 
discussed in `Should I Use ac.composite.execute or ac._dispatch?`_.

.. _dyn_defined_mojits_intro-execute:

ac.composite.execute
--------------------

The `Composite addon <../../api/classes/Composite.common.html>`_ includes 
the ``execute`` method that allows parents to run  one or more dynamically 
defined children mojits by passing the ``children`` object. The ``execute`` 
method is different than the ``done`` method of the ``Composite`` addon in 
that the ``done`` method runs child mojit instances that are defined in 
``application.json``. See `Composite Mojits <./mojito_composite_mojits.html>`_ 
to learn how to use the ``done`` method of the ``Composite`` addon.

.. _dyn_defined_mojits_intro-dispatch:

ac._dispatch
------------

Mojito also provides the ``dispatch`` method that can be called from the 
``ActionContext`` object to run a dynamically defined child mojit. The 
``dispatch`` method also allows you to define your own ``flush``, 
``done``, and ``error`` functions for the child mojit instance.
The ``done`` and ``error`` methods are executed synchronously,
but the ``flush`` method is executed asynchronously.

.. _dyn_defined_mojits-use_cases:

Use Cases
=========

- A mojit needs to pass data to another mojit.
- A mojit wants to attach the rendered view of the dynamically defined mojit 
  to its template.
- A mojit binder invokes the controller to run an instance of another mojit. 
  The mojit renders its view, which is then returned it to the binder.

.. _dyn_defined_mojits-exec_v_dispatch:

Should I Use ac.composite.execute or ac._dispatch?
=================================================

If you need fine-grained control over your child instances, you will want to 
use ``ac._dispatch``. In most other cases, and particularly when dynamically 
defining and running more than one child instance, you will most likely want 
to use ``ac.composite.execute`` because it is easier to use. Also, in the case 
of running multiple child instances, ``ac.composite.execute`` keeps track of 
the configuration and metadata for your child instances; whereas, your parent 
mojit will need to manage its children if ``ac._dispatch`` was used.

.. _dyn_defined_mojits-composite:

Using the Composite Addon
=========================

For a mojit to run dynamically defined mojit instances using the ``Composite`` 
addon, you need to pass a configuration object to ``ac.composite.execute``. 
The next sections will look at the configuration object, the controller code, 
and then the template of the parent mojit.

.. _dyn_defined_mojits_comp-child:

Configuring Child Instances
---------------------------

The configuration object passed to ``ac.composite.execute`` must have the
``children`` object to defines one or more mojit instances. In the ``cfg`` 
object below, the child mojit instances ``news`` and ``sidebar`` are defined. 
You can also specify the action to execute and pass configuration information 
that includes parameters and assets.

.. code-block:: javascript

   var cfg = {
     "children":
     {
       "news": {
         "type": "News",
         "action": "index"
       },
       "sidebar": {
         "type": "Sidebar",
         "action": "index",
         "params": {
           "route": {},
           "url": {},
           "body": {},
           "file": {}
         }
       }
     },
     "assets": {
       "top": [
           "/static/sidebar/assets/index.css"
       ]
     }
   }

.. _dyn_defined_mojits-run_mojits:

Running Mojit Instances
-----------------------

The ``ac.composite.execute`` takes two parameters. The first parameter is the 
configuration object discussed in `Configuring Child Instances`_ that define 
the child mojit instance or instances. The second parameter is a callback that 
returns an object containing the rendered data from the child mojit 
instances and an optional object containing the metadata of the children. 
The metadata contains information about the children's binders, assets, 
configuration, and HTTP headers and is required for binders to execute and 
attach content to the DOM.

In the example controller below, the child instances ``header``, ``body``, 
and ``footer`` are dynamically defined in ``cfg`` and then run with 
``actionContext.composite.execute``. The rendered views of the child mojits 
are returned in the callback and then made available to the mojit's template.

.. code-block:: javascript

   YUI.add('frame', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(actionContext) {
         var cfg = { view: "index", 
                     children: { 
                       header: { type: "Header", action: "index" }, 
                       body: { type: "Body", action: "index" }, 
                       footer: { type: "Footer", action: "index" }
                     }
                   };
         // The 'meta' object containing metadata about the children's binders, assets, 
         // configuration, and HTTP header info is passed to the callback. This 'meta' 
         // object is required for binders to execute and attach content to the DOM.
         actionContext.composite.execute(cfg,function(data, meta){
           actionContext.done(data, meta);
        });
       }
     }
   ;}, '0.0.1', {requires: ['mojito-composite-addon']});


.. _dyn_defined_mojits-templates:

Templates
---------

The rendered output from each of the dynamically defined child mojit instances can be 
injected into the template of the parent mojit using Handlebars expressions. If the child 
mojit instances ``header``, ``footer``, and ``body`` were defined in the configuration 
object passed to ``ac.composite.execute``, you could add the rendered content from those 
child mojit instances to the parent mojit's template with the Handlebars expressions 
``{{{header}}}``, ``{{{footer}}}``, and ``{{{body}}}`` as shown in the example template
below. The Handlebars expressions using triple braces insert unescaped HTML into the page.

.. code-block:: html 
   
   <div id="{{mojit_view_id}}">
     {{{header}}}
     {{{body}}}
     {{{footer}}}
   </div>

.. _dyn_defined_mojits-exs:

Example
-------

.. _dyn_defined_mojits_exs-controllers:

Controllers
###########

.. _dyn_controllers-parentmojit:

Parent
******

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
      Y.namespace('mojito.controllers')[NAME] = { 
        index: function(ac) {
          var cfg = {
            "children": {
              "dynamic_child": {
                "type": "DynamicChild",
                  "config": {
                    "caller": "Parent"
                  }
                }
              }
            };
            ac.composite.execute(cfg,function(data, meta){
              // The 'meta' object containing metadata about the children's binders, 
              // assets, configuration, and HTTP header info is passed to the callback. 
              // This 'meta' object is required for binders to execute and attach content 
              // to the DOM.
              ac.done(data, meta);
            });
          }
        };
      }, '0.0.1', {requires: ['mojito', 'mojito-composite-addon']});

.. _dyn_controllers-dynchild:

DynamicChild
************

.. code-block:: javascript

   YUI.add('dynamicchild', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var caller = ac.config.get("caller");       
         if("Parent"==caller){
           ac.done({ "content": "I have been dynamically defined and run by " + caller + "."});
         }
         else {
           ac.done({"content": "I was called directly and have no parent." });
         }
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-config-addon']});


.. _dyn_defined_mojits_exs-templates:

Templates
#########

.. _dyn_templates-parentmojit:

Parent
******

.. code-block:: html

   <div id="{{mojit_view_id}}">
     {{{dynamic_child}}}
   </div>

.. _dyn_templates-dynchild:

DynamicChild
************

.. code-block:: html

   <div id="{{mojit_view_id}}">
      {{{content}}}
   </div>


.. _dyn_defined_mojits_exs-rendered_views:

Rendered Views
##############

- ``localhost:8666/@Parent/index``

   ::

      I have been dynamically defined and run by Parent.

- ``localhost:8666/@DynamicChild/index``

   ::
  
      I was called directly and have no parent.  

.. _dyn_defined_mojits-dispatch:

Using ac._dispatch
==================

Using ``ac._dispatch`` not only allows you to run a dynamically defined child 
mojit instance like ``ac.composite.execute``, but you also have more 
fine-grained control over how the child mojit instance runs. The content from 
the child mojit's controller may be passed to its template or the child mojit's 
rendered template is passed to the parent mojit. 

.. _dyn_dispatch-config:

Configuring a Child Instance
----------------------------

Two configuration objects are passed to ``ac._dispatch``, each having a 
different function. The ``command`` object defines the instance, the action 
to execute, the context, and any parameters. This lets the parent mojit have 
greater control over its child instances. The ``adapter`` object lets you 
define custom ``flush``, ``done``, and ``error`` functions for the child mojit 
instances. 

Although you can also pass the ``ActionContext`` object as the ``adapter`` to 
use the default ``flush``, ``done``, and ``error`` functions, it is not 
recommended because the ``ActionContext`` object contains both parent and child 
mojit metadata, which could cause unexpected results.

Command Object
##############

In the ``command`` object below, a mojit instance of type ``Messenger`` and 
the action to execute are specified. The new mojit instance is also passed 
parameters.

.. code-block:: javascript

   var command = {
     "instance" : {
       "type": "Messenger"
     },
     "action": "index",
     "context": ac.context,
     "params": {
       "route": { "path": "/message" },
       "url": { "message_type": "email" },
       "body": { "content": "Dispatch a mojit" }

     }
   };

.. _dyn_dispatch-adapter:   

Adapter Object
##############

In the ``adapter`` object below, the ``ac.done``, ``ac.flush``, or ``ac.error`` 
are defined and will override those functions in the child mojit instance. 
See `Adapter Functions`_ for more information.

.. code-block:: javascript
  
   var adapter = {
      flush: function(data, meta){...},
      done: function(data, meta){
        var body = ac.params.body();
        var output = { "data": data, "body": body };
        ac.done(output);
      },
      error: function(err){ Y.log(err); }
   }; 

.. _dyn_dispatch-adapter_funcs:   

Adapter Functions
#################
   
The functions ``ac.done``, ``ac.flush``, and ``ac.error`` defined in the ``adapter``
object are actually implemented by the Mojito framework. For example, before 
``adapter.done`` is executed, Mojito runs the ``done`` function defined in 
`action-context.common.js <https://github.com/yahoo/mojito/blob/develop/lib/app/autoload/action-context.common.js>`_,
which collects metadata and configuration. 

.. _dyn_dispatch-controller:

Controller
----------

The controller of the mojit that is dynamically creating mojit instances 
defines the mojit instance and passes custom versions of ``done``, ``flush``, 
and ``error``. 

.. code-block:: javascript

   YUI.add('Motherlode', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var adapter = {
           done: function(data, meta){
             var body = ac.params.body();
             var output = { "data": data, "body": body };
             ac.done(output);
           },
           error: function(err){ Y.log(err); }
         }; 
         var command = {
           "instance" : {
             "type": "Messenger",
             "action": "index"
           },
           "context": ac.context,
           "params": {
             "route": { "path": "/message" },
             "url": { "message_type": "email" },
             "body": { "content": "Dispatch a mojit" }
           }
         };
         ac._dispatch(command,adapter);
      }
    };
  }, '0.0.1', {requires: ['mojito']});

.. _dyn_dispatch-templates:

Templates
---------

The template that is rendered depends on the ``adapter`` object passed to 
``ac._dispatch``. If you pass the ``ac`` object as the ``adapter`` parameter,
as in ``ac._dispatch(command,ac)``, the ``ac.done`` in the dynamically defined 
mojit will execute and its template will be rendered. If you pass a custom 
``adapter`` object defining ``done``, you can call ``ac.done`` inside your 
defined ``done`` method to pass data to the parent mojit and render its 
template.

.. _dyn_dispatch_templates-exs:

Examples
########

.. _dyn_dispatch-templates_ex_one:

Example One
***********

In this example, the mojit ``Creator`` dynamically creates the child
mojit instance of type ``Spawned``. The child mojit instance gets data 
from its parent mojit and then renders its template. The rendered template 
is returned to the parent mojit, which inserts the content into its own 
template.


.. _dyn_dispatch-templates_exs-app_config:

Application Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: javascript
  
   [
     {
       "settings": [ "master" ],
       "specs": {
         "creator": {
           "type":"Creator"
         }
       }
     }
   ]

.. _dyn_dispatch-templates_exs-controllers:

Controllers  
^^^^^^^^^^^

.. _templates_exs_controllers-creatormojit:

Creator
```````

.. code-block:: javascript 

   YUI.add('creator', function(Y, NAME) {
   
      Y.namespace('mojito.controllers')[NAME] = { 
        index: function(ac) {
          var buffer = '';
          var command = {
            "instance" : {
              "type": "Spawned",
              "action": "index"
            },
            "context": ac.context,
            "params": {
              "route": { "name":"creator" },
              "url": { "path":"/creator" },
              "body": { "message":"I have been defined and run by Creator." }
            }
          };
          var adapter = {
            "error": function(childErr) {
              Y.log('-- child error');
              ac.error(childErr);
            },
            "flush": function(childData, childMeta) {
              Y.log('-- child flush');
              buffer += childData;
            },
            "done": function(childData, childMeta) {
              console.log('-- child done');
              var meta = {};
              buffer += childData;
              Y.mojito.util.metaMerge(meta, childMeta);
              ac.done({ "child_slot": buffer }, meta);
            }
          };
          ac._dispatch(command,adapter);
        }
     };
   }, '0.0.1', {requires: ['mojito']});

.. _templates_exs_controllers-spawnedmojit:

Spawned
```````

.. code-block:: javascript 

   YUI.add('spawned', function(Y, NAME) {
   
      Y.namespace('mojito.controllers')[NAME] = { 

        "index": function(ac) {
          ac.done({ "route": ac.params.route('name'), 
                    "url": ac.params.url('path'), 
                    "body": ac.params.body("message")
          });
        }
     };
   }, '0.0.1', {requires: ['mojito']});


.. _dyn_dispatch-templates_exs-templates:

Templates
^^^^^^^^^ 

.. _templates_exs-templates_spawnedmojit:

Spawned
```````

.. code-block:: html 

   <div id="{{mojit_view_id}}">
     <h3>Child Mojit Instance</h3>
     <ul>
       <li>Route: {{route}}</li> 
       <li>Path: {{url}}</li> 
       <li>Message: {{body}}</li>
     </ul>
   </div>

.. _templates_exs-templates_creatormojit:

Creator
```````
   
.. code-block:: html

   <div id="{{mojit_view_id}}">
   <h3>Parent Mojit</h3>
     {{{child_slot}}}
   </div>

.. _dyn_dispatch-templates_ex_two:

Example Two
***********

In this example, the binder invokes its controller to dynamically define an 
instance of another mojit. The dynamically defined mojit instance renders its 
view, which is then sent to the binder to be attached to the DOM.

.. _templates_ex_two-app_config:

Application Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^

``application.json``

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame" : {
           "type" : "HTMLFrameMojit",
           "config" : {            
             "title" : "Fun with Dispatch",
             "deploy" : true,              
             "child" : {                   
               "type" : "Parent"                   
             }                             
           }                       
         }                 
       }           
     }
   ]
   
``app.js``

.. code-block:: javascript

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

   app.use(libmojito.middleware());
   app.set('port', process.env.PORT || 8666);

   // map "/" to "hello.index"
   app.get('/', libmojito.dispatch('frame.index'));
   app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
   });

.. _templates_ex_two-controllers:

Controllers  
^^^^^^^^^^^

.. _templates_ex_two-controllers_parentmojit:

Parent
``````

.. code-block:: javascript

   YUI.add('parent', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.assets.addCss("/static/parent/assets/index.css", "top");
         ac.done();
       },
       dispatch: function(ac) { 
         var command = {
           "instance" : {
             "type" : "Child",
           },
           "context" : ac.context,
           "params" : {}
         };
         ac._dispatch(command, ac);
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-assets-addon']});


.. _templates_ex_two-controllers_childmojit:

Child
`````

.. code-block:: javascript

   YUI.add('child', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         ac.assets.addCss("/static/child/assets/index.css", "top");
         var content = Math.floor(Math.random()*10001);
         ac.done({ "random_content" : content });
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-assets-addon']});
   
.. _templates_ex_two-binders:
   
Binders
^^^^^^^

.. _templates_ex_two-binders_parentmojit:

Parent
``````

.. code-block:: javascript

   YUI.add('parent-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       bind: function(node) {
         this.node = node;
         Y.one("#btndispatch").on("click", function (e) {
           this.mojitProxy.invoke("dispatch", {}, this.dispatchCallback);
         }, this);
       },
       dispatchCallback: function(error, data, meta) {
         if (error) {
           alert("error dispatching mojit :: " + Y.JSON.stringify(error));
         } else {
           Y.one("#output").append(data);
         }
       }
     };
   }, '0.0.1', {requires: ['mojito-client']});

.. _templates_ex_two-binders_childmojit:

Child
`````

.. code-block:: javascript

   YUI.add('child-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       bind: function(node) {
         this.node = node;
         var btn = node.all("#btn_remove");
         btn.on("click", function (e) {
           this.mojitProxy.destroySelf(false);
         }, this);      
         btn = null;    
       }
     };
   }, '0.0.1', {requires: ['mojito-client']});
   

.. _templates_ex_two-templates:
   
Templates
^^^^^^^^^

.. _templates_ex_two-templates_parentmojit:

Parent
``````

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <div>
       <button id="btndispatch">Dispatch a child</button>
     </div>
     <div id="output"></div>
   </div>

.. _templates_ex_two-templates_childmojit:

Child
`````

.. code-block:: html

   <div id="{{mojit_view_id}}" class="child">
     <button id="btn_remove">Remove</button>
     {{random_content}}
   </div>

.. _dyn_defined_mojits-execute:

Using ac._dispatch with ac.composite.execute
============================================

You can combine both methods to dynamically define and run a more complex 
set of mojits. The mojit that initiates the process uses ``ac._dispatch`` to 
define and run a parent mojit instance that uses ``ac.composite.execute`` in 
its controller to define and run child mojit instances. This chain of running 
dynamically defined mojit instances can be extended even further if one or more 
of the child mojit instances is using ``ac._dispatch`` or 
``ac.composite.execute``. When running a set of dynamically defined mojits, 
you should be aware that you may run into memory issues.

Because the configuration, controllers, and templates are the same when using 
``ac._dispatch`` and ``ac.composite.execute`` independently or together, please 
see `Using the Composite Addon`_ and `Using ac._dispatch`_ for implementation details. 


.. _dyn_defined_mojits-execute_ex:

Example
-------

In this example, the ``SecondLevel`` uses ``ac._dispatch`` to create a 
child mojit instance of type ``ThirdLevel``, which in turn creates a child 
mojit instance of type ``FourthLevel``. The child instance of type 
``FourthLevel`` is executed and its rendered view is returned to its 
parent mojit instance of type ``ThirdLevel``. The content is then attached 
to the parent mojit instance's template, which gets rendered and returned as 
the response.


.. _execute_ex-app_config:

Application Configuration
#########################

``application.json``

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "first_level" : {
           "type" : "HTMLFrameMojit",
           "config" : {
             "title" : "Fun with Dispatch and Execute",
             "deploy" : true,
             "child" : {
               "type" : "SecondLevel"
             }
           }
         }
       },
       "assets": {
             "css": [
               "assets/css/defaults.css"
             ]
        }
     }
   ]


``app.js``

.. code-block:: javascript

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

   app.use(libmojito.middleware());
   app.set('port', process.env.PORT || 8666);

   // map "/" to "hello.index"
   app.get('/', libmojito.dispatch('first_level.index'));
   app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
   });

.. _execute_ex-controllers:

Controllers  
###########   

.. _execute_ex-controllers_secondlevelmojit:

SecondLevel
***********

.. code-block:: javascript

   YUI.add('SecondLevel', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var command = {
           "instance": {
             "type" : "ThirdLevel",
             "action": "index"
           },
           "context" : ac.context,
           "params" : {
             "body": {
               "whoami": "ThirdLevel",
               "creator": "SecondLevel"
             }
           }
         };
         ac._dispatch(command, ac);
       }
     };
   }, '0.0.1', {requires: ['mojito']});
   
.. _execute_ex-controllers_thirdlevelmojit:

ThirdLevel
**********

.. code-block:: javascript

   YUI.add('ThirdLevel', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var params = ac.params.body();
         var cfg = {
           "view": "index",
           "children": {
             "child": {
               "type": "FourthLevel",
               "action": "index",
               "config": {
                 "creator": "ThirdLevel",
                 "whoami": "FourthLevel"
               }
             }
           }
         };
         ac.composite.execute(cfg,function(data, meta){
           ac.done(Y.merge(params, data), meta);
         });
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-composite-addon']});

.. _execute_ex-controllers_fourthlevelmojit:

FourthLevel
***********

.. code-block:: javascript

   YUI.add('FourthLevel', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var data = { "creator": ac.config.get("creator"), "whoami": ac.config.get("whoami") };
         ac.done(data);
       }
     };
   }, '0.0.1', {requires: ['mojito']});


.. _execute_ex-templates:

Templates
#########

.. _execute_ex-templates_fourthlevelmojit:

FourthLevel
***********

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h3>I am the {{whoami}} dynamically defined and run by {{creator}}.</h3>
   </div>

.. _execute_ex-templates_thirdlevelmojit:

ThirdLevel
**********

.. code-block:: html

   <link rel="stylesheet" type="text/css" href="/static/multiple_dynamic_mojits/assets/css/index.css"/>
   <div id="{{mojit_view_id}}">
     <h2>I am the {{whoami}} dynamically defined and run by {{creator}}.</h2>
     {{{child}}}
   </div>


