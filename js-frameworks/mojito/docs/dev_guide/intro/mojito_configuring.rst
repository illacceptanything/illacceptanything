==================
Configuring Mojito
==================

.. _mojito_configuring-basic:

Basic Information
=================

Mojito can be configured at the framework, application, and mojit levels. 
Each level is configured differently, but uses same general file format 
consisting of JSON or YAML. If configuration files exist in both JSON and YAML,
Mojito will use the YAML configuration file.

.. _config_basic-file:

File Format
-----------

.. _config_basic_file-json:

JSON
####

By default, configuration files in Mojito have a general top-level 
structure and are in JSON format. At the top level of each configuration 
file is an array. Each item of the array is an object containing configuration that 
targets a specific context and runtime. This allows you to have discrete configurations 
for different regions, devices, and development environments.

The context for a configuration object is specified by the ``settings`` property. For
each configuration object in a file, the ``settings`` property must specify a unique 
context, otherwise Mojito fails. For example, the ``application.json``  file
cannot have two configuration objects specifying the context ``environment:development``. 
See `Using Context Configurations <../topics/mojito_using_contexts.html>`_
for more information about contexts.

Below is the skeleton of a configuration file with two configuration
objects, each identified by contexts defined by the ``settings`` property. 
See `Application Configuration`_ and `Mojit Configuration`_ for details about specific 
configuration files.

**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         ...
       }
     },
     {
       "settings": [ "environment:development" ],
       "specs": {
         ...
       }
     },
     ...
   ]

.. _config_basic_file-yaml:

YAML
####

Mojito also supports configuration files in YAML format (JSON is a subset of YAML). 
The YAML file extension could be ``.yaml`` or ``.yml``. Mojito allows comments in the 
YAML files. When both  the JSON file (e.g., ``application.json``) and the YAML file 
(e.g., ``application.yaml``) are present, the YAML file is used and a warning is issued. 
For the data types of the YAML elements, please see the JSON configuration tables in 
:ref:`Application Configuration <configure_mj-app>`, 
:ref:`Routing <configure_mj-routing>`, and :ref:`Mojit Configuration <configure_mj-mojit>`.

**application.yaml**

.. code-block:: yaml

   ---
     # Example application configuration in YAML, which allows comments.
     -
       # The master context for default configurations.
       settings:
         - "master"

       # You can create mojit instances in the 'specs' object.
       specs:
     -
       # The context 'environment:development' that you can use for development.
       settings:
         - "environment:development"
       specs:

To convert JSON to YAML, we recommend using a command-line utility such as the npm module 
`json2yaml <https://npmjs.org/package/json2yaml>`_.

.. _configure_mj-app:

Application Configuration
=========================

Both the server and client runtimes of an application can be configured. The
application is configured in the ``application.json`` file in the application 
directory. The file consists of an array of zero or more ``configuration`` 
objects. Using the ``configuration`` object, you can configure the following 
for your application:

- location of routing files
- path to static assets
- YUI 3
- URL path for client and server communication
- declare mojit instances
- logging
- static resources

The tables below describe the ``configuration`` object and its properties. 
Those properties that have object values have tables below describing their 
properties as well except the ``config`` object, which is user defined.
To learn how to use select configurations based on the runtime
environment, see `Using Context Configurations <../topics/mojito_using_contexts.html>`_.

.. _app-configuration_obj:

configuration Object
--------------------

+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| Property                                               | Data Type            | Default Value     | Description                                            |
+========================================================+======================+===================+========================================================+
| ``actionTimeout``                                      | number               | 60000             | The number of milliseconds that an action can          |
|                                                        |                      |                   | run without calling ``ac.done`` or ``ac.error`` before |
|                                                        |                      |                   | Mojito logs a warning and invokes ``ac.error`` with a  |
|                                                        |                      |                   | Timeout error.                                         |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| `builds <#builds-obj>`_                                | object               | N/A               | Specifies configuration for builds.                    |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``mojitDirs``                                          | array of strings     | []                | The list of directories specifying where to find a     |
|                                                        |                      |                   | single mojit type. The mojits specified by             |
|                                                        |                      |                   | ``mojitDirs`` are loaded after the mojits in           |
|                                                        |                      |                   | ``mojitsDirs``. If a directory doesn't start with      |
|                                                        |                      |                   | a "/", it is taken as relative to the application      |
|                                                        |                      |                   | directory.                                             |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``mojitsDirs``                                         | array of strings     | ['mojits']        | The list of directories specifying where to find       |
|                                                        |                      |                   | mojit types. If a directory doesn't start with a       |
|                                                        |                      |                   | "/", it is taken as relative to the application        |
|                                                        |                      |                   | directory.                                             |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``routesFiles``                                        | array of strings     | ['routes.json']   | The list of files specifying where to find routing     |
|                                                        |                      |                   | information. If a file doesn't start with a "/",       |
|                                                        |                      |                   | it is taken as relative to the application             |
|                                                        |                      |                   | directory. Note, the ``routes.json`` is deprecated.    |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``selector``                                           | string               | N/A               | The version of the resource. A resource is either a    |
|                                                        |                      |                   | file to Mojito or metadata to the `Resource Store <../ |
|                                                        |                      |                   | topics/mojito_resource_store.html>`_. For example,     |
|                                                        |                      |                   | ``"selector": "iphone"`` would configure the Resource  |
|                                                        |                      |                   | Store to find resources with the identifier ``iphone`` |
|                                                        |                      |                   | such as ``index.iphone.hb.html``.                      |
|                                                        |                      |                   | See the `selector Property <../topics/mojito_resource  |
|                                                        |                      |                   | _store.html#selector-property>`_ and `Selectors <../   |
|                                                        |                      |                   | topics/mojito_resource_store.html#selectors>`_ for     |
|                                                        |                      |                   | for more information.                                  |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``settings``                                           | array of strings     | ["master"]        | Defines the context of the configuration. The          |
|                                                        |                      |                   | context consists of a key-value pair that can          |
|                                                        |                      |                   | specify the environment and environment                |
|                                                        |                      |                   | configurations. These key-value pair corresponds       |
|                                                        |                      |                   | to the configuration objects that are elements of      |
|                                                        |                      |                   | the ``dimensions`` array in the ``dimensions.json``    |
|                                                        |                      |                   | file. For example, the following contexts could be     |
|                                                        |                      |                   | used to specify the testing environment and the        |
|                                                        |                      |                   | English language : ``"environment:testing"``,          |
|                                                        |                      |                   | ``"lang:en"``. See `Using Context Configurations       |
|                                                        |                      |                   | <../topics/mojito_using_contexts.html>`_.              |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| `specs <#specs-obj>`_                                  | object               | N/A               | Specifies the mojit instances. See the                 |
|                                                        |                      |                   | :ref:`specs_obj` for details.                          |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| `staticHandling <#statichandling-obj>`_                | object               | N/A               | Gives details on the handling of static resources.     |
|                                                        |                      |                   | See the :ref:`staticHandling_obj`                      |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``tunnelPrefix``                                       | string               | "/tunnel/"        | The URL prefix for the communication tunnel            |
|                                                        |                      |                   | from the client back to the server.                    |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| ``tunnelTimeout``                                      | number               | 10000             | The timeout in milliseconds for the communication      |
|                                                        |                      |                   | tunnel from the client back to the server.             |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| :ref:`viewEngine <viewEngine_obj>`                     | object               | N/A               | Contains information about caching and preloading      |
|                                                        |                      |                   | templates.                                             |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+
| `yui <#yui-obj>`_                                      | object               | N/A               | When Mojito is deployed to client, the                 |
|                                                        |                      |                   | :ref:`yui_obj` specifies where and how to obtain       |
|                                                        |                      |                   | YUI 3. The ``yui.config`` object also contains         |
|                                                        |                      |                   | logging configurations.                                |
+--------------------------------------------------------+----------------------+-------------------+--------------------------------------------------------+

