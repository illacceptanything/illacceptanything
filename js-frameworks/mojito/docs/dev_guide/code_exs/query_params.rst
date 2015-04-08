======================
Using Query Parameters
======================

**Time Estimate:** 15 minutes

**Difficulty Level:** Intermediate

.. _code_exs_qp-summary:

Summary
=======

This example shows how to access query parameters from the URL, the POST body,
and the routing configuration of your Mojito application.

The following topics will be covered:

- using the `Params addon <../../api/classes/Params.common.html>`_ to access
  parameters
- setting and getting parameters from your route configuration

.. _code_exs_qp-notes:

Implementation Notes
====================

The mojit controller of this code example has four functions, each using
methods from the ``Params`` addon to access different types of parameters.
Let's start by learning how to access the query string parameters in the
first function.

The ``example1`` function below gets all of the query string parameters using
``params.getFromUrl``. To get a specific parameter, just pass a key to
``params.getFromUrl(key)``. In the code below, the key-value pairs that are
fetched by ``params.getFromUrl()`` are wrapped in objects that are pushed to
the array ``paramsArray``. The array is assigned to ``params``, which is then
passed to the ``example1`` template. By default, the function sends data to the
template with the same name.

.. code-block:: javascript

   ...
     // Read from query string
     // e.g., GET /example1?foo=bar
     example1: function(actionContext) {
       // Returns all of the key-value pairs in
       // the query string.
       var params = actionContext.params.getFromUrl(),
       paramsArray = [];
       // Create an object for each key-value pair and
       // push those objects to an array, which is then
       // assigned to 'params' that is available in
       // index template (index.hb.html).
       Y.Object.each(params, function(param, key) {
         paramsArray.push({key: key, value: param});
       });
       Y.log('GET PARAMS: ' + Y.dump(paramsArray));
       actionContext.done(
         {
           title: "Show all query string parameters",
           params: paramsArray
         },
         {name: 'index'}
       );
     },
   ...
   }, '0.0.1', {requires: ['dump', 'mojito-params-addon']});

The ``example2`` function below uses ``params.getFromBody()`` to extract
parameters from the POST body. Once again, the array of objects containing
the key-value pairs is passed to the ``example2`` template, where the array
is available through the ``params`` variable.

.. code-block:: javascript

   ...
     // Read parameters from POST body
     // e.g., POST /example2 with POST body
     example2: function(actionContext) {
       var params = actionContext.params.getFromBody(),
       paramsArray = [];
       Y.Object.each(params, function(param, key) {
         paramsArray.push({key: key, value: param});
       });
       actionContext.done(
         {
           title: "Show all POST parameters",
           params: paramsArray
         },
         {name: 'index'}
       );
     },
   ...
   }, '0.0.1', {requires: ['dump', 'mojito-params-addon']});

The ``example3`` function below uses ``params.getFromRoute()`` to access the
parameters that are specified in ``app.js``, which we will look at in
the next code snippet.

.. code-block:: javascript

   ...
     // Read parameters from routing system
     example3: function(actionContext) {
       var params = actionContext.params.getFromRoute(),
       paramsArray = [];
       Y.Object.each(params, function(param, key) {
         paramsArray.push({key: key, value: param});
       });
       actionContext.done(
         {
           title: "Show all ROUTING parameters (see app.js)",
           params: paramsArray
         },
         {name: 'index'}
       );
     },
   ...
   }, '0.0.1', {requires: ['dump', 'mojito-params-addon']});

In the ``app.js`` file below, you see parameters are set for the
``example3`` and ``example4`` route. Notice that ``example3`` only accepts
HTTP GET calls, whereas ``example4`` allows both HTTP GET and POST calls.
Storing parameters in your routing configuration allows you to associate
them with a function, an HTTP method, and a URL path.

