====================
Mojito: A Quick Tour
====================

Before starting to develop Mojito applications, we would like to introduce some of the 
main features of Mojito. This simple introduction should give you the bird's eye view of 
how Mojito works and what it can offer to developers.

.. _mojito_quicktour-node:

Node.js
=======

`Node.js <http://nodejs.org/>`_ is not a feature of Mojito, but Mojito, as a Node.js 
module,  greatly benefits from the speed and scalability of Node.js.  Mojito also takes 
advantage of the core modules of Node.js: The Mojito framework uses the ``http``, ``url``, 
and ``querystring`` modules to handle requests and parse URLs, and the Mojito command line 
relies heavily on the ``util``, ``fs``, ``path``, and ``vm`` modules. Mojito also 
leverages npm packages, such as the `express package <http://expressjs.com/>`_ to create a 
server and parse cookies. Mojito application developers also use Node.js core modules and 
npm modules to their advantage. For example, your application could use the ``fs`` core 
module to cache data or use the ``connect`` package as network middleware.

.. _mojito_quicktour-framework:

Mojito Framework
================

The Mojito framework offers an extensive API with modules for executing code, making REST 
calls, handling cookies and assets, accessing parameters and configuration, and more. 
The framework can can detect the type of calling devices and serve the appropriate HTML 
markup.

.. _mojito_quicktour-cmdline:

Mojito Command-Line Tool
========================

The Mojito command-line tool, besides being used to create and start applications, also 
offers developers with a variety of utilities. Developers can use the ``mojito`` command 
to run unit tests, create documentation, sanitize code with JSLint, and build projects for 
iOS and Android applications.

.. _mojito_quicktour-yui3:

YUI 3
=====

YUI 3 forms the backbone of Mojito. The models and controllers in the Mojito MVC use 
`Y.Base <http://yuilibrary.com/yui/docs/base/>`_, and the addons, which extend 
functionality in Mojito, are based on 
`YUI Plugins <http://yuilibrary.com/yui/docs/plugin/>`_. Many important features of 
Mojito, such as testing, logging, internationalization, and cookie handling are also 
derived from YUI 3. Because of the tight integration of Mojito with YUI 3, developers can 
easily extend the functionality of Mojito applications by adding YUI 3 modules.

.. _mojito_quicktour-apps:

Mojito Applications
===================

Mojito applications are JavaScript applications that fuse configuration and MVC 
architecture. Because the application code is written in JavaScript, your applications are 
portable, being able to move freely from the server to the client or just execute on the 
server in the Node.js environment. Being on the client does not restrict your application 
because it can still communicate with the server through event-driven modules called 
binders. The binders make it simple to update content or dynamically change the page. 
Your application can also customize views for different devices by rendering HTML markup 
from templating systems such as `Handlebars <http://handlebarsjs.com/>`_.
