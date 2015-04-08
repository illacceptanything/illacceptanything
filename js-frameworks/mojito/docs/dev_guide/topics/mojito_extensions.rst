================
Extending Mojito
================

.. _mojito_extending-intro:

Introduction
============

The Mojito framework lets you add features and extend functionality through 
addons, libraries, and middleware. This chapter discusses how to create 
extensions and where to place files in the Mojito 
framework.

.. _mojito_extending-addons:

Addons
======

In addition to the `Action Context <../../api/classes/ActionContext.html>`_ 
addons that Mojito provides, you can create your own addons to add functionality 
to controller actions.

Addons allows you to do the following:

- wrap third-party Node.js libraries and YUI libraries
- inspect the content of the ``ActionContext`` object
- call methods on the ``ActionContext`` object, which can be references through 
  ``this.get('host')``

.. _addons-creating:

Creating New Addons
-------------------

An addon is simply a JavaScript file that contains a YUI module. You can create 
addons at the application and mojit level. Application-level addons are 
available to all mojits in the application, whereas, mojit-level addons are 
only available to its mojit.

.. _extending_addons-naming:

Naming Convention
#################

The name of an addon should have the following syntax where ``{addon_namespace}`` 
is the namespace of the addon and ``{affinity}`` is 
``server``, ``common``, or ``client``. 

``{addon_namespace}.{affinity}.js``


.. _extending_addons-loc:

Location of Addons
##################

Application-level addons should be placed in the following directory:

``{app_dir}/addons/ac/``

Mojit-level addons should be placed in the following directory:

``{mojit_dir}/addons/ac/``


.. _extending_addons-namespace:

Namespace of Addon
##################

To access addon methods and properties, you use the Action Context object 
(i.e., ``ac``) and the addon namespace. For example, the namespace of the 
addon ``addons/ac/foo.common.js`` would be ``foo``.  Thus, to call the method 
``save`` of the ``foo`` addon, you would use ``ac.foo.save``.

See :ref:`Using Your Addon <extending_addons-using>` for an example
of how to call methods from the addon namespace.


.. _extending_addons-writing:

Writing the Addon
#################

The ActionContext object and ActionContext addons simulate the 
`YUI Base <https://developer.yahoo.com/yui/3/base/>`_ and 
`YUI Plugin <https://developer.yahoo.com/yui/3/plugin/>`_ infrastructure, so  
creating a new addon is similar to creating a new YUI Plugin.

The addon must do the following:

- register a plugin name, which is the string passed to ``YUI.add``
- constructor with a ``prototype`` property
- assign the constructor to a namespace of ``Y.mojito.addons.ac``, 
  so Mojito can access your addon
- require the 'mojito' module, otherwise the namespace ``Y.mojito.addons.ac`` might 
  not be available (unless you use ``Y.namespace()``).

**Optional:** ``requires`` array to include other modules.
The code snippet below shows the skeleton of an addon with the registered 
plugin name (``'addon-ac-cheese'``), the constructor (``CheeseAcAddon``) with its
``prototype`` property, and requires the ``mojito`` module.

.. code-block:: javascript

   // Register the plugin name (must be unique)
   YUI.add('addon-ac-cheese', function(Y, NAME) {
     // Constructor for addon
     function CheeseAcAddon(command, adapter, ac) {
       // The "command" is the Mojito internal details
       // for the dispatch of the mojit instance.
       // The "adapter" is the output adapter, containing
       // the "done()", "error()", etc, methods.
       // The "ac" is the ActionContext object to which
       // this addon is about to be attached.
     }
     // Use the prototype property to add
     // methods and other properties
     CheeseAcAddon.prototype = {
     // Put methods and properties here
     }
     // Assign the constructor of the addon to a
     // namespace of Y.mojito.addons.ac
     Y.mojito.addons.ac.cheese = CheeseAcAddon;
     // Optional: 'requires' array could include other
     // YUI modules if needed.
   }, '0.0.1', {requires: ['mojito']});


.. _extending_addons-writing_ex:

Example Addon
#############

In the example addon ``cheese.common.js`` below, the ``YUI.add`` method 
registers the ``addon-ac-cheese`` plugin. The addon namespace 
is ``cheese``, and the addon has the one method ``cheesify``, which is 
added through the ``prototype`` property.

