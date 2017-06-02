=================================
Basic Configuring of Applications
=================================

**Time Estimate:** 10 minutes

**Difficulty Level:** Beginning

.. _code_exs_basic_config-summary:

Summary
=======

This example shows how to configure a mojit and the routing for your application.

.. _code_exs_basic_config-notes:

Implementation Notes
====================

The ``application.json`` file is used to specify the mojits that your application can use. 
The example ``application.json`` below specifies that the application use the mojit 
``Simple``.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "simple": {
           "type": "Simple"
         }
       }
     }
   ]

The routing configuration for Mojito applications is contained in ``app.js``. 
In this example ``app.js``, the Mojito server is told to call the ``index`` 
method in the controller when HTTP GET called on the root path.

.. code-block:: javascript

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('../../../'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.get('/', libmojito.dispatch('simple.index'));

The ``index`` method is a canned method in the controller when you create a 
mojit. To learn how to create templates that get data from the controller, 
see `Creating a Simple View with Handlebars <simple_view_template.html>`_.

.. _code_exs_basic_config-setup:

Setting Up this Example
=======================

To set up and run ``simple_config``:

#. Create your application.

   ``$ mojito create app simple_config``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit Simple``
#. To specify that your application use ``Simple``, replace the code in 
   ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "simple": {
              "type": "Simple"
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
#. From the application directory, run the server.

   ``$ node app.js``
#. To view your application, go to the URL:

   http://localhost:8666

.. _code_exs_basic_config-src:

Source Code
===========

- `Simple Config Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_config/>`_

