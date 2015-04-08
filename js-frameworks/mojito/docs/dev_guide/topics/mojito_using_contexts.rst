============================
Using Context Configurations
============================

.. _context_configs-intro:

Introduction
============

Context configurations are how Mojito enables different configurations to be 
used based on various runtime factors. Many factors are predefined such as 
language and device, but you can create custom ones as well. These runtime 
factors are called **contexts** in Mojito and are mapped to user-defined 
configurations. For example, you could set the configuration 
``logLevel`` to ``error`` in the production context and set it to ``info`` 
in the development context.

.. _context_configs_intro-why:

Why Use Context Configurations?
-------------------------------

Context configurations make it possible to do the following:

- Create sets of configurations associated with environments without affecting 
  the application running with the *master* configurations 
  ``"setting: ["master"]``. 
- Customize content for users: Applications can dynamically apply language and
  device configurations by determining the user's language preferences and the 
  device making the HTTP request. 

.. _context_configs-what:

What is a Context?
==================

The context is the runtime parameters that are either statically set 
(base context) on the command line or dynamically set (request context) 
in the HTTP headers and/or the request query string. The configurations 
for request contexts override those of the base context.

.. _context_configs_what-base:

Base Context
------------

The base context is statically set in ``app.js`` by passing a ``context`` object
to the ``extend`` method that is called from an instance of Mojito. You pass the
instance of an Express application and the ``context`` object to ``extend`` to
set the base context.

In the example ``app.js`` below, ``libmojito`` is an instance of Mojito, and ``app``
is an instance of Express. You pass ``app`` to ``libmojito.extend`` as well as
the ``context`` object that specifies the ``development`` environment.

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

.. note:: Mojito v0.8.x and Earlier

          In versions of Mojito before v0.9, you used the Mojito CLI utility to specify
          the context with the ``--context`` option on the command line. We recommend
          adapt your applications to Mojito v0.9, which in general only involves creating
          the file ``app.js``, removing the ``server.js`` file, and using ``node app.js`` to
          start applications.

.. _context_configs_what-request:

Request Contexts
----------------

Contexts that are dynamically invoked by HTTP requests are called request 
contexts. When Mojito receives an HTTP request that specifies a context, 
the configurations mapped to that context will be dynamically applied. 
The contexts can be specified in HTTP request as a parameter in the query 
string or in the HTTP header.

.. _context_request-headers:

Request Headers
###############

The contexts for languages can be requested using the HTTP header 
``Accept-Language``. After starting an application with the context 
``"environment:testing"``, you can dynamically apply the configurations 
for the context ``"environment:testing,lang:fr"`` by sending the HTTP 
header ``"Accept-Language: fr"``. In the same way, the contexts for 
devices can be requested using the HTTP header ``User-Agent``. The 
configurations for the context "device:android" could be requested 
with the HTTP header ``"User-Agent: Mozilla/5.0 (Linux; U; Android 2.3; en-us)"``.

.. _context_request-query_str:

Query String Parameters
#######################

The  query string can also dynamically set the context.

.. _request_query_str-syntax:

Syntax
``````
 
``?key1=value1,key2=value2``

.. _request_query_str-ex:

Example
```````

For example, if an application is started with the base context 
``"environment:testing"`` and you want to dynamically apply the context 
``"environment:testing,device:iphone"``, you could append the following 
query string to the application URL: 

``?device=iphone``



.. _contexts-predefined:

Mojito Predefined Contexts
--------------------------

The following lists the contexts that are defined by Mojito. You can define 
configurations for these predefined contexts. You can combine multiple contexts 
to form a compound context as well. For example, if you wanted a context to map 
to configurations for Android devices in a testing environment, you could use 
the following compound context: ``"environment:test,device:android"``

- ``environment:development``
- ``environment:production``
- ``environment:dev``
- ``environment:test``
- ``environment:stage``
- ``environment:prod``
- ``device:android``
- ``device:blackberry``
- ``device:iemobile``
- ``device:iphone``
- ``device:ipad``
- ``device:kindle``
- ``device:opera-mini``
- ``device:palm``
- ``lang:{BCP 47 language tag}``
- ``runtime:client``
- ``runtime:server``



You can view the supported BCP 47 language tags and default contexts in the 
`dimensions.json <https://github.com/yahoo/mojito/blob/develop/lib/dimensions.json>`_ 
file of Mojito. You can also :ref:`create custom contexts <context_configs-custom>` 
if the Mojito default contexts don't meet the needs of your application.

