================
Static Resources
================

Mojito also lets you statically serve files such as controllers, binders, 
assets (CSS and JavaScript), etc. You can access static resources through a 
URL that contains the following three components:

- **prefix** - the basename directory of the static URL.
- **source path** - the directory of either the Mojito framework,  the 
  application, or the mojit depending on the level of the resource.
- **relative path** - the path relative to the source path.

The URL of the static URL has the following syntax: ``/static/{source_path}/{relative_path}``

.. _static_resources-prefix:

Prefix
======

The prefix default is ``/static/``, but can be changed through the
`staticHandling object <./mojito_configuring.html#app-statichandling-obj>`_ 
in the ``configuration`` object of ``application.json``.

.. _static_resources-src_path:

Source Path
===========

The source path is based on resource level within Mojito. The three resource 
levels are framework, application, and mojit.

The source paths for the three levels are the following:

- ``/mojito/`` - framework-level resources that are available to the entire 
  framework
- ``/{application_name}/`` - application-level resources where the source 
  path is based on the name of the application.  For example, for the ``news`` 
  application, the source path would be ``/news/``. This resource can be 
  accessed by the application or any of its mojits.
- ``/{mojit_name}/`` - mojit-level resources where the source path is based 
  on the name of the mojit. For example, for the ``paging`` mojit, the source 
  path would be ``/paging/``. Only the mojit can access this resource.

.. _static_resources-rel_path:

Relative Path
=============

The relative path is the path to the resource relative to the source path. 
For example, the binder  ``index.js`` for the Foo mojit would have the 
relative path ``/binders/index.js``.

.. _static_res_rel_path-ex:

Examples
########

- Static URL to the framework-level resource ``helper.js``:

   ``/static/mojito/helper.js``

- Static URL to the application-level resource ``photos/binders/photo_search.js``:

   ``/static/photos/binders/photo_search.js``

   The application is ``photos``.

- Static URL to the mojit-level resource ``scroll/assets/body.css``:

   ``/static/scroll/assets/body.css``

   The mojit is ``scroll``.

- Static URL to the application-level resource ``finance/assets/ticker.css``:

   ``/app_resources/finance/assets/ticker.css``

   In this example, the default prefix was overridden in the ``staticHandling`` 
   object to be ``app_resources``.