.. note:: Some of the values for the properties above can be dynamically changed by code 
          or a new context (runtime environment) may use a configuration
          object that has different ``settings``, and thus, a different set of 
          configurations. Other configurations are considered static, meaning that they
          cannot be changed once an application is started in a base context (environment).
          See `Static Configurations <../topics/mojito_using_contexts.html#static-configurations>`_
          for more information and a list of the static configurations.


.. note:: Setting Default Port

          The property ``appPort` has been deprecated and is no longer available after Mojito v0.8.  In Mojito 
          v0.9 and later, you set the port in ``app.js`` with the following
          code: ``app.set('port', process.env.PORT || 8666);``
           
          The variable ``process.env.PORT`` can be with the CLI: ``$ export PORT=800`` 

.. _builds_obj:

builds Object
#############

+---------------------------------+---------------+--------------------------------------------------------------------------------+
| Property                        | Data Type     | Description                                                                    |
+=================================+===============+================================================================================+
| `html5app <#html5app-obj>`_     | object        | Specifies configuration for HTML5 applications                                 |
|                                 |               | created with ``$ mojito build html5app``.                                      | 
+---------------------------------+---------------+--------------------------------------------------------------------------------+


.. _html5app_obj:

html5app Object
***************

+------------------------+---------------+-----------+---------------+-------------------------------------------+
| Property               | Data Type     | Required? | Default Value | Description                               |
+========================+===============+===========+===============+===========================================+
| ``attachManifest``     | boolean       | no        | ``false``     | When ``true``, the ``manifest``           |
|                        |               |           |               | attribute is added to ``<html>``.         |
+------------------------+---------------+-----------+---------------+-------------------------------------------+
| ``buildDir``           | string        | no        | none          | The path to the built HTML5 application.  |
|                        |               |           |               | If not specified, the HTML5 application   |
|                        |               |           |               | will be placed in                         |
|                        |               |           |               | ``artifacts/build/html5app``. The         |
|                        |               |           |               | specified path for ``buildDir`` will be   |
|                        |               |           |               | overridden if a build path is given to    |
|                        |               |           |               | the following command:                    |
|                        |               |           |               | ``mojito build html5app [<build_path>]``  |
+------------------------+---------------+-----------+---------------+-------------------------------------------+
| ``forceRelativePaths`` | boolean       | no        | ``false``     | When ``true``, the server-relative paths  |
|                        |               |           |               | (those starting with "/") are converted   |
|                        |               |           |               | into paths relative to the generated      |
|                        |               |           |               | file.                                     |
+------------------------+---------------+-----------+---------------+-------------------------------------------+
| ``urls``               | array of      | yes       | none          | Lists the routing paths to views that     | 
|                        | strings       |           |               | be rendered into static pages and then    |
|                        |               |           |               | cached so that the page can be viewed     |
|                        |               |           |               | offline. For example, if the running      |
|                        |               |           |               | application renders the view              |
|                        |               |           |               | ``view.html``, you could configure the    |
|                        |               |           |               | application to statically create and      | 
|                        |               |           |               | cache ``view.html`` in                    |
|                        |               |           |               | ``{app_dir}/artifacts/builds/html5app``   |
|                        |               |           |               | using the following:                      |
|                        |               |           |               | ``urls: [ '/view.html']``                 |
+------------------------+---------------+-----------+---------------+-------------------------------------------+



.. _specs_obj:

specs Object
############

The ``specs`` object can contain one or more mojit instances that are named by 
the developer. Each mojit instance is represented by an object and has
a type that specifies a mojit that was created with ``mojito create mojit <mojit_name>``
or a built-in `frame mojit <../topics/mojito_frame_mojits.html>`_.
The table below contains the properties that a mojit instance object can contain.

.. _mojit_instance_obj:

Mojit Instance Object
*********************

