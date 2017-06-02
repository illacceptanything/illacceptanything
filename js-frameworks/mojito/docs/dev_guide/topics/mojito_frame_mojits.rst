============
Frame Mojits
============

.. _mojito_fw_mojits-intro:

Introduction
============

Mojito comes with the built-in utility mojits that make developing applications easier. 
Mojito currently comes with the ``HTMLFrameMojit`` that constructs Web pages from the 
skeleton HTML to the styling and content and the ``LazyLoadMojit`` that allows you to 
lazily load mojit code. Mojito plans to offer additional frame mojits in the future.


.. _mojito_fw_mojits-htmlframe:

HTMLFrameMojit
==============

The ``HTMLFrameMojit`` builds the HTML skeleton of a Web page. When you use 
``HTMLFrameMojit`` the ``<html>``, ``<head>``, and ``<body>`` elements are automatically 
created and the content from child mojits are inserted into the ``<body>`` element.  
The ``HTMLFrameMojit`` can also automatically insert assets such as CSS and JavaScript 
files into either the ``<head>`` or ``<body>`` elements.

Because it builds the Web page from the framework to the content and styling, the 
``HTMLFrameMojit`` must be the top-level mojit in a Mojito application. As the top-level 
or parent mojit, the ``HTMLFrameMojit`` may have one or more child mojits.

To see examples of applications using ``HTMLFrameMojit``, see the 
code examples `Using the HTML Frame Mojit <../code_exs/htmlframe_view.html>`_ 
and `Attaching Assets with HTMLFrameMojit <../code_exs/framed_assets.html>`_.

.. _fw_mojits_htmlframe-config:

Configuration
-------------

As with defining instances of other mojit types, you define an instance of the 
``HTMLFrameMojit`` in 
`configuration object <../intro/mojito_configuring.html#configuration-object>`_ 
of ``application.json``. Because ``HTMLFrameMojit`` must be the top-level mojit, 
its instance cannot have a parent instance, but may have one or more child 
instances.

In the example ``application.json`` below, ``frame`` is an instance of 
``HTMLFrameMojit`` that has the ``child`` instance of the ``framed`` mojit. 
After the HTML skeleton is created, the ``HTMLFrameMojit`` will insert the 
value of the ``title`` property into the ``<title>`` element and the content 
created by the ``frame`` mojit into the ``<body>`` element.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame" : {
           "type" : "HTMLFrameMojit",
           "config": {
             "title": "Title of HTML page",
             "child" : {
               "type" : "framed"
             }
           }
         }
       }
     }
   ]

To have multiple child instances, the ``HTMLFrameMojit`` instance must use the 
``children`` object to specify the child instances. In this example ``application.json``, 
the ``page`` instance of ``HTMLFrameMojit`` uses the ``children`` object to specify three 
child instances that can create content for the rendered view.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "page" : {
           "type" : "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "title": "HTMLFrameMojit Example with Children",
             "child": {
               "type": "Body",
               "config": {
                 "children" : {
                   "nav": {
                     "type": "Navigation"
                   },
                   "content": {
                     "type": "articleBuilder"
                   },
                   "footer" {
                     "type": "Footer"
                   }
                 }
               }
             }
           }
         }
       }
     }
   ]


.. _htmlframe_config-deploy:

Deploying to Client
###################

To configure Mojito to deploy code to the client, you set the ``deploy`` property of the 
`config <../intro/mojito_configuring.html#configuration-object>`_ object to ``true`` 
as shown below.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame" : {
           "type" : "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "child" : {
               "type" : "framed"
             }
           }
         }
       }
     }
   ]


.. _config_deploy-what:

What Gets Deployed?
*******************

The following is deployed to the client:

- Mojito framework
- binders (and their dependencies)

When a binder invokes its controller, if the controller has the ``client`` or ``common`` 
affinity, then the controller and its dependencies are deployed to the client as well. If 
the affinity of the controller is ``server``, the invocation occurs on the server. In 
either case, the binder is able to transparently invoke the controller.

.. _htmlframemojit-assets:

Adding Assets with HTMLFrameMojit
---------------------------------

You specify the assets for ``HTMLFrameMojit`` just as you would specify assets 
for any mojit. The basic difference is that ``HTMLFrameMojit`` will 
automatically attach ``<link>`` elements for CSS and ``<script>`` elements 
for JavaScript files to the HTML page. When using assets with other mojits, 
you have to manually add ``<link>`` elements that refer to assets to templates.  
See `Assets <./mojito_assets.html>`_ for general information about using 
assets in Mojito.

