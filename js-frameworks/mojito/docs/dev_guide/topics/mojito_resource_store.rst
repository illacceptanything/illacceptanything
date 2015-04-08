==============
Resource Store
==============

.. _rs-intro:

Overview
========

The Resource Store (RS) is the Mojito subsystem that manages metadata about 
the files in your Mojito applications. Thus, it is responsible for finding 
and classifying code and configuration files. When you start a Mojito 
application, Mojito can find, track, and resolve versions of files in your 
application, such as mojits, configuration files, binders, views, assets, 
addons, etc., because of the |RS|.

.. _intro-who:

Intended Audience
-----------------

Only advanced Mojito application developers needing finer grain control over 
the management of resources or to extend the functionality of the resource 
store need to read this documentation.

.. _intro-prereqs:

.. _intro-prereqs:

Prerequisites
-------------

In addition to being an advanced Mojito user, you should also understand the following 
before using the |RS|:

- |YUIPlugin|_
- `Mojito addons <../topics/mojito_extensions.html#addons>`_
- `aspect-orient programming (AOP) <http://en.wikipedia.org/wiki/Aspect-oriented_programming>`_

.. _intro-use:

How Can the Resource Store Help Developers?
-------------------------------------------

.. _intro_how-reflection:

Reflection
##########

The |RS| API has methods that can be used (as-is, no addons 
required) to query for details about an application. For example, when you
run the commands ``mojito compile`` and ``mojito gv``, the |RS| API methods 
``getResources`` and ``getResourceVersions`` are called to get information 
about your application.

.. _intro_how-define_types:

Define/Register New Resource Types
##################################

You can write custom |RS| addons using the aspect-oriented features of
the |RS| to define resource types. The |RS| has aspect-oriented features 
because it is implemented as an extension of 
`Y.Base <http://yuilibrary.com/yui/docs/base/>`_, and the |RS| addons are 
implemented as `YUI Plugins <http://yuilibrary.com/yui/docs/plugin/>`_.

For example, you could write your own |RS| addon so that the Mojito 
command-line tool will register files and resources for your application. 

.. _intro_how-extend:

Extend/Modify Functionality of the |RS|
#######################################

You can also write addons or create custom versions of built-in |RS| addons to 
modify how the resource store works. Your addon could map contexts to 
:ref:`selectors <resolution-selectors>`, track new file types, augment the 
information that the |RS| stores about files or code, or augment/replace the 
information returned by the |RS|.          
         

.. _rs-resources:

Resources
=========

.. _resources-what:

What is a Resource?
-------------------

In Mojito, the meaning of the term **resource** is different depending on the 
context. Before we discuss the |RS| in more detail, let's differentiate and 
define the definition of resource in the contexts of Mojito and the |RS|.

.. _what-to_mojito:

To Mojito
#########

The Mojito framework primarily views a **resource** as something useful found 
on the filesystem.

.. _what-to_rs:

To the Resource Store
#####################

The |RS| primarily cares about the *metadata* about each resource, so it 
sees the metadata as the *resource*.  To the |RS|, the **resource** is just 
a JavaScript object containing metadata.  The |RS| defines certain keys with 
specific meanings.  The |RS| addons can add, remove, or modify those 
keys/values as they see fit.  For example, the YUI |RS| addon adds, for 
resources that are YUI modules, the ``yui`` property with metadata about 
the YUI module aspect of the resource. The |RS| itself, however, doesn't 
populate the ``yui`` key of each resource.


.. _resources-versions:

Resource Versions
-----------------

Because there can be multiple files that are all conceptually different 
versions of the same thing (e.g., ``views/index.hb.html`` and 
``views/index.iphone.hb.html``), the |RS| defines **resource version** 
as the metadata about each file and resource as the metadata about the file 
chosen among the possible choices.

The process of choosing which version of a resource to use is called 
*resolution* (or "resolving the resource").  This act is one of the 
primary responsibilities of the |RS|.

See :ref:`Resolution and Priorities <how-resolution>` to learn how the 
|RS| resolves different versions of the same resource.

.. _resources-scope:

Resource Scope
--------------

.. _scope-application:

Application-Level Resources
###########################

Application-level resources are truly global to the application.
At the application level, resources include archetypes, commands, 
configuration files, and middleware. 


.. _scope-mojit:

Mojit-Level Resources
#####################

At the mojit level, resources include controllers, models, binders, 
configuration files, and views. These resources are limited in scope to a mojit.

.. _scope-shared:

Shared Resources
################

Some resources (and resource versions) are *shared*, meaning that they are 
included in **all** mojits.  Most resource types that are mojit level can also 
be shared.  Examples of mojit-level resource types that can't be shared are 
controllers, configuration files (such as ``definition.json``), and YUI language 
bundles.

