==============
Simple Logging
==============

**Time Estimate:** 15 minutes

**Difficulty:** Intermediate

.. _code_exs_logging-summary:

Summary
=======

This example shows how to configure the log levels for the client and the 
server in Mojito. Also, see `Logging <../topics/mojito_logging.html>`_
for more information such as including and excluding log messages from 
specified modules.

The following topics will be covered:

- configuring the logging levels
- configuring client-side and server-side logging
- using ``Y.log`` to set log levels, specify reporting modules

.. _code_exs_logging-notes:

Implementation Notes
====================

.. _logging_notes-config:

Log Configuration
-----------------

Logging is configured in the ``application.json`` file with the ``yui.config`` 
object. With the ``yui.config`` object, you can configure the log levels and some 
elements of the output for logs. See the 
`config object <../intro/mojito_configuring.html#config-object>`_ for the 
configurations that can be set.


.. _logging_notes-levels:

Log Levels
----------

Mojito offers the log levels below that you configure in 
``application.json`` or set with ``Y.log``. The default
log level is ``debug``.

- ``debug``
- ``info``
- ``warn``
- ``error``
- ``mojito``
- ``none``

Setting a log level of ``warn`` will filter out all ``debug`` and ``info`` 
messages, while ``warn``, ``error``, and ``mojito`` log messages will be 
processed. To see all log messages, set the log level to ``debug``. The 
``mojito`` log level is for showing Mojito framework-level logging that 
indicate important framework events are occurring.


.. _logging_notes-levels:

Example Log Configuration
-------------------------

In the example, you can see that you use the ``yui.config``
object to configure the log level.

.. code-block:: javascript

       "yui": {
         "config": {
           "debug": true,
           "logLevel": "info"
         }
       }

To configure log levels, the property ``debug`` must be set to ``true``, which
is the default value. For example, if ``yui.config`` contained ``debug: false``,
then the ``logLevel`` property would be ignored.

.. _logging_notes-client_server:

Configuring Client and Server Logging
-------------------------------------

You can use context configurations to set different logging configurations
for the client and server. More specifically, you use the 
the ``master`` and ``runtime:client`` contexts, each with their
own ``yui.config`` object as shown below:

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "logLevel": "debug"
         }
       }
     },
     {
       "settings": [ "runtime:client" ],
        "yui": {
          "config": {
            "logLevel": "info"
          }
        }
     }
   ]


.. _logging_notes-set_levels:

Using Y.log
-----------

The function ``Y.log`` allows you to not only log messages,
but to also set the log level, specify the reporting module,
and suppress a logging event. See the YUI API documentation for
`log <http://yuilibrary.com/yui/docs/api/classes/YUI.html#method_log>`_ for
more information.

In Mojito applications, we recommend that you specify the log level and
the reporting module. For example, the first use of ``Y.log`` 
below will report the message at the log level that is configured in 
``application.json`` or use the default (``debug``) if
no log level is set with ``logLevel``. The second use of ``Y.log`` will 
use the log level ``info``. Both statements specify the reporting module 
``log-binder-index``.

.. code-block:: javascript

   Y.log("This message will be reported at the log level set in application.json or the default level.", null, "log-binder-index");
   Y.log("This log message will be reported at the INFO log level.", "info", "log-binder-index");

.. _code_exs_logging-setup:

Setting Up this Example
=======================

To set up and run ``simple_logging``:

#. Create your application.

   ``$ mojito create app simple_logging``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit log``
#. To configure the log levels for the client and server, replace the code in 
  ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "frame": {
              "type": "HTMLFrameMojit",
              "config":{
                "deploy": true,
                "child":{
                  "type": "log"
                }
              }
            }
          },
          "yui": {
            "config": {
              "debug": true,
              "logLevel": "debug"
            }
          }
        },
        {
          "settings": [ "runtime:client" ],
          "yui": {
            "config": {
              "debug": true,
              "logLevel": "info"
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

#. Change to ``mojits/log``.
#. Modify your controller so that one log message uses the default log level and one log 
   message has the log level set by ``Y.log`` by replacing the code in 
   ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('log', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   
          index: function(ac) {
            Y.log('[CONTROLLER]: Default log-level message with date: ' + new Date(), null, NAME);
            Y.log('[CONTROLLER]: Warn message.','warn', NAME);
            var data = {
                log_config: Y.config.logLevel,
            };
            ac.done(data);
          }
        };
      }, '0.0.1', { requires: ['mojito','mojito-config-addon']});

#. To display your client logging,  replace the content of ``binders/index.js`` with the 
   following:

   .. code-block:: javascript

      YUI.add('log-binder-index', function(Y, NAME) {
        Y.namespace('mojito.binders')[NAME] = {
          init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
          },
          bind: function(node) {
            Y.log("[BINDER]: Default Log level: " + Y.config.logLevel, null, NAME);
            Y.log('[BINDER]:  Error log message.', "error", NAME);
            Y.one("#client_config").all("b").item(0).insert(Y.config.logLevel,"after");
            this.node = node;
          }
        };
      }, '0.0.1', {requires: ['mojito-client']});


#. Modify the default template by replacing the code in ``views/index.hb.html`` with the 
   following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <h2 style="color: #606; font-weight:bold;">Simple Log Configuration </h2>
        This app is to demonstrate the the logging level and its configuration.
        <div id="server_config">
          <h3> Server Configuration </h3>
          <b>Log level: </b>{{log_config}}<br/>
        </div>
        <div id="client_config">
          <h3> Client Configuration </h3>
          <b>Log level: </b> <br/>
        </div>
      </div> 

#. From the application directory, run the server.

   ``$ node app.js``
#. Open the URL below in a browser and look at the output from the Mojito 
   server. You should see the log messages from the controller that start 
   with the string "\[CONTROLLER]:". Notice that the two messages have 
   different log levels: one is the default (``debug``) and the other sets
   the log level ``warn`` with ``Y.log``. 

   http://localhost:8666/

#. Open your browser's developer console, such as Firebug, and view the console 
   logs. You should see the client log messages from the binder that start with 
   the string "\[BINDER]". Again, you will see log messages using different log
   levels.

.. _code_exs_logging-src:

Source Code
===========

- `Simple Logging App <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_logging/>`_
- `Logging Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_logging/application.json>`_
- `Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_logging/mojits/log/controller.server.js>`_
- `Binder <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/simple_logging/mojits/log/binders/index.js>`_

