===================
Using Global Assets
===================

**Time Estimate:** 15 minutes

**Difficulty:** Intermediate

.. _code_exs_assets-summary:

Summary
=======

This example shows how to configure and use global assets in Mojito 
applications. In this example, the ``HTMLFrameMojit`` inserts global assets 
into the rendered view. See `Adding CSS <./adding_assets.html>`_ for an example 
that uses mojit-level assets.

The following topics will be covered:

- configuring your application to insert global assets into the view
- creating and storing your global assets
- static URLs to global assets

.. _code_exs_assets-notes:

Implementation Notes
====================

.. _assets-notes-what:

What Are Assets?
----------------

Assets are resources that are required on the clients. These resources 
are primarily CSS but can also be JavaScript. Your assets should not be 
the core components of your application.

.. _assets-notes-loc:

Location of Assets
------------------

Mojito applications can have both global and local assets. Global assets are 
placed in the ``assets`` directory under the application directory. Assets at 
the mojit level are placed in the ``assets`` directory under the mojit directory.

The directory structure of this example below shows the location of the global 
``assets`` directory with the asset files.

::

   global_assets/
   ├── application.json
   ├── assets
   │   ├── favicon.ico
   │   ├── ohhai.css
   │   └── sadwalrus.jpeg
   ├── mojits
   │   └── OhHai
   │       ├── binders
   │       │   └── index.js
   │       ├── controller.server.js
   │       ├── models
   │       │   └── foo.server.js
   │       └── views
   │           └── index.hb.html
   ├── package.json
   ├── routes.json (deprecated)

.. _assets-notes-static_url:

Static URLs to Assets
---------------------

Mojito provides static URLs to application-level and mojit-level assets. You 
can refer to these assets in your templates, or if you are using the 
``HTMLFrameMojit``, you configure your application to automatically insert 
the assets into the rendered view.

.. _static_url-syntax:

Syntax
######

For application-level assets, the static URL has the following syntax:

``/static/{application_name}/assets/{asset_file}``

.. _static_url-ex:

Examples
########

The path to the application-level asset ``sadwalrus.jpeg`` of the ``global_assets`` 
application would be the following:

``/static/global_assets/assets/sadwalrus.jpeg``

In the template, the application-level assets above can be referred to using the 
static URLs as seen here.

.. code-block:: html

   <div id="{{mojit_view_id}}" class="lolrus">
     <img src="/static/global_assets/assets/sadwalrus.jpeg" alt="walrus smile fail" height="497" width="486">
   </div>

.. _assets-notes-htmlframemojit:

Configuring HTMLFrameMojit to Include Assets
--------------------------------------------

When using the ``HTMLFrameMojit``,  assets are listed in the ``assets`` object in 
``application.json.`` The ``assets`` object can contain a ``top`` object and/or a 
``bottom`` object. The assets listed in ``top`` will be inserted into the ``head`` 
element of the HTML page. The assets listed in ``bottom`` are inserted at the 
bottom of the ``body`` element.

In the example ``application.json`` below, which is taken from this code example, 
the global asset ``ohhai.css`` is inserted into the ``head`` element of the rendered 
view.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "ohhai": {
           "type": "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "title": "global assets",
             "child": {
               "type": "OhHai"
             },
             "assets": {
               "top": {
                 "css": [
                   "/static/global_assets/assets/ohhai.css"
                 ]
               }
             }
           }
         }
       },
       "staticHandling": {
         "appName": "global_assets"
       }
     }
   ]

.. _code_exs_assets-setup:

Setting Up this Example
=======================

To set up and run ``global_assets``:

#. Create your application.

   ``$ mojito create app global_assets``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit OhHai``
#. To specify that your application use ``HTMLFrameMojit`` with a child mojit, replace the 
   code in ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "ohhai": {
              "type": "HTMLFrameMojit",
              "config": {
                "deploy": true,
                "title": "global assets",
                "child": {
                  "type": "OhHai"
                },
                "assets": {
                  "top": {
                    "css": [
                      "/static/global_assets/assets/ohhai.css"
                    ]
                  }
                }
              }
            }
          },
          "staticHandling": {
            "appName": "global_assets"
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

          app.get('/', libmojito.dispatch('ohhai.index'));
          app.get('/status', function (req, res) {
              res.send('200 OK');
          });

          app.listen(app.get('port'), function () {
              debug('Server listening on port ' + app.get('port') + ' ' +
              'in ' + app.get('env') + ' mode');
          });
          module.exports = app;

#. Confirm that your ``package.json`` has the correct dependencies as show below. If not,
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

#. Create the file ``assets/ohhai.css`` using the following:

   .. code-block:: css

      div.lolrus img {
        margin: 5em;
      }

#. Add the following image files to the ``assets`` directory.

   - ``$ wget -O assets/sadwalrus.jpeg http://fypa.net/wp-content/uploads/2011/08/10-sad-walrus.jpeg``
   - ``$ wget -O assets/favicon.ico http://static.treehugger.com/images/favicon.ico``

#. Change to ``mojits/OhHai``.
#. Modify the mojit controller to get data from the model by replacing the code in 
   ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('ohhai', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   
          index: function(ac) {
            ac.done();
          }
        };
      }, '0.0.1', {requires: ['mojito']});

#. Modify your ``index`` template to explicitly include the global asset ``sadwalrus.jpeg`` 
   by replacing the code in ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="lolrus">
        <img src="/static/global_assets/assets/sadwalrus.jpeg" alt="walrus smile fail" height="497" width="486">
      </div>

#. From the application directory, run the server.

   ``$ node app.js``
#. To view your application with the sad walrus image, go to the URL:

   http://localhost:8666
#. View the source code to see that the global asset ``ohhai.css`` was inserted into the 
   ``head`` element.

.. _code_exs_assets-src:

Source Code
===========

- `Application Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/global_assets/application.json>`_
- `Assets <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/global_assets/assets/>`_
- `Global Assets Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/global_assets/>`_

