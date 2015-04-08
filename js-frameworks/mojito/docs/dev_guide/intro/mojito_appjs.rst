================================================================
Creating and Using the Mojito Application Bootstrap File: app.js
================================================================


.. _appjs-overview:

Overview
========

As of Mojito 0.9, applications are started with the file ``app.js`` that must be directly under the application
directory. In this chapter, we'll cover how to start your applications, and use your ``app.js`` to
define routing paths, create middleware, set the port and base contexts, and offer a simple set of instructions
to convert older applications so that they are compatible with Mojito v0.9.

.. _appjs-template:

Basic Template
==============

For most applications, your ``app.js`` will look similar and perform the same functions. In summary, these
functions are creating an instance of Mojito, Express, attching the routing paths defined in ``routes.json``,
use application configurations and Mojito's built-in middleware. 

Below is what can be considered a general use template that should be sufficient for most applications.
The inline comments offer a short explanation of the significant lines of the code.

.. _appjs_basic-template:

.. code-block:: javascript


   'use strict';
   
   // Create instances for Express and Mojito as well
   // as allowing for debugging.
   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;
   
   // Create an Express application. 
   app = express();

   // Set the port with a default or use the value of the environment variable `PORT`.
   app.set('port', process.env.PORT || 8666);

   //  Mojito extends the functionality of the Express app, so that it Mojito can dispatch mojits, etc.
   libmojito.extend(app);
   
   // Include Mojito's middleware.
   app.use(libmojito.middleware());

   // Use the routes defined in `routes.json`.
   app.libmojito.attachRoutes();
   
   // Define a simple routing path with Express-like syntax for doing a sanity check.
   app.get('/status', function (req, res) {
       res.send('200 OK');
   });
   
   // Listen for incoming HTTP requests on the port set earlier.
   app.listen(app.get('port'), function () {
       debug('Server listening on port ' + app.get('port') + ' ' +
                  'in ' + app.get('env') + ' mode');
   });
   // Export the app for use by other libraries/hosting environments, etc.
   module.exports = app;

.. _appjs-start_apps:

Starting Applications
=====================

To start applications, install your application dependencies and use ``node`` directly with ``app.js``:

#. ``$ npm install``
#. ``$ node app.js``

.. _appjs-route_config:

Configuring Routing
===================

You configure routing in ``app.js``, much like you would do for Express applications, but
in Mojito applications, you are explicitly executing actions of mojit instances when
requests are received for a given path. See `Routing <mojito_routing.html>`_
to learn how to define routes and explicitly dispatch mojit instances in ``app.js``.



.. _appjs-middleware:

Using Middleware
================

In Mojito v0.9 and later, you have to explicitly include Mojito middleware in ``app.js`` with the
following code:

.. code-block:: javascript

   // Create instances for Express and Mojito as well
   // as allowing for debugging.
   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;      // Include Mojito's middleware.
   
   // Include the Mojito middleware. 
   app.use(libmojito.middleware());


You can also select which Mojito middleware to include and write custom middleware. See 
`Middleware <../topics/mojito_extensions.html#middleware>`_  for details.


.. _appjs-base_contexts:

Setting Base Contexts
=====================

The base context was set with the Mojito CLI command ``mojito start --context {base_context}`` in Mojito 
versions 0.8.x and earlier. With the remove of the ``start`` command, you now set the base context in
``app.js`` by passing a ``context`` object to ``libmojito.extend``, where ``libmojito`` is an instance of
Mojito. In the ``context`` object, you can then specify the environment, device, runtime, language, etc.

For example, to set the base context as the ``development`` environment, you could use the following:

.. code-block:: javascript

   var express = require('express'),
       libmojito = require('mojito'),
       app = express();
   libmojito.extend(app, {
       context: {
           runtime: 'server',
           environment: 'development'
       }
   });

The request context is set by incoming HTTP request, so nothing changes in Mojito v0.9.


.. _appjs-converting:

Converting Mojito v0.8 and Earlier Applications
===============================================


#. Delete the file `server.js`.
#. Create a basic `app.js` with the :ref:`Basic Template <appjs_basic-template>`.
#. Review the following sections to see if you need to make further changes:

   - `Configuring Routing in app.js <mojito_configuring.html#appjs-routing>`_ 
   - `Middleware <../topics/mojito_extensions.html#middleware>`_
   - `Base Contexts <mojito_using_contexts.html#base-context>`_
#. See examples of app.js in the `code examples on GitHub <https://github.com/yahoo/mojito/tree/develop/examples/developer-guide>`_.

