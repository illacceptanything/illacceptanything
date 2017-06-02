===================
Configuring Routing
===================

**Time Estimate:** 10 minutes

**Difficulty Level:** Beginning

.. _code_exs_routing-summary:

Summary
=======

This example shows how to configure routing for your Mojito application. 
In Mojito, routing is the mapping of URLs to mojit actions.

.. _code_exs_routing-notes:

Implementation Notes
====================

Before you create routes for your application, you need to specify one or 
more mojit instances that can be mapped to URLs. In the ``application.json`` 
below, the ``mapped_mojit`` instance of the ``Routing`` mojit is created, which 
can then be associated in a route defined in ``app.js``.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "mapped_mojit": {
           "type": "Routing"
         }
       }
     }
   ]

The example ``app.js`` below associates the ``mapped_mojit`` instance 
defined in ``application.json`` with a path and explicitly calls the 
``index`` action. If the controller for the ``Routing`` mojit had the function 
``myFunction``, you would use the following to call it: ``mapped_mojit.myFunction``.  
When an HTTP GET call is made on  the URL ``http:{domain}:8666/custom-route``, the ``index`` 
action is called from the ``custom-route`` instance.

.. code-block:: javascript

   'use strict';

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   // Define the port to listen to or use the value given to the environment
   // variable PORT. 
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.use(libmojito.middleware());

   // Defining route `/custom-route` and executing (dispatching)
   // the action `index` of the mojit instance `mapped_mojit`.
   app.get('/custom-route', libmojito.dispatch('mapped_mojit.index'));

   app.listen(app.get('port'), function () {
      debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
   });
   module.exports = app;

.. note:: Routing for Mojito v0.8 and Earlier
 
          In versions of Mojito v0.8 and earler, routing was configured
          in the file ``routes.json``. If you are converting an older 
          application and want to use the routing configured in 
          ``routes.json``, you can do so by calling 
          ``app.mojito.attachRoutes()``, but be aware that
          the ``routes.json`` file has been deprecated, so it may 
          not be supported in the future.

The name of the mojit instance is arbitrary. For example, the mojit instance 
``mapped_mojit`` above could have just as well been called ``mojit-route``. 
Just remember that the name of the mojit instance in ``app.js`` has to 
be defined and have a mojit type in ``application.json``.

You can also configure multiple routes and use wildcards in ``app.js``. 
The modified ``app.js`` below uses the wildcard to configure a route 
for handling HTTP POST requests and calls the method ``post_params`` from the 
``post-route`` mojit instance.

.. code-block:: javascript

   'use strict';

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   // Define the port to listen to or use the value given to the environment
   // variable PORT.
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.use(libmojito.middleware());
   // Attach any routes in `routes.json`, which is deprecated.
   // app.mojito.attachRoutes();

   // Defining the route `/*` and executing (dispatching)
   // the action `post_params` of the mojit instance `post-route`.
   app.post('/*', libmojito.dispatch('post-route.post_params'));

   app.listen(app.get('port'), function () {
      debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
   });
   module.exports = app; 

The ``app.js`` above configures the routes below. Notice that the wildcard 
used for the path of ``"another-route"`` configures Mojito to execute 
``post_params`` when receiving any HTTP POST requests.

- ``http://localhost:8666/custom-route``
- ``http://localhost:8666/{any_path}``

.. _code_exs_routing-setup:

Setting Up this Example
=======================

To set up and run ``configure_routing``:

#. Create your application.

   ``$ mojito create app configure_routing``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit Routing``
#. To create an instance of the ``Routing`` mojit, replace the code in 
   ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "mapped_mojit": {
              "type": "Routing"
            }
          }
        }
      ]


#. Update your ``app.js`` with the following:

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

          // Defining route `GET /` and executing (dispatching)
          // the action `index` of the mojit instance `mapped_mojit`.
          app.get('/', libmojito.dispatch('mapped_mojit.index'));

          // Defining route `GET /index` and executing (dispatching)
          // the action `index` of the mojit instance `mapped_mojit`.
          app.get('/index', libmojito.dispatch('mapped_mojit.index'));

          // Defining the route `POST /*` and executing (dispatching)
          // the action `post_params` of the mojit instance `post-route`.
          app.post('/show', libmojito.dispatch('mapped_mojit.show'));

          app.get('/status', function (req, res) {
              res.send('200 OK');
          });

          app.listen(app.get('port'), function () {
              debug('Server listening on port ' + app.get('port') + ' ' +
              'in ' + app.get('env') + ' mode');
          });
          module.exports = app;

   The ``mapped_mojit`` instance created in ``application.json`` is 
   configured here to be used when HTTP GET calls are made on the paths 
   ``/index`` or ``/show`` and HTTP POST calls made on the path ``/show``.

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

#. Change to ``mojits/Routing``.
#. Modify your controller to contain the ``index`` and ``show`` actions by 
   replacing the code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('routing', function(Y, NAME) {
        // Builds object containing route information
        function route_info(ac) {
          var methods = "",
              name = "",
              action = ac.action,
              path = ac.http.getRequest().url,
              routes = ac.config.getRoutes();
          if (action === "index" && path === "/") {
            name = "root_route";
            method = 'GET';
          } else if (action==="index") {
            name = "index_route";
          } else {
            name = "show_route";
            methods = 'POST'; 
          }
          methods = methods.toUpperCase();
          return {
            "path": path,
            "name": name,
            "methods": methods.replace(/, $/, "")
          };
        }
        Y.namespace('mojito.controllers')[NAME] = {
          index: function (ac) {
            ac.done(route_info(ac));
          },
          show: function (ac) {
            ac.done(route_info(ac));
          }
        };
      }, '0.0.1', {requires: ['mojito-config-addon', 'mojito-http-addon']});

#. To display your route information in your ``index`` template, replace the content of 
   ``index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}">
        <b>Route Path:</b> {{path}}<br/>
        <b>HTTP Methods:</b> {{methods}}<br/>
        <b>Route Name:</b> {{name}}
      </div>

#. To display your route information in your ``show`` template, create the file 
   ``show.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}">
        <b>Route Path:</b> {{path}}<br/>
        <b>HTTP Methods:</b> {{methods}}<br/>
        <b>Route Name:</b> {{name}}
      </div>

#. Run the server and open the following URL in a browser to see the ``index`` 
   route: http://localhost:8666/index
#. To see the ``show`` route, open the following URL in a browser:

   http://localhost:8666/show

.. _code_exs_routing-src:

Source Code
===========

- `Application Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/configure_routing/application.json>`_
- `Route Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/configure_routing/app.js>`_
- `Configure Routing Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/configure_routing/>`_