.. _context_configs-resolution:

How Does Mojito Resolve Context Configurations?
===============================================

When a request is made to a Mojito application, Mojito has to resolve 
configurations, defined contexts (``dimensions.json``), and the base/requested 
contexts before the correct context configurations can be applied.

The following are the steps taken by Mojito to apply the correct context
configurations:

#. **Determines Valid Contexts:**

   Mojito looks for a local ``dimensions.json``. If one is found, Mojito 
   replaces Mojito's ``dimensions.json`` with it. Mojito then uses 
   ``dimensions.json`` to determine which contexts are valid. Contexts 
   defined earlier in ``dimensions.json`` override contexts defined later 
   in the file.
   
#. **Merges Configurations**

   Mojito merges configurations for all contexts, with the configurations
   in ``application.json`` overriding those in ``defaults.json``. If contexts
   are found that are not defined in ``dimensions.json``, Mojito will throw
   an error.
   
#. **Determines Context**

   - Mojito checks if a base context was specified (statically) on the command 
     line with the ``--context`` option. 
   - When Mojito receives an HTTP request, it looks for a request context in 
     the query string, HTTP headers, or through the execution of a child mojit 
     with configuration information. 
   - Mojito merges the base context (if any) with the request context (if any). 
     For example, if the base context is ``"environment:develop``" and the 
     request context found in the query string is ``"?lang=de"``, then the 
     compound context in the ``setting`` array in configuration files would 
     be ``["environment:development", "lang:de"]``. 
   - If no base or request context is found, Mojito then uses the default 
     context ``master``.

#. **Resolves Context Configurations**

   Mojito then searches for configurations associated with the determined 
   context. The contexts are found in the ``setting`` object in configuration 
   files. Mojito will use the more qualified contexts if present over more 
   general contexts. For example, if the merged base and request context is 
   ``"environment:prod, device:iphone"``, then Mojito will use it over either 
   ``"device:iphone"`` or ``"environment:prod"``. If ``"environment:prod, device:iphone"`` 
   is not present, Mojito will use the request context over the base context 
   as the resolved context. 
  
#.  **Applies Context Configuration**

    Mojito applies the configurations associated with the resolved context. 
    
   
.. _context_configs-define:

Defining Configurations for Contexts
====================================

Configurations for contexts are defined in the application configuration file 
``application.json``.  Default configurations are defined 
in the ``defaults.json`` file of a mojit. All configurations are merged when an 
application starts. The configuration values in ``application.json`` override 
those in ``defaults.json``.

.. _context_configs_define-obj:

Configuration Objects
---------------------

The ``application.json`` file in the application directory 
and the ``defaults.json`` file in a mojit's directory consist of an array of 
configuration objects. The configuration object has a ``settings`` array that 
specifies the context. The configuration objects in ``application.json`` also 
have a ``specs`` object containing mojit instances, which may also have a 
``config`` object that has data in the form of key-value pairs. The configuration 
objects in ``defaults.json`` do not have a ``specs`` object because they do not 
define mojits, but do have a ``config`` object for storing key-value pairs. 

.. _context_configs_obj-setting:

setting
#######

The ``settings`` array specifies the context or the default ("master") that is 
then mapped to configurations.

.. _context_obj_setting-default:

Default Configurations
**********************

The configurations for the ``"master"`` context are used when no context is given. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         ...
       }
     },
     ...
   ]

.. _context_obj_setting-simple:

Simple Context Configuration
****************************

