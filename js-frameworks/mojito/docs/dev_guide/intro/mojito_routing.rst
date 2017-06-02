=======
Routing
=======

In Mojito, routing is the mapping of URLs to specific mojoit actions. 
From version v0.9 and later, you define the routing paths in ``app.js``, specifying the 
paths and explicitly executing (dispatching) an action of a mojit instance. 

In the following sections, we'll show how you how to use the routes defined 
in ``routes.json``, which has been deprecated, and then show you how to define
different types of routing paths.

See `Code Examples: Configuring Routing <../code_exs/route_config.html>`_ for
examples of configuring routing in a Mojito application.

.. _routing-routesjson:

Using Routing Defined in routes.json
====================================

Although ``routes.json`` has been **deprecated** in Mojito v0.9, you can still 
attach (use) the configured routes by calling the method ``attachRoutes``
from your Mojito instance as shown below:

your ``app.js``:

.. code-block:: javascript
   :emphasize-lines: 10-17

   'use strict';
   
   // Create instances for Express and Mojito as well
   // as allowing for debugging.
   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;
   
   // Create an Express application. 
   app = express();

   //  Mojito extends the functionality of the Express app, so that it Mojito can dispatch mojits, etc.
   libmojito.extend(app);

   // Use the routes defined in `routes.json`.
   app.libmojito.attachRoutes();


.. _routing-single:

Single Route
============

To create a route, you need to create a mojit instance that can be mapped to a 
path. In the ``application.json`` below, the ``hello`` instance of type 
``Hello`` is defined.

**application.json**

.. code-block:: javascript
   :emphasize-lines: 5

   [
     {
       "settings": [ "master" ],
       "specs": {
         "hello": {
           "type": "Hello"
         }
       }
     }
   ]

The ``hello`` instance and a function in the ``Hello`` controller can now 
be mapped to a route path in ``app.js`` file. In the ``app.js`` below, 
the ``index`` function is called when an HTTP GET call is made on the root path.

**app.js**

.. code-block:: javascript
   :emphasize-lines: 8,9

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

   // map "/" to "hello.index"
   app.get('/', libmojito.dispatch('hello.index'));

.. _appjs-routing-anonymous:

Routing for Anonymous Mojits
============================

Instead of using a mojit instance defined in the ``application.json``, 
you can create an anonymous instance by prepending "@" to the mojit name.
You can then use parametrized paths in ``app.js`` to execute an action of the
anonymous mojit instacne with the following:

.. code-block:: javascript
   :emphasize-lines: 7,8

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();

   // Allow anonymyous mojit instances w/ actions to be dispatched
   app.get('/:mojit/:action', libmojito.dispatch("{mojit}.{action}"));

.. _appjs-routing-multiple:

Multiple Routes
===============

To specify multiple routes, you simply specify different paths with 
``app.[verb]``, where ``verb`` is any HTTP method. 

In the example ``app.js`` below, routing paths for ``/foo``, ``/bar``, and
``/*`` are defined: 

.. code-block:: javascript
   :emphasize-lines: 8-11

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

   // Remember that libmojito.dispatch() returns a middleware fn.
   app.get('/bar', libmojito.dispatch('bar-1.index', {page: 1, log_request: true}));
   app.get('/foo', libmojito.dispatch('foo-1.index'));
   app.get('/*', libmojito.dispatch('foo-1.index'));



.. _appjs-routing-params:

Adding Routing Parameters
=========================

You can configure a routing path to have routing parameters by passing an object
with key-value pairs to the ``dispatch`` method. Your mojits can then access the
routing parameters from the ``ActionContext`` object 
using the `Params addon <../../api/classes/Params.common.html>`_.

In the example ``app.js`` below, the routing parameters ``page`` and
``log_request`` are passed to ``dispatch`, allowing a mojit controller
To get the value for ``page`` using ``ac.params.getFromRoute("page")``. 

.. code-block:: javascript
   :emphasize-lines: 8-15

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

   libmojito.dispatch('foo-1.index', { page: 1, log_request: true}));
   /* OR the verbose way
       app.get('/*', function (req, res, next) {
           req.params.page = 1;
           req.params.log_request = true;
           next();
       }, libmojito.dispatch('foo-1.index'));
   */


.. _appjs-routing-parameterized:

Using Parameterized Paths to Call a Mojit Action
================================================

Your routing configuration can also use parameterized paths to call mojit 
actions. We looked at this feature in :ref:`Routing for Anonymous Mojits <appjs-routing-anonymous>`.

In the ``app.js`` below, the ``:mojit_action`` property uses parameters 
to capture a part of the matched URL and then uses that captured part to 
replace ``{{mojit-action}}`` with the captured value. Any 
value can be used for the parameter as long as it is prepended with a 
colon (e.g., ``:foo``). After the parameter has been replaced by a value 
given in the path, the call to the action should have the following syntax: 
``{mojit_instance}.(action}`` 

.. code-block:: javascript
   :emphasize-lines: 10-13

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

   var methods = ['get', 'post', 'put'];

   methods.forEach(function (verb) {
       app.[verb]('/foo/:mojit_action', libmojito.dispatch('@foo-1.{mojit_action}'));
       app.[verb]('/bar/:mojit_action', libmojito.dispatch('@bar-1.{mojit_action}'));
   });


.. _appjs-routing-regex:

Using Regular Expressions to Match Routing Paths
================================================

You can also use regular expressions to match a routing path.

For example, in the ``app.js`` below, if the path of the request 
matches the regular expression ``\\d{1,2}_[Mm]ojitos?``, the ``index``
action of the mojit instance ``myMojit`` is called. 


.. code-block:: javascript
   :emphasize-lines: 8-13

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   libmojito.extend(app);

    // Specify a regular expression to match routing paths.
   app.get(/\d{1,2}_[Mm]ojitos?/, libmojito.dispatch('myMojit.index'));


Based on the above regular expression, the following URLs would be matched: 

- http://localhost:8666/1_mojito
- http://localhost:8666/99_Mojitos


.. _generate_urls:

Generate URLs from the Controller
=================================

The Mojito API includes the `Url addon <../../api/classes/Url.common.html>`_ 
that allows you to create a URL with the mojit instance, the action, and parameters from 
the controller. 

To create a URL based on routing paths, you have to register a routing path in ``app.js``
with the ``app.map`` method. For example, in the ``app.js`` below, the routing path ``/``
is defined with ``app.get``, but the ``Url`` addon cannot create a URL based on that 
routing definition until ``app.map`` registers the route.

.. code-block:: javascript

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('../../../'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.get('/', libmojito.dispatch('hello.index'));
   app.map('/', 'hello_index');


In this code snippet from ``controller.js``, the `Url addon <../../api/classes/Url.common.html>`_ 
with the ``make`` method uses the routing path ``hello_index`` registered in our
example ``app.js`` above to to create the URL ``/`` with the query string parameters 
``?foo=bar``.

.. code-block:: javascript

   ...
     index: function(ac) {
       ac.url.make('hello_index', 'index', null, 'GET', {'foo': 'bar'});
     }
   ...

The ``index`` function above returns the following URL: ``http://localhost:8666/?foo=bar``
