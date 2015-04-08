========================
Mojito and npm Packaging
========================

.. _mojito_package-overview:

Overview
========

Having installed Mojito with npm 1.0, you already understand that Mojito is an 
npm package. What may not be as clear is that Mojito applications are also npm 
packages. Being an npm package, Mojito applications can have their own 
dependencies that are installed using npm. For example, after you create a 
Mojito application, you can use npm to install a local copy of the Mojito 
framework in the ``node_modules`` directory. If you deployed your application 
to a cloud server that has a Node.js runtime environment, your application 
could be run by this locally installed copy of the Mojito 
framework.

Your Mojito application can also install other npm modules, even those that 
contain Mojito resources, such as mojits or middleware. Conversely, you can 
create npm modules that contain Mojito resources, so other developers can 
reuse your code.

Because npm allows you to use other modules or create your own, this chapter 
is divided into two sections to meet the needs of the following two audiences:

- :ref:`developers using shared mojits <using_shared_mojits>`
- :ref:`authors creating npm modules that contain shared Mojito resources <author_npm_mod_shared_mojito_resource>`

.. _package_overview-resource:

Mojito Resources
----------------

A *Mojito resource* is a piece of code or functionality used by Mojito. These 
resources can be installed with npm or live directly in the Mojito application. 
Examples of Mojito resources could be shared mojits and middleware. Developers 
using shared mojits and those authoring npm modules that contain code used by 
Mojito should be familiar with the meaning of *Mojito resource* as it will be
used throughout this chapter.


.. _using_shared_mojits:

Using Shared Mojits
===================

Mojito applications can have any number of different resources installed with 
npm. Each of these resources should be specified in the  package descriptor 
file ``package.json`` of the Mojito application. When users run ``npm install`` 
in the application directory, npm modules containing Mojito resources and 
those not containing Mojito resources will be installed into the 
``node_modules`` directory. Your Mojito application will have access to 
all of the installed npm modules as soon as the application starts.


For details about npm packages, see the 
`npm's package.json handling <http://npmjs.org/doc/json.html>`_.

.. _process_spec_install_dependencies:

General Process of Using Shared Mojits
--------------------------------------

The following steps are just a guideline and not definitive instructions. 
Your application may not need to install any npm modules.

#. Create a Mojito application.
#. Add any needed dependencies to ``dependencies`` object in ``package.json``.
#. Install dependencies with npm.
    
   ``{app_dir}$ npm install``  
#. When Mojito starts, your application will have access to the installed 
   npm modules.    

.. _process_spec_install_dependencies_ex:
    
Example package.json
####################

The dependencies include Mojito, the ``async`` module, and the shared mojit 
``form_mojit`` (example) that will be installed in ``node_modules`` when you 
run ``npm install`` from the Mojito application directory.

.. code-block:: javascript

   {
      "name": "helloworld",
      "description": "My Mojito application",
      "version": "0.0.1",
      "author": {
        "name": "Your Name",
        "email": "nobody@yahoo-inc.com"
      },
      "contributors": [
        {
          "name": "Your Name",
          "email": "nobody@yahoo-inc.com"
        }
      ],
      "scripts": {
          "start": "node app.js"
      },
      "engines": {
          "node": "> 0.8",
          "npm": "> 1.0"
      },
      "dependencies": {
          "debug": "*",
          "node-markdown": "*",
          "mojito": "~0.9.0"
      },
      "devDependencies": {
          "mojito-cli": ">= 0.2.0"
      }
  }

   
.. _author_npm_mod_shared_mojito_resource:

Authoring an npm Module Containing Shared Mojito Resources
==========================================================

Developers who have created Mojito resources that they would like to share with 
others can package the Mojito resources in an npm module. The npm module is 
simply a container for the Mojito resource(s). The npm module must specify that 
it contains a Mojito resource in its ``package.json``.  

.. _res_def_metadata:

General Process of Authoring an npm Module Containing Shared Mojito Resources
-----------------------------------------------------------------------------

#. Create your Mojito resource.
#. Specify that the npm module contains Mojito resources in ``package.json``. 
   See :ref:`Resource Definition Metadata <resource_def_metadata>` to learn how.
#. Publish the module to the `npm registry <http://npmjs.org/doc/registry.html>`_.


.. _resource_def_metadata:

Resource Definition Metadata
----------------------------
                            
The npm module containing a Mojito resource is specified by the ``mojito`` object 
in ``package.json``. The ``mojito`` object, a property of the ``yahoo`` object, 
defines the type and location of the resource as well as the required version of 
Mojito to use the resource as shown in the example below. See :ref:`moj_object` 
for details about the properties of the ``mojito`` object.

.. code-block:: javascript

   "yahoo": {
     "mojito": {
       "version": "{required Mojito version}",
       "type":  "{resource_type}",
       "location": "{location_of_resource}" 
     }
   }


.. _moj_object:

mojito object
#############

The following table describes the properties of the ``mojito`` object that
specifies the resource type and location.