The context is specified in the ``settings`` array of the configuration object.

.. code-block:: javascript

   [
     ...
     {
       "settings": [ "environment:development" ],
       "specs": {
        ...
       }
     },
     ...
   ]

.. _context_obj_setting-compound:

Compound Context Configuration
******************************

Compound contexts are specified in the settings array as a series of contexts 
separated by commas as seen below.

.. code-block:: javascript

   [
     ...
     {
       "settings": [ "environment:development", "device:android" ],
       "specs": {
         ...
       }
     },
     ...
   ]

.. _context_obj_setting-routing:
   
Routing Context Configuration
*****************************

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "master_route": {
         ...
       }
     },
     {
       "settings": [ "environment:development"],
       "dev_route" : {
         ...
       }
     }
   ]

.. _context_configs_obj-specs:

specs
#####

The ``specs`` object contains the mojit instances associated with a context.

.. code-block:: javascript

   [
     ...
     {
       "settings": [ "environment:production" ],
       "specs": {
         "photos": {
           "type": "Photo"
         }
       }
     },
     ...
   ]

.. _context_configs_obj-config:

config
######

The ``config`` object stores configuration for a mojit that is mapped to the context.

.. code-block:: javascript

   [
     ...
     {
       "settings": ["device:iphone"],
       "specs": {
         "iphone": {
           "type": "iPhone",
           "config": {
             "viewport_width": 320
           }
         }
       }
     },
     ...
   ]

.. _context_configs_define-exs:

Examples
--------

.. _context_configs_exs-applicationjson:

application.json
################

The configuration objects in ``application.json`` below define default 
configurations and three context configurations. The last context configuration 
contains two strings containing key-value pairs and is, thus, called a compound 
context configuration.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "mainPage": {
           "type": "Test"
           "config": {
             "env": "This is the default environment."
           }
         }
       }
     },
     {
       "settings": [ "environment:development" ],
       "specs": {
         "mainPage": {
           "type": "Test",
           "config": {
             "env": "I am in the development environment."
           }
         }
       }
     },
     {
       "settings": [ "environment:production" ],
       "specs": {
         "mainPage": {
           "type": "Test",
           "config": {
             "env": "I am in the production environment."
           }
         }
       }
     },
     {
       "settings": [ "environment:production", "device:kindle" ],
       "specs": {
         "mainPage": {
           "type": "Test",
           "config": {
             "env": "I am in the production environment for Kindles."
           }
         }
       }
     }
   ]

.. _context_config_exs-defaults_json:

defaults.json
#############

The configuration ``gamma`` in the example ``defaults.json`` below is mapped 
to contexts for languages.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "config": {
         "alpha" : "I am the first!",
         "beta" : "I am the second!",
         "gamma": "I am the third!"
       }
     },
     {
       "settings": [ "lang:de" ],
       "config": {
         "gamma": "I am (when lang=de is passed) the third!"
       }
     },
     {
       "settings": [ "lang:fr" ],
       "config": {
         "gamma": "defaults.json - (when lang=fr is passed) the third!"
       }
     }
   ]

.. _context_configs_define-static:

Static Configurations 
=====================

Certain context configurations can only be set once by the base context.
In other words, once the application starts with a given base context, the
values for certain configurations are static: they will not change until
the application is started with another base context that's either  
specified on the command line or configured in the ``server.js`` file.

The following configurations are static:

- `actionTimeout <../intro/mojito_configuring.html#configuration-object>`_
- `builds <../intro/mojito_configuring.html#builds-object>`_ - (only used by the ``mojito build`` command)
- `mojitDirs <../intro/mojito_configuring.html#configuration-object>`_
- `mojitsDirs <../intro/mojito_configuring.html#configuration-object>`_
- `routesFiles <../intro/mojito_configuring.html#configuration-object>`_
- `staticHandling <../intro/mojito_configuring.html#statichandling-object>`_
- `tunnelPrefix <../intro/mojito_configuring.html#configuration-object>`_
- `viewEngine <../intro/mojito_configuring.html#viewengine-object>`_
- `yui <../intro/mojito_configuring.html#yui-object>`_


