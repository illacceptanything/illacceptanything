===========
Mojito: FAQ
===========

This page answers some of the most common questions we get about Mojito. For 
troubleshooting issues, see 
`Mojito: Troubleshooting <../reference/mojito_troubleshooting.html>`_.

Questions
=========

General
-------

* :ref:`What languages can I use to develop mojits? <moj_langs>`  
* :ref:`Can I work on Mojito applications offline? <moj_offline>` 
* :ref:`Does Mojito work in a Windows environment? <moj_windows>`
* :ref:`Do I have to restart Mojito to see updates that I've made to my application? <moj_nodemon>`
* :ref:`Is the Mojito Y instance persistent across requests? <moj_req_persistent>`
* :ref:`Does Mojito support lazy loading? <moj_lazyloading>`
* :ref:`Does Mojito have API methods for handling caching? If not, what's the best way for Mojito 
  applications to handle caching? <moj_caching>`
* :ref:`Can Mojito make runtime decisions to run client-side or server-side code? <moj_run_client_server>`
* :ref:`How can I improve the performance of my Mojito application? <moj_optimize_performance>`
* :ref:`Can I start an application so that it listens to requests made to a specific IP address or hostname? <moj_ip_address>`


Configuration
-------------

* :ref:`How do I configure Mojito to deploy my application to the client? <moj_deploy>`
* :ref:`How do I configure mojits to run on the client? <moj_client>`
* :ref:`How do I configure the Mojito server to listen to specific port? <moj_config_port>`
* :ref:`What is the difference between 'definition.json' and 'defaults.json'? <moj_definition_defaults>`   


Data
----

* :ref:`How do I get content for my mojits? <moj_content>`  
* :ref:`How is data shared among mojits? <moj_share_data>`
* :ref:`How does a parent mojit send data to the template? <moj_parent_mojit_data>`
* :ref:`In Mojito applications, how are parameters passed from the controller to binders? <moj_params_controller_binder>`
* :ref:`How can data be passed from a parent mojit to its child mojits? <moj_pass_data_parent_to_child>`

Binders
-------

* :ref:`What are binders and what do they do? <moj_binders>`  


CSS/JavaScript Assets
---------------------

* :ref:`How do I add assets templates? <moj_assets>`
* :ref:`How are assets dynamically added to views? <moj_dyn_assets>`


HTTP Headers/Responses
----------------------

* :ref:`Can Mojito applications access the 'request' or 'response' instances of the Node.js classes 
  'http.ServerRequest' and 'http.ServerResponse'? <moj_req_res_instances>`
* :ref:`Is it possible to access HTTP headers from a Mojito application? <moj_access_headers>`
* :ref:`How does a Mojito application set the expiration of a cookie in the HTTP response? <moj_cookie_exp>`


Mojits
------

* :ref:`What is a mojit? <mojit_exp>`
* :ref:`Can mojits have child mojits? <moj_children>`
* :ref:`How do mojits share objects? <moj_objects>`
* :ref:`Can mojit instances be dynamically defined and then run? <moj_dynamic_creation>`
* :ref:`Is there a way to make all of the resources, such as assets, addons, binders, models, of one mojit available to other mojits? <moj_resources>`
* :ref:`Why does Mojito replace hyphens in the names of my mojits with underscores? <moj_names_hyphens>`
* :ref:`Can I extend or re-use mojits? <moj_reuse>`   


Views
-----

* :ref:`Does Mojito support view partials? <moj_partials>`  
* :ref:`Do I have to create separate mojits for different devices? <moj_devices>` 
* :ref:`How do I refresh the view on the client? <moj_refresh_view>`
* :ref:`Can I use a different templating system other than Handlebars? <moj_different_templating>`
* :ref:`Can the mojit controller specify which template should be rendered? <moj_controller_specify_view>`

Logging/Testing
---------------

* :ref:`How do I change the logging levels for my Mojito application? <moj_log_level>`
* :ref:`Is there a way to exclude specific modules from logging on the client? <moj_mod_log_exclusion>`
* :ref:`Can logging be configured to be different for the client and server? <moj_client_server_logging>`


Addons/Libraries/Middleware
---------------------------

* :ref:`Does the Mojito framework offer API methods or addons? <moj_api_addons>`
* :ref:`Can I repurpose or extend existing YUI modules? <moj_yui>`   


                      
Answers
=======                           
                            
General
-------

.. _moj_langs:
.. topic::  **What languages can I use to develop mojits?**

    JavaScript is the programming language of Cocktails. All modules are written in 
    JavaScript, and JavaScript only. Modules that require server-side execution do so 
    through a server-side JavaScript execution engine. 
    