.. code-block:: javascript

   YUI.add('addon-ac-cheese', function(Y, NAME) {
     function CheeseAcAddon(command, adapter, ac) {
       // The "command" is the Mojito internal details
       // for the dispatch of the mojit instance.
       // The "adapter" is the output adapter, containing
       // the "done()", "error()", etc, methods.
       // The "ac" is the ActionContext object to which
       // this addon is about to be attached.
     }
     CheeseAcAddon.prototype = {
       // The "namespace" is where in the ActionContext
       // the user can find this addon. The namespace
       // must be the same as the first part of the addon file.
       // Thus, this addon file must be named 'cheese'.{affinity}.js'
       namespace: 'cheese',
       cheesify: function(obj) {
         var n;
         if (Y.Lang.isString(obj)) {
           return 'cheesy ' + obj;
         }
         for (n in obj) {
           if (obj.hasOwnProperty(n)) {
             obj[n] = this.cheesify(obj[n]);
           }
         }
         return obj;
       }
     };
     // If this addon depends on another, that can
     // be specified here. Circular dependencies are not
     // supported or automatically detected,
     // so please be careful.
     CheeseAcAddon.dependsOn = ['http'];
     Y.mojito.addons.ac.cheese = CheeseAcAddon;
   }, '0.0.1', {requires: ['mojito']});


.. _extending_addons-using:

Using Your Addon
################

The addon in `Example Addon`_ registered the plugin ``addon-ac-cheese`` and 
made its constructor available through the namespace ``cheese``. This namespace
must match the first part of the addon file name, so the addon file name is 
``cheese.common.js``.

Addons are not automatically added to the ActionContext, so to access an 
addon, your controller needs to add the YUI plugin name to the ``requires`` 
array. The YUI plugin name is the string passed to ``YUI.add`` in the addon. 
To invoke the addon methods, you use the Action Context object (``ac``) with 
the addon namespace: ``ac.cheese.{addon_method}``


.. code-block:: javascript

   YUI.add('foo', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         // Use the namespace defined by the 
         // addon file name ('cheese') with
         // the addon function 'cheesify'
         var cheesy = ac.cheese.cheesify({
           food: "nachos",
           things: "jokes"
         });
       }
     };
     // To use your addon, add 'addon-ac-cheese' to your
     // 'requires' array.
   }, '0.0.1', {requires: [ 'mojito', 'addon-ac-cheese']});


.. _mojito_extending-middleware:

Middleware
==========

.. _extending_middleware-intro:

Introduction
------------

Middleware is code that can handle (or modify) the HTTP request in the server. 
As of Mojito v0.9, you no longer configure the middleware directory in
``application.json`` and instead use ``app.js`` to include middleware, by either
directly writing the middleware code into the file or including it.

Because Mojito middleware is based on the HTTP middleware 
`Connect <http://senchalabs.github.com/connect/>`_,  the code must follow 
the Connect API. Next, we'll cover the available Mojito middleware and
show examples how to include the Mojito middleware, select specific Mojito
middleware modules, and write your write your own custom middleware.

.. _extending_middleware-configure:

Configuring Middleware
----------------------

Mojito ships with a default list of middleware to facilitate the use of the
Mojito dispatcher. These middleware are responsible for augmenting the request
object for dispatcher to function, or just simply giving access to the RPC
tunnel or static assets. Here is the default list in order:

.. code-block:: javascript

   [
      'mojito-handler-static',
      'mojito-parser-body',
      'mojito-parser-cookies',
      'mojito-contextualizer',
      'mojito-handler-tunnel'
   ]

Mojito also provides a sugar method to use them in your Express application
through the method ``app.use``, just like any traditional Connect middleware. This is
the recommended way of using the default Mojito middleware:

.. code-block:: javascript

    var express = require('express'),
        libmojito = require('mojito'),
        app;

    app = express();
    libmojito.extend(app);
    app.use(libmojito.middleware());

By calling ``libmojito.middleware``, your Express application will execute
the default Mojito middleware in the correct order.

.. _extending_middleware-example:

Example
-------

If the default list of mojito middleware does not fit your needs, you can 
individually select middleware to provide a custom order and a custom organization.
The following is an example of the mix-in between the default Mojito middleware and
application specific middleware:

.. code-block:: javascript

    var express = require('express'),
        libmojito = require('mojito'),
        app;

    app = express();
    libmojito.extend(app);
    app.use(function (req, res, next) {
      // inline middleware to intercept an HTTP request and lowercases
      // URLs containing the string "module_"
      if (req.url.indexOf('module_') > -1) {
        req.url = req.url.toLowerCase();
      }
      next();
    });
    app.use(libmojito.middleware['mojito-handler-static']());
    app.use(libmojito.middleware['mojito-parser-body']());
    app.use(libmojito.middleware['mojito-parser-cookies']());
    app.use(libmojito.middleware['mojito-contextualizer']());
    app.use(customContextualizerMiddleware());
    app.use(libmojito.middleware['mojito-handler-tunnel']());
    app.use(anotherCustomMiddleware());

.. _mojito_extending-libraries:

Libraries
=========

Mojito allows you to use YUI libraries, external libraries, or customized 
libraries. To use any library in Mojito, you need to specify the module in 
either the ``requires`` array in the controller for YUI libraries or by using 
the ``require`` method for Node.js modules.

.. _extending_libraries-yui:

YUI Library
-----------

YUI libraries can be made available at the application or the mojit level. 
Each file can only have one ``YUI.add`` statement. Other components, such 
as controllers, models, etc., needing the library should specify the YUI 
module name in the ``requires`` array.

.. _libraries_yui-naming:

File Naming Convention
######################

The file name of a YUI module should have the following syntax where 
``{yui_mod_name}`` is a unique YUI module name defined by the user and 
``{affinity}`` is ``server``, ``common``, or ``client``.

``{yui_mod_name}.{affinity}.js``

.. _libraries_yui-loc:

Location of YUI Modules
#######################

Application-level YUI modules should be placed in the following directory:

``{app_dir}/yui_modules/``

Mojit-level YUI modules should be placed in the following directory:

``{mojit_dir}/yui_modules/``


.. _libraries_yui-using:

Using the YUI Module
####################

In the sections below, we provide an example YUI module and then show how
to require and use that YUI module from a controller.

.. _yui_using-hello-uid:

hello-uid Module
****************

In the code example below, the ``create_id`` function becomes the constructor 
for the ``UID`` namespace. This will let you create an instance, and the 
``prototype`` object then allows you to access the method ``log`` from that 
instance.

``{mojit_dir}/yui_modules/hello-uid.server.js``

.. code-block:: javascript

   YUI.add('hello-uid', function(Y){
     function create_id(){
       var uid = Math.floor(Math.random()*10000000);
       this.uid = uid;
     }
     create_id.prototype = {
       log: function(user_name){
         Y.log(user_name + "'s UID is " + '['+this.uid+']');
       }
     };
     Y.namespace('mojito').UID = create_id;
   });


Using the hello-uid Module
**************************

In the example mojit controller below, the YUI module ``hello-uid`` is loaded 
because the module is in the ``requires`` array. The ``log`` method can then be 
called from an instance (i.e., ``uid``) of the module.

``{mojit_dir}/controller.server.js``

.. code-block:: javascript

   YUI.add('hello', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(ac) {
         var user_name = ac.params.getFromMerged("name") || "User";
         var uid = new Y.mojito.UID();
         uid.log(user_name);
         ac.done('Hello World!');
       }
     };
   }, '0.0.1', {requires: ['hello-uid']});


.. _extending_libraries-other:

Other Libraries
---------------

Non-YUI libraries can also be used at either the application or mojit level. 
Because Node.js and **not** Mojito will read the contents of the library files, 
you need to use the function ``require`` to include the library. Mojito will only confirm 
that the files exist.

.. _libraries_other-loc:

Location of Non-YUI Libraries
#############################

Application-level libraries should be placed in the following directory:

``{app_dir}/libs/``

Mojit-level libraries should be placed in the following directory:

``{mojit_dir}/libs``


.. _mojito_extending-ve:

View Engines
============

.. _extending_ve-overview:

Overview
--------

A view engine is the piece of code that takes the data returned by a controller 
and applies it to a view. This is most often done by interpreting the view as 
a template. View engines in Mojito can be at either the application or mojit 
level. Application-level view engines are available to all mojits.