.. _context_configs-dynamic:

Dynamically Changing Configurations
===================================

You may dynamically change the configurations for any context by having a parent 
mojit execute a child mojit with new configurations. This is different than 
getting different configurations by requesting a new context or specifying a 
different base context. Regardless of the context being used, you can use the 
same context and change the configurations by executing a child mojit with 
new configurations. The parent mojit uses the ``execute`` method of the 
`Composite addon <../../api/classes/Composite.common.html>`_ to execute the 
child mojit. Let's look at an example to see how it works.

In the example controller below, if the ``child`` parameter is found in the 
routing, query string, or request body, a child instance with its own 
configuration is executed, allowing the application to add new or change 
configurations of the current context.

.. code-block:: javascript


   YUI.add('test', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {
       index: function(ac) {
         var cfg = {
           children: {
             "one": {
               "type": "Child",
               "action": "index",
               "config": {
                 "alpha": "Creating a new 'alpha' key or replacing the value of the alpha 
                          key mapped to the context being used. The context, however, does 
                          not change."
               }
             }
           }
         };
         var child = ac.params.getFromMerged('child');
         if (child){
           ac.composite.execute(cfg, function (data,meta){
             ac.done(data["one"]);
           });
         } else{
           ac.done(
             'config key "alpha": ' + ac.config.get('alpha', '[alpha not found]')
           );
         }
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-config-addon', 'mojito-params-addon', mojito-composite-addon']});


.. _context_configs-custom:

Creating Custom Contexts
========================

The Mojito framework defines default contexts that developers can map 
configurations to. These default contexts are defined in the file 
`dimensions.json <https://github.com/yahoo/mojito/blob/develop/source/lib/dimensions.json>`_ 
found in the Mojito source code. Developers can create an application-level 
``dimensions.json`` to define custom contexts that can be mapped to configurations 
as well. 

The local ``dimensions.json`` replaces the Mojito's ``dimensions.json``, so to 
create custom contexts, you will need to copy Mojito's ``dimension.json`` to 
your application directory and then add your custom contexts to the file. 
Defining and applying configurations for custom contexts is done in the same 
way as for default contexts.

.. _context_configs_custom-create:

Who Should Create Custom Contexts?
----------------------------------

Developers who create applications that require a degree of personalization 
that extends beyond language and device would be good candidates to create 
custom contexts. Before beginning to create your own ``dimensions.json`` file, 
you should review the :ref:`contexts-predefined` to make sure that you truly 
need custom contexts.

.. _context_configs_custom-dimensions:

Dimensions File
---------------

The key-value pairs of the context are defined in the ``dimensions.json`` 
file in the application directory. Once contexts are defined in the 
``dimensions.file``, you can then map configurations to those contexts. 
If your application has configurations for a context that has not been 
defined by Mojito or at the application level in ``dimensions.json``, 
an error will prevent you from starting the application.

.. _dimensions-syntax:

Syntax for JavaScript Object
############################

In the ``dimension.json`` file, the ``dimensions`` array contains JavaScript 
objects that define the contexts. The keys of the context are the names of 
the objects, and the values are the object's properties as seen below.

.. code-block:: javascript

   [
     {
       "dimensions":[
         {
           "region": {
           "us": null,
           "jp": null,
           "cn": null
         },
         ...
        ]
     }
   }

.. _dimensions-ex:

Example dimensions.js
#####################

Based on the example ``dimensions.json`` below, the following are 
valid contexts:

- ``"account_type:basic"``
- ``"account_type:premium"``
- ``"account_type:basic,region:us"``
- ``"account_type:premium,region:fr"``

.. code-block:: javascript

   [
     {
       "dimensions": [
         ...
         {
           "account_type": {
             "basic": null,
             "premium": null
           }
         },
         {
           "region":{
             "us": null,
             "gb": null,
             "fr": null
           }
         }
         ...
     }
   ]