.. code-block:: javascript


   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('../../../'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.use(libmojito.middleware());

   app.get('/', libmojito.dispatch('frame.index'));
   app.get('/example1', libmojito.dispatch('frame.example1'));
   app.get('/example2', libmojito.dispatch('frame.example2'));
   app.post('/example2', libmojito.dispatch('frame.example2'));
   app.get('/example3', libmojito.dispatch('frame.example3', { "from": "routing", "foo": "bar", "bar": "foo" }));
   app.get('/example4', libmojito.dispatch('frame.example4', { "from": "routing", "foo3": "bar3" }));
   app.post('/example4', libmojito.dispatch('frame.example4', { "from": "routing", "foo3": "bar3" }));


In the ``example4`` function below, you find the parameters catch-all method
``params.getFromMerged``. Using ``params.getFromMerged``, you can get the query
string parameters, the POST body parameters, and the parameters set in
``app.js`` at one time. You can also get a specific parameter by passing
a key to ``params.getFromMerged(key)``. For example,
``params.getFromMerged("from")`` would return the value "routing" from the
parameters set in the ``app.js`` shown above.

.. code-block:: javascript

   ...
     // Read the merged map created by Mojito of all
     // input parameters from the URL query string (GET),
     // the POST body, and any routing parameters
     // that may have been attached during the routing look up.
     // Priority of merging is : Route -> GET -> POST
     example4: function(actionContext) {
       var params = actionContext.params.getFromMerged(),
       paramsArray = [];
       Y.Object.each(params, function(param, key) {
         paramsArray.push({key: key, value: param});
       });
       actionContext.done(
         {
           title: "Show all ROUTING parameters (see app.js)",
           params: paramsArray
         },
         {name: 'index'}
       );
     }
   ...
   }, '0.0.1', {requires: ['dump', 'mojito-params-addon']});

The methods of the ``Params`` addon have the following aliases for simplification:

+---------------------+--------------+
| Method              | Alias        |
+=====================+==============+
| ``getAll()``        | ``all()``    |
+---------------------+--------------+
| ``getFromBody()``   | ``body()``   |
+---------------------+--------------+
| ``getFromFiles()``  | ``files()``  |
+---------------------+--------------+
| ``getFromMerged()`` | ``merged()`` |
+---------------------+--------------+
| ``getfromRoute()``  | ``route()``  |
+---------------------+--------------+
| ``getFromUrl()``    | ``url()``    |
+---------------------+--------------+


For more information, see the `Params addon <../../api/classes/Params.common.html>`_ in
the Mojito API documentation.

.. _code_exs_qp-ex:

Setting Up this Example
=======================

To set up and run ``using_parameters``:

#. Create your application.

   ``$ mojito create app using_parameters``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit Query``
#. To specify that your application use the mojit ``Query``, replace the code in
   ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "frame": {
              "type": "Query"
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
          app.get('/', libmojito.dispatch('frame.index'));
          app.get('/example1', libmojito.dispatch('frame.example1'));
          app.get('/example2', libmojito.dispatch('frame.example2'));
          app.post('/example2', libmojito.dispatch('frame.example2'));
          app.get('/example3', libmojito.dispatch('frame.example3', { "from": "routing", "foo": "bar", "bar": "foo" }));
          app.get('/example4', libmojito.dispatch('frame.example4', { "from": "routing", "foo3": "bar3" }));
          app.post('/example4', libmojito.dispatch('frame.example4', { "from": "routing", "foo3": "bar3" }));

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

#. Change to ``mojits/Query``.
#. Modify the controller to access different query parameters by replacing the code in
   ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('query', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {

          index: function(actionContext) {
          actionContext.done('Mojito is working.');
          },
          // Read from query string
          // e.g. GET /example1?foo=bar
          example1: function(actionContext) {
            var params = actionContext.params.getFromUrl(),
            paramsArray = [];
            Y.Object.each(params, function(param, key) {
              paramsArray.push({key: key, value: param});
            });
            actionContext.done(
            {
              title: "Show all query string parameters",
              params: paramsArray
            },
            {name: 'index'}
             );
          },
          // Read parameters from POST body
          // e.g. POST /example2 with POST body
          example2: function(actionContext) {
            var params = actionContext.params.getFromBody(),
            paramsArray = [];
            Y.Object.each(params, function(param, key) {
              paramsArray.push({key: key, value: param});
            });
            actionContext.done(
              {
                title: "Show all POST parameters",
                params: paramsArray
              },
              {name: 'index'}
            );
          },
          // Read parameters from routing system
          example3: function(actionContext) {
            var params = actionContext.params.getFromRoute(),
            paramsArray = [];
            Y.Object.each(params, function(param, key) {
              paramsArray.push({key: key, value: param});
            });
            actionContext.done(
              {
                title: "Show all ROUTING parameters (see app.js)",
                params: paramsArray
              },
              { name: 'index'}
            );
          },
          // Read the merged map created by Mojito
          // of all input parameters from URL query
          // string (GET), the POST body, and any
          // routing parameters that may have been
          // attached during routing look up..
          // Priority of merging is : Route -> GET -> POST
          example4: function(actionContext) {
            var params = actionContext.params.getFromMerged(),
            paramsArray = [];
            Y.Object.each(params, function(param, key) {
              paramsArray.push({key: key, value: param});
            });
            actionContext.done(
              {
                title: "Show all ROUTING parameters (see app.js)",
                params: paramsArray
              },
              {name: 'index'}
            );
          }
        };
      }, '0.0.1', {requires: ['dump', 'mojito-params-addon']});

