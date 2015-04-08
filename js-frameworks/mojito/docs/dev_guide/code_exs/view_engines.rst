======================================
Creating and Using a View Engine Addon 
======================================

**Time Estimate:** 15 minutes

**Difficulty Level:** Intermediate

.. _code_exs_view_engine_addon-summary:

Summary
=======

This example shows how to install a third-party rendering engine 
(Embedded Javascript), create a view engine addon that uses the 
installed rendering engine, and create a template for the view engine. 
Mojito uses the `Handlebars <https://github.com/wycats/handlebars.js/>`_ 
rendering engine by default.

The following topics will be covered:

- using ``npm`` to install the rendering engine
- creating a view engine addon
- using Embedded JavaScript (EJS) in the template

.. _code_exs_view_engine_addon-notes:

Implementation Notes
====================

Before you create your application, you should take a look at the following 
sections to better understand how the application works. The focus here is 
to give you a practical example that you can use to add your own view engines 
and also to show some of important points of using view engines in Mojito 
applications. For more comprehensive but less hands-on documentation, see 
`Developer Topics: View Engines <../topics/mojito_extensions.html#view-engines>`_.

.. _ve_addon_notes-what:

What Is a View Engine?
----------------------

A view engine is code that applies data returned by the controller to a view. 
This is most often done by interpreting the view as a template. View engines 
in Mojito can function at either the application or mojit level. This example 
uses an application-level view engine addon, allowing multiple mojits to use it 
although the example only uses one mojit.

.. _ve_addon_notes-install:

Installing a Rendering Engine
-----------------------------

You could write your own rendering engine or copy code into your Mojito 
application, but this example follows the most common use case of installing 
a rendering engine with ``npm``. We will be installing the rendering engine
`EJS <http://embeddedjs.com/>`_ with ``npm``.

Because your Mojito application is simply a ``npm`` module, you can have a 
``node_modules`` directory for locally installing other modules. Thus, from 
your application directory, you would use the following ``npm`` command to 
install ``ejs``:

``$ npm install ejs``

After you have installed ``ejs``, a ``node_modules`` directory will be created 
with the contents similar to the following:

::

   node_modules
   ├── ejs/
   │   ├── History.md
   │   ├── Makefile
   │   ├── Readme.md
   │   ├── benchmark.js
   │   ├── ejs.js
   │   ├── ejs.min.js
   │   ├── examples/
   │   │   ├── client.html
   │   │   ├── list.ejs
   │   │   └── list.js
   │   ├── index.js
   │   ├── lib/
   │   │   ├── ejs.js
   │   │   ├── filters.js
   │   │   └── utils.js
   │   ├── package.json
   │   ├── support/
   │   │   └── compile.js
   │   └── test/
   │       ├── ejs.test.js
   │       └── fixtures/
   ...
       
.. _ve_addon_notes-create:
       
Creating the View Engine Addon
------------------------------

The view engine addon like other addons is simply a YUI module that lives in the 
``addons/view-engines`` directory. For the application-level view engine addons 
that this example is using, the view engine addon will be in 
``{app_dir}/addons/view-engines``.

.. _ve_addon_create-req:

Requirements
############

The view engine addon must have the following:

- a ``YUI.add`` statement to register the addon. For example, we register the 
  view engine addon with the name ``addons-viewengine-ejs`` in our code example 
  as seen below.

  .. code-block:: javascript

     YUI.add('addons-viewengine-ejs', function(Y, NAME) {
    
       // The addon name 'addons-viewengine-hb' is registered by YUI.add
    
     }, '0.1.0', {requires: []});
      
- a prototype of the object has the following two methods ``render`` and 
  ``compiler`` as shown below. We will look at the ``render`` and ``compile`` 
  methods more closely in the next section.

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
        
- an object that is assigned to ``Y.mojito.addons.viewEngines.{view_engine_name}``. 
  In our example, the constructor ``EjsAdapter`` is assigned to the namespace 
  ``Y.namespace('mojito.addons.viewEngines').ejs`` or ``Y.mojito.addons.viewEngines.ejs``.
   
  .. code-block:: javascript
      
     ...
        
       function EjsAdapter(viewId) {
         this.viewId = viewId;
       }
       ...
       Y.namespace('mojito.addons.viewEngines').ejs = EjsAdapter;

.. _ve_addon_create-render_compile:      

render and compile
##################

The ``render`` method renders the template and sends the output to the 
methods ``adapter.flush`` or ``adapter.done`` that execute and return the 
page to the client.

The implementation of how the ``render`` method is up to the developer. 
You could write code or use a library to render the template, but in this 
example we use the instance ``ejs`` to compile the view.

.. code-block:: javascript

     ...
     
     /**
     * Renders the EJS template using the data provided.
     * @method render
     * @param {object} data The data to render.
     * @param {string} mojitType The name of the mojit type.
     * @param {string} tmpl The name of the template to render.
     * @param {object} adapter The output adapter to use.
     * @param {object} meta Optional metadata.
     * @param {boolean} more Whether there will be more content later.
     */
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
       var result = ejs.render(this.compile(tmpl),data);
       console.log(result);
       adapter.done(result,meta);
 
     },
     ...
        
The ``compile`` method is required to run the command ``mojito compile views``. 
In our example, the ``compile`` method also reads the template file and returns 
a string to ``render`` so that it can be rendered by ``ejs``. 

.. code-block:: javascript

   ...
   
   compiler: function(tmpl) {
     return fs.readFileSync(tmpl, 'utf8');
   }


In the above code snippet, the ``compile`` method simply returns the template 
file to the ``render`` method, where the instance of the EJS rendering engine 
calls ``render`` to render the template file into a string. The implementation 
of the ``compile`` method in the addon could have been written to call
``ejs.render``.

