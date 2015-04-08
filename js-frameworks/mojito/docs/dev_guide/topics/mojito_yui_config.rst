
=========================
Configuring YUI in Mojito
=========================

.. _yui_config-intro:

Overview
========

Mojito allows you to configure the YUI seed file and use YUI groups for dynamically
loading modules in applications. By customizing YUI configuration in Mojito,
you have finer grain control over the modules included in the seed 
and the combo handler used for dynamically loading YUI modules. Developers
can also just use Mojito's default YUI configurations, which are optimized
for improved performance.

.. _yui_config_intro-benefits:

Benefits of Customizing YUI Configuration
----------------------------------------- 

Developers can customize YUI configuration in Mojito applications 
to do the following:

- select which YUI modules are included in the YUI seed file
- configure the combo handler to use a CDN
- optimize performance for environments that may have latency issues
  or have limited CPU power
- limit the loading of certain YUI modules for specific languages


.. _yui_config-seed:

YUI Seed File
=============

.. _seed-yui:

Seed File in YUI Applications
-----------------------------

To use YUI in Web pages, you include a small JavaScript file called the 
YUI seed file. The YUI seed file allows you to load other YUI components on your page. 
The seed file is added to your Web page by with following ``<script>`` tag.

``<script src="http://yui.yahooapis.com/3.8.0/build/yui/yui-min.js"></script>``

From the URL to the seed file, the YUI library can infer the version of the library that 
should be used, the filter that you want to use (min, debug or raw), and the CDN that is 
serving the library. 

.. _seed-mojito:

Seed File in Mojito Applications
--------------------------------

In Mojito applications, the YUI seed is configured in ``application.json`` rather than 
including a ``<script>`` tag in templates. Thus, the information inferred from 
the URL to the YUI seed file in YUI applications is instead provided 
in the ``yui.config.seed`` object of ``application.json``.
We will look at ``yui.config.seed`` in :ref:`Configuration of the Seed File <seed-configure>`.

Mojito uses configuration for the YUI seed because of the following reasons:

- The YUI library is bundled with the application using npm, so loading
  modules is done differently.
- Mojito applications may run as mobile applications that have connectivity
  issues preventing access to the YUI seed file.
- When applications are started, new YUI modules, part of the Mojito code, and part of the 
  application code are loaded in the same way as the YUI Core modules, so
  it is difficult to simply include the YUI seed file in a template.

.. _seed-default:

Mojito's Default Seed File
##########################

Mojito creates a default configuration for the YUI seed, so most users do not 
need to configure the YUI seed as the default configuration is sufficient.
Developers who want finer grain control over the loader for performance 
optimization should consider customizing the configuration for the YUI seed.

.. _seed-configure:

Configuration of the Mojito Seed File
=====================================

Starting from Mojito v0.5.0, developers can configure the YUI seed 
using the ``yui.config.seed`` object in ``application.json`` file. 

In the example ``application.json`` below, the YUI seed includes
the modules specified in the ``seed`` object. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "seed": [
             "yui-base",
             "loader-base",
             "loader-yui3",
             "loader-app"
           ]
         }
       }
     }
   ]

If your application has language bundles, Mojito will also auto-generate language versions 
of the ``loader-app`` module (e.g, ``loader-app_en-US``). The ``loader-app`` module and 
its language versions are synthetic modules, which are different than
the other modules in the example ``application.json`` above that are just YUI Core modules. 
We will discuss the differences in more detail in 
:ref:`Synthetic Modules in Mojito <seed_configure-modules>`.

.. _seed_configure-modules:

What Modules Should Be in the Seed?
-----------------------------------

When including modules as part of the seed, developers need to decide
which modules are critical and understand what modules are available to 
Mojito applications. In theory, all YUI modules that are part of Mojito core, YUI core, 
and your application can be part of the seed. You can also add non-core YUI modules to 
be part of the seed, but we recommend that you don't unless you have a strong reason to 
do so because the seed file should be as small as possible. 

In addition, Mojito generates a few more virtual files (in memory, not actual physical 
files) that we will call *synthetic* modules, which you can also include in the ``seed``
object. We will discuss synthetic modules and how to use them next.


.. _seed_configure-synthetic:

Synthetic Modules in Mojito
---------------------------

.. _synthetic_mods-what:

What Are Synthetic Modules?
###########################

When you run ``node app.js`` or use an alternative way to boot your application, 
the Mojito store analyzes the directory structure and dependencies to try to understand the 
structure and then make assumptions. From this analysis, the synthetic modules 
create application metadata that can be used by YUI Loader to load the application and 
Mojito modules on demand. Without this metadata, the application cannot function.

