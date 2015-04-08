=======
Logging
=======

Mojito relies on YUI for logging. When you call ``Y.log`` from within your mojits, your 
log messages are handled by a YUI instance that Mojito creates based on YUI configurations 
defined in ``application.json`` or ``application.yaml``. You can set logging levels to 
control the degree of detail in your log reports. 

Mojito does not write logs to a file. Instead, Mojito writes logs to the Node.js console. 
Thus, once logs are passed to Node.js, Mojito has no control over whether Node.js writes 
logs to a file, transmits them into an aggregated hub for multiple cores, or uses another 
implementation for logging.

.. _mojito_logging-levels:

Log Levels
==========

Mojito has the following six log levels:

- ``debug``
- ``mojito``
- ``info``
- ``warn``
- ``error``
- ``none``

All of them should be familiar except ``mojito``, which is the logging level for
capturing framework-level messages that indicate that an important framework event is 
occurring (one that users might want to track).

Setting a log level of ``warn`` will filter out all ``debug`` and ``info`` messages, while 
``warn``, ``error``, and ``mojito`` log messages will be processed. To see all 
log messages, set the log level to ``debug``.

.. _mojito_logging-defaults:

Log Defaults
============

The server and client log settings have the following default values:

- ``debug: true`` - turns logging on so that messages are displayed in the console.
- ``logLevel: "debug"`` - log level filter.
- ``logLevelOrder: ['debug', 'mojito', 'info', 'warn', 'error', 'none']`` - the order in 
  which the log levels are evaluated. 
  


.. _mojito_logging-config:

Log Configuration
=================

All the values above are configurable through the 
`yui.config object <../intro/mojito_configuring.html#yui_config>`_ in the 
``application.json`` file. In the example ``application.json`` below, the ``yui.config`` 
object overrides the default for ``logLevel``. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "debug": true,
           "logLevel": "error"
         }
       },
       ...
     }
   ]

.. note:: To set ``logLevel``, the property ``debug`` must be set to ``true``, which
          is the default.

.. _logging_config-prod:

Recommended Logging Configuration for Production
------------------------------------------------

For production, we recommend that you use the ``environment:production``
context with the log configuration shown below:

.. code-block:: javascript

   [
     {
       "settings": [ "environment:production" ],
       "yui": {
         "config": {
           "debug": false,
           "logLevel": "none"
         }
       },
       ...
     }
   ]


.. _mojito_logging-custom:

Customizing Logging
===================

.. _logging_custom-rt_context:

Client and Server Logging
-------------------------

You can use the ``master`` and  the ``runtime:client`` contexts to create different 
logging settings for the client and server.

In the ``application.json`` file, create two configuration
objects that use the ``master`` context for the server-side log configuration
and the ``runtime:client`` context for the client-side log configuration 
as shown below. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
     },
     {
       "settings": [ "runtime:client" ],
     },

   ]

For each context, configure your logging with
the ``yui.config`` object.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       ...
       "yui": {
         "config": {
           "debug": true,
           "logLevel": "info"
         }
       }
     },
     {
       "settings": [ "runtime:client" ],
       ...
       "yui": {
         "config": {
           "debug": true,
           "logLevel": "warn"
         }
       }
     }
   ]


.. _logging_custom-using_ylog:

Using Y.log in Mojito Applications
----------------------------------

You use ``Y.log`` in Mojito as you would in any application
using YUI. See the YUI API documentation for
`log <http://yuilibrary.com/yui/docs/api/classes/YUI.html#method_log>`_ for
details about the parameters and return values.

We recommend that you pass the first three parameters to
``Y.log`` in your Mojito application:

- ``msg`` - the message to log
- ``cat`` - the log level or category, such as 'info', 'error', 'warn'
- ``src`` - the name of the module reporting the error

In the example binder below, ``Y.log`` logs
a message at the ``info`` level and specifies the module
through ``NAME``, which in this case contains the value ``demo-binder-index``.

.. code-block:: javascript

   YUI.add('demo-binder-index', function(Y, NAME) {
    Y.namespace('mojito.binders')[NAME] = {
        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },
        bind: function(node) {
            Y.log("Log message", "info", NAME);
            this.node = node;
        }
    };
  }, '0.0.1', {requires: ['mojito-client']});


.. logging_levels-define:

Customizing the Log Level Order
-------------------------------

You can reorder and create log levels with the ``logLevelOrder`` property of the 
``yui.config`` object. In the example ``yui.config`` object below,
the order of the log levels is switched for ``warn`` and ``info`` and 
the new log level ``danger`` is created.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "debug": true,
           "logLevelOrder": [ "debug", "warn", "info", "error", "danger", "none" ]
         }
       },
       ...
     }
   ]


.. _logging_custom-include_exclude_src:

Including and Excluding Modules From Logging
--------------------------------------------

You can use the ``logExclude`` and ``logInclude`` properties
of the ``yui.config`` object to include or exclude logging
from YUI modules of your application. 

The configuration below excludes logging from the YUI module 
``finance-model-stocks``:

.. code-block:: javascript

   "yui": {
     "config": {
      "debug": true,
      "logLevel": "info",
      "logExclude": { "finance-model-stocks": true } 
     }
   }


Based on the logging configurations above, the
``Y.log`` messages in the model below will be excluded
from the log:

.. code-block:: javascript

   YUI.add('finance-model-stocks', function (Y, NAME) {

     Y.namespace('mojito.models')[NAME] = {

       init: function (config) {
         // The following log message will be excluded from the log
         // because "logExclude": { "finance-model-stocks" }.
         // NAME => "finance-model-stocks"
         Y.log('this message will be excluded', 'info', NAME);
         this.config = config;
       },
       ...
     };

   }, '0.0.1', {requires: []});
