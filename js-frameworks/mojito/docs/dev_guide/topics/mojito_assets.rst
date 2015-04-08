======
Assets
======

.. _mojito_assets-intro:

Introduction
============

Assets are resources that are required on the clients. These resources are 
primarily CSS but can also be JavaScript that is ancillary to and not a 
core component of the Mojito application. This topic discusses the following:

- location of assets
- configuring applications to use assets
- accessing assets from controllers and views

To see code examples that demonstrate how to use assets, see 
`Code Examples: Assets <../code_exs/#assets>`_.

.. _mojito_assets-loc:

Location of Asset Files
=======================

Assets can be used at the application level and the mojit level. For 
application-level assets,  CSS and JavaScript files are placed in 
the ``{application_name}/assets`` directory. For mojit-level assets,  
CSS and JavaScript files are placed in the 
``{application_name}/mojits/{mojit_name}/assets`` directory.

To better organize your assets, you can create separate directories for CSS and 
JavaScript files under the ``assets`` directory. The names of the directories 
that you create are arbitrary, but the convention is to create the directories 
``css`` for CSS files and  ``js`` for JavaScript files. For example, the 
application-level CSS assets could be placed in the following directory: 
``{application_name}/assets/css``

.. _mojito_assets-config:

Configuration
=============

You specify the location of your assets in the ``assets`` object specified in 
the configuration file ``application.json``. Mojito will read the configuration 
file and create a static path to your assets that you can use from your views.

.. _assets_config-assets_obj:

assets Object
-------------

In the ``application.json`` file, you use the ``assets`` object to specify the 
type of asset, the location, and where you would like Mojito to include 
the asset in the view.  The tables below describe the ``assets`` object and its 
fields.

+----------------+----------------------+---------------+------------------------------------------------------------------+------------------------------------------------------------------+
| Property       | Data Type            | Required?     | Example                                                          | Description                                                      |
+================+======================+===============+==================================================================+==================================================================+
| ``top``        | object               | No            | ``"top": { "css":[ "/mojits/framed/assets/css/index.css",``      | Contains asset information that Mojito will automatically        |
|                |                      |               | ``"/assets/css/index.css" ] }``                                  | insert into the HTML ``<head>`` tag when using the               |
|                |                      |               |                                                                  | ``HTMLFrameMojit``.                                              |
+----------------+----------------------+---------------+------------------------------------------------------------------+------------------------------------------------------------------+
| ``bottom``     | object               | No            | ``"bottom": { "css":[ "/mojits/framed/assets/css/index.css",``   | Contains asset information that Mojito will automatically        |
|                |                      |               | ``"/assets/css/index.css" ] }``                                  | insert into the HTML ``<body>`` tag when using the               |
|                |                      |               |                                                                  | HTMLFrameMojit.                                                  |
+----------------+----------------------+---------------+------------------------------------------------------------------+------------------------------------------------------------------+
| ``css``        | array of strings     | Yes           | ``"css":[ "/mojits/framed/assets/css/index.css",``               | List of one or more paths to CSS files. Required if you want     |
|                |                      |               | ``"/assets/css/index.css" ]``                                    | to include CSS assets.                                           |
+----------------+----------------------+---------------+------------------------------------------------------------------+------------------------------------------------------------------+
| ``js``         | array of strings     | Yes           | ``"js":[ "/mojits/framed/assets/js/bells.js",``                  | List of one or more paths to JavaScript files. Required if       |
|                |                      |               | ``"/assets/js/whistles.css" ]``                                  | you want to include JavaScript assets.                           |
+----------------+----------------------+---------------+------------------------------------------------------------------+------------------------------------------------------------------+


.. _assets_config-assets_ex:

Examples
--------

In the ``application.json`` below, the ``assets`` object specifies the paths to 
the CSS and JavaScript assets:

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "simple" : {
           "type": "simple"
         },
         "assets": {
           "css": [
             "/mojits/framed/assets/css/index.css",
             "/assets/css/defaults.css"
           ],
           "js": [
             "/mojits/framed/assets/js/index.js"
           ]
         }
       }
     }
   ]


This ``application.json`` configures Mojito to use the ``HTMLFrameMojit`` 
that automatically inserts a ``<link>`` tag pointing to ``index.css`` into 
the ``<head>`` tag of the rendered view.

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
                   "/static/framed/assets/dog/index.css"
                 ]
               }
             }
           }
         }
       }
     }
   ]


.. _mojito_assets-accessing:

Accessing Assets from an Application
====================================

When specified in ``application.json``, assets can be accessed through a static 
URL created by Mojito. The static URLs start with ``/static/`` and point to 
either the ``assets`` directory under the mojit or application directory, 
depending on whether the asset is at the application or mojit level.

.. _assets_access-static_url:

Syntax for Static URL
---------------------

For application-level assets, the static URL has the following syntax:

``/static/{application_name}/assets/{asset_file}``

For mojit-level assets, the static URL has the following syntax:

``/static/{mojit_name}/assets/{asset_file}``


.. _static_url-refer:

Referring to the Static URL in the Template
-------------------------------------------

Once Mojito has created a static URL to an asset, you can use the ``<link>`` 
tag in your view to refer to the asset. In the example index template below, 
the ``<link>`` tag refers to the static URL to the asset ``index.css``.

.. code-block:: html

   <link rel="stylesheet" type="text/css" href="/static/simple/assets/css/index.css"/>
   <div id="{{mojit_view_id}}" class="mojit">
     <h2 id="header">{{title}}</h2>
     <ul class="toolbar">
       {{#colors}}
         <li>{{id}}</li>
       {{/colors}}
     </ul>
   </div>

From the static URL, you cannot tell whether the asset is at the mojit or application 
level, but you do know that either the application or the mojit is ``simple``.

.. _mojito_assets-using:

Using the Assets Addon
======================

Mojito provides an `Assets addon <../../api/classes/Assets.common.html>`_ 
that allows you to add inline assets or links to asset files. Using the ``Assets`` 
addon, you can dynamically add assets to an HTML page. Two possible use cases would 
be adding CSS if the HTTP request is coming from a particular device or adding 
JavaScript if a user takes a particular action.

In the mojit controller below, the ``Assets`` addon is used to add metadata and CSS 
for requests from iPhones. The ``assets.addBlob`` method adds 
the ``<meta>`` tag and the ``addCss`` method adds the device-specific CSS.

.. code-block:: javascript

   YUI.add('device', function(Y, NAME){
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var device = ac.context.device, css = "./simple";
         if (device === 'iphone') {
           // Attach viewport meta-data
           ac.assets.addBlob('<meta name = "viewport" content = "width = device-width">', 'top');
           ac.assets.addBlob('<meta name = "viewport" content = "initial-scale = 1.0">', 'top');
           // Modify the style sheet name.
           css += '.' + device;
         }
         // Attach the style sheet.
         css += '.css';
         ac.assets.addCss(css, 'top');
         // Push data to the template.
         ac.done(
           {
             title: "Device Assets",
             colors: [
               {
                 id: "green", rgb: "#616536"
               },
               {
                 id: "brown", rgb: "#593E1A"
               },
               {
                 id: "grey",
                 rgb: "#777B88"
               },
               {
                 id: "blue",  rgb: "#3D72A4"
               },
               {
                 id: "red",   rgb: "#990033"
               }
             ]
           }
         );
       }
     };
   }, '0.0.1', {requires: ['mojito-assets-addon']});


.. _mojito_assets-yui_assets:

YUI Assets
==========

YUI modules should be placed in the  ``yui_modules`` directory and **not** 
the ``assets`` directory. When your mojit code wants to use one of the YUI 
modules in the ``yui_modules`` directory, you add the module name in the 
``requires`` array, and Mojito will automatically load the module.

For example, to use a YUI module called ``substitute`` in your mojit 
controller, you would place the ``substitute.js`` file in the 
``yui_modules`` directory and then add the module name in the ``requires`` 
array as seen in the example mojit controller below.

.. code-block:: javascript

   YUI.add('textprocessor', function(Y, NAME){
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         // Use the 'substitute' module
         var name = Y.substitute("Welcome {name}!", {"name":ac.getFromMerged("name")});
         ac.done (name);
       }
     }
   }, '0.0.1', {requires: ['substitute']});



.. _mojito_assets-rollup:

Rolling Up Static Assets
========================

Mojito lets you `compile views, configuration, and YUI modules <../reference/mojito_cmdline.html#compile-system>`_, 
but has no native support for rolling up static assets. Fortunately, you can 
use the npm module `Shaker <https://github.com/yahoo/mojito-shaker>`_ to roll 
up static assets for Mojito applications. Shaker lets you create production 
rollups at build time, push rollups to a `content delivery network (CDN) <http://en.wikipedia.org/wiki/Content_delivery_network>`_, 
customize rollups based on `context configurations <../topics/mojito_using_contexts.html>`_, 
and more. See the `Shaker documentation <../../../shaker/>`_ for more information.

.. _mojito_assets-inline:

Inline CSS
==========

You can use the Mojito command-line tool to compile a mojit's CSS so that the 
CSS is automatically inlined in rendered views. The mojit, however, **must** 
be a child of the `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_.

When you run ``mojito compile inlinecss``, the CSS files in 
``/mojits/{mojit_name}/assets/`` are compiled into the YUI module 
``/mojits/{mojit_name}/yui_modules/compiled/inlinecss.common.js``.
Mojito will use the compiled CSS and insert inline CSS into the ``<head>`` 
element of the rendered view. See also 
`Compiling Inline CSS <../reference/mojito_cmdline.html#compiling-inline-css>`_.