+------------------------------+---------------+-------------------------------------------------------------------------+
| Property                     | Data Type     | Description                                                             |
+==============================+===============+=========================================================================+
| ``action``                   | string        | Specifies a default action to use if the mojit instance wasn't          |
|                              |               | dispatched with one. If not given and the mojit wasn't dispatched       |
|                              |               | with an explicit action, the action defaults to ``index``.              |
+------------------------------+---------------+-------------------------------------------------------------------------+
| ``base``                     | string        | Specifies another mojit instance to use as a "base". Any changes        |
|                              |               | in this instance will override those in the base. Only mojit            |
|                              |               | instances with an ID can be used as a base, and only mojit              |
|                              |               | instances specified at the top-level of the ``specs`` object in         |
|                              |               | ``application.json`` have an ID. The ID is the instance's name in       |
|                              |               | the ``specs`` object. Either the ``type`` or ``base`` property is       |
|                              |               | required in the ``specs`` object.                                       |
+------------------------------+---------------+-------------------------------------------------------------------------+
| `config <#config-obj>`_      | object        | This is user-defined information that allows you to configure the       |
|                              |               | controller. Mojito does not interpret any part of this object. You can  |
|                              |               | access your defined ``config`` in the controller using the `Config      |
|                              |               | addon <../../api/classes/Config.common.html>`_. For example:            |
|                              |               | ``ac.config.get('message')``                                            |
+------------------------------+---------------+-------------------------------------------------------------------------+
| ``defer``                    | boolean       | If ``true`` and the mojit instance is a child of the                    |
|                              |               | ``HTMLFrameMojit``, an empty node will initially be rendered and        |
|                              |               | then content will be lazily loaded. See                                 |
|                              |               | `LazyLoadMojit <../topics/mojito_frame_mojits.html#lazyloadmojit>`_     |
|                              |               | for more information.                                                   |
+------------------------------+---------------+-------------------------------------------------------------------------+
| ``propagateFailure``         | boolean       | If ``true``, when a child mojit calls the method ``ac.error``, the      |
|                              |               | error message is passed to the parent and the parent mojit fails.       |
|                              |               | When ``false`` (the default value), the child mojit can call            |
|                              |               | ``ac.error`` to pass an error message to the parent, but the parent     |
|                              |               | will not fail. See `Propagating Child Mojit Errors to Parent Mojit <../ |
|                              |               | topics/mojito_composite_mojits.html#mojito_composite-child_errors>`_    |
|                              |               | for more information.                                                   |
+------------------------------+---------------+-------------------------------------------------------------------------+
| ``proxy``                    | object        | This is a normal mojit spec to proxy this mojit's execution             |
|                              |               | through. This feature only works when defined within a child            |
|                              |               | mojit. When specified, Mojito will replace this mojit child with a      |
|                              |               | mojit spec of the specified type, which is expected to handle the       |
|                              |               | child's execution itself. The proxy mojit will be executed in           |
|                              |               | place of the mojit being proxied. The original proxied child mojit      |
|                              |               | spec will be attached as a *proxied* object on the proxy mojit's        |
|                              |               | ``config`` for it to handle as necessary.                               |
+------------------------------+---------------+-------------------------------------------------------------------------+
| ``type``                     | string        | Specifies the mojit type. Either the ``type`` or ``base`` property is   |
|                              |               | required in the ``specs`` object.                                       |
+------------------------------+---------------+-------------------------------------------------------------------------+


.. _config_obj:

config Object
+++++++++++++

+--------------------------+---------------+--------------------------------------------------------------------------------+
| Property                 | Data Type     | Description                                                                    |
+==========================+===============+================================================================================+
| ``child``                | object        | Contains the ``type`` property that specifies mojit type and may also          |
|                          |               | contain a ``config`` object. This property can only be used when the mojit     |
|                          |               | instance is a child of the ``HTMLFrameMojit``. See                             |
|                          |               | `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_ for      |              
|                          |               | more information.                                                              |
+--------------------------+---------------+--------------------------------------------------------------------------------+
| ``children``             | object        | Contains one or more mojit instances that specify the mojit type with          |
|                          |               | the property ``type``. Each mojit instance may also contain a ``config``       |
|                          |               | objects.                                                                       |
+--------------------------+---------------+--------------------------------------------------------------------------------+
| ``deploy``               | boolean       | If set to ``true``, Mojito application code is deployed to the client.         |
|                          |               | See :ref:`deploy_app` for details. The default value is ``false``. Your        |
|                          |               | mojit code will only be deployed if it is a child of ``HTMLFrameMojit``.       |
+--------------------------+---------------+--------------------------------------------------------------------------------+
| ``title``                | string        | If application is using the frame mojit ``HTMLFrameMojit``,                    |
|                          |               | the value will be used for the HTML ``<title>`` element.                       |    
|                          |               | See `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_      |
|                          |               | for more information.                                                          |
+--------------------------+---------------+--------------------------------------------------------------------------------+
| ``{key}``                | any           | The ``{key}`` is user defined and can have any type of configuration value.    |
+--------------------------+---------------+--------------------------------------------------------------------------------+


.. _staticHandling_obj:

staticHandling Object
#####################

+-----------------------+---------------+-----------------------------+--------------------------------------------------------+
| Property              | Data Type     | Default Value               | Description                                            |
+=======================+===============+=============================+========================================================+
| ``appName``           | string        | {application-directory}     | Specifies the path prefix for assets that              |
|                       |               |                             | originated in the application directory, but which     |
|                       |               |                             | are not part of a mojit.                               |
+-----------------------+---------------+-----------------------------+--------------------------------------------------------+
| ``cache``             | boolean       | false                       | When ``true``, Mojito caches files in memory           |
|                       |               |                             | indefinitely until they are invalidated by a           |
|                       |               |                             | conditional GET request. When given ``maxAge``,        |
|                       |               |                             | Mojito caches file for the duration given by           |
|                       |               |                             | ``maxAge``.                                            |
+-----------------------+---------------+-----------------------------+--------------------------------------------------------+
| ``forceUpdate``       | boolean       | false                       | When ``false``, static assets are returned with the    |
|                       |               |                             | HTTP headers (``Last-Modified``, ``Cache-Control``,    |
|                       |               |                             | ``ETag``) for browser caching. Set to ``true`` to      |
|                       |               |                             | prevent these headers from being sent.                 |                     
+-----------------------+---------------+-----------------------------+--------------------------------------------------------+
| ``frameworkName``     | string        | "mojito"                    | Specifies the path prefix for assets that              |
|                       |               |                             | originated from Mojito, but which are not part of      |
|                       |               |                             | a mojit.                                               |
+-----------------------+---------------+-----------------------------+--------------------------------------------------------+
| ``maxAge``            | number        | 0                           | The time in milliseconds that the browser should       |
|                       |               |                             | cache.                                                 |
+-----------------------+---------------+-----------------------------+--------------------------------------------------------+
| ``prefix``            | string        | "static"                    | The URL prefix for all statically served assets.       |
|                       |               |                             | Specified as a simple string and wrapped in "/".       |
|                       |               |                             | For example ``"static"`` becomes the URL prefix        |
|                       |               |                             | ``/static/``.                                          |
+-----------------------+---------------+-----------------------------+--------------------------------------------------------+


