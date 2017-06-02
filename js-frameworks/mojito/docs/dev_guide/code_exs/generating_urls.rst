===============
Generating URLs
===============

**Time Estimate:** 15 minutes

**Difficulty Level:** Intermediate

.. _code_exs_gen_urls-summary:

Summary
=======

This example shows you a way to generate URLs to a particular view independent of the 
controller or action of the mojit.

The following topics will be covered:

- configuring routing paths to call actions from mojit instances
- creating a URL in the mojit controller with the `Url addon <../../api/classes/Url.common.html>`_

.. _code_exs_gen_urls-notes:

Implementation Notes
====================

The route paths for this code example are defined in the routing configuration file 
``app.js``. You can define any path and then associate that path with a mojit 
instance and an action. When the client makes an HTTP request on that path, the associated 
action on the mojit instance defined in ``application.json`` will be executed. Before 
creating the routes for the application, you first need to create the mojit instance.

In the ``app.js`` below, you configure the application to use an instance of the 
mojit ``GenerateURL``. The instance in this example is ``mymojit``, but the instance name 
can be any string as defined by `RFC 4627 <http://www.ietf.org/rfc/rfc4627.txt>`_.

.. code-block:: javascript
   
   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);
   app.get('/', libmojito.dispatch("mymojit.index"));

In the ``app.json``, you not only can define route paths, but you can also configure 
Mojito to respond to specific HTTP methods called on those paths. The ``app.js`` below 
defines two route paths that only respond to HTTP GET calls. When HTTP GET calls are made 
on these two paths, Mojito executes different methods from the ``mymojit`` instance. The 
``index`` method is executed when the root path is called, and the ``contactus`` method 
is executed when the ``/some-really-long-url-contactus`` path is called.  
The ``app.map`` method is used to register routes so that the ``Url`` addon can use these
routes later to create a URL.

.. code-block:: javascript

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('../../../'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);
   
   app.use(libmojito.middleware());
   //app.mojito.attachRoutes();
   
   app.get('/status', function (req, res) {
       res.send('200 OK');
   });
   
   app.get('/', libmojito.dispatch("mymojit.index"));
   app.get('/some-really-long-url-that-we-dont-need-to-remember-contactus', libmojito.dispatch("mymojit.contactus"));
   // The `map` method registers the route so that it can be referenced later, much like 
   // the routes were configured in `routes.json` in Mojito v0.8.x and earlier.
   app.map('/some-really-long-url-that-we-dont-need-to-remember-contactus', 'get#mymojit.contactus');

The mojit controller, however, can use the ``Url`` addon to access routes that 
were registered with ``app.map``. For example, in the ``controller.server.js`` below, the 
route path that calls the ``contactus`` action is formed with ``url.make`` in the ``index`` 
function. You just pass the instance and action to ``url.make`` to create the URL based on 
the path defined in ``app.js``.

.. code-block:: javascript

   YUI.add('generateurl', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {   

       index: function(actionContext) {
         var url = actionContext.url.make('mymojit', 'contactus', '');
         actionContext.done({contactus_url: url});
       },
       contactus: function(actionContext) {
         var currentTime = actionContext.intl.formatDate(new Date());
         actionContext.done({currentTime: currentTime});
       }
     };
   }, '0.0.1', {requires: ['mojito-intl-addon', 'mojito-url-addon']});

.. _code_exs_gen_urls-setup:

Setting Up this Example
=======================

To set up and run ``generating_urls``:

#. Create your application.

   ``$ mojito create app generating_urls``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit GenerateURL``
#. To configure your application to use ``GenerateURL``, replace the code in 
   ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "mymojit": {
              "type": "GenerateURL"
            }
          }
        }
      ]

#. Update your ``app.js`` with the code below to use Mojito's middleware, configure routing and the port, and
   have your application listen for requests. Notice that we're using ``app.map`` to register a routing path
   for later use in the controller.

   .. code-block:: javascript

      'use strict';

      var debug = require('debug')('app'),
          express = require('express'),
          libmojito = require('../../../'),
          app;

      app = express();
      app.set('port', process.env.PORT || 8666);
      libmojito.extend(app);

      app.use(libmojito.middleware());

      app.get('/', libmojito.dispatch("mymojit.index"));
      app.get('/some-really-long-url-that-we-dont-need-to-remember-contactus', libmojito.dispatch("mymojit.contactus"));
      app.map('/some-really-long-url-that-we-dont-need-to-remember-contactus', 'get#mymojit.contactus');

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

#. Change to ``mojits/GenerateURL``.
#. Enable the controller to create a URL using the route path that we registered in ``app.js`` 
   by replacing the code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('generateurl', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   

          index: function(actionContext) {
            var url = actionContext.url.make('mymojit', 'contactus', '');
            actionContext.done({contactus_url: url});
          },
          contactus: function(actionContext) {
            var currentTime = actionContext.intl.formatDate(new Date());
            actionContext.done({currentTime: currentTime});
          }
        };
      }, '0.0.1', {requires: ['mojito-intl-addon', 'mojito-url-addon']});

#. To display the rendered ``index`` template when HTTP GET is called on the root path, 
   replace the code in ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <div>
          <p>This is the default page that is visible on the root path.</p>
          <p>The purpose of this demo is to show that as a developer, you don't have to remember any 
          custom routing path you specify in app.js configuration file.</p>
          <p>All you need is the mojit identifier (e.g. mymojit), and the action that you are calling 
            on the mojit (e.g. contactus). See the mojits/GenURLMojit/controller.server.js for more details.
          </p>
        </div>
        <div style="text-align: center; background-color: #0776A0">
          <p>Click <a href="{{contactus_url}}">here</a> on how to Contact Us.</p>
        </div>
      </div>

#. To display the rendered ``contactus`` template when the ``contactus`` action is executed,  
   replace the code in ``views/contactus.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <div>
          <p>This is the contact page last viewed on: <strong>{{currentTime}}</strong>
          </p>
        </div>
        <div>
          <p>Yahoo Inc, 701 First Avenue, Sunnyvale CA 94089</p>
        </div>
      </div>

#. Run the server and open the following URL in a browser: http://localhost:8666/
#. From your application, click on the 
   `here <http://localhost:8666/some-really-long-url-that-we-dont-need-to-remember-contactus>`_ 
   link to see the URL with the long path.

.. _code_exs_gen_urls-src:

Source Code
===========

- `Routing Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/generating_urls/app.js>`_
- `Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/generating_urls/mojits/GenURLMojit/controller.server.js>`_
- `Generating URLs Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/generating_urls/>`_