Synthetic modules are not physical files. If you need
to generate physical files for a CDN from the synthetic modules, you can 
use `Shaker <https://developer.yahoo.com/cocktails/shaker/>`_. 
For production, we recommend using Shaker, especially in the case that your mojits contain 
language resource bundles.

.. _synthetic_mods-mult_langs:

Synthetic Modules for Multiple Languages
########################################

Your application can run in multiple languages, but you should not load all available 
language bundles in the client runtime for performance reasons. Instead, you can use 
synthetic modules to load modules based on the languages specified in the request 
information and the user preferences. Mojito will locate the corresponding synthetic 
module name based on the language context. 


.. _synthetic_mult_langs-restriction:

Restrictions
************

Not all synthetic modules can be customized per language. 
Only **loader-app-base** synthetic module can have language versions. 
Also, the default synthetic modules ``loader-app-base``,
always exists, so, if no language is specified, but many language resource bundles 
exist for a mojit, then the default synthetic module will load the metadata for all of 
the modules. If an application has multiple mojits each with dozens of language bundles,
the amount of metadata can be considerable.

.. _synthetic_mods-create:

Creation of Synthetic Files
###########################

In terms of extending Mojito's functionality, if you create a Resource Store addon, you 
can create new synthetic modules and control the seed generation by piping into 
`getAppSeedFiles <https://developer.yahoo.com/cocktails/mojito/api/classes/RSAddonYUI.html#method_getAppSeedFiles>`_ 
method of the `RSAddonYUI Class <https://developer.yahoo.com/cocktails/mojito/api/classes/RSAddonYUI.html>`_. 

.. _yui-getting_to_app:

Getting YUI to Your Application
===============================

Your application can use the methods and any combination of 
the methods below to load YUI modules.

- Use the YUI version that comes with Mojito.
- Get YUI from the YUI CDN.
- Use a custom CDN to serve YUI.
- Bundle YUI with the application and configure your application to use it.

In the following sections, we'll discuss the methods above, list the pros and 
cons of each, and provide an example application configuration.

To better understand how to configure YUI in Mojito, we also recommend that you 
refer to documentation for the configuration object 
`yui.config <../intro/mojito_configuring.html#yui-conf>`_, which provides descriptions 
as well as possible and default values of its properties.

.. _serving_to_app-yui_with_mojito:

Using the YUI Version Bundled with Mojito
-----------------------------------------

Mojito comes with YUI, so developers don't need to worry about getting 
YUI or which version to use. Using the bundled YUI version also allows Mojito applications 
to load YUI modules more quickly and efficiently. Developers can use the default 
configurations, and thus, not need to do any configuration, or configure Mojito to 
include specific YUI modules to be part of the seed. In either case, Mojito will combo 
handle the modules in the seed, so that only one HTTP request is needed.

Before looking at the default configurations or configuring Mojito to include specific
modules in the seed, let's look at the pros and cons of using the YUI version bundled 
with Mojito.

.. _yui_with_mojito-pros_cons:

Pros
####

- The default configuration is optimized, so developers don't need to provide any
  additional configuration.
- Mojito will handle the combo handling for you, so only one HTTP request is needed
  to fetch the YUI modules.

.. _yui_with_mojito-cons:

Cons
####

- You are limited to using the YUI version bundled with Mojito. 

.. _yui_with_mojito-default:

Default Configuration
#####################

Without setting any YUI configurations with ``yui.config`` in ``application.json``,
Mojito by default will use the configurations below to load the YUI modules ``yui-base``, 
``loader-base``, and ``loader-yui3`` from the version of YUI that comes with Mojito. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "seed": [
             "yui-base",
             "loader-base",
             "loader-yui3",
             "loader-app",
             "loader-app-base{langPath}"
           ]
         }
       }
     }
   ]

.. note:: The ``loader-app`` and ``loader-app-base{langPath}`` modules are
          :ref:`synthetic module <seed_configure-synthetic>`
          that loads the information and organizes the Mojito code and modules that your 
          application needs, but does not load YUI or affect the loading of YUI.

.. _yui_with_mojito-specifying:

Specifying YUI Modules
######################

As we mentioned earlier, you can also configure Mojito to load specific modules from the 
YUI version bundled with Mojito by adding the module names to the ``yui.config.seed`` 
array.

The example ``application.json`` below configures Mojito to load the YUI
modules ``json-parse`` and ``json-stringify`` as well as the default modules. 
Remember though, when you load additional modules, you are increasing the size
of the seed file and may negatively impact performance. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "seed": [
             "yui-base",
             "loader-base",
             "loader-yui3",
             "json-parse",
             "json-stringify",
             "loader-app",
             "loader-app-base{langPath}"
           ]
         }
       }
     }
   ]

Note: modules in ``seed`` array will not be expanded, which means that their dependencies will
not be automatically included until they are used by your application code.

.. _serving_to_app-yui_cdn:

Using the YUI CDN
-----------------

You can also fetch YUI directly from the YUI CDN by specifying the URLs to the 
version of YUI from the YUI CDN in the ``seed`` object. 

.. _yui_cdn-pros:

Pros
####

- By serving YUI from the YUI CDN, you can choose the version of YUI to serve 
  and have your application from the client load YUI. 

.. _yui_cdn-cons:

Cons
####

- Your application will need to make separate HTTP requests to get YUI from the YUI CDN in
  addition to loading ``loader-app`` and ``loader-app-base{langPath}`` from mojito directly
  to get the application ready.

.. _yui_cdn-ex:  

Example
#######

In the example ``application.json`` below, the ``seed`` array includes the ``yui-base`` 
module from the YUI bundled with Mojito and the ``loader-base`` and ``loader-yui3`` modules
from the YUI CDN. To serve the YUI modules in the seed, your application will have
to make two HTTP requests, something you should consider when choosing to get
modules from the YUI CDN. 

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "seed": [
             "http://yui.yahooapis.com/combo?3.8.1/yui-base/yui-base-min.js&3.8.1/loader-base/loader-base-min.js&3.8.1/loader-yui3/loader-yui3-min.js",
             "loader-app",
             "loader-app-base{langPath}"
           ]
         }
       }
     }
   ]

.. _serving_to_app-custom_cdn:

Using a Custom CDN to Load YUI
------------------------------

Using a custom CDN to load YUI is done in the same way as loading YUI from the
YUI CDN. 

.. _custom_cdn-pros:

Pros
####

- By serving YUI from a custom CDN, you can choose the version of YUI to serve 
  and have your application from the client load YUI. 

.. _custom_cdn-cons:

Cons
####

- You application need to make separate HTTP requests to get YUI from the YUI CDN and
  any YUI modules from the YUI that comes bundled with Mojito.

.. _custom_cdn-ex:

Example
#######

In the example below, we simply fetch the YUI seed files from an Amazon S3 over SSL. 
Again, Mojito will make three separate HTTP requests to get the files from the CDN.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "seed": [
             "https://mybucket.s3.amazonaws.com/my_yui_version/yui-base/yui-base-min.js",
             "https://mybucket.s3.amazonaws.com/my_yui_version/yui-base/loader-base-min.js",
             "https://mybucket.s3.amazonaws.com/my_yui_version/yui-base/loader-yui3-min.js",
             "loader-app",
             "loader-app-base{langPath}"
           ],
           "gallery": "gallery-2013.01.16-21-05",
           "base": "http://yui.yahooapis.com/3.8.1/build/",
           "comboBase": "http://yui.yahooapis.com/combo?",
           "root": "3.8.1/build/"
         }
       }
     }
   ]

.. _serving_to_app-yui_app:

Using the YUI Bundled With Your Application
-------------------------------------------

Unless you have a strong reason for bundling YUI with your application,
we strongly recommend that you use the version of YUI that comes with Mojito or
load YUI from the YUI CDN. 

To bundle YUI with your application, you install YUI in a directory within your 
application, and then configure the application to point to the directory, so your
application can serve it as a static asset. You use the ``yui.config`` object in 
``application.json`` to specify the file path and whether you want to combo handle the 
modules.

Let's review the pros and cons and then show you an example configuration for
serving YUI bundled with an application.


.. _yui_app-pros:

Pros
####

- By bundling YUI with your application, you can choose the version of YUI to serve. 
- If you deploy the YUI bundled with your application to the client, the application
  can use the YUI when offline. For example, your application can run as an HTML5 
  or hybrid application (e.g., PhoneGap).


.. _yui_app-cons:

Cons
####

- Node.js is not very good at serving static assets (i.e., YUI), so the performance of 
  your application may be negatively impacted.
- Static assets are not versioned and are generally cached in browsers. Thus, 
  although you update the YUI version bundled with your application, the client
  may still be using an older version: this is the most convincing reason to use
  a CDN, which avoids this problem.

.. _yui_app-ex:
 
Example
#######

In the example ``application.json`` below, the ``base`` property
specifies the file path for loading YUI bundled in the application. 
Because ``combine`` is ``false``, the ``comboBase``, ``root``, and
``comboSep`` will not be be used to load YUI with the built-in combo handler.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "base": "/static/yui/",
           "combine": false,
           "comboBase": "/combo~",
           "comboSep": "~",
           "root": "/static/yui/"
         }
       }
     }
   ]


.. _yui_config-app_grp:

YUI App Group
=============

.. _app_grp-intro:

Introduction
------------

By default, YUI defines the three groups ``default``, ``gallery``, and ``yui2``. 
In Mojito v0.5.0, we introduce the group ``app`` as part of the loader metadata. This 
new group aggregates all the YUI modules defined in Mojito core and in the application and 
contains configuration that define how YUI manages those modules when they are needed.