The view engine consists of an addon that we will refer to as the view engine 
addon to differentiate it from other addons. The view engine addon can include 
code that renders templates or use a rendering engine, such as 
`Embedded JavaScript (EJS) http://embeddedjs.com/>`_, to render templates. 
In the latter case, the view engine addon acts as an interface between the 
Mojito framework and the rendering engine. 

In the following sections, we will be discussing how to create a view engine 
addon that relies on a rendering engine, not how to write code that renders
templates.

.. _ve_overview-term:

Terminology
###########

The following list may help clarify the meaning of commonly used terms in this 
section.

- **view engine** - The code used to apply data to a view. In Mojito, the view 
  engine consists of a view engine addon. 
- **view engine addon** - The Mojito addon that compiles and renders templates. 
  The addon typically relies on a rendering engine to compile and render templates, 
  but may include code to do the compilation and rendering. 
- **rendering engine** - The rendering engine is typically off-the-shelf 
  technology, such as `Dust <http://akdubya.github.com/dustjs>`_, 
  `Jade <http://jade-lang.com/>`_, or `EJS <http://embeddedjs.com/>`_, that 
  renders the template into markup for an HTML page.
- **template** - The template file (chosen by the controller) that contains 
  tags and HTML that is rendered into markup for an HTML page.

.. _extending_ve-steps:

General Steps for Creating View Engines
---------------------------------------

#. Use ``npm`` to install the rendering engine into your Mojito application or 
   copy it into a directory such as ``{app_dir}/libs``.
#. Create a view engine addon that references the rendering engine with a 
   ``require`` statement and 
   meets the :ref:`requirements of the view engine addon <reqs_ve_addon>`.
#. Create templates using the templates for the rendering engine and place 
   them in ``{mojit_dir}/views``. 

.. _extending_ve-naming:

File Naming Conventions 
-----------------------

.. _ve_naming-addon:

View Engine Addon
#################

The name of the addon should have the following syntax where ``{view_engine_name}`` 
is the view engine and ``{affinity}`` is ``server``, ``common``, or ``client``.

``{view_engine_name}.{affinity}.js``

.. _ve_naming-template:

Template
########

The name of the template should have the following syntax where 
``{view_engine_name}`` should be the same as the ``{view_engine_name}`` in 
the file name of the view engine addon.

``{action}.{view_engine_name}.html``

.. _extending_ve-loc:

File Locations
--------------

.. _ve_loc-app_level:

Application-Level View Engine Addons
####################################

``{app_dir}/addons/view-engines``

.. _ve_loc-mojit_level:

Mojit-Level View Engine Addons
##############################

``{mojit_dir}/addons/view-engines``

.. _ve_loc-rendering:

Rendering Engines
#################

Mojito does not require rendering engines to be in a specific location. The 
recommended practice is to use ``npm`` to install rendering engines into 
the ``node_modules`` directory or copy the rendering engine into the ``libs`` 
directory as shown below:

``{app_dir}/node_modules/{rendering_engine}``

``{app_dir}/libs/{rendering_engine}``

``{mojit_dir}/libs/{rendering_engine}}``

.. note:: If you are using mojit-level view engine addons, the rendering engine 
          should be at the mojit level as well, such as 
          ``{mojit_dir}/libs/{rendering_engine}``.


.. _reqs_ve_addon:

Requirements of the View Engine Addon
-------------------------------------

The view engine addon must have the following:

- a ``YUI.add`` statement to register the addon. For example:

  .. code-block:: javascript

     YUI.add('addons-viewengine-ejs', function(Y, NAME) {
    
       // The addon name 'addons-viewengine-ejs' is registered by YUI.add
    
     }, '0.1.0', {requires: []});

- an object that is assigned to ``Y.mojito.addons.viewEngines.{view_engine_name}`` 
  as seen below:
   
  .. code-block:: javascript
      
     ...
       function EjsAdapter(viewId) {
         this.viewId = viewId;
       }
       ...
       Y.namespace('mojito.addons.viewEngines').ejs = EjsAdapter;
     ...      