In the example ``application.json`` below, the ``HTMLFrameMojit`` instance 
``frame`` has one child mojit with a CSS asset. Because the assets are 
listed in the ``top`` object, the ``HTMLFrameMojit`` will attach the ``<link>`` 
element pointing to ``index.css`` to the ``<head>`` element.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame" : {
           "type" : "HTMLFrameMojit",
           "config": {
             "child" : {
               "type" : "framed"
             },
             "assets": {
               "top": {
                 "css": [
                   "/static/framed/assets/css/index.css"
                 ]
               }
             }
           }
         }
       }
     }
   ]

The rendered view that was constructed by the ``HTMLFrameMojit`` should look 
similar to the HTML below.

.. code-block:: html

   <!DOCTYPE HTML>
   <html>
     <head><script type="text/javascript">var MOJITO_INIT=Date.now();</script>
       <meta name="creator" content="Yahoo Mojito 0.1.0">
       <title>Powered by Mojito 0.1.0</title>
       <link rel="stylesheet" type="text/css" href="/static/framed/assets/css/index.css"/>
     </head>
     <body>
       <div id="yui_3_3_0_3_131500627867611" class="mojit">
         <h2 id="header">Framed Assets</h2>
         <p>Page Content</p>
       </div>
     </body>
   </html>


.. _mojito_fw_mojits-lazyloadmojit:

LazyLoadMojit
=============

``LazyLoadMojit`` allows you to defer the loading of a mojit instance by first 
dispatching the ``LazyLoadMoit`` as a proxy to the client. From the client, 
``LazyLoadMojit`` can then request Mojito to load the proxied mojit. This allows 
your Mojito application to load the page quickly and then lazily load parts of 
the page.


.. _fw_mojits_lazyload-how:

How Does It Work?
-----------------

The ``LazyLoadMojit`` is really a proxy mojit that dispatches its binder and an 
empty DOM node to the client. From the client, the binder sends a request to the 
controller to execute the code of the proxied (original) mojit. The output from 
the executed mojit is then returned to the binder of the ``LazyLoadMojit``, which 
attaches the output to the empty DOM node. The binder of ``LazyLoadMojit`` destroys 
itself, leaving the DOM intact with the new content.


.. _fw_mojits_lazyload-config:

Configuring Lazy Loading
------------------------

To use the ``LazyLoadMojit``, the ``application.json`` must do the following:

- create a top-level mojit instance of type ``HTMLFrameMojit``
- deploy the mojit instance of type ``HTMLFrameMojit`` to the client (``"deploy": true``)
- create a container mojit that has children mojit instances (``"children": { ... }``)
- defer the dispatch of the mojit instance that will be lazily loaded (``"defer": true``)

In the example ``application.json`` below, the child mojit instance ``myLazy`` is 
configured to be lazily loaded. The action (``hello``) of the proxied mojit is also 
configured to be executed after lazy loading is complete.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame": {
           "type": "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "child": {
               "type": "Container",
               "config": {
                 "children": {
                   "myLazy": {
                     "type": "LazyPants",
                     "action": "hello",
                     "defer": true
                   },
                   "myActive": {
                      "type": "GoGetter",
                   }
                 }
               }
             }
           }
         }
       }
     }
   ]


.. _fw_mojits_lazyload-ex:

Example
-------

This example shows you application configuration as well as the code for the 
parent mojit and the child mojit that is lazy loaded.  If you were to run 
this lazy load example, you would see the content of the parent mojit first 
and then see the rendered view of the child mojit loaded onto the page. 


.. _lazyload_ex-app_config:

Application Configuration
#########################

The application configuration for this example (shown below) meets the 
requirements for using ``LazyLoadMojit``:

- creates the ``frame`` mojit instance of type ``HTMLFrameMojit``
- sets ``"deploy"`` to ``true`` for ``frame`` so that the code is deployed 
  to the client
- creates the ``child`` mojit instance that has the ``children`` object 
  specifying child mojit instance
- configures the ``myLazy`` mojit instance to defer being dispatched, which 
  causes it to be lazily loaded by ``LazyLoadMojit``

In this ``application.json``, the ``parent`` mojit instance has the one child 
``myLazy``. The ``myLazy`` mojit instance of type ``LazyChild`` is 
the mojit that will be lazily loaded by ``LazyLoadMojit``. In a production 
application, you could configure the application to have many child instances 
that are lazily loaded after the parent mojit instance is already loaded onto 
the page.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame": {
           "type": "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "parent": {
               "type": "Container",
               "config": {
                 "children": {
                   "myLazy": {
                     "type": "LazyChild",
                     "action": "hello",
                     "defer": true
                   }
                 }
               }
             }
           }
         }
       }
     }
   ]