------------    

.. _moj_offline:
.. topic:: **Can I work on Mojito applications offline?**

    The tools used in developing mojit modules (mojits) work offline, without requiring 
    access to networked resources. When networked (cloud) resources are required due to the 
    modules own nature, e.g., retrieving data from a back-end service, Cocktails provides 
    facilities to mock these resources and simulate their behavior with dummy (local) 
    implementations.

------------

.. _moj_windows:
.. topic:: **Does Mojito work in a Windows environment?**

    Unfortunately, Mojito is only supported on Unix-based systems. We hope to support
    Mojito on Windows in the future, so please check the 
    `Mojito Releases Notes <`https://github.com/yahoo/mojito/wiki>`_ on GitHub for the
    latest updates.


.. _moj_nodemon:
.. topic:: **Do I have to restart Mojito to see updates that I've made to my application?**

    No, you can use the development environment with certain configurations or use
    an external module such as ``nodemon`` or ``supervisor`` to start Mojito, which will 
    automatically update any changes that you've made to your application.


    **I. Using the Development Environment**

    #. In your ``application.json``, add the configuration object with the 
       ``settings`` property given the array ``[ "environment:development" ]`` with the
       configurations for ``staticHandling``, ``viewEngine``, and ``yui`` shown 
       below. 

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
                      "type": "foo"
                    }
                  }
                }
              }
            },
            {
              "settings": [ "environment:development" ],
              "staticHandling": {
                "forceUpdate": true
              },
              "viewEngine": {
                "cacheTemplates": false
              },
              "yui": {
                "config": {
                  "combine": false
                }
              }
            }
          ]
   
       .. note:: Your application has to deploy code to the client, so 
                ``deploy`` must be set to ``true``.

    #. Modify your ``app.js`` to start with the context ``environment:development``:

       .. code-block:: javascript

          libmojito.extend(app, {
              context: {
                  runtime: 'server',
                  environment: 'development'
              }
          });
    #. Open your application in a browser.
    #. Modify code in your binders or templates.
    #. Refresh your browser to see the updates in your application.

    **Caveats**

    Although binder code and templates will be updated after you refresh your browser,
    resources, such as a model or controller, with a ``server`` or ``common`` affinity 
    are executed in the Node.js environment, and therefore, will not get refreshed. 
    You will need to restart your application to see changes. Also, meta data for modules,
    such as adding a module dependency to the ``requires`` array will not be reflected 
    until you restart the application.

    **II. Using External Modules**    
    
    You will need ``npm`` to install ``nodemon`` or ``supervisor``. To use ``nodemon``, 
    you will need a script to start the server. 
    
    Follow the steps below to use ``nodemon``:
    
    1. Install ``nodemon``:  ``$ sudo npm install nodemon -g``
    
    2. In your Mojito application directory, create the file ``devel.js`` with the 
       following:
    
       .. code-block:: javascript
    
          require('./server.js').listen(8666, null, function() {
            console.log('----STARTED----');
            console.log('Nodemon will not honor the app port from the settings');
            console.log('The application is running under http://localhost:8666/');
          });
    
    3. From your application directory, start Mojito with ``nodemon``:
    
       ``$ nodemon devel.js``
       
    To use `supervisor <https://github.com/isaacs/node-supervisor>`_, follow these steps:
    
    #. Globally install ``supervisor``.
    
       ``$ sudo npm install supervisor -g``
    #. From a Mojito application directory, start Mojito with ``supervisor``:
    
       ``$ supervisor -x node app.js``
    #. View your application in a browser.
    #. In a separate terminal window, make changes to the source code of the application.
    #. Refresh the page to view the modified application.
    
    .. note:: ``supervisor`` will only update files with extensions ``node`` or ``js`` by 
              default. To have ``supervisor`` watch files with other extensions, use the 
              option ``-e`` or ``--extensions``: ``$ supervisor -e js,json,html -x node app.js``

------------    
    
.. _moj_req_persistent:
.. topic:: **Is the Mojito Y instance persistent across requests?**

    Yes, each mojit will have a Y instance that is persistent (by default) for every 
    successive request. Computing YUI dependencies for every request negatively affects 
    performance. You can change this behavior through the settings or you can deal with it 
    at the mojit level, recreating the object.
    
------------ 
 