.. _viewEngine_obj:

viewEngine Object
#################

+--------------------------------+----------------------+-------------------+------------------------------------------------------+
| Property                       | Data Type            | Default Value     | Description                                          |
+================================+======================+===================+======================================================+
| ``cacheTemplates``             | boolean              | true              | Specifies whether the view engine should attempt     |
|                                |                      |                   | to cache the view. Note that not all view engines    |
|                                |                      |                   | support caching.                                     |
+--------------------------------+----------------------+-------------------+------------------------------------------------------+
| ``preloadTemplates``           | boolean              | false             | Determines if templates are preloaded in memory.     |
|                                |                      |                   | This is beneficial for small applications, but not   |     
|                                |                      |                   | recommended for applications with many views and     |
|                                |                      |                   | partials because it may require a significant amount |
|                                |                      |                   | of memory that could degrade the performance.        |
+--------------------------------+----------------------+-------------------+------------------------------------------------------+


.. _yui_obj:

yui Object
##########

See `Example Application Configurations`_ for an example of the ``yui`` object. 

+--------------------------------+----------------------+------------------------------------------------------------------------+
| Property                       | Data Type            | Description                                                            |
+================================+======================+========================================================================+
| :ref:`config <yui_conf>`       | object               | Used to populate the `YUI_config <http://yuilibrary.com/yui/docs/yui/  |
|                                |                      | #yui_config>`_ global variable that allows you to configure every YUI  |
|                                |                      | instance on the page even before YUI is loaded. For example, you can   |
|                                |                      | configure logging or YUI not to load its default CSS with the          |
|                                |                      | following: ``"yui": { "config": { "fetchCSS": false } }``              |
+--------------------------------+----------------------+------------------------------------------------------------------------+
| ``showConsoleInClient``        | boolean              | Determines if the YUI debugging console will be shown on the           |
|                                |                      | client.                                                                |
+--------------------------------+----------------------+------------------------------------------------------------------------+


.. _yui_conf:

config Object
*************

The ``config`` object can be used to configure all the options for the YUI instance. 
To see all the options for the ``config`` object, see the 
`YUI config Class <http://yuilibrary.com/yui/docs/api/classes/config.html>`_. Some of the 
properties of the ``config`` object used for configuring logging are shown 
below. For more information about how to configure YUI for Mojito applications, see 
`Configuring YUI in Mojito <../topics/mojito_yui_config.html>`_.



+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| Property             | Data Type        | Default Value                                          | Description                                                           |
+======================+==================+========================================================+=======================================================================+
| ``base``             | string           | ``"http://yui.yahooapis.com/{YUI VERSION}/build/?"``   | The base URL for a dynamic combo handler. This will be used           |
|                      |                  |                                                        | to make combo-handled module requests if ``combine`` is set           |
|                      |                  |                                                        | to ``true``. You can also set the base to a local path to             |
|                      |                  |                                                        | serve YUI, such as ``/static/yui``.                                   |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``combine``          | boolean          | true                                                   | If ``true``, YUI will use a combo handler to load multiple            |    
|                      |                  |                                                        | modules in as few requests as possible. Providing a value for         |
|                      |                  |                                                        | the ``base`` property will cause combine to default to                |
|                      |                  |                                                        | ``false``.                                                            |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``comboBase``        | string           | ``"http://yui.yahooapis.com/combo?"``                  | The base URL for a dynamic combo handler. This will be used           |    
|                      |                  |                                                        | to make combo-handled module requests if combine is set to            |
|                      |                  |                                                        | ``true``.                                                             |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``comboSep``         | string           | ``"&"``                                                | The default separator to use between files in a combo URL.            |    
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``root``             | string           | ``"{YUI VERSION}/build/"``                             | Root path to prepend to module path for the combo service.            |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``debug``            | boolean          | true                                                   | Determines whether ``Y.log`` messages are written to the              |    
|                      |                  |                                                        | browser console.                                                      |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``logExclude``       | object           | none                                                   | Excludes the logging of the YUI module(s) specified.                  |
|                      |                  |                                                        | For example: ``logExclude: { "logModel": true }``                     |  
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``logInclude``       | object           | none                                                   | Includes the logging of the YUI module(s) specified.                  |
|                      |                  |                                                        | For example: ``logInclude: { "DemoBinderIndex": true }``              |  
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``logLevel``         | string           | "debug"                                                | Specifies the lowest log level to include in the                      |
|                      |                  |                                                        | log output. The log level can only be set with ``logLevel``           |
|                      |                  |                                                        | if ``debug`` is set to ``true``. For more information,                | 
|                      |                  |                                                        | see `Log Levels <../topics/mojito_logging.html#log-levels>`_.         |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``logLevelOrder``    | array of strings | ``['debug', 'mojito',                                  | Defines the order of evaluating log levels. Each log                  |
|                      |                  | 'info', 'warn', 'error'                                | level is a superset of the levels that follow, so messages            |
|                      |                  | 'none']``                                              | at levels within the set will be displayed. Thus, at the              |
|                      |                  |                                                        | ``debug`` level, messages at all levels will be displayed,            |
|                      |                  |                                                        | and at the ``mojito`` level, levels ``info``, ``warn``,               |
|                      |                  |                                                        | ``error`` will be displayed, etc.                                     |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+
| ``seed``             | array of strings | ``["yui-base", "loader-base", "loader-yui3",           | Similar to the YUI seed file as explained in the `YUI Quickstart <htt |
|                      |                  | "loader-app", "loader-app-base{langPath}"]``           | p://yuilibrary.com/yui/quick-start/>`_ you use the ``seed`` array     |
|                      |                  |                                                        | to specify the YUI components to load for your application. You can   |
|                      |                  |                                                        | also specify URLs to the YUI seed files, allowing the client to load  |
|                      |                  |                                                        | YUI. See :ref:`Seed File in Mojito Applications <seed-mojito>` for    |
|                      |                  |                                                        | more information.                                                     |
+----------------------+------------------+--------------------------------------------------------+-----------------------------------------------------------------------+




.. _config-multiple_mojits:

Configuring Applications to Have Multiple Mojits
------------------------------------------------

Applications not only can specify multiple mojit instances in ``application.json``, but 
mojits can have one or more child mojits as well.

.. _config_mult_mojits-app:

Application With Multiple Mojits
################################

Your application configuration can specify multiple mojit instances of the same or 
different types in the ``specs`` object. In the example ``application.json`` below, the 
mojit instances ``sign_in`` and ``sign_out`` are defined:

**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "sign_in": {
           "type": "SignIn"
         },
         "sign_out": {
           "type": "SignOut"
         }
       }
     }
   ]

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs:
         sign_in:
           type: "SignIn"
         sign_out:
           type: "SignOut"


   