.. _lazyload_ex-container_mojit:

Container Mojit
###############

The ``Container`` mojit uses ``ac.composite.done`` to execute its child mojits.

.. code-block:: javascript

   YUI.add('container', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
     /**
     * Method corresponding to the 'index' action.
     *
     * @param ac {Object} The ActionContext that
     * provides access to the Mojito API.
     */
       index: function(ac) {
         ac.composite.done();
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-composite-addon']});

Instead of waiting for the child mojit to execute, the partially rendered view 
of the ``Container`` mojit is immediately sent to the client. After the child 
mojit is lazily loaded, the content of the executed child replaces the Handlebars 
expression ``{{{myLazy}}}``.

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h1>Lazy Loading</h1>
     <hr/>
       {{{myLazy}}
     <hr/>
   </div>


.. _lazyload_ex-lazychild_mojit:

LazyChild Mojit
###############

The ``LazyLoadMojit`` in the ``application.json`` is configured to lazily load 
the mojit instance ``myLazy`` and then call the action ``hello``. Thus, 
the ``index`` function in the ``LazyChild`` mojit below is never called.

.. code-block:: javascript

   YUI.add('lazychild', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       hello: function(ac) {
         ac.done({time: new Date()});
       },
       index: function(ac) {
         ac.done("This is never seen.");
       }
     };
   }, '0.0.1', {requires: ['mojito']});

The template ``hello.hb.html`` is rendered on the server and then lazily loaded 
to the client.

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h2>I was lazy-loaded at {{{time}}}.</h2>
   </div>

.. _mojito_fw_mojits-create:

Creating Custom Frame Mojits
============================

In addition to the frame mojits that come with Mojito, you can create your own
frame mojit, which is just another mojit that manages assets, metadata, executes 
child mojits, constructs the HTML page, and anything else that you want it to do.

Before creating a custom frame mojit, we recommend that you have 
done the following:

- used the ``HTMLFrameMojit`` and the ``Composite`` addon in a Mojito application
- examined the `HTMLFrameMojit code <https://github.com/yahoo/mojito/tree/master/lib/app/mojits/HTMLFrameMojit>`_
  that is part of Mojito

.. _create_frame_mojits-why:

Why Create a Custom Frame Mojit?
--------------------------------

By being able to create a custom frame mojit, you can control how the HTML page
is constructed, from the HTML skeleton, the metadata, and attachment of assets, to
the rendering of mojits in the page. For example, you could create a dynamic
HTML title, add custom metadata in the ``head`` element, or change the organization
of the page based on the runtime environment.

.. _create_frame_mojits-cannot_do:

What Frame Mojits Cannot Do
---------------------------

Code from your frame mojit cannot be deployed to run on the client. It **must** run on
the server. Thus, your frame mojit cannot have binders, and the controller of 
your frame mojit must have the ``server`` affinity. The frame mojit's child mojits 
handle dynamic content and user interaction.

.. _create_frame_mojits-responsibilities:

Responsibilities of the Frame Mojit
-----------------------------------

The frame mojit is responsible for the following:

- constructing the HTML page
- collecting the assets of your children and attaching them to the page
- deploying the client code of its children to the client
- executing child mojits and attaching the output to the HTML page

How your frame mojit accomplishes the tasks above largely depends on your implementation.
We will delve into these responsibilities in more detail in the following sections. 

.. _create_frame_mojits-configuring:

Configuring Your Frame Mojit
----------------------------

The configuration to use a custom frame mojit should be similar to the configuration
for using the ``HTMLFrameMojit``, but you have more flexibility because you
have control over the implementation of the frame mojit.

In the ``application.json`` below, the instance ``fm`` of the frame mojit ``MyFrame`` 
is configured to have children mojits. The implementation of ``MyFrame`` will
need to get the configuration of the ``children`` and then execute them
using the Composite addon.

.. code-block:: javascript

   [
     {
       "settings": ["master"],
       "specs": {
         "fm": {
           "type": "MyFrame",
           "config": {
             "children": {
               "header": {
                 "type": "Header"
               },
               "body": {
                 "type": "Body"
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

You can also implement your frame mojit to use a ``child`` that has its own children 
mojits. Implementing the controller of the frame mojit to use a ``child`` with its own
children is more difficult, but the advantage is that the 
child of your frame mojit can have binders and direct control over its children
using the ``Composite`` addon. 

In the case of your frame mojit having a ``child`` with its own ``children``, 
your ``application.json`` might be similar to the following:

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "fm": {
           "type": "MyFrame",
           "config": {
             "title": "Title of HTML page",
             "child" : {
               "type" : "Body",
               "config": {
                 "children": {
                   "header": {
                     "type": "Header"
                   },
                   "body": {
                     "type": "Content"
                   },
                   "footer": {
                     "type": "Footer"
                   }
                 }
               }
             }
           }
         }
       }
     }
   ]


.. _create_frame_mojits-controller:

Frame Mojit Controller
----------------------

.. _frame_mojits_controller-reqs:

Requirements
############

Frame mojits must meet the following requirements:

- Be the top-level mojit. Your frame mojit cannot be the child of another mojit instance.

- Be on the server. In other words, the frame mojit's controller file must have the
  ``server`` affinity, and the frame mojit **cannot** have binders.

- Handle the assets of the children. The implementation is up to the developer, but
  one way to do this would be to use the Assets addon to attach the assets of the children 
  to the page, where ``meta.assets`` is from the children: 
  ``ac.assets.addAssets(meta.assets);``

- Deploy the children's client code to the client using the following:
  ``ac.deploy.constructMojitoClientRuntime(ac.assets, meta.binders);``

- Execute the children with the Composite addon using either the ``execute`` or
  ``composite`` method. 

.. _create_frame_mojits-ex:

Example
-------

The following example only provides the application configuration and 
the frame mojit code. For the entire application, 
see `frame_app <https://github.com/caridy/Mojito-Apps/tree/master/frame_app>`_.

application.json
################

.. code-block:: javascript

   [
     {
       "settings": ["master"],
       "specs": {
         "ms": {
           "type": "IntlHTMLFrame",
           "config": {
             "children": {
               "header": {
                 "type": "Header"
               },
               "body": {
                 "type": "Body"
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

IntlHTMLFrame
#############

controller.server.js
********************

.. code-block:: javascript

   YUI.add('intlhtmlframe', function (Y, NAME) {
     'use strict';
     Y.namespace('mojito.controllers')[NAME] = {
       index: function (ac) {

         // The frame mojit uses the Composite addon to execute the children 
         // instances defined in the 'specs' object of application.json.
         ac.composite.execute(ac.config.get(), function (data, meta) {
           // Required: implementation up to developer
           // Add the assets from the children to the frame mojit before 
           // doing anything else.
           ac.assets.addAssets(meta.assets);
           // Required: must use 'constructMojitoClientRuntime'
           // The frame mojit deploys the client code (YUI) from the 
           // children to the client.
           ac.deploy.constructMojitoClientRuntime(ac.assets, meta.binders);
           //  Required: must execute the frame mojit with 'ac.done'
           // 1. Merge the bottom and top fragments from assets into
           //    the template data, along with title and Mojito version.
           // 2. Merge the meta object with the children's meta object. 
           // 3. Add HTTP header information and view name.
           ac.done(
             Y.merge(data, ac.assets.renderLocations(), {
               "name": NAME,
               "page-title": "some fancy title... from intl",
               "greeting": ac.intl.lang("GREETING"),
               "says": ac.intl.lang("SAYS"),
               "preposition": ac.intl.lang("PREPOSITION"),
               "today": ac.intl.formatDate(new Date()),
               "mojito_version": Y.mojito.version
             }),
             Y.merge(meta, {
               http: {
                 headers: {
                  'content-type': 'text/html; charset="utf-8"'
                 }
               },
               view: {
                 name: 'index'
               }
             })
           );
          });
        }
      };
    }, '0.1.0', {requires: [
       'mojito',
       'mojito-assets-addon',
       'mojito-deploy-addon',
       'mojito-config-addon',
       'mojito-composite-addon',
       'mojito-intl-addon'
   ]});

index.hb.html
*************

.. code-block:: html

   <html>
     <head>
     {{#meta}}
       <meta name="{{name}}" content="{{content}}">
     {{/meta}}
     {{^meta}}
       <meta name="creator" content="Yahoo Mojito {{mojito_version}}">
     {{/meta}}
       <title>{{page-title}}</title>
        {{{top}}}
     </head>
     <body>
       <div id="intl_frame" class="header" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">
         <h2>{{{name}}} {{{says}} {{{greeting}}} {{{preposition}}} {{{today}}}</h2>
         <div id="{{mojit_view_id}}" class="mojit" style="border: dashed black 1px;">
           <h3>{{title}}</h3>
           <div class="header" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">
             {{{header}}}
           </div>
           <div class="body" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">
             {{{body}}}
           </div>
           <div class="footer" style="border: dashed black 1px; margin: 10px 10px 10px 10px;">
             {{{footer}}}
           </div>
         </div>
       </div>
       {{{bottom}}}
     </body>
   </html>