.. _moj_lazyloading:
.. topic:: **Does Mojito support lazy loading?** 

    Yes, the Mojito framework comes with the frame mojit ``LazyLoadMojit`` specifically 
    for lazy loading. The ``LazyLoadMojit`` allows you to defer the loading of a mojit 
    instance by first dispatching the ``LazyLoadMojit`` as a proxy to the client. From the 
    client, ``LazyLoadMojit`` can then request Mojito to load the proxied mojit. This 
    allows your Mojito application to load the page quickly and then lazily load parts of 
    the page. See `LazyLoadMojit <../topics/mojito_frame_mojits.html#lazyloadmojit>`_ 
    to learn more.

------------

.. _moj_caching:
.. topic:: **Does Mojito have API methods for handling caching? If not, what's the best 
           way for Mojito applications to handle caching?** 

    Mojito currently does not have any API methods for handling caching. Although there is 
    no definitive solution for caching for Mojito, here are a few possible ways to cache 
    for Mojito applications:
    
    - **Client-Side Caching**
    
       - `Create an HTML5 application <../reference/mojito_cmdline.html#html5app>`_ and 
         use the ``manifest.cache`` file.
       - Leverage the HTML5 ``localStorage`` object.
       - Use the `Storage Lite <http://yuilibrary.com/gallery/show/storage-lite>`_ YUI 
         module that selects the best available local storage API supported by the browser 
         it's running in.
       - `Create an addon <../topics/mojito_extensions.html#creating-new-addons>`_ that 
         uses a singleton. 
       
    - **Server-Side Caching (implementation depends on server)**
    
       - Use the ``fs`` Node.js module to write to the file system.
       - Use one of the Node.js modules for caching: `cradle <http://cloudhead.io/cradle>`_, 
         `nodejs_redis <https://github.com/mranney/node_redis>`_, 
         `node-optimist <https://github.com/substack/node-optimist>`_, etc.
       - Implement a tried and tested caching solution such as `memcached <http://memcached.org/>`_.

------------

.. _moj_run_client_server:
.. topic:: **Can Mojito make runtime decisions to run client-side or server-side code?**

    Yes, but it's up to the programmer to configure the application to be deployed to the 
    client. Also, your application should have client and server versions, indicated by an 
    `affinity <../reference/glossary.html#affinity>`_, of the controller, models, addons, 
    etc., that you want to run on both the client and the server. To configure Mojito to 
    deploy application code to the client, you set the ``deploy`` property of the 
    application configuration to ``true``. 
    See `Configuring Applications to Be Deployed to Client <../intro/mojito_configuring.html#configuring-applications-to-be-deployed-to-client>`_ 
    for more information.
    
    Mojito determines the client device based on the HTTP header ``User-Agent`` or the 
    value of the query string parameter ``device`` and then will render the appropriate 
    view if it exists--it's up to the developer to create the views. For example, if an 
    iPhone is making a request, Mojito will render the view ``index.iphone.hb.html`` if it 
    has been created. See `Views: Naming Conventions <../intro/mojito_mvc.html#naming-convention>`_ 
    for more information.    

------------
  
    
.. _moj_optimize_performance:
.. topic:: **How can I improve the performance of my Mojito application?**

    The following sections offer some ideas about how to improve the performance of your 
    Mojito application, but are by no means exhaustive. You should also review online 
    articles about improving Node.js performance, such as 
    `Blazing fast node.js: 10 performance tips 
    from LinkedIn Mobile <http://bit.ly/uFyio2>`_ written by software engineer Shravya 
    Garlapati.
    
    **Don't Add User Data to ac.context**
    
    The ``context`` property of the ``ActionContext`` object contains a small set of 
    key-value pairs that define the run-time environment under which a mojit runs. These 
    key-value pairs are used as a cache key. Adding your own key/values to ``ac.context`` 
    will cause the cache to bloat. 
    
    As an alternative, you can share data using the following methods:
    
       * Parent mojits can share data with the child mojits by attaching data to the 
         ``ActionContext`` object in the parent mojit's controller. For example, in the 
         parent mojit, you could add an object to ``ac.composite.command.params.body`` 
         that the children can then access with 
         ``ac.composite.command.params.body['{obj_name}']``.
       * From the server and before mojits are executed, middleware can be used to share
         information about static handling and routing.
       * Assets and data can be shared through the 
         `template <../reference/glossary.html#view-template>`_ of a parent mojit or 
         through a frame mojit such as 
         `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_ that 
         creates a parent template.
    
    **Rollup/Minify Assets** 
    
    Rolling up and minifying assets will reduce the number of network calls and improve 
    load time. For **rolling up assets**, we recommend that you use 
    `Shaker <https://github.com/yahoo/mojito-shaker>`_, which is a static asset rollup 
    manager. 
    
    Mojito also allows you to configure your app to use rollups by setting the 
    ``useRollups`` property in the ``application.json`` file to ``true`` as shown below::
   
      "staticHandling": {
        "useRollups": true
      }
    
    You can also compile rollups, inline CSS, or views using the Mojito command-line 
    utility. See the `Compile System <../reference/mojito_cmdline.html#compile-system>`_ 
    to learn how.
    
    For **minification**, we recommend Shaker again. Other choices could be `YUI Compressor 
    <http://yuilibrary.com/download/yuicompressor/>`_ or an npm module such as 
    `UglifyJS <https://github.com/mishoo/UglifyJS>`_. 
    
    
    **Use Lazy Loading**
    
    From the client, your Mojito application should lazy load assets as often as possible.
    For example, the `YUI ImageLoader Utility <http://yuilibrary.com/yui/docs/imageloader/>`_ 
    can be used to help you lazy load images. You can even lazy load a mojit from the client
    using the `LazyLoadMojit <../topics/mojito_frame_mojits.html#lazyloadmojit>`_.
   