.. _config_mult_mojits-parent_child:

Parent Mojit With Child Mojit
#############################

A mojit instance can be configured to have a child mojit using the ``child`` 
object. In the example ``application.json`` below, the mojit instance ``parent`` 
of type ``Parent`` has a child mojit of type ``Child``.


**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "parent": {
           "type": "Parent",
           "config": {
             "child": {
               "type": "Child"
             }
           }
         }
       }
     }
   ]

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs:
         parent:
           type: "Parent"
           config:
             child:
               type: "Child"


.. _config_mult_mojits-parent_children:

Parent Mojit With Children
##########################

A mojit instance can also be configured to have more than one child mojits using 
the ``children`` object that contains mojit instances. To execute the children, 
the parent mojit would use the ``Composite addon``. 
See `Composite Mojits <../topics/mojito_composite_mojits.html#composite-mojits>`_
for more information.

In the example ``application.json`` below, the mojit instance ``father`` of type 
``Parent`` has the children ``son`` and ``daughter`` of type ``Child``.


**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "father": {
           "type": "Parent",
           "config": {
             "children": {
               "son": {
                 "type": "Child"
               },
               "daughter": {
                 "type": "Child"
               }
             }
           }
         }
       }
     }
   ]

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs:
         father:
           type: "Parent"
           config:
             children:
               son:
                 type: "Child"
               daughter:
                 type: "Child"



.. _config_mult_mojits-child_children:

Child Mojit With Children
#########################

A parent mojit can have a child that has its own children. The parent mojit 
specifies a child with the ``child`` object, which in turn lists children in the 
``children`` object. For the child to execute its children,it would use the ``Composite`` 
addon. See `Composite Mojits <../topics/mojito_composite_mojits.html#composite-mojits>`_ 
for more information.

The example ``application.json`` below creates the parent mojit ``first_level`` with a 
``child`` that has the children ``third_level_a`` and ``third_level_b``.

**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "first_level": {
           "type": "FirstLevel",
           "config": {
             "child": {
                 "type": "SecondLevel",
                 "children": {
                   "third_level_a": {
                     "type": "ThirdLevel"
                   },
                   "third_level_b": {
                     "type": "ThirdLevel"
                   }
                 }
               }
             }
           }
         }
       }
     }
   ]

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs:
         first_level:
           type: "FirstLevel"
           config:
             child:
                 type: "SecondLevel"
                 children:
                   third_level_a:
                     type: "ThirdLevel"
                   third_level_b:
                     type: "ThirdLevel"


The child mojits can also have their own children, but beware that 
having so many child mojits may cause memory issues. In our updated 
example ``application.json`` below, the ``third_level_b`` mojit now has
its own children:

**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "first_level": {
           "type": "FirstLevel",
           "config": {
             "child": {
               "type": "SecondLevel",
               "children": {
                 "third_level_a": {
                   "type": "ThirdLevel"
                 },
                 "third_level_b": {
                   "type": "ThirdLevel",
                   "children": {
                     "fourth_level_a": {
                        "type": "FourthLevel"
                     },
                     "fourth_level_b": {
                        "type": "FourthLevel"
                     }
                   }
                 }
               }
             }
           }
         }
       }
     }
   ]

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs:
         first_level:
           type: "FirstLevel"
           config:
             child:
               type: "SecondLevel"
               children:
                 third_level_a:
                   type: "ThirdLevel"
                 third_level_b:
                   type: "ThirdLevel"
                   children:
                     fourth_level_a:
                       type: "FourthLevel"
                     fourth_level_b:
                       type: "FourthLevel"


.. _deploy_app:

Configuring Applications to Be Deployed to Client
-------------------------------------------------

To configure Mojito to deploy code to the client, you must be using the ``HTMLFrameMojit`` 
as the parent mojit and also set the ``deploy`` property of the :ref:`app-configuration_obj` 
object to ``true`` in the ``config`` object of your mojit instance.

.. _deploy_app-what:

What Gets Deployed?
###################

The following is deployed to the client:

- Mojito framework
- binders (and their dependencies)

When a binder invokes its controller, if the controller has the ``client`` or ``common`` 
affinity, then the controller and its dependencies are deployed to the client as well. If 
the affinity of the controller is ``server``, the invocation occurs on the server. In 
either case, the binder can invoke the controller.

.. _deploy_app-ex:

Example
#######

The example ``application.json`` below uses the ``deploy`` property to configure the 
application to be deployed to the client.

**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame": {
           "type": "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "child": {
               "type": "Pager"
             }
           }
         }
       }
     }
   ]
   
**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs:
         frame:
           type: "HTMLFrameMojit"
           config:
             deploy: true
             child:
               type: "Pager"


.. _app_config-ex:

Example Application Configurations
----------------------------------

This example ``application.json`` defines the two mojit instances ``foo`` and ``bar``. 
The ``foo`` mojit instance is of type ``MessageViewer``, and the ``bar`` mojit instance 
uses ``foo`` as the base mojit. Both have metadata configured in the ``config`` object.

**application.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "yui": {
         "showConsoleInClient": false,
         "config": {
            "fetchCSS": false,
            "combine": true,
            "comboBase": "http://mydomain.com/combo?",
            "root": "yui3/"
          }
       },
       "specs": {
         "foo": {
           "type": "MessageViewer",
           "config": {
             "message": "hi"
           }
         },
         "bar": {
           "base": "foo",
           "config": {
             "message": "hello"
           }
         }
       }
     }
   ]

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       yui:
         showConsoleInClient: false
         config:
           fetchCSS: false
           combine: true
           comboBase: "http://mydomain.com/combo?"
           root: "yui3/"
       specs:
         foo:
           type: "MessageViewer"
           config:
             message: "hi"
         bar:
           base: "foo"
           config:
             message: "hello"