- a prototype of the object has the following two methods ``render`` and ``compiler`` 
  as shown below:

  .. code-block:: javascript
   
     ...
       EjsAdapter.prototype = {
       
         render: function(data, mojitType, tmpl, adapter, meta, more) {
           ...
         },
         compiler: function(tmpl) {
            ...
         }
     ...

    
.. _reqs_ve-methods: 
  
Methods for the View Engine Addon
---------------------------------

.. _ve_methods-render: 

render
######

.. _ve_render-desc: 

Description
***********

Sends a rendered template as the first argument to the methods ``adapter.flush`` 
or ``adapter.done``.

.. _ve_render-sig: 

Signature
*********

``render(data, mojitType, tmpl, adapter, meta, more)``

.. _ve_render-params: 

Parameters
**********

- ``data`` (Object) - the data to render.
- ``mojitType`` (String) - the mojit whose view is being rendered.
- ``tmpl`` - (String) - path to template to render.
- ``adapter`` (Object) - the output adapter to use.
- ``meta`` (Object) - the metadata that should be passed as the second argument 
  to ``adapter.flush`` 
  or ``adapter.done``
- ``more`` (Boolean) - if ``true``, the addon should call the method 
  ``adapter.flush``, and if ``false``, call the method ``adapter.done``.


.. _ve_render-return: 
Return
******

None

.. _ve_methods-compiler: 

compiler
########

.. _ve_compiler-desc: 

Description
**********

Returns the compiled template. The ``compiler`` method is only used when you 
run the following command: ``mojito compile views``

.. _ve_compiler-sig: 

Signature
*********

``compile(tmpl)``

.. _ve_compiler-params: 

Parameters
**********

- ``tmpl`` (String) - path to the template that is to be rendered


.. _ve_compiler-return: 

Return
******

``String`` - compiled template

.. _ve_engine_view: 

View Engine Addon and Its View
------------------------------

A naming convention associates a view engine and its templates. For example, 
the view engine ``{mojit_dir}/addons/view-engines/big_engine.server.js`` will 
be used to render the template ``{mojit_dir}/views/foo.big_engine.html``. 
Having two templates that only differ by the view engine will cause an error 
because Mojito will not be able to decide which view engine to use 
(which to prioritize above the other) to render the template.

.. _ve_engine_ex: 

Example
-------

.. _ve_engine_ex-ejs: 

Embedded JavaScript (EJS)
#########################

The following example is of the `EJS view engine <http://embeddedjs.com/>`_. 

.. _ve_engine_ex-ejs_engine: 

EJS Rendering Engine
********************

You install ``ejs`` locally with ``npm`` so that the EJS rendering engine is 
installed in the ``node_modules`` directory as seen below:


::

   {app_dir}/node_modules
   └── ejs
       ├── History.md
       ├── Makefile
       ├── Readme.md
       ├── benchmark.js
       ├── ejs.js
       ├── ejs.min.js
       ├── examples
       ├── index.js
       ├── lib
       ├── package.json
       ├── support
       └── test

.. _ve_engine_ex-ejs_addon: 

View Engine Addon
*****************

``{app_dir}/addons/view-engines/ejs.server.js``


.. code-block:: javascript

   YUI.add('addons-viewengine-ejs', function(Y, NAME) {
	
     var ejs = require('ejs'),
     fs = require('fs');
     function EjsAdapter(viewId) {
       this.viewId = viewId;
     }
     EjsAdapter.prototype = {
        
       render: function(data, mojitType, tmpl, adapter, meta, more) {
         var me = this,
         handleRender = function(output) {
		    
           output.addListener('data', function(c) {
	     adapter.flush(c, meta);
           });
	   output.addListener('end', function() {
	     if (!more) {
	       adapter.done('', meta);
	     }
	   });
	 };
	 Y.log('Rendering template "' + tmpl + '"', 'mojito', NAME);
	 var result = ejs.render(this.compiler(tmpl),data);
         adapter.done(result,meta);
       },
       compiler: function(tmpl) {
         return fs.readFileSync(tmpl, 'utf8');
       }
     };
     Y.namespace('mojito.addons.viewEngines').ejs = EjsAdapter;
   }, '0.1.0', {requires: []});    


.. _ve_engine_ex-ejs_template: 

Template
********

``{app_dir}/mojits/{mojit_name}/views/foo.ejs.html``

.. code-block:: html

   <h2> <%= title %></h2>
   <div id=<%= mojit_view_id %>>
     <h3><%= ul.title %></h3>
     <ul>
     <% for(var i=0;i<view_engines.length;i++){ %>
       <li><%= view_engines[i] %></li>
     <% } %>
     </ul>
   </div> 