.. _resources-types:

Resource Types
--------------

The resource type is defined by the ``type`` property in the metadata for a 
given resource. See :ref:`Types of Resources <metadata-types>` for descriptions 
of the resource types. Developers can also create their own types of resources 
to fit the need of their applications. 


.. _rs-metadata:

Resource Metadata
=================

.. _metadata-intro:

Intro
-----

The |RS| uses metadata to track information about each resource. This metadata 
is used by the rest of Mojito to find, load, and parse the resources. The 
metadata is generated by the |RS| or by |RS| addons |---| it has no 
representation on the filesystem.  


.. _metadata-obj:

Metadata Object
---------------
        

+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| Property                  | Data Type     | Required?     | Default Value       | Possible Values                   | Description                                    | 
+===========================+===============+===============+=====================+===================================+================================================+
| ``type``                  | string        | yes           | none                | See :ref:`Types of Resources      | Specifies the type of resource.                | 
|                           |               |               |                     | <types_resources>`.               |                                                |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``subtype``               | string        | no            | none                | See the section                   | Some resource types have multiple subtypes     |
|                           |               |               |                     | :ref:`Subtypes <types-subtypes>`  | that can be specified with ``subtype``. See    |
|                           |               |               |                     |                                   | :ref:`Subtypes <types-subtypes>` for           |
|                           |               |               |                     |                                   | more information.                              |   
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``name``                  | string        | yes           | none                | N/A                               | The name of the resource that is common to     |
|                           |               |               |                     |                                   | all versions (i.e., iPhone/Android, etc.)      | 
|                           |               |               |                     |                                   | of the resource. Example: the name for         |
|                           |               |               |                     |                                   | for the resources ``index.iphone.hb.html``     |
|                           |               |               |                     |                                   | and ``index.hb.html`` is ``index``.            |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``id``                    | string        | yes           | none                | N/A                               | A unique ID that is common to all versions     | 
|                           |               |               |                     |                                   | of the  resource. The ``id`` has the           |
|                           |               |               |                     |                                   | following syntax convention:                   |
|                           |               |               |                     |                                   | ``{type}-{subtype}-{name}``                    | 
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``mojit``                 | string        | no            | none                | N/A                               | The mojit, if any, that uses this resource     | 
|                           |               |               |                     |                                   | The value ``"shared"`` means the resource      |
|                           |               |               |                     |                                   | is available to all mojits.                    | 
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``affinity``              | string        | yes           | See :ref:`Note      | ``server``, ``client``,           | The affinity of the resource, which            |
|                           |               |               | About Default       | ``common``                        | indicates where the resource will be used.     |           
|                           |               |               | Values <def_vals>`. |                                   |                                                |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``selector``              | string        | no            | "*"                 | N/A                               | The version of the resource. For example, a    |
|                           |               |               |                     |                                   | resource could have a version for iPhones,     |
|                           |               |               |                     |                                   | Android devices, fallbacks, etc. (This concept |
|                           |               |               |                     |                                   | of version should not to be confused code      |
|                           |               |               |                     |                                   | revisions, which mark the change of something  |
|                           |               |               |                     |                                   | over time.) For more info, see                 |
|                           |               |               |                     |                                   | :ref:`selector Property <sel_prop>`.           |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| :ref:`source <src_obj>`   | object        | yes           | none                | N/A                               | Specifies where the resource came from.        |
|                           |               |               |                     |                                   | See :ref:`source Object <src_obj>` for         |
|                           |               |               |                     |                                   |  details.                                      |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| ``url``                   | string        | no            | none                | N/A                               | The path used to load the resource             | 
|                           |               |               |                     |                                   | onto the client. Used only for resources       |
|                           |               |               |                     |                                   | that can be deployed by reference to the       |
|                           |               |               |                     |                                   | client.                                        |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| :ref:`view <view_ob>`     | object        | yes, if       | none                | N/A                               | Specifies the output format such as HTML, XML, |
|                           |               | ``type:view`` |                     |                                   | JSON, etc., and the engine that renders the    |
|                           |               |               |                     |                                   | template into the output format.               |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+
| :ref:`yui <yui_obj>`      | object        | no            | none                | N/A                               | The metadata about YUI modules. See the        |
|                           |               |               |                     |                                   | :ref:`yui Object <yui_obj>` for more           |
|                           |               |               |                     |                                   | details.                                       |
+---------------------------+---------------+---------------+---------------------+-----------------------------------+------------------------------------------------+

.. _def_vals:

.. admonition:: Note About Default Values

   Some values for the properties of the metadata object do have defaults, but 
   it depends on the value of the ``type`` property and/or comes from the file 
   name of the resource being represented. For example, the affinity of views 
   is ``common`` (because views are used on both client and server); however, 
   the affinity for controllers comes from the file name, so there is no default.