.. _configure_mj-mojit:

Mojit Configuration
===================

Although mojit instances are defined at the application level, you configure metadata and 
defaults for the mojit at the mojit level. The following sections will cover configuration 
at the mojit level as well as examine the configuration of the mojit instance.


.. _configure_mojit-metadata:

Configuring Metadata
--------------------

The ``definition.json`` file in the mojit directory is used to specify metadata about the 
mojit type. The contents of the file override the mojit type metadata that Mojito 
generates from the contents of the file system.

The information is available from the controller using the 
`Config addon <../../api/classes/Config.common.html>`_. For example, you would use 
``ac.config.getDefinition('version')`` to get the version information.

The table below describes the ``configuration`` object in ``definition.json``.

+------------------+----------------------+-------------------+--------------------------------------------------------+
| Property         | Data Type            | Default Value     | Description                                            |
+==================+======================+===================+========================================================+
| ``appLevel``     | boolean              | false             | When set to ``true``, the actions, addons, assets,     |
|                  |                      |                   | binders, models, and view of the mojit are             |
|                  |                      |                   | *shared* with other mojits. Mojits wanting to use      |
|                  |                      |                   | the resources of the shared mojit must                 |
|                  |                      |                   | include the YUI module of the application-level        |
|                  |                      |                   | mojit in the ``requires`` array.                       |
+------------------+----------------------+-------------------+--------------------------------------------------------+
| ``setting``      | array of strings     | "master"          | The default value is "master", which maps to the       |
|                  |                      |                   | default configurations for an application. You can     |
|                  |                      |                   | also provide a context to map to configurations.       |
|                  |                      |                   | See `Using Context Configurations                      |
|                  |                      |                   | <../topics/mojito_using_contexts.html>`_ for more      |
|                  |                      |                   | information.                                           |
+------------------+----------------------+-------------------+--------------------------------------------------------+


.. _configure_mojit-app_level:

Configuring and Using an Application-Level Mojit
------------------------------------------------

The ``definition.json`` file lets you configure a mojit to be available at the application 
level, so that other mojits can use its actions, addons, assets, binders, models, and 
views. Mojits available at the application level are not intended to be run alone, and 
some of its resources, such as the controller and configuration, are not available to 
other mojits.

To configure a mojit to be available at the application level, you set the ``appLevel`` 
property in ``definition.json`` to ``true`` as seen below:

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "appLevel": true
     }
   ]

To use an application-level mojit, other mojits must include the YUI module name in the 
``requires`` array of the controller. For example, to use the ``foo-model`` module of 
the application-level ``Foo`` mojit, the controller of the ``Bar`` mojit would include 
``'foo-model'`` in the ``requires`` array as seen below:

.. code-block:: javascript

   YUI.add('bar', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(actionContext) {
         actionContext.done({title: "Body"});
       }
     };
   }, '0.0.1', {requires: ['foo-model']});


.. _configure_mojit-defaults:

Configuring Defaults for Mojit Instances
----------------------------------------

The ``defaults.json`` file in the mojit type directory can be used to specify 
defaults for each mojit instance of the type. The format is the same as the mojit 
instance as specified in the ``specs`` object of ``application.json``. This means 
that you can specify a default action, as well as any defaults you might want to 
put in the ``config`` object.

.. _configure_mojit-instances:

Mojit Instances
---------------

A mojit instance is defined with configuration. This configuration specifies 
which mojit type to use and configures an instance of that type. The mojit 
instances are defined in the ``specs`` object of the ``application.json`` file.

See :ref:`configure_mj-app` and :ref:`app_config-ex` for details of the ``specs`` 
object.

.. _configure_mojit_instances-using:

Using Mojit Instances
#####################

When a mojit instance is defined in ``application.json``, you can
map an action of that mojit instance to a routing path in ``app.js``. 
Actions are references to functions in the mojit controllers. When a client 
makes an HTTP request on a defined routing path, the function in the mojit 
controller that is referenced by the action from the mojit instance is called.

For example, the ``application.json`` below defines the ``foo`` mojit instance 
of the mojit type ``Foo``.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "foo": {
           "type": "Foo",
           "config": {
             "message": "hi"
           }
         }
       }
     }
   ]

The ``routes.json`` below uses the ``foo`` instance to call the ``index`` action 
when an HTTP GET request is made on the root path. The ``index`` action references 
the ``index`` function in the controller of the ``Foo`` mojit.

.. code-block:: javascript

   use strict';

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.use(libmojito.middleware());

   app.get('/', libmojito.dispatch("foo.index.")); 

   app.listen(app.get('port'), function () {
       debug('Server listening on port ' + app.get('port') + ' ' +
             'in ' + app.get('env') + ' mode');
   });

.. _configure_mj-routing_deprecated:

Routing Configuration: routes.json (Deprecated)
===============================================

As of Mojito v0.9, you define routing paths in ``app.js``, just as you would
do for Express applications. You can, however, until further notice, still
use routing configured in ``routes.json`` by following the instructions
in `Using Routing Defined in routes.json <mojito_routing.html#routing-routesjson>`_.

We strongly suggest that you use ``app.js`` to define your routing paths and update
the routing paths from older applications to use ``app.js`` as future versions of Mojito 
may not allow you to use the routing path defined in``routes.json``. 
See `Routing <mojito_routing.html>`_ for implementation details.


.. note:: Regular Expressions for Matching Routing Paths

          Using regular expressions for matching routing paths, however, is no longer supported
          in Mojito v0.9 and later. You will need to 


.. _configure_routing-file:

Routing Configuration File
--------------------------

The ``routes.json`` file contains the routing configuration information in JSON. The JSON 
consists of an array of one or more ``configuration`` objects that include ``route`` 
objects specifying route paths, parameters, HTTP methods, and actions.

The table below describes the properties of the ``route`` object of  ``routes.json``.