+--------------+----------------+-----------+----------------------------+
| Field Name   | Data Type      | Required? | Description                |
+==============+================+===========+============================+
| ``location`` | String         | No        | The subdirectory in the    | 
|              |                |           | npm package where the      |
|              |                |           | resource can be found. The |
|              |                |           | default location is the    |
|              |                |           | package directory.         |
+--------------+----------------+-----------+----------------------------+
| ``type``     | String         | Yes       | Specifies the resource     |
|              |                |           | type. The following are    |
|              |                |           | the possible values:       |
|              |                |           | ``"mojit"``, ``"bundle"``  |
|              |                |           | See :ref:`res_types` for   |
|              |                |           | details.                   |
+--------------+----------------+-----------+----------------------------+
| ``version``  | String         | No        | The version of Mojito      |
|              |                |           | required to use the        |
|              |                |           | resource.                  | 
|              |                |           | For example: ``">0.4"``    |
+--------------+----------------+-----------+----------------------------+

                                   
.. _res_types:

Mojito Package Types
####################

Currently, Mojito packages can be of type ``mojit`` or ``bundle``. See the 
sections below for more details. 


.. _mojit_type:

mojit
*****

The ``mojit`` type specifies that the npm module contains a mojit. The 
resources in the mojit (controller, models, views, etc.) will be looked for at 
the location specified by the ``"location"`` field of the ``mojito`` object. 
For example, the controller will be looked for in the following location:

``{location}/controller.{affinity}.{selector}.js``

.. _bundle_type:

bundle
******

The ``bundle`` type specifies that the npm module contains several resources. 

The following table shows where Mojito will automatically search for the different 
resources. The ``{location}`` is the location specified by the ``location`` property of 
the ``mojito`` object.


+--------------------+---------------------------------------+----------------------------------+
| Resource           | Auto-Detected Location                | Notes                            |
+====================+=======================================+==================================+
| mojits             | ``{location}/mojits/``                |                                  |
+--------------------+---------------------------------------+----------------------------------+
| actions            | ``{location}/actions/``               |                                  |
+--------------------+---------------------------------------+----------------------------------+
| addons             | ``{location}/addons/{subtype}/``      | The ``{subtype}`` for addons     |
|                    |                                       | can be ``ac`` for ActionContext  |   
|                    |                                       | addons or ``view-engines`` for   |
|                    |                                       | template rendering engine.       | 
+--------------------+---------------------------------------+----------------------------------+
| assets             | ``{location}/assets/``                |                                  |
+--------------------+---------------------------------------+----------------------------------+
| binders            | ``{location}/binders/``               |                                  |
+--------------------+---------------------------------------+----------------------------------+
| lang               | ``{location}/lang/``                  |                                  |
+--------------------+---------------------------------------+----------------------------------+
| models             | ``{location}/models/``                |                                  |
+--------------------+---------------------------------------+----------------------------------+
| views              | ``{location}/views/``                 |                                  |
+--------------------+---------------------------------------+----------------------------------+
| YUI modules        | ``{location}/yui_modules/``           |                                  |
|                    | ``{location}/autoload/``              |                                  |
+--------------------+---------------------------------------+----------------------------------+


.. _resource_def_examples:

Examples
--------

**package.json**

The example ``package.json`` has the ``yahoo`` object that specifies that this 
npm module contains a Mojito resource.


.. code-block:: javascript

   {
     "name": "mojito_sample_app",
     "description": "A test app to show how to create the package.json file",
     "version": "0.0.2",
     "author": "Joe Hacker <jhacker@yahoo.com>",
     "contributors": [
        {"name": "Noel Jays", "email": "njays@yahoo.com"}
     ],
     "yahoo": {
       "mojito": {
         "type": "mojit",
         "version": "0.3.0"
       }
     },
     "engines": {
       "node": "> 0.4",
       "npm": "> 1.0"
     },
     "dependencies": {
       "mojito": "~0.3.0"
     }
   }

   

**Mojito Application Using Shared Resources**

:: 

   mojito_app/
              application.json
              package.json
              mojits/
                     A/
                       A.common.js
                       definition.json
                       views/
                       index/
                             index.hb.html
                       binders/
                               index/
                                     index.js
                     B/
                       ...
                     C/
                       ...
              yui_modules/
                          liba.js
                          libb.js
              node_modules/
                           mojito-mojit-RMP/
                                            package.json
                                            {
                                              "yahoo": {
                                                "mojito": {
                                                  "type": "mojit",
                                                  "version": "*"
                                                }
                                              }
                                            }
                                            controller.common.js
                           mojito-middleware-redirect/
                                                      package.json
                                                      {
                                                        "yahoo": {
                                                          "mojito": {
                                                            "type": "bundle",
                                                            "version": "*"
                                                          }
                                                        }
                                                      }
                                                      middleware/
                                                                 mojito-middleware-redirect.js
                           mojito-viewengine-dust/
                                                  package.json
                                                  {
                                                    "yahoo": {
                                                      "mojito": {
                                                        "type": "bundle",
                                                           "version": "*"
                                                      }
                                                    }
                                                  }
                                                  mojito-viewengine-dust.common.js
                                                  node_modules/
                                                               dust/
                                                                    ... actual dust library ...
                           mojito/
                                  package.json
                                  {
                                    "yahoo": {
                                      "mojito": {
                                        "type": "bundle",
                                        "location": "lib/app",
                                        "version": "*"
                                      }
                                    }
                                  }
                                  lib/
                                  app/
                                      ...
                           async/
                                 LICENSE
                                 Makefile
                                 README.md
                                 index.js
                                 lib/
                                     async.js
                                 package.json
                                 
