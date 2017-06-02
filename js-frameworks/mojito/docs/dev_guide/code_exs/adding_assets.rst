==========
Adding CSS
==========

**Time Estimate:** 10 minutes

**Difficulty:** Beginner

.. _code_exs_css-summary:

Summary
=======

This example shows how to include assets (CSS) in the template of a mojit.

The following topics will be covered:

- configuring an application to have assets
- including assets in the template

.. _code_exs_css-notes:

Implementation Notes
====================

Each application has an ``assets`` directory for placing global CSS files that can be 
accessed by all of your mojits. Each mojit has its own ``assets`` directory for local 
CSS files that are only accessible by the mojit.

The global assets are located in the ``{app_dir}/assets`` directory as shown here:

::

   simple_assets/
   |-- application.json
   |-- assets/
   |-- index.js
   |-- mojits/
   |-- package.json
   |-- routes.json (deprecated)

In the ``simple`` mojit below, you see the local ``assets`` directory for CSS files only 
available to the ``simple`` mojit:

::

   mojits/
   `-- simple/
       |-- assets/
       |-- binders/
       |-- controller.server.js
       |-- defaults.json
       |-- definition.json
       |-- models/
       |-- tests/
       `-- views/

This code example only uses local CSS, so the ``simple.css`` file is placed in the 
``assets`` directory under the ``simple`` mojit.

.. code-block:: css

   .mojit {  
     text-align: center;
   }
   ul.toolbar {  
     display: block;  
     margin: 0 auto;  
     width: 17.0em;
   }
   .toolbar li { display:inline; }

The CSS files in the mojit ``assets`` directory can be accessed in the template using the 
following path syntax:

``/static/{mojit}/assets/{css_file}.css``

This code example uses the ``simple`` mojit and the ``simple.css`` asset. To access 
``simple.css``, you would use the following path:

``/static/simple/assets/simple.css``

The ``index.hb.html`` template below includes ``simple.css`` from the ``assets`` directory 
using the path above.

.. code-block:: html

   <html>
     <head>
       <link rel="stylesheet" type="text/css" href="/static/simple/assets/simple.css"/>
       <script type="text/javascript">
         // Changes background color of the header.
         // Note: JavaScript code should not be hard
         // coded into the template. It's done
         // here to simplify the code example.
         function setColor(id, color) {
           document.getElementById(id).style.backgroundColor = color;
         }
       </script>
     </head>
     <body>
       <div id="{{mojit_view_id}}" class="mojit">
         <h2 id="header">{{title}}</h2>
         <ul class="toolbar">
         {{#colors}}
           <li><a href="#" onClick="setColor('header','{{rgb}}');">{{id}}</a></li>
         {{/colors}}
         </ul>
       </div>
     </body>
   </html>

To access the global assets for the application, you use a similar syntax, replacing the 
mojit name with the application name. Thus, if the application name is ``simple_assets`` 
and ``simple.css`` is in ``simple_assets/assets/``, you would access ``simple.css`` with 
the following path:

``/static/simple_assets/assets/simple.css``

.. note:: For the purpose of simplifying this code example, the ``setColor`` function was 
          hardcoded into the template. In your Mojito applications, you should avoid 
          mixing the business and presentation logic of your application by hardcoding 
          JavaScript into your template.

.. _code_exs_css-setup:

Setting Up this Example
=======================

To create and run ``simple_assets``:

#. Create your application.

   ``$ mojito create app simple_assets``

#. Change to the application directory.

#. Create your mojit.

   ``$ mojito create mojit simple``

#. To configure your application to use the ``simple`` mojit, replace the code in 
   ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "simple" : {
              "type": "simple"
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
      
      app.get('/', libmojito.dispatch('simple.index'));
      
      app.listen(app.get('port'), function () {
          debug('Server listening on port ' + app.get('port') + ' ' +
                     'in ' + app.get('env') + ' mode');
      });
      module.exports = app;

#. Confirm that your ``package.json`` has the correct dependencies as shown below. If not,
   update ``package.json``.

   .. code-block:: javascript

      "dependencies": {
          "debug": "*",
           "mojito": "~0.9.0"
      },
      "devDependencies": {
          "mojito-cli": ">= 0.2.0"
      },
#. From the application directory, install the application dependencies:

   ``$ npm install``
#. Change to the directory of ``simple`` mojit.

   ``$ cd mojits/simple``

#. Modify your controller to pass an array of objects to the template by replacing the 
   code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('simple', function(Y, NAME) {

        Y.namespace('mojito.controllers')[NAME] = {     

          /**
          * Method corresponding to the 'index' action.
          * @param ac {Object} The action context that
          * provides access to the Mojito API.
          */
          index: function(ac) {
            var data = {
              title: "Simple Assets",
              colors: [
                {id: "green", rgb: "#616536"},
                {id: "brown", rgb: "#593E1A"},
                {id: "grey",  rgb: "#777B88"},
                {id: "blue",  rgb: "#3D72A4"},
                {id: "red",  rgb: "#990033"}
              ]
            };
            ac.done(data);
          }
        };
      }, '0.0.1', {requires: []});

#. Include the assets in your template by replacing the code in ``views/index.hb.html`` 
   with the following:

   .. code-block:: html

      <html>
        <head>
          <link rel="stylesheet" type="text/css" href="/static/simple/assets/simple.css"/>
          <script type="text/javascript">
            // Changes background color of the header.
            // Note: JavaScript code should not be hard
            // coded into the template. It's done
            // here to simplify the code example.
            function setColor(id, color) {
              document.getElementById(id).style.backgroundColor = color;
            }
          </script>
        </head>
        <body>
          <div id="{{mojit_view_id}}" class="mojit">
            <h2 id="header">{{title}}</h2>
            <ul class="toolbar">
            {{#colors}}
              <li><a href="#" onClick="setColor('header','{{rgb}}');">{{id}}</a></li>
            {{/colors}}
            </ul>
          </div>
        </body>
      </html>

#. Create the file ``assets/simple.css`` for the CSS of your page with the following:

   .. code-block:: css

      .mojit {  
        text-align: center;
      }
      ul.toolbar {  
        display: block;  
        margin: 0 auto;  
        width: 17.0em;
      }
      .toolbar li { display:inline; }

#. From the application directory, run the server.

   ``$ node app.js``
#. To view your application, go to the URL:

   http://localhost:8666

.. _code_exs_css-src:

Source Code
===========

- `Mojit Assets <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_assets/mojits/simple/assets/>`_
- `Index Template <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_assets/mojits/simple/views/index.hb.html>`_
- `Simple Assets Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_assets/>`_