+----------------+----------------------+---------------+--------------------------------------------------------+
| Property       | Data Type            | Required?     | Description                                            |
+================+======================+===============+========================================================+
| ``call``       | string               | Yes           | The mojit instance defined in ``application.json``     |
|                |                      |               | and the method that is called when an HTTP call is     |
|                |                      |               | made on the path specified by ``path``. For            |
|                |                      |               | example, to call the ``index`` method from the         |
|                |                      |               | ``hello`` mojit instance, you would use the            |
|                |                      |               | following: ``call: "hello.index"`` An anonymous        |
|                |                      |               | mojit instance can also be created by prepending       |
|                |                      |               | "@" to the mojit type. For example, the following      |
|                |                      |               | would create an anonymous mojit instance of type       |
|                |                      |               | ``Hellot`` and call the ``index`` action for           |
|                |                      |               | the ``Hello`` mojit: ``call:                           |
|                |                      |               | "@Hello.index"``                                       |
+----------------+----------------------+---------------+--------------------------------------------------------+
| ``params``     | string               | No            | Query string parameters that developers can            |
|                |                      |               | associate with a route path. The default value is an   | 
|                |                      |               | empty string "". The query string parameters should    |
|                |                      |               | be given an object:                                    |
|                |                      |               | ``params: { "name": "Tom", "age": "23" }``             |
|                |                      |               |                                                        |
|                |                      |               | **Deprecated**:  ``params: "name=Tom&age=23"``         |
+----------------+----------------------+---------------+--------------------------------------------------------+
| ``path``       | string               | Yes           | The route path that is mapped to the action in the     |
|                |                      |               | ``call`` property. The route path can have variable    |
|                |                      |               | placeholders for the mojit instance and action         |
|                |                      |               | that are substituted by the mojit instance and         |
|                |                      |               | actions used in the ``call`` property.  See also       |
|                |                      |               | :ref:`parameterized_paths`.                            |
+----------------+----------------------+---------------+--------------------------------------------------------+
| ``regex``      | object               | No            | An object containing a key-value pair, where the key   |
|                |                      |               | is a path parameter and the value contains the regular |
|                |                      |               | expression. For example:                               |
|                |                      |               | ``"regex": { "path_param":  "?:(.*).html" }``          |
|                |                      |               | See :ref:`Using Regular Expressions to Match Routing   |
|                |                      |               | Paths <regex_paths>` for more information.             |
+----------------+----------------------+---------------+--------------------------------------------------------+
| ``verbs``      | array of strings     | No            | The HTTP methods allowed on the route path defined     |
|                |                      |               | by ``path``. For example, to allow HTTP GET and        |
|                |                      |               | POST calls to be made on the specified path, you       |
|                |                      |               | would use the following: ``"verbs": [ "get",           |
|                |                      |               | "post" ]``                                             |
+----------------+----------------------+---------------+--------------------------------------------------------+


.. _configure_routing-mapping:

Map Routes to Specific Mojit Instances and Actions
--------------------------------------------------

This type of route configuration is the most sophisticated and recommended for 
production applications. To map routes to a mojit instance and action, you create 
the file ``routes.json`` in your application directory. The ``routes.json`` file 
allows you to configure a single or multiple routes and specify the HTTP method 
and action to use for each route.


.. _routing_mapping-single:

Single Route
############

To create a route, you need to create a mojit instance that can be mapped to a 
path. In the ``application.json`` below, the ``hello`` instance of type 
``Hello`` is defined.

**application.json**

.. code-block:: javascript

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

**application.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       specs: 
         hello: 
           type: "Hello"


The ``hello`` instance and a function in the ``Hello`` controller can now 
be mapped to a route path in ``routes.json`` file. In the ``routes.json`` below, 
the ``index`` function is called when an HTTP GET call is made on the root path.

**routes.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "hello index": {
         "verbs": ["get"],
         "path": "/",
         "call": "hello.index"
       }
     }
   ]

**routes.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       hello index:
         verbs:
           - "get"
         path: "/"
         call: "hello.index"


Instead of using the ``hello`` mojit instance defined in the ``application.json`` 
shown above, you can create an anonymous instance of ``Hello`` for mapping 
an action to a route path. In the ``routes.json`` below,  an anonymous instance 
of ``Hello`` is made by prepending "@" to the mojit type.

**routes.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "hello index": {
         "verbs": ["get"],
         "path": "/",
         "call": "@Hello.index",
         "params": { "first_visit": true }
       }
     }
   ]

**routes.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       hello index:
         verbs:
           - "get"
         path: "/"
         call: "@Hello.index"
         params:
           first_visit: true

.. _routing_mapping-multiple:

Multiple Routes
###############

To specify multiple routes, you create multiple route objects that contain 
``verb``, ``path``, and ``call`` properties in ``routes.json`` as seen here:

**routes.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "root": {
         "verb": ["get"],
         "path": "/*",
         "call": "foo-1.index"
       },
       "foo_default": {
         "verb": ["get"],
         "path": "/foo",
         "call": "foo-1.index"
       },
       "bar_default": {
         "verb": ["get"],
         "path": "/bar",
         "call": "bar-1.index",
         "params": { "page": 1, "log_request": true }
       }
     }
   ]

**routes.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       root:
         verb:
           - "get"
         path: "/*"
         call: "foo-1.index"
       foo_default:
         verb:
           - "get"
         path: "/foo"
         call: "foo-1.index"
       bar_default:
         verb:
           - "get"
         path: "/bar"
         call: "bar-1.index"
         params:
           page: 1
           log_request: true


The ``routes.json`` and ``routes.yaml`` files above create the following routes:

- ``http://localhost:8666``
- ``http://localhost:8666/foo``
- ``http://localhost:8666/bar``
- ``http://localhost:8666/anything``

Notice that the ``routes.json`` above uses the two mojit instances ``foo-1`` and 
``bar-1``; these instances must be defined in the ``application.json`` file before 
they can be mapped to a route path. Also, the wildcard used in ``root`` object 
configures Mojito to call ``foo-1.index`` when HTTP GET calls are made on any 
undefined path.


.. _routing_params:

Adding Routing Parameters
-------------------------

You can configure a routing path to have routing parameters with the ``params`` 
property. Routing parameters are accessible from the ``ActionContext`` object 
using the `Params addon <../../api/classes/Params.common.html>`_.

In the example ``routes.json`` below, routing parameters are added with an object. 
To get the value for the routing parameter ``page`` from a controller, you would 
use ``ac.params.getFromRoute("page")``. 

**routes.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "root": {
         "verb": ["get"],
         "path": "/*",
         "call": "foo-1.index",
         "params": { "page": 1, "log_request": true }
       }
     }
   ]