.. _src_obj:

source Object
#############

+------------------------+---------------+-----------+---------------+-------------------------------+---------------------------------------------+
| Property               | Data Type     | Required? | Default Value | Possible Values               | Description                                 |
+========================+===============+===========+===============+===============================+=============================================+
| ``fs``                 | object        | yes       | none          | N/A                           | Contains the filesystem details of a        |
|                        |               |           |               |                               | resource. See :ref:`fs Object <fs_obj>`.    |
+------------------------+---------------+-----------+---------------+-------------------------------+---------------------------------------------+
| ``pkg``                | object        | yes       | none          | N/A                           | Contains the ``npm`` package details of a   |
|                        |               |           |               |                               | resource. See :ref:`pkg Object <pkg_obj>`.  |
+------------------------+---------------+-----------+---------------+-------------------------------+---------------------------------------------+


.. _fs_obj:

fs Object
*********

+------------------------+---------------+-----------+---------------+-------------------------------+
| Property               | Data Type     | Required? | Default Value | Possible Values               | 
+========================+===============+===========+===============+===============================+
| ``basename``           | string        | yes       | none          | N/A                           |     
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``ext``                | string        | yes       | none          | N/A                           |  
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``fullPath``           | string        | yes       | none          | N/A                           |
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``isFile``             | boolean       | yes       | none          | N/A                           | 
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``rootDir``            | string        | yes       | none          | N/A                           |
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``rootType``           | string        | yes       | none          | See :ref:`Types of Resources  |
|                        |               |           |               | <metadata-types>`.            | 
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``subDir``             | string        | yes       | none          | N/A                           |
+------------------------+---------------+-----------+---------------+-------------------------------+
| ``subDirArray``        | array         | yes       | none          | N/A                           |
+------------------------+---------------+-----------+---------------+-------------------------------+


.. _pkg_obj:

pkg Object
**********

+------------------------+---------------+-----------+---------------+-------------------------------+----------------------------------------------+
| Property               | Data Type     | Required? | Default Value | Possible Values               | Description                                  |
+========================+===============+===========+===============+===============================+==============================================+
| ``depth``              | number        | yes       | none          | N/A                           | The depth in ``npm`` dependencies in the     |
|                        |               |           |               |                               | ``node_modules`` directory where the package |
|                        |               |           |               |                               | is found.                                    |
+------------------------+---------------+-----------+---------------+-------------------------------+----------------------------------------------+
| ``name``               | string        | yes       | none          | N/A                           | The name of the package in which the         |
|                        |               |           |               |                               | resource is found.                           |
+------------------------+---------------+-----------+---------------+-------------------------------+----------------------------------------------+
| ``version``            | string        | yes       | none          | N/A                           | The version of the package.                  |
+------------------------+---------------+-----------+---------------+-------------------------------+----------------------------------------------+


.. _view_obj:

view Object
###########

+------------------------+---------------+-----------+---------------+-------------------------------+-----------------------------------------------+
| Property               | Data Type     | Required? | Default Value | Possible Values               | Description                                   |
+========================+===============+===========+===============+===============================+===============================================+
| ``engine``             | string        | yes       | none          | Any view engine found         | The engine that renders the template.         |  
|                        |               |           |               | in ``addons/view-engines/``   | Two examples of rendering engines are         |
|                        |               |           |               | of the application.           | Dust and Handlebars.                          |
+------------------------+---------------+-----------+---------------+-------------------------------+-----------------------------------------------+
| ``outputFormat``       | string        | yes       | none          | N/A                           | The output format that a template is          |
|                        |               |           |               |                               | rendered into, such as HTML, XML, and JSON.   |
|                        |               |           |               |                               | The ``outputFormat`` matches the file         |
|                        |               |           |               |                               | extension of the template. For example,       |
|                        |               |           |               |                               | the output format for ``index.hb.html`` would |
|                        |               |           |               |                               | be HTML.                                      |
+------------------------+---------------+-----------+---------------+-------------------------------+-----------------------------------------------+

.. _yui_obj:

yui Object
##########

The ``yui`` property of the ``metadata`` object is created by the ``yui`` |RS| addon. 
The ``yui`` property can be any data type, but in general, it is an object 
containing metadata about YUI modules.  You can think of the ``yui`` object as 
a container for the arguments to the ``YUI.add`` method that is used to register
reusable YUI modules.

The following table lists the typical properties that are 
part of the ``yui`` object.