.. _ve_addon_notes-ejs_templates:

EJS Templates
-------------

EJS is similar to ``ERB`` that is used by 
`Ruby on Rails <http://rubyonrails.org/>`_. The embedded JavaScript is 
wrapped in ``<%`` and ``%>``. If you want to evaluate code so that
the returned value is inserted into the HTML string, you use ``<%=`` as 
seen below, where the variable ``title`` is substituted with a value.

.. code-block:: html

   <h2> <%= title %></h2>

You can do most of the same things with EJS as you can with JavaScript. 
For example, you can iterate through an array in the same way as shown here:

.. code-block:: html

   <ul>
     <% for(var i=0;i<view_engines.length;i++){ %>
     <li><%= view_engines[i] %></li>
     <% } %>
   </ul>

EJS also has view helpers for creating links and forms, much like ``ERB``. 
See `Getting Started with EJS <http://embeddedjs.com/getting_started.html>`_ 
for more information.

.. _code_exs_view_engine_addon-setup:

Setting Up this Example
=======================

To set up and run ``adding_view_engines``:

#. Create your application.

   ``$ mojito create app adding_view_engines``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit myMojit``

#. To specify that your application use ``myMojit``, replace the code in 
   ``application.json`` with  the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "myMojit": {
              "type": "myMojit"
            }
          }
        }
      ]


#. Update your ``app.js`` with the following to use Mojito's middleware, configure routing and the port, and 
   have your application listen for requests:

   .. code-block:: javascript

      'use strict';

      var debug = require('debug')('app'),
          express = require('express'),
          libmojito = require('mojito'),
          app;

          app = express();
          app.set('port', process.env.PORT || 8666);
          libmojito.extend(app);

          app.use(libmojito.middleware());

          app.get('/status', function (req, res) {
              res.send('200 OK');
          });
          app.get('/', libmojito.dispatch('myMojit.default_ve'));
          app.get('/ejs', libmojito.dispatch('myMojit.added_ve'));

          app.listen(app.get('port'), function () {
              debug('Server listening on port ' + app.get('port') + ' ' +
              'in ' + app.get('env') + ' mode');
          });
          module.exports = app;

#. Confirm that your ``package.json`` has the correct dependencies as show below. If not,
   update ``package.json``. Notice that we're adding the ``ejs`` module for rendering
   embedded JavaScript (EJS) templates.

   .. code-block:: javascript

      "dependencies": {
          "debug": "*",
           "mojito": "~0.9.0"
      },
      "devDependencies": {
          "ejs": "*",
          "mojito-cli": ">= 0.2.0"
      },

#. From the application directory, install the application dependencies:

   ``$ npm install``

#. Create the addons directory for your view engine addon.

   ``$ mkdir -p addons/view-engines``
#. Change to the ``addons/view-engines`` directory that you created.
#. Create the view engine addon file ``ejs.server.js`` with the following code:

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
		    console.log(result);
		    adapter.done(result,meta);
		  },
		  compiler: function(tmpl) {
		    return fs.readFileSync(tmpl, 'utf8');
		  }
		};
		Y.namespace('mojito.addons.viewEngines').ejs = EjsAdapter;
      }, '0.1.0', {requires: []});

#. Change to the ``adding_view_engines/mojits/myMojit`` directory.
#. Replace the code in ``controller.server.js`` with the following:

   .. code-block:: javascript
   
      YUI.add('mymojit', function(Y, NAME) {

        Y.namespace('mojito.controllers')[NAME] = {   
  
          default_ve: function(ac) {
            ac.done({
              "title": "Handlebars at work!",
              "view_engines": [ 
                {"name": "EJS"},
                {"name": "Jade"}, 
                {"name": "dust"},
                {"name": "underscore" }
              ],
              "ul": { "title": 'Here are some of the other available rendering engines:' },
            });
          },
          added_ve: function(ac) {
            ac.done({
              "title": "EJS at work!",
              "view_engines": [ "Jade", "Dust","underscore" ],
              "ul": { "title": 'In addition to Handlebars and EJS, you can also use these rendering engines:' }
            });  
          }
        };
      }, '0.0.1', {requires: ['mojito', 'mymojit-model-foo']});
 
#. Create the template ``views/default_ve.hb.html`` that uses Handlebar 
   expressions with the following:

   .. code-block:: html
   
      <h2>{{title}}</h2>
      <div id="{{mojit_view_id}}">
        <h3>
        {{#ul}}
          {{title}} 
        {{/ul}}
        {{^ul}}
          Besides Handlebars, here are some other rendering engines:
        {{/ul}}  
        </h3>
        <ul>
        {{#view_engines}}
          <li>{{name}}</li>
        {{/view_engines}} 
        </ul>
      </div>

#. Create the template ``views/added_ve.ejs.html`` that uses EJS with the 
   following:

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

#. From your application directory, start Mojito.

   ``$ node app.js``
#. Open the following URL in your browser to see the template rendered by the 
   Handlebars rendering engine.   

   `http://localhost:8666/ <http://localhost:8666/>`_
#. Now see the template rendered by the EJS rendering engine at the following 
   URL:

   `http://localhost:8666/ejs <http://localhost:8666/ejs>`_   
#. Great, your application is using two different rendering engines. You should
   now be ready to add your own view engine that uses a rendering engine such as 
   Jade.   

.. _code_exs_view_engine_addon-src:

Source Code
===========

- `View Engines <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/adding_view_engines/>`_
- `View Engine Addon <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/adding_view_engines/addons/view-engines/ejs.server.js>`_
- `Templates <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/adding_view_engines/mojits/myMojit/views/>`_