**routes.yaml**

.. code-block:: yaml

   ---
     -
       settings:
         - "master"
       root:
         verb:
           - "get"
         path: "/*"
         call: "foo-1.index"
         params:
           page: 1
           log_request: true
   

.. admonition:: Deprecated

   Specifying routing parameters as a query string, such as 
   ``"params": "page=1&log_request=true"``, 
   is still supported, but may not be in the future.

.. _parameterized_paths:

Using Parameterized Paths to Call a Mojit Action
------------------------------------------------

Your routing configuration can also use parameterized paths to call mojit 
actions. In the ``routes.json`` below, the ``path`` property uses parameters 
to capture a part of the matched URL and then uses that captured part to 
replace ``{{mojit-action}}`` in the value for the ``call`` property. Any 
value can be used for the parameter as long as it is prepended with a 
colon (e.g., ``:foo``). After the parameter has been replaced by a value 
given in the path, the call to the action should have the following syntax: 
``{mojit_instance}.(action}`` 

**routes.json**

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "_foo_action": {
         "verb": ["get", "post", "put"],
         "path": "/foo/:mojit-action",
         "call": "@foo-1.{mojit-action}"
       },
       "_bar_action": {
         "verb": ["get", "post", "put"],
         "path": "/bar/:mojit-action",
         "call": "@bar-1.{mojit-action}"
       }
     }
   ]

**routes.yaml**

.. code-block:: yaml
  
   ---
     -
       settings:
         - "master"
       _foo_action:
         verb:
           - "get"
           - "post"
           - "put"
         path: "/foo/:mojit-action"
         call: "@foo-1.{mojit-action}"
       _bar_action:
         verb:
           - "get"
           - "post"
           - "put"
         path: "/bar/:mojit-action"
         call: "@bar-1.{mojit-action}"

For example, based on the ``routes.json`` and ``routes.yaml`` above, an HTTP GET call made on the 
path ``http://localhost:8666/foo/index`` would call the ``index`` function in 
the controller because the value of ``:mojit-action`` in the path (``index`` in 
this case) would be then replace ``{mojit-action}}`` in the ``call`` property. 
The following URLs call the ``index`` and ``myAction`` functions in the controller.

- ``http://localhost:8666/foo/index``
- ``http://localhost:8666/foo/myAction``
- ``http://localhost:8666/bar/index``





No Longer Supported in routes.json
----------------------------------

As of Mojito v0.9, you can no longer use the ``regex`` project 
in ``routes.json`` to define a key-value pair that defines a path parameter and 
a regular expression. You can use regular expressions in ``app.js`` though to
define a path parameter. See `Using Regular Expressions to Match Routing Paths <mojito_routing.html#appjs-routing-regex>`_
to learn how.



.. _mojito_configuring-access:

Accessing Configurations from Mojits
====================================

The model and binder can access mojit configurations from the ``init`` 
function. The controller and model are passed ``configuration`` objects. The controller 
can access configuration with the ``Config`` addon. 
The ``init`` function in the binder, instead of a configuration object, is passed the 
``mojitProxy`` object, which enables you to access configurations.  


.. _configuring_access-applevel:

Application-Level Configurations
--------------------------------

Only the mojit controller has access to application-level configurations 
using the ActionContext ``Config`` addon.

.. _access-applicationjson:

application.json
################

The controller functions that are passed an ``actionContext`` object can get the 
application configurations in ``application.json`` with the method ``getAppConfig``
of the ``Config`` addon.

For example, if you wanted to access the ``specs`` object defined in ``application.json``,
you would use ``ac.config.getAppConfig().specs`` as shown here:

.. code-block:: javascript

      YUI.add('mymojit', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {
          index: function(ac) {
            // Get the 'specs' object from the application configuration 
            // through the Config addon.
            var app_specs = ac.config.getAppConfig().specs;
            Y.log(app_specs);
            ac.done({ status: "Showing app config in the log."});
          }
        };
      }, '0.0.1', {requires: ['mojito', 'mojito-config-addon']});

.. _access-routesjson:

app.js
######

One of the functions of the ``app.js`` is for defining routing paths. You
can access the defined routing paths with the method ``getRoutes``
of the ``Config`` addon.


.. code-block:: javascript

      YUI.add('mymojit', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {
          index: function(ac) {
            // Get the routing configuration through
            // the Config addon.
            var route_config = ac.config.getRoutes();
            Y.log(route_config);
            ac.done({ status: "Showing routing config in the log."});
          }
        };
      }, '0.0.1', {requires: ['mojito', 'mojito-config-addon']});

.. _access_configs-context:

Application Context
-------------------

The contexts for an application specify environment variables such as the runtime 
environment, the location, device, region, etc. Once again, only the controller that is 
passed the ``actionContext`` object can access the context. You can access the context 
using ``ac.context``. 


Below is an example of the ``context`` object:

.. code-block:: javascript

   { 
     runtime: 'server',
     site: '',
     device: '',
     lang: 'en-US',
     langs: 'en-US,en',
     region: '',
     jurisdiction: '',
     bucket: '',
     flavor: '',
     tz: '' 
   }

.. _configuring_access-mojit:

Mojit-Level Configurations
--------------------------

Mojit-level configurations can be specified in two locations. You can specify mojit-level 
configurations in the ``config`` object of a mojit instance in ``application.json`` or 
default configurations for a mojit in ``mojits/{mojit_name}/defaults.json``. The 
configurations of ``application.json`` override those in ``defaults.json``.


.. _access_mojit-controller:

Controller
##########

Controllers can access mojit-level configurations from the ``actionContext`` object 
using the `Config addon <../../api/classes/Config.common.html>`_. 
Use ``ac.config.get`` to access configuration values from ``application.json`` and 
``defaults.json`` and ``ac.config.getDefinition`` to access definition values from 
``definition.json``.


.. _access_mojit-model:

Model
#####

The ``init`` function in the model is also passed the mojit-level configurations. 
If other model functions need the configurations, you need to save the 
configurations to the ``this`` reference because no ``actionContext`` object is 
passed to the model, so your model does not have access to the ``Config`` addon.


.. _access_mojit-binder:

Binder
######

As mentioned earlier, you access configurations through the ``mojitProxy`` object by 
referencing the ``config`` property: ``mojitProxy.config``