#. To display the key-value pairs from the query string parameters, create the template
   ``views/example1.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <h2>{{title}}</h2>
        List of key value pairs:
        <ul>
        {{#params}}
          <li>{{key}} => {{value}}</li>
        {{/params}}
        </ul>
      </div>

#. To display the key-value pairs from the POST request body parameters, create the
   template ``views/example2.hb.html`` with the following:

   .. code-block:: html

      <div id="post_params">
        <h2>Form for Posting Parameters</h2>
        <form method="post">
          <p>
            Framework: <input type="text" name="framework" value="Mojito"/><br/>
            Addon Used: <input type="text" name="addon" value="params"/><br/>
            Method Called: <input type="text" name="method" value="getFromBody()"/><br/>
            <h3>Type of Parameters Passed</h3>
            <input type="radio" name="param_type" value="POST" checked> POST Body</input><br/>
            <input type="radio" name="param_type" value="query string"> Query String</input><br/><br/>
            <input type="submit" value="Submit"/>
            <input type="reset"/>
          </p>
        </form>
      </div>
      <div id="{{mojit_view_id}}" class="mojit">
        <h2>{{title}}</h2>
        List of key value pairs:
        <ul>
          {{#params}}
          <li>{{key}} => {{value}}</li>
          {{/params}}
        </ul>
      </div>

#. To display the key-value pairs set in ``app.js``, create the template
   ``views/example3.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <h2>{{title}}</h2>
        List of key value pairs:
        <ul>
          {{#params}}
          <li>{{key}} => {{value}}</li>
          {{/params}}
        </ul>
      </div>

#. To display all of the available parameters, create the template
   ``views/example4.hb.html`` with the following:

   .. code-block:: html

      <div id="post_params">
        <h2>Form for Posting Parameters</h2>
        <form method="post">
          <p>
            Framework: <input type="text" name="framework" value="Mojito"/><br/>
            Addon Used: <input type="text" name="addon" value="params"/><br/>
            Method Called: <input type="text" name="method" value="getFromBody()"/><br/>
            <h3>Type of Parameters Passed</h3>
            <input type="radio" name="param_type" value="POST" checked> POST Body</input><br/>
            <input type="radio" name="param_type" value="query string"> Query String</input><br/><br/>
            <input type="submit" value="Submit"/>
            <input type="reset"/>
          </p>
        </form>
      </div>
      <div id="{{mojit_view_id}}" class="mojit">
        <h2>{{title}}</h2>
        List of key value pairs:
        <ul>
          {{#params}}
          <li>{{key}} => {{value}}</li>
          {{/params}}
        </ul>
      </div>

#. From the application directory, run the server.

   ``$ node app.js``
#. To see the query string parameters fetched by the controller, go to the URL with the
   query string below:

   http://localhost:8666/example1?foo=bar&bar=foo
#. To see the POST body parameters fetched by the controller, go to the URL below and
   submit the form on the page.

   http://localhost:8666/example2
#. To see the parameters set in ``app.js``, go to the URL below:

   http://localhost:8666/example3
#. To see the query string parameters, the post body parameters, and those set in
   ``app.js``, go to the URL below and submit the form on the page:

   http://localhost:8666/example4?foo=bar&bar=foo

.. _code_exs_qp-src:

Source Code
===========

- `Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/using_parameters/mojits/Query/>`_
- `Routing Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/using_parameters/app.js>`_
- `Using Parameters Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/using_parameters/>`_