.. _moj_ip_address:
.. topic:: **Can I start an application so that it listens to requests made to a specific IP address or hostname?**

    Although the Mojito CLI utility does not allow you to specify an IP address or hostname for your application
    to listen to when it's started, you can modify the ``server.js`` file and start your application directory with
    ``node``. 

    In the example ``server.js`` below, you pass the port and hostname to the ``listen`` method so that the application
    will listen to requests at the given hostname and port when you start the application with ``node server.js``:

    .. code-block:: javascript

       var mojito = require('mojito').createServer(),
           port = 8111,
           host = '127.0.0.1';

        mojito.listen(port, host, function(err, app) {
          console.log('listening on %s:%d', host, port);
        });
 

------------


Configuration
-------------

.. _moj_deploy:
.. topic:: **How do I configure Mojito to deploy my application to the client?**

    Binders always get deployed to the client, but to deploy your controller to the
    client, you need to use the `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_ 
    and set the ``deploy`` field to ``true`` in the ``application.json`` file. See 
    `Deploying to Client <../topics/mojito_frame_mojits.html#deploying-to-client>`_ 
    for more details.

------------ 
 
.. _moj_client:
.. topic:: **How do you configure mojits to run on the client?** 

    Run Mojito at build time to generate the HTML page using ``mojito build html5app``. 
    This runs the Mojito infrastructure as if it were a running server instance and prints 
    out the resulting HTML+JSON required to bootstrap a client-side mojit. 
    Among other things, it leads down a path where it's very hard to do 
    incremental builds because the Web server abstraction makes it hard to do the timestamp 
    resolution that incremental builds require. A better approach would be to allow people 
    to hard-code the top-level mojit bootstrap code by publishing mojit creation APIs that 
    can be called from the top level.

------------

.. _moj_config_port:
.. topic:: **How do I configure the Mojito server to listen to specific port?** 

    In ``app.js``, you define the port or use the value of the exported variable ``PORT`` with the
    following line of code:
    
        app.set('port', process.env.PORT || 8666);

    To define the variable ``process.env.PORT``, you do the following from the CLI: ``$ export PORT={port_number}``
                                                    
------------
    
.. _moj_definition_defaults:
.. topic:: **What is the difference between 'definition.json' and 'defaults.json'?**

    The ``definitions.json`` file stores the class-level mojit values and is ideal for 
    storing metadata. The ``defaults.json`` file stories default configurations for your 
    mojit instance that will be overridden if they are found in the ``application.json`` 
    file. See `Configuring Defaults for Mojit Instances <../intro/mojito_configuring.html#configuring-defaults-for-mojit-instances>`_
    and `Mojit Configuration: Configuring Metadata <../intro/mojito_configuring.html#mojit-configuration>`_ 
    for more information.



Data
----

.. _moj_content:
.. topic:: **How do I get content for my mojits?**

    YQL is the preferred method for accessing data in Mojito applications. YUI 3 also has 
    a `YQL module <http://yuilibrary.com/gallery/show/yql>`_ that makes calling the YQL 
    Web Service easy.
    
------------

.. _moj_share_data:
.. topic:: **How is data shared among mojits?**

    You can create an application-level mojit that can share data with its children. Your 
    application-level mojit would have a model to get data that can be stored data in a 
    Model object. The child mojits can then access this data through the application-level 
    mojit's model.

------------