+------------------------+---------------+-----------+---------------+-------------------------------+------------------------------------------------+
| Property               | Data Type     | Required? | Default Value | Example Values                | Description                                    |
+========================+===============+===========+===============+===============================+================================================+
| ``name``               | string        | yes       | none          | ``"scroll"``                  | The name of the YUI module.                    |
+------------------------+---------------+-----------+---------------+-------------------------------+------------------------------------------------+
| ``meta``               | array         | yes       | none          | ``["scroll","node","cache"]`` | Contains a list of YUI modules required by     |
|                        |               |           |               |                               | this resource. The ``meta`` object contains    |
|                        |               |           |               |                               | the same properties as the ``details`` object  |
|                        |               |           |               |                               | that is passed to the `YUI add method <http:// |
|                        |               |           |               |                               | yuilibrary.com/yui/docs/api/classes/YUI.html#m |
|                        |               |           |               |                               | ethod _add>`_.                                 |
+------------------------+---------------+-----------+---------------+-------------------------------+------------------------------------------------+


.. _metadata-types:

Types of Resources
------------------

The ``type`` property of the ``metadata`` object can have any of the following 
values:

- ``config``      - a piece of configuration, sometimes for another resource
- ``controller``  - the controller for a mojit
- ``model``       - a model for a mojit
- ``view``        - a view for a mojit
- ``binder``      - a binder for a mojit
- ``asset``       - an asset (css, js, image, etc.)
- ``addon``       - an addon to the mojito system
- ``archetype``   - the commands to create resources as described in the output from 
  ``mojito help create`` 