.. _app_grp_intro-why:

Why Use the App Group?
######################

Groups are an important part of the YUI Loader configuration because they allow 
developers to define buckets of files that can be loaded from different mediums and 
sources. For example, by using the ``app`` group, you can load YUI modules from a CDN
and change the group configurations for a particular environment. 

For more details about the group configuration, refer to the 
`groups <http://YUIlibrary.com/YUI/docs/api/classes/config.html#property_groups>`_
property of the `YUI config Class <http://yuilibrary.com/YUI/docs/api/classes/config.html>`_.

. _app_grp-using:

Configuration of  the App Group
-------------------------------

In the ``application.json`` file, you can use the ``yui.config.groups`` object
to configure the following properties for the combo handler.

+--------------------+------------+---------------------------------------------------+----------------------------+
| Property           | Data Type  | Example                                           | Description                |
+====================+============+===================================================+============================+
| ``combine``        | boolean    | ``combine: true``                                 | Determines whether this    |
|                    |            |                                                   | group has a combo service. |
+--------------------+------------+---------------------------------------------------+----------------------------+
| ``comboSep``       | string     | ``comboSep: ';'``                                 | The separator for this     |
|                    |            |                                                   | group's combo handler.     |   
+--------------------+------------+---------------------------------------------------+----------------------------+
| ``maxURLLength``   | number     | ``maxURLLength: 500``                             | The maximum length of the  |
|                    |            |                                                   | URL for this server.       |
+--------------------+------------+---------------------------------------------------+----------------------------+
| ``base``           | string     | ``base: 'http://yui.yahooapis.com/3.8.0/build/'`` | The base path/URL for      |
|                    |            |                                                   | non-combo paths.           |
+--------------------+------------+---------------------------------------------------+----------------------------+
| ``comboBase``      | string     | ``comboBase: 'http://mycompany.com/cdn/'``        | The path/URL to the combo  |
|                    |            |                                                   | service.                   |              
+--------------------+------------+---------------------------------------------------+----------------------------+
| ``root``           | string     | ``root: '0.1.0/mybuild/'``                        | A prefix to the path       |
|                    |            |                                                   | attribute when building    |
|                    |            |                                                   | combo URLs.                |
+--------------------+------------+---------------------------------------------------+----------------------------+


In the example ``application.json``, the ``app`` group is configured
so that YUI modules are loaded from a CDN.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "groups": {
             "app": {
               "combine": false,
               "maxURLLength": 516,
               "base": "http://companycdn.com/path/to/files"
             }
           }
         }
       }
     }
   ]

.. _app_grp-default_combo:

Default Combo Handler of Mojito
-------------------------------

Mojito comes with an extended version of the 
``mojito-handler-static`` middleware that implements a fully functional
combo handler that supports cache, fallbacks when proxies cut the URL, and more. This 
combo handler adheres to the recommendations in the blog post 
`Managing your JavaScript Modules with YUI 3 Stockpile <http://www.YUIblog.com/blog/2012/11/06/managing-your-javascript-modules-with-YUI-3-stockpile-2/>`_
by `John Lindal <http://jjlindal.net/jafl/>`_, and it is the default configuration used 
for the Mojito application if the ``app`` group is not configured. 
 
The following are the default configurations for the ``app`` group:

- ``comboBase: "/combo~"``
- ``comboSep: "~"``
- ``root: ""``
- ``maxURLLength: 1024``

.. _app_grp-inherit_default:

Inheritance of Default Group Configurations
-------------------------------------------

You can inherit the default configurations of the ``app`` group by setting
the ``yui.config.combine`` property to ``true``. 

.. code-block:: javascript

   [
     {
       "settings": [ "environment:development" ],
       "yui": {
         "config": {
           "combine": true
         }
       }
     }
   ]



You can also use the ``combo`` property to disable the combo handler. In the 
example ``application.json`` below, the combo handler is disabled in
the ``environment:development`` context:

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "config": {
           "combine": true
         }
       }
     },
     {
       "settings": [ "environment:development" ],
       "yui": {
         "config": {
           "combine": false
         }
       }
     }
   ]

By disabling the combo handler, the YUI Core modules will not be using the combo handler, 
and the ``app`` group will also inherit that configuration.

.. _app_grp-shaker:

Shaker Integration
------------------

The ``mojito-shaker`` 3.x extension will be able to control the configurations defined
by the ``app`` group if you decide to push your assets into a CDN like Amazon. Shaker will 
also version the files and create the necessary rollups to accelerate caching and booting 
in the client runtime. To learn how to use the ``mojito-shaker`` extension, 
see the `Shaker documentation <https://developer.yahoo.com/cocktails/shaker/>`_.