.. _moj_parent_mojit_data:
.. topic:: **How does a parent mojit send data to the template?** 

    From the controller of the parent mojit, pass an object to ``ac.composite.done`` 
    containing key-value pairs that can be added 
    to the view template as Handlebars expressions. For example, the key ``foo``
    shown below can be included in the template as the Handlebars expression ``{{foo}}``.
    The ``{{foo}}`` expression will be replaced by the value 'bar' when the template 
    is rendered.::
    
      // Inside parent mojit
      ac.composite.done({"foo": "bar"});
    
------------    
    
.. _moj_params_controller_binder:
.. topic:: **In Mojito applications, how are parameters passed from the controller to binders?** 

    Your controllers can pass parameters to binders using the ``Data`` addon.
    The controller requires the ``Data`` addon and then uses the ``data`` object with
    the ``set`` method to set (or expose) data for the client code (binders or templates):
    ``ac.data.set('app_framework', "Mojito");`` 

    From the binder, the ``mojitProxy`` object can then access the data set by the controller
    from the ``data`` object with the ``get`` method: ``mojitProxy.data.get('app_framework');``

    The mojit's template can access the set data through a Handlebars expression, so
    the ``index.hb.html`` could use ``{{app_framework}}`` to get the data set in the controller.
    
    The ``Data`` addon also allows you to share page-level data, so that mojits on the
    page can share data through ``ac.pageData.set`` and ``ac.pageData.get`` or ``mojitProxy.pageData.get``.
    See  `Sharing Data <../topics/mojito_data.html#sharing>`_ for more information and example code.

------------

.. _moj_pass_data_parent_to_child:
.. topic:: **How can data be passed from a parent mojit to its child mojits?**

    Currently the only way to do this is to pass data to the children in either the 
    children config or parameters. If you use ``ac.composite.execute`` you can 
    create/modify the children configuration in code before calling 
    ``ac.composite.execute``. See `ac.composite.execute <../../api/classes/Composite.common.html#method_execute>`_ 
    for more information.

    If you want to pass the data to the children in the parameters, you can do that with 
    the ``ac._dispatch`` function. See 
    `ac._dispatch <../../api/classes/ActionContext.html#method__dispatch>`_ for more 
    information.

Binders
-------

.. _moj_binders:
.. topic:: **What are binders and what do they do?**

    Binders are mojit code that is only deployed to the browser. A mojit may have zero, 
    one, or many binders. The code can perform the following three functions:
    
       * allow event handlers to attach to the mojit DOM node
       * communicate with other mojits on the page
       * execute actions on the mojit that the binder is attached to
       

       
CSS/JavaScript Assets
---------------------

.. _moj_assets:
.. topic:: **How do I add assets templates?**

    You define the location of application-level or mojit-level assets in the 
    ``application.json`` file. Once the location of your assets has been configured, you 
    can statically add the path to the assets in your template. You can also add assets to 
    your view using the `Assets addon <../../api/classes/Assets.common.html>`_ if your 
    application is using the ``HTMLFrameMojit``. See the `Assets <../topics/mojito_assets.html>`_ 
    documentation for implementation details.
    
------------

.. _moj_dyn_assets:
.. topic:: **How are assets dynamically added to views?**

    The `Assets addon <../../api/classes/Assets.common.html>`_ allow you to dynamically 
    add to your view. You need to use the ``HTMLFrameMojit``, however, to use the 
    ``Assets addon``. See 
    `Using the Assets Addon <../topics/mojito_assets.html#using-the-assets-addon>`_ for 
    more information.

HTTP Headers/Responses
----------------------

.. _moj_req_res_instances:    
.. topic:: **Can Mojito applications access the 'request' or 'response' instances of the 
           Node.js classes 'http.ServerRequest' and 'http.ServerResponse'?**

    Yes, the Mojito API has the ``ActionContext`` addon ``Http.server`` that has methods 
    for getting the ``request`` and ``response`` instances of the Node.js classes 
    ``http.ServerRequest`` and ``http.ServerResponse``. From the ``ActionContext`` object 
    ``ac`` shown below, you call ``http.getRequest`` and ``http.getResponse`` to get the 
    ``request`` and ``response`` instances. See `Class Http.server <../../api/classes/Http.server.html>`_ 
    for more information.
    
    .. code-block:: javascript
    
       var request = ac.http.getRequest();
       var response = ac.http.getResponse();


------------

.. _moj_access_headers:
.. topic:: **Is it possible to access HTTP headers from a Mojito application?**

    Yes, the Mojito API has the ``ActionContext`` addon ``Http.server`` that allows you to 
    get, set, and add HTTP headers. See `Class Http.server <../../api/classes/Http.server.html>`_ 
    for the available methods.

------------

.. _moj_cookie_exp:
.. topic:: **How does a Mojito application set the expiration of a cookie in the HTTP response?**

    The controller of your application can get the HTTP response with the ``Http``
    addon and then set the cookie and the max age (expiration) with the ``Cookie`` addon.
    In the example controller below, the cookie is set to expire in two weeks. 
    Note that the value for ``maxAge`` is in milliseconds and that you need to
    require ``mojito-cookie-addon`` and ``mojito-http-addon`` to use the addons.
 
    .. code-block:: javascript

  
       YUI.add('setCookie', function(Y, NAME) {

         Y.namespace('mojito.controllers')[NAME] = {

           index: function(ac) {
            ac.http.getResponse().cookie("cookie_set", "true", { "maxAge": 1209600000 });
            ac.done({
                status: 'Cookie set to expire in two weeks (maxAge in milliseconds).',
            });
          }
        };
      }, '0.0.1', {requires: ['mojito', 'mojito-cookie-addon', 'mojito-http-addon']});


Mojits
------

.. _mojit_exp:
.. topic:: **What is a mojit?** 

    The basic unit of composition and reuse in a Mojito application. It typically 
    corresponds to a rectangular area of a page and uses MVC.

------------
    
.. _moj_children:
.. topic:: **Can mojits have child mojits?** 

    Yes, you can configure your application to have mojits that have one or more child 
    mojits. The parent mojit can execute the child mojits using the 
    `Composite addon <../../api/classes/Composite.common.html>`_. 
    See `Configuring Applications to Have Multiple Mojit <../intro/mojito_configuring.html#configuring-applications-to-have-multiple-mojits>`_ 
    and `Composite Mojits <../topics/mojito_composite_mojits.html#composite-mojits>`_. 

    You can also use frame mojits, such as `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_ 
    that can execute one or more child mojits.       

------------
    
    
.. _moj_objects:
.. topic:: **How do mojits share objects?** 

    You create an application-level middleware or an ActionContext addon that all mojits 
    can access. Your mojits can use this middleware or the ActionContext addon to share 
    objects. See `Creating Addons <../topics/mojito_extensions.html#creating-new-addons>`_ 
    and `Middleware <../topics/mojito_extensions.html#middleware>`_ for implementation 
    details.

------------
    
.. _moj_dynamic_creation:
.. topic:: **Can mojit instances be dynamically defined and then run?** 

    You can run dynamically defined instances of mojits that you created with the Mojito 
    command-line tool. You would create these instances in a mojit controller using the 
    ``ActionContext`` object with either the ``_dispatch`` or ``execute`` methods. 
    See `Running Dynamically Defined Mojit Instances <../topics/mojito_run_dyn_defined_mojits.html>`_ 
    for more information.

------------
    
.. _moj_resources: 
.. topic:: **Is there a way to make all of the resources, such as assets, addons, binders, 
           models, of one mojit available to other mojits?**

    To make the resources of one mojit available to other mojits, you set the ``appLevel`` 
    property in the ``definition.json`` file to ``true``. Mojits wanting to use the 
    resources of application-level mojit must include the YUI module of the 
    application-level mojit in the ``requires`` array. 
    See `Configuring Metadata <../intro/mojito_configuring.html#configuring-metadata>`_ 
    for more information.

------------

.. _moj_names_hyphens:
.. topic:: **Why does Mojito replace hyphens in the names of my mojits with underscores?** 

    The ECMAScript syntax for ``Identifiers`` does not allow hyphens, so Mojito replaces 
    them with underscores. See the section **Identifier Names and Identifiers** in the 
    `ECMAScript Documentation <http://www.ecmascript.org/docs.php>`_ for the syntax rules 
    for ``Identifier`` and ``IdentifierName``.

------------
    
.. _moj_reuse:
.. topic:: **Can I extend or re-use mojits?**

    Although inheritance isn't supported in mojits, there are ways of 
    re-using mojit definitions and configuration, application building blocks, and 
    extending mojits. 

    The following sections offer a brief introduction to re-use and extension in Mojito 
    as well as what cannot be re-used or extended.
    
    **Re-Using Mojit Definitions**

    You can re-use a mojit definition by using the `specs.base <../intro/mojito_configuring.html#specs-object>`_ 
    property in ``application.json``, so that mojit instance uses the same mojit 
    definition of another mojit instance. This allows you to define a mojit instance
    once and have many instances use it. 

    For example, the mojit instances ``index_page`` and ``profile_page`` use the
    re-use the ``basic_page`` mojit definition in the ``application.json``
    below:

    .. code-block:: javascript

       [
         {
           "settings": [ "master" ],
           "specs": {
             "basic_page": {
	             "type": "HTMLFrameMojit",
	             "config": {
	               "child" : {
	                "config" : {
	                  "children" : {
	                    "header" : {
	                      "type" : "Header"
	                    },
	                    "footer" : {
	                      "type" : "Footer"
	                    }
	                  }
	                }
	              },
               "index_page": {
                 "base" : "basic_page",
                   "config": {
	                   "title": "Home Page",
	                   "child" : {
	                     "type" : "Index"
                     }
                   }
                 }
               },
               "profile_page": {
                 "base" : "basic_page",
                   "config": {
	                   "title": "Your Profile",
	                   "child" : {
	                     "type" : "Profile"
                     }
                   }
                 }
               }
             }
           }
         }
       ]

    **Re-Using Application Building Blocks**

    In general, instead of extending a mojit, you would create a new 
    mojit and then re-use building blocks such as YUI modules, assets,
    models, etc. For example, instead of having the ``stockProfile`` mojit
    extend the ``stockQuote`` mojit to get stock quotes, your ``stockProfile`` 
    mojit could use the same addon, YUI module, or model to get the stock quotes.
    
    See the following chapters on assets, extending Mojito, and models:
   
       - `Assets <../topics/mojito_assets.html>`_ - learn how to configure, access, and 
         use the ``Assets`` addon.
       - `Extending Mojito <../topics/mojito_extensions.html>`_ - learn how
         to create addons and add YUI modules.
       - `Models <../intro/mojito_mvc.html#models>`_ - learn how to create and access
         models.


    **Requiring the Controller of Another Mojit**

    Starting with Mojito v0.5, you can create a new controller that requires 
    the controller from another mojit. You simply require and merge the controller
    of the other mojit as shown below:
  
    .. code-block:: javascript

       YUI.add('bar', function(Y, NAME) {

         Y.namespace('mojito.controllers')[NAME] = Y.merge(Y.mojito.controllers.Foo, {
           anotherNewAction: function (ac){
            // do something
           },
           redefinedAction: function (ac) {
             // do something
             // you can also play with:
             // Y.mojito.controllers.Foo.redefinedAction.apply()
           }
         });

       }, '0.0.1', {requires: ['foo']});

    The same principle applies to binders and models.

    **What Can't Be Re-Used/Extended?**
  
    The mojit configuration files ``definition.json`` and ``defaults.json`` cannot be
    re-used like the controller, binder(s), and model(s). In the near future, you will
    be able to re-use these configurations with the help of the ``import_settings`` 
    directive in JSON and YAML files. Assets are also not extensible, so they will 
    have to be included manually.


Views
-----

.. _moj_partials:
.. topic:: **Does Mojito support view partials?**

    Mojito does not support partials, but you do have the following options for rendering 
    data through a template:
    
       * use a child mojit instead of a view partial 
       * render data from a binder through a specific template with the
         `render <../../api/classes/MojitProxy.html#method_render>`_ method. 
       * render data from the controller using `ac.partial.render <../../api/classes/Partial.common.html#method_render>`_.     
  
    Not clear what view partials are? See `view partial <../reference/glossary.html#view-partial>`_ 
    in the `Mojito: Glossary <../reference/glossary.html>`_.

------------

.. _moj_devices:
.. topic:: **Do I have to create separate mojits for different devices?**

    The platform's capabilities allow mojits to be executed (and their results displayed) 
    on every device in either set. For a module developer, the benefit is obvious: a single 
    codebase that can address a wide range of devices. Mojits may still need to be 
    customized for a specific device (or device class), however, to take advantage of 
    device-specific capabilities. The platform does not perform an automated 
    translation/degradation of HTML5 views to simpler layouts, for example. 

------------

.. _moj_refresh_view:
.. topic:: **How do I refresh the view on the client?** 

    To refresh a view, you need to deploy a binder on the client. From the ``mojitProxy`` 
    object of the binder, you can call the ``refreshView`` method to render a new DOM node 
    for the current mojit and its children, as well as reattach all of the existing 
    binders to their new nodes within the new markup. Because all binder instances are 
    retained, state can be stored within a binder's scope. See 
    `Refreshing Views <../intro/mojito_binders.html#refreshing-views>`_ and the 
    `MojitProxy Class <../../api/classes/MojitProxy.html>`_ in the Mojito API 
    documentation for more information.

------------

.. _moj_different_templating:
.. topic:: **Can I use a different templating system other than Handlebars?**

    Mojito currently only comes with a Handlebars rendering engine, but you can add other 
    rendering engines for templating systems such as EJS or Jade. See the
    `View Engine <../topics/mojito_extensions.html#view-engines>`_ documentation for 
    implementation details. 