- ``spec``        - the configuration for a mojit instance
- ``yui-lang``    - a YUI 3 language bundle
- ``yui-module``  - a YUI 3 module (that isn't one of the above)

.. _types-subtypes:

Subtypes
########

You can use a subtype to specify types of a ``type``. For example, a 
resource of ``type:addon`` might have subtypes, such as ``subtype:ac`` 
for AC addons,  ``subtype:view-engine`` for view engines, or ``subtype:rs`` 
for |RS| addons. 

For ``type:archetype``, the subtypes could be ``subtype:app``  or 
``subtype:mojit``.  The subtype specifies what archetype Mojito should create,
such as an application or mojit. (There may be more in the future!)       


.. _sel_prop:

selector Property
-----------------

The  **selector** is an arbitrary user-defined string, which is used to 
*select* which version of each resource to use.  The selector is defined in 
the ``application.json`` with the ``selector`` property. Because the selector 
is a global entity, you cannot define it at the mojit level. For example, you 
cannot define the selector in the ``defaults.json`` of a mojit.

The value of the ``selector`` property is a string that must not have a 
period (``'.'``) or slash (``'/'``) in it.  In practice, it's suggested to use
alphanumeric and hyphen ('-') characters only.
 
Only one selector can be used in each configuration object identified by the 
``setting`` property, which defines the context. The specified selectors 
must match the selector found in the resource file names.  So, for example, 
the template ``views/index.iphone.hb.html`` has the selector ``iphone``.


.. _sel_prop-ex:

Example
#######

The selector is typically used in conjunction with a context to specify a 
resource for a particular device. In the example ``application.json`` below, 
the selector ``ipad`` is defined when the context is ``device:ipad``. If an 
application is running in the ``device:ipad`` context, Mojito will select 
resources with ``ipad`` identifier. Thus, Mojito might render the template 
``index.ipad.hb.html`` and **not** ``index.iphone.hb.html``.

.. code-block:: javascript

   [
     { 
       "settings": ["master"],
       ...
     },
     {
       "settings": ["device:ipad"], 
       "selector":"ipad",
       "specs": {
         "iPad": {
           "type": "iPadReader",
         }
       }
     }
   ]  
    


.. _metatdata-versions:

Resource Versions
-----------------

Resources can have many versions that are identified by the 
:ref:`selector property <sel_prop>` and the affinity. The selector is defined 
by the user and indicates the version of the resource and the affinity is 
defined by the resource itself.

For example, developer might decide to use the selector ``selector: iphone`` 
for the iPhone version  and ``selector: android`` for the Android version of a 
resource. Using these two selectors, you could have the following two versions 
of the ``index`` resource of type ``view``:

- ``index.iphone.hb.html``
- ``index.android.hb.html``


.. _metadata-ex:

Example
-------


.. code-block:: javascript

   {
     "source": {
       "fs": {
         "fullPath": /"home/me/github-mojito/examples/getting-started-guide/part4/paged-yql/mojits/PagedFlickr/views/index.hb.html",
         "rootDir": "/home/me/github-mojito/yahoo/mojito/github-drewfish/examples/getting-started-guide/part4/paged-yql/mojits/PagedFlickr",
         "rootType": "mojit",
         "subDir": ".",
         "subDirArray": [],
         "isFile": true,
         "ext": ".html",
         "basename": "index.hb"
       },
       "pkg": {
         "name": "paged-yql",
         "version": "0.1.0",
         "depth": 0
       }
     },
     "type": "view",
     "name": "index",
     "id": "view--index",
     "mojit": "PagedFlickr",
     "affinity": "common",
     "selector": "iphone",
     "view": {
       "outputFormat": "html",
       "engine": "hb"
     },
     "url": "/static/PagedFlickr/views/index.hb.html"
   } 
     

.. _rs-how_work:

How Does the Resource Store Work?
=================================

Understanding the |RS| will allow you to debug your 
application and write |RS| addons to customize how it works.

.. _how_work-overview:

Overview
--------

In short, the resource store walks through the application-level, 
mojit-level, and ``npm`` module files (in that order) of a Mojito application, 
determines what type of resource each file is, creates metadata about the resource, 
and then registers the resource.

During this process, the resource store also does the following:

- pre-calculates ("resolves") which resource versions are used for each version 
  of the mojit.
- also keeps track of application-level resources (archetypes, commands, 
  config files, and middleware).
- provides methods and events, including those specialized for AOP.
- explicitly uses the addons :ref:`selector <intro-selector>` and 
  :ref:`config <intro-config>`.

In the following sections, we'll look at the process in a little more details.
To see the code for the resource store, see the |SS|_ file.

.. _how-walk_fs:

Walking the Filesystem
----------------------

Resource versions are discovered by the |RS| at server-start time. The |RS| 
method ``preload`` first walks all the files in the application, excluding the 
``node_modules`` directory. Next, all the files in the packages in ``node_modules`` 
are walked.  The packages are walked in breadth-first fashion, so that *shallower* 
packages have precedence over *deeper* ones. (Not all the packages are used: only 
those that have declared themselves as extensions to Mojito.) Finally, if Mojito 
wasn't found in ``node_modules``, the globally-installed version of Mojito is walked.

After all that, the |RS| knows about all the resource versions. Then it resolves 
those versions into the resources as described in 
:ref:`Resolution and Priorities <how-resolution>`.  

.. _how-resolution:

Resolution and Priorities
-------------------------

The resolving of resource version happens in the |RS| ``preload`` method as well.
The act of resolving the resource versions is really just resolving the 
affinities and selectors. See :ref:`Resource Versions <metatdata-versions>` 
for a brief explanation about how affinities and selectors determine different 
versions of a resource. The following sections discuss what the |RS| uses to 
resolve versions and create a **priority-ordered selector list (POSL)**.

.. _resolution-affinities:

Affinities
##########

The choice of a resource version depends on the affinity. If we're resolving 
versions for the server, versions with ``affinity:server`` will have higher 
priority than ``affinity:common``, and ``affinity:client`` will be completely ignored.

.. _resolution-selectors:

Selectors
#########

The order of the selectors is defined by a POSL, which depends on the runtime context. 

Suppose an application has the following resources:

- ``controller.common.js``
- ``controller.common.iphone.js``
- ``controller.server.js``
- ``controller.server.iphone.js``

In this application, the POSL for context ``{device:browser}`` might 
be ``['*']``, but the POSL 	for the context ``{device:iphone}`` might be 
``['iphone','*']``. We need to use a (prioritized) list of selectors instead of 
just a "selector that matches the context" because not all versions might exist 
for all selectors.  In the example above, if ``controller.server.iphone.js`` 
didn't exist, we should still do the right thing for context ``{device:iphone}``.

.. _resolution-sources:

Sources
#######

The final consideration for priority is the source. Mojit-level versions have 
higher priority than shared versions.  Let's take a different application with 
the following resources:

- ``mojits/Foo/models/bar.common.js``
- ``models/bar.common.js``

In this application, the second resource is shared with all mojits. The mojit 
``Foo``, however, has defined its own version of the same resource 
(``id: model--bar``), and so that should have higher priority than the shared 
one.

.. _resolution-relationships:

Relationships
#############

Finally, there's a relationship between the different types of priority.

#. The source has the highest priority.
#. The selector has the next highest priority.
#. The affinity has the least highest priority.

That means that if there exists, for example, both a ``controller.server.js`` 
and ``controller.common.iphone.js``, for the server and context 
``{device:iphone}``, the second version will be used because its selector 
is a higher priority match than its affinity.

All this is pre-calculated for each resource and for each possible runtime 
configuration (client or server, and every appropriate runtime context).

.. _how-get_data:

Getting Data from the Resource Store
------------------------------------

Besides the standard ways that Mojito uses the resource store, there are two 
generic methods for getting resources and resource versions from the |RS|.

- ``getResourceVersions(filter)``
- ``getResources(env, ctx, filter)``

The APIs are intentionally similar.  Both return an array of resources, and the 
``filter`` argument can be used to restrict the returned resources 
(or versions). The ``filter`` is an object whose keys and values must match 
the returned resources (or versions).  Think of it as a *template* or *partial 
resource* that all resources must match. For example, a filter of 
``{type:'view'}`` will return all the views.

For mojit-level resources or resource versions, specify the mojit name in the 
filter. For example, filter ``{mojit:'Foo'}`` will return all resources 
(or versions) in the ``Foo`` mojit.

.. note:: Because of the resolution process, the resources returned for filter 
          ``{mojit:'Foo'}`` might contain shared resources.

To get mojit-level resources (or versions) from multiple mojits, you'll have to 
call the method ``getResourceVersions`` or ``getResources`` for each mojit.  
You can call ``listAllMojits`` to get a list of all mojits.


.. _rs-creating_rs_addons:

Creating Your Own Resource Store Addons
=======================================

.. _creating_rs_addons-intro:

Intro
-----

In this section, we will discuss the key methods, events, and give a simple 
example of a custom |RS| addon. By using the provided example as a model 
and referring to the |RSC|_ in the API documentation, you should be able to 
create your own custom |RS| addons. 

.. _creating_rs_addons-anatomy:

Anatomy of a |RS| Addon
-----------------------

The resource store addons are implemented using the |YUIPlugin|_ mechanism. 
In essence, a Mojito addon is a YUI plugin, so the skeleton of a |RS| addon 
will be the same as that of a YUI Plugin. 

See the |RSC|_ for the parameters and return values for the |RS| methods.

.. _anatomy-key_methods:

Key Methods
###########

.. _key_methods-initialize:

.. js:function:: initialize(config)

    This method sets the paths to find the application, Mojito, and |RS| files. 
    Addons should hook into |RS| methods (using AOP) or events fired by the |RS| 
    in this method. 
    
    The following host methods are called:
       
       - :js:func:`preloadResourceVersions`
       - :js:func:`resolveResourceVersions` 
       
    After ``preload`` has finished executing, you can call  
    ``afterHostMethod('preload', ...)``.
    
    :param Object config: Contains configuration information with the following properties:     

       - .. js:attribute:: config.appRoot
       
           (*String*) -- contains the the directory of the application. 
       
       - .. js:attribute:: config.mojitoRoot 
       
           (*String*) -- contains the directory of the Mojito framework code.  
    :returns: None
      
.. js:function:: preload()

    Addons are loaded during this method, so they cannot be called before ``preload`` 
    is called. 


.. js:function:: preloadResourceVersions()

    The |RS| walks the filesystem in this method. Before ``preloadResourceVersions`` is 
    called, not much is known, though the static application configuration is available 
    using the method ``getStaticAppConfig``.
    
    Within the ``preloadResourceVersions`` method, the following host methods are called:  
    
       - ``findResourceVersionByConvention``
       - :ref:`parseResourceVersion <key_methods-parseResourceVersion>`
       - :ref:`addResourceVersion <key_methods-addResourceVersion>`
       
    After ``preloadResourceVersions`` has been called:
    
       - All the resource versions have been loaded and are available through the method 
         ``getResourceVersions``.
       - The |RS| has ``selectors`` object whose keys are all selectors in the application. 
         The values for the keys are just ``true``.


.. js:function:: findResourceVersionByConvention()

    This method is called on each directory or file being walked and is used to decide if 
    the path is a resource version. The return value can be a bit confusing, so read the 
    API documentation carefully and feel free to post any questions that you have to the 
    `Yahoo Mojito Forum <https://developer.yahoo.com/forum/Yahoo-Mojito/>`_.
    
    Typically, you would hook into this method with the ``afterHostMethod`` method to 
    register your own resource version types. This method should work together with your 
    own version of the ``parseResourceVersion`` method.
    
.. js:function:: parseResourceVersion()    

    This method creates an actual resource version. Typically, you would hook into this 
    method with the ``beforeHostMethod`` method to create your own resource versions. This 
    should work together with your own version of the 
    :js:func:`findResourceVersionByConvention` method.

.. js:function:: addResourceVersion() 

    This method is called to save the resource version into the |RS|. Typically, if you 
    want to modify/augment an existing resource version, hook into this with the
    ``beforeHostMethod`` method.


.. js:function:: resolveResourceVersions()

    This method resolves the resource versions into resources. As a resource version is 
    resolved, the ``mojitResourcesResolved`` event is called. After the method has been 
    executed, all resource versions have been resolved.
    
.. js:function:: serializeClientStore()

    This method is called during runtime as Mojito creates the configuration for the 
    client-side Mojito.

.. _key_methods-access:

Accessing the Resource Store
****************************

To access the |RS|, you call ``this.get('host')``. The method returns the
|RS|.
   
.. _anatomy-key_events:

Key Events
##########

.. _key_events-mojitResourcesResolved:

mojitResourcesResolved
**********************

This event is called when the resources in a mojit are resolved.

.. _key_events-getMojitTypeDetails:

getMojitTypeDetails
*******************

This event is called during runtime as Mojito creates an *instance* used to 
dispatch a mojit.

.. _creating_rs_addons-ex:

Example
-------

.. _creating_rs_addons_ex-rs_addon:

|RS| Addon
##########

The following |RS| addon registers the new resource type ``text`` for text 
files.

``addons/rs/text.server.js``

.. code-block:: javascript


   YUI.add('addon-rs-text', function(Y, NAME) {

     var libpath = require('path');

     function RSddonText() {
       RSAddonText.superclass.constructor.apply(this, arguments);
     },
     RSAddonText.NS = 'text';
     RSAddonText.ATTResourceStore = {};

     Y.extend(RSAddonText, Y.Plugin.Base, {

       initializer: function(config) {
         this.appRoot = config.appRoot;
         this.mojitoRoot = config.mojitoRoot;
         this.afterHostMethod('findResourceVersionByConvention', this.findResourceVersionByConvention, this);
         this.beforeHostMethod('parseResourceVersion', this.parseResourceVersion, this);
       },

       destructor: function() {
         // TODO:  needed to break cycle so we don't leak memory?
       },

       /**
       * Using AOP, this is called after the ResourceStore's version.
       * @method findResourceVersionByConvention
       * @param source {object} metadata about where the resource is located
       * @param mojitType {string} name of mojit to which the resource likely belongs
       * @return {object||null} for config file resources, returns metadata signifying that
       */
       findResourceVersionByConvention: function(source, mojitType) {
         // We only care about files
         if (!source.fs.isFile) {
           return;
         }

         // We only care about txt files
         if ('.txt' !== source.fs.ext) {
           return;
         }
         
         return new Y.Do.AlterReturn(null, {
           type: 'text'
         });
       },

       /**
       * Using AOP, this is called before the ResourceStore's version.
       * @method parseResourceVersion
       * @param source {object} metadata about where the resource is located
       * @param type {string} type of the resource
       * @param subtype {string} subtype of the resource
       * @param mojitType {string} name of mojit to which the resource likely belongs
       * @return {object||null} for config file resources, returns the resource metadata
       */
       parseResourceVersion: function(source, type, subtype, mojitType) {
         var res;

         if ('text' !== type) {
           return;
         }
         res = {
           source: source,
           type: 'text',
           affinity: 'server',
           selector: '*'
         };
         if ('app' !== source.fs.rootType) {
           res.mojit = mojitType;
         }
         res.name = libpath.join(source.fs.subDir, source.fs.basename);
         res.id = [res.type, res.subtype, res.name].join('-');
         return new Y.Do.Halt(null, res);
       }
     });
     Y.namespace('mojito.addons.rs');
     Y.mojito.addons.rs.text = ResourceStoreAddonText;

   }, '0.0.1', { requires: ['plugin', 'oop']});

.. _creating_rs_addons_ex-text_addon:

Text ActionContext Addon
########################

The Text Addon provides accessors so that the controller can access resources 
of type ``text``. You could use this example addon as a model for writing an 
addon that allows a controller to access other resource types such as ``xml`` 
or ``yaml``.

``addons/ac/text.server.js``

.. code-block:: javascript


   YUI.add('addon-ac-text', function(Y, NAME) {

     var libfs = require('fs');

     function Addon(command, adapter, ac) {
       this._ctx = ac.command.context;
     }
     Addon.prototype = {
     
       namespace: 'text',

       setStore: function(store) {
         this._store = store;
       },
       list: function() {
         var r, res, ress, list = [];
         ress = this._store.store.getResources('server', this._ctx, {type:'text'});
         for (r = 0; r < ress.length; r += 1) {
           res = ress[r];
           list.push(res.name);
         }
         return list;
       },
       read: function(name, cb) {
         var ress;
         ress = this._store.store.getResources('server', this._ctx, {type:'text', name:name});
         if (!ress || 1 !== ress.length) {
           cb(new Error('Unknown text file ' + name));
         }
         libfs.readFile(ress[0].source.fs.fullPath, 'utf-8', function(err, body) {
           cb(err, body);
         });
       }
     };
     Y.mojito.addons.ac.text = Addon;
     }, '0.1.0', {requires: ['mojito']}
   );
   
.. _creating_rs_addons_ex-controller:   

Controller
##########

``mojits/Viewer/controller.server.js``


.. code-block:: javascript

   YUI.add('viewer', function(Y, NAME) {
   
     Y.namespace('mojito.controllers')[NAME] = { 

       index: function(ac) {
         var chosen; // TODO:  use form input to choose a text file
         if (!chosen) {
           var list;
           list = ac.text.list();
           chosen = list[0];
         }
         ac.assets.addCss('./index.css');
         ac.text.read(chosen, function(err, body) {
           if (err) {
             return ac.error(err);
           }
           ac.done({body: body});
         });
       }
     };
   }, '1.0.1', {requires: ['mojito', 'mojito-assets-addon', 'addon-ac-text']});
   

.. _rs-addons:

Resource Store Built-In Addons
==============================

.. _addons-intro:

Intro
-----

Mojito comes with built-in resource store addons that are used by the |RS|
and the Mojito framework. These resource store addons are required by the |RS|
and the Mojito framework. Thus, particular care must be taken when creating 
custom versions of them. 

The |RS| comes with the following four built-in addons:  

- ``config``
   - registers new resource type ``config`` found in JSON configuration files
   - provides an API for reading both contextualized and straight-JSON files
   - provides sugar for reading an application's dimensions
- ``selector``
   - decides the priority-ordered list (POSL) to use for a context
   - looks  for ``selector`` in ``application.json``. Because 
     ``application.json`` is a context configuration file, the ``selector`` can 
     be contextualized there.
- ``url``
   - calculates the static handler URL for appropriate resources (and resource versions)
   - stores the URL in the ``url`` key of the resource
   - calculates the asset URL base for each mojit
- ``yui``
   - registers new resource type ``yui-module`` found in the directories 
     ``yui_modules`` or ``autoload``
   - registers new resource type ``yui-lang`` found in the ``lang`` directory
   - calculates the ``yui`` metadata for resource versions that are YUI modules
   - pre-calculates corresponding YUI module dependencies when resources are 
     resolved for each version of each mojit 
   - appends the pre-calculated YUI module dependencies for the controller and 
     binders when Mojito queries the |RS| for the details of a mojit 
     (``getMojitTypeDetails`` method) 
   - provides methods used by Mojito to configure its YUI instances
  

.. _addons-custom:

Creating Custom Versions of Built-In |RS| Addons
------------------------------------------------

We will be examining the ``selector`` and ``url`` addons to help you create 
custom versions of those addons. We do not recommend that you create custom 
versions of the ``config`` or ``yui`` addons, so we will not be looking at 
those addons. Also, this documentation explains what the |RS| expects the 
addon to do, so you can create your own version of the addons. To learn what 
the |RS| built-in addons do, please refer to the |RSC|_ in the API 
documentation.


.. _custom-selector:

selector
########

.. _selector-desc:

Description
***********

If you wish to use a different algorithm for to determine the selectors
to use, you can implement your own version of this |RS| addon in the
``addons/rs/selector.server.js`` file of your application.  


.. _selector-reqs:

Requirements
************

Because the ``selector`` addon is used directly by the the resource store, all 
implementations need to provide the following method:

- :js:func:`getPOSLFromContext(ctx)`

.. _selector-methods:

Methods
*******

.. js:function:: getPOSLFromContext(ctx)

    Returns the priority-ordered selector list (POSL) for the context.

    :param String ctx: The context that the application is running in. 
    :returns: Array


.. js:function:: getAllPOSLs()

    Returns all POSLs in the application.


.. _url-intro:

url
###

.. _url-desc:

Description
***********

The ``url`` addon calculates and manages the static handler URLs for resources.
The addon is not used by resource store core, but used by the static handler 
middleware.

If you wish to use a different algorithm to determine the URLs, you can
implement your own version of this |RS| addon in the
``addons/rs/url.server.js`` file of your application.

After the method ``preloadResourceVersions`` sets ``res.url`` to the static 
handler URL for the resource, the method ``getMojitTypeDetails`` sets the 
mojit's ``assetsRoot``. The static handler URL can be a rollup URL.


.. _url-reqs:

Requirements
************

Your addon is required to do the following:

- Set the ``url`` property in the resource ``metadata`` object.


   

.. |RS| replace:: Resource Store
.. |RSC| replace:: ResourceStore.server Class
.. _RSC: https://developer.yahoo.com/cocktails/mojito/api/classes/ResourceStore.server.html
.. |YUIPlugin| replace:: YUI Plugin
.. _YUIPlugin: http://yuilibrary.com/yui/docs/plugin/
.. |SS| replace:: server.store.js
.. _SS: https://github.com/yahoo/mojito/blob/develop/lib/store.server.js
.. |--| unicode:: U+2013   .. en dash
.. |---| unicode:: U+2014  .. em dash
   :trim:
