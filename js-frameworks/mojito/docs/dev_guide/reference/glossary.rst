========
Glossary
========

action
------

   A method of a mojit instance (``{mojit_instance}.{action}``) that invokes a call to a 
   function of a mojit controller when an HTTP request is made on an associated routing 
   path. For example, suppose the root path is associated with the mojit instance and 
   action ``hello.index``, and the ``hello`` instance is of type ``Hello``. When an 
   HTTP request is made on the root path, the action ``index`` would invoke the ``index`` 
   function of the ``Hello`` controller.


Action Context
--------------

   The Action Context is an essential element of the Mojito framework that gives you 
   access to the frameworks features from within a controller function. See 
   `Action Context <../api_overview/mojito_action_context.html>`_ in the 
   `Mojito API Overview <../api_overview/>`_ and the 
   `ActionContext Class <../../api/classes/ActionContext.html>`_ in the 
   `Mojito API documentation <../../api/>`_ for more detailed information.

addon
-----

   A namespacing object attached directly to the Action Context object that provides 
   additional functions. The ``ActionContext`` object is available in every controller 
   function. See `Mojito API Overview: Addons <../api_overview/mojito_addons.html>`_ for 
   more information.

affinity
--------

   The location where a resource is available. For example, the value for ``{affinity}`` 
   in the file ``controller.{affinity}.js`` could be ``server``, ``client``, or 
   ``common``, depending on where the resource is available. The affinity ``common`` means 
   that the resource is available in both the client and server. If both ``common`` and 
   ``client`` are given, then the ``client`` file is used on the client and the ``common`` 
   file is used on the server. Likewise, if both ``common`` and ``server`` are given, 
   then the ``common`` file is used on the client and the ``server`` file is used on the server.

archetype
---------

   A type of resource that the Mojito CLI utility can generate boilerplate code for. The Mojito CLI
   utility recognizes the archetypes ``simple``, ``default``, and ``full`` to generate code for
   different types of applications and mojits, but users can define archetypes so that Mojito can
   generate code for any type of resource. See 
   `Creating Code from Archetypes <../reference/mojito_cmdline.html#creating-code-from-archetypes>`_ 
   for more information. 

assets
------

   File resources that are required on the clients. These resources are primarily CSS but 
   can also be JavaScript that is ancillary to and not a core component of the Mojito 
   application. See `Mojito Developer Topics: Assets <../topics/mojito_assets.html>`_ to 
   learn how to use assets in Mojito applications.


binder
------

   Mojit code deployed to the browser that can allow event handlers to attach to the mojit 
   DOM node, communicate with other mojits on the page, and execute actions on the mojit 
   that the binder is attached to. A mojit may have zero, one, or many binders within 
   the ``binders`` directory. See `Mojito Binders <../intro/mojito_binders.html>`_ for 
   more information.

CDN
---

   `CDN <http://en.wikipedia.org/wiki/Content_delivery_network>`_ stands for **content 
   delivery network**, which is a largely distributed system of servers deployed in multiple 
   data centers in the Internet. CDNs serve content to end-users with high availability 
   and high performance. YUI has a CDN for delivering
   the `YUI library <http://yuilibrary.com/>`_. 


composite mojits
----------------

   When a parent mojit controls the execution and layout of child mojits. See 
   `Mojito Developer Topics: Composite Mojits <../topics/mojito_composite_mojits.html>`_
   for more information.

context
-------

   The runtime environment of a Mojito application that determines which configurations are applied. 
   For example, your application may run on different devices or in different language environments.  
   Each runtime environment could be a context that is associated with a set of configuations. 
   The context can be statically set (base context) on the command line or dynamically set (request context) 
   in the HTTP headers and/or the request query string. The configurations for request contexts override 
   those of the base context. See `Using Context Configurations <../topics/mojito_using_contexts.html>`_
   for more information.

controller
----------

   In Mojito, the controller is mojit code that can either do all of the work or delegate 
   the work to models and/or views. In the typical case, the mojit controller requests the 
   model to retrieve data and then the controller will serve that data to the views. 
   See `MVC: Controllers <../intro/mojito_mvc.html#controllers>`_ for more information.
   
mojit
-----

   The basic unit of composition and reuse in a Mojito application. It typically 
   corresponds to a rectangular area of a page and is constructed using JavaScript and the 
   `MVC`_.
   
mojitProxy
----------

   The proxy object given to binders that allows them to interact with the mojit it 
   represents as well as with other mojits on the page. See the
   `mojitProxy Object <../intro/mojito_binders.html#mojitproxy-object>`_ and the 
   `MojitProxy Class <../../api/classes/MojitProxy.html>`_
   for more information.
   
Mojito
------

   A Web framework with which applications are written entirely in JavaScript, using an 
   `MVC`_ approach and allowing for transportable code between client (browser) and 
   server. The framework addresses the combined needs of connected devices and desktops, 
   including disconnected application usage.
   
MVC
---

   Acronym for Model-View-Controller. A software architecture pattern used in software 
   engineering. The pattern isolates "domain logic"  (the application logic for the user) 
   from the user interface (input and presentation), permitting independent development, 
   testing and maintenance of each (separation of concerns). See 
   `Mojito Intro: MVC <../intro/mojito_mvc.html>`_ to learn how MVC is used in Mojito.
   
Node.js
-------

   An evented I/O framework for the V8 JavaScript engine on Unix-like platforms that is
   intended for writing scalable network programs such as Web servers. See 
   `nodejs.org <http://nodejs.org>`_ for more information.

npm
---

   The package manager for `Node.js`_, which can be used to install and publish code 
   libraries and manage the dependencies among them. 
   See `npmjs.org <http://npmjs.org>`_ for more information.
   
   
OAuth
-----

   An open standard that enables users to share information stored on one site with 
   another site without giving out the user ID and password. See the 
   `Yahoo OAuth Quick Start Guide <https://developer.yahoo.com/oauth/guide/oauth-guide.html>`_ 
   for more information.

Shaker
------

   An asset rollup management tool for Mojito applications that improves performance by 
   reducing the number of HTTP requests. See the `Shaker documentation <https://developer.yahoo.com/cocktails/shaker/>`_
   and the `Shaker GitHub repository <https://github.com/yahoo/mojito-shaker>`_ for more
   information.

template
--------

   Template files that are rendered into HTML and served to a device. These templates can 
   contain expressions (Handlebars) or tags (Mustache) that are replaced with values by a 
   view rendering engine.
   
   
view
----

   The display element of Mojito that is served to a device. The view is rendered from the 
   template and consists of HTML and CSS. 
   See `MVC: Views <../intro/mojito_mvc.html#views>`_ for more information.
   
view partial
------------

   Also referred to as partials, partial views, and partial collection. View partials are 
   collections that can be iterated through to create a document fragment. Using a view 
   partial, you can create that document fragment instead of iterating through the 
   collection in the view.


YUI
---

   Acronym for `Yahoo User Interface <https://developer.yahoo.com/yui/>`_. A set of 
   utilities, written in JavaScript and CSS, 
   for building rich, interactive Web applications.
   