------------

.. _moj_controller_specify_view:  
.. topic:: **Can the mojit controller specify which template should be rendered?** 

    Yes, you can pass a ``view`` object as the second parameter to ``ac.done`` to specify
    which template should receive the data and be rendered.
    See `Controllers: Specifying the View <../intro/mojito_mvc.html#specifying-the-view>`_ 
    for details.
    
  
Logging/Testing
---------------

.. _moj_log_level:
.. topic:: **How do I change the logging levels for my Mojito application?** 

    You can set log levels for your application using the ``yui.config`` object in 
    ``application.json``. You can also set default log levels using the ``yui.config`` 
    object in the ``defaults.json`` at the application or mojit level.

    See `Logging <../topics/mojito_logging.html>`_ for details and the code example 
    `Simple Logging <../code_exs/simple_logging.html>`_.
    

------------

.. _moj_mod_log_exclusion:
.. topic:: **Is there a way to exclude specific modules from logging on the client?** 

    Yes, you can use the ``logExclude`` property of the ``yui.config`` object to
    exclude the logging of specified modules.

    For example, the logging configuration set by ``yui.config`` below excludes 
    logging from the module ``paging-binder-index``:

    .. code-block:: javascript

       ...
          "yui": {
            "config": {
              "logExclude": "paging-binder-index",
              "logLevel": "info"
            }
          }
      ...

    See `Including and Excluding Modules From Logging <../topics/mojito_logging.html#including-and-excluding-modules-from-logging>`_
    for more information.

------------
    
.. _moj_client_server_logging:
.. topic:: **Can logging be configured to be different for the client and server?** 

    Yes, the ``application.json`` configuration file uses the ``yui.config`` object 
    for configuring logging. You can use contexts to customize logging for 
    different runtime environments. For instance, you could use the ``"runtime:client"``
    context to configure client-side logging and the ``"master"`` context to configure
    server-side logging. You can also use the properties ``logExclude`` and ``logInclude`` 
    of the ``yui.config`` object to log or not log certain components
    of your application. 

    See `config Object <../intro/mojito_configuring.html#yui_config>`_ 
    and the `Log Configuration <../topics/mojito_logging.html#log-configuration>`_ for 
    implementation details.            


Addons/Libraries/Middleware
---------------------------

.. _moj_api_addons:
.. topic:: **Does the Mojito framework offer API methods or addons?** 

    The Mojito framework provides API methods and addons through the ``ActionContext`` 
    object. For an overview of the API and addons, see 
    `Mojito API Overview <../api_overview/>`_. To see the API specifications and the 
    available addons, see the `Mojito API documentation <../../api/>`_.    

------------

.. _moj_yui:
.. topic:: **Can I repurpose or extend existing YUI modules?**

    Although Mojito application developers will have access to a library of modules, we realistically 
    expect modules to require some tweaking before they can be re-purposed. Mojito, 
    however, does offer facilities that make it possible and easy to extend existing 
    modules. 

    A simple and common use case would be extending a built-in addon. For example, 
    suppose you want to add a method to the ``Config`` addon that 
    fetches other instance configurations. You could accomplish this in a number of different ways,
    several of which are listed below:

    - Add the method to the ``Config`` addon (``mojito/lib/app/addons/ac/config.common.js``) in the local copy of Mojito. 
      This is not the recommended practice, but probably the easiest and fastest way to extend the addon.
    - Overwrite the ``Config`` addon by creating the addon ``config.common.js`` in ``{app_dir}/addons/ac`` or 
      ``mojits/{mojit_name}/addons/ac`` with the same code as the built-in ``Config`` addon and then add your
      method. This requires more work and some redundancy, but your custom add will have the functionality 
      of the ``Config`` addon as well as your additional method.
    - Create a custom addon that uses the same constructor as the built-in ``Config`` and add your new method.
      You'll be able to access your method through the namespace you defined in your custom addon. You can
      use the ``Config`` addon for its functionality and your own addon for the additional method.
    - Create a custom addon that includes ``mojito-config-addon`` in the ``requires`` array. In the 
      constructor of your addon, mix the prototype of the ``Config`` addon with the ``prototype``
      of your addon, which might look something like the following: 
      ``Y.mix({YourConstructore}.prototype,Y.namespace("mojito.addons.ac").config.prototype);``
      You can then add your own method, and your controller can use your addon that has the functionality
      of the built-in ``Config`` addon and your added method. 

    See the `Addons <../topics/mojito_extensions.html#addons>`_ section to learn how to create custom addons.
