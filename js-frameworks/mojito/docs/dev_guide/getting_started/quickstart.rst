=================
Mojito Quickstart
=================

.. _mojito_quickstart-reqs:

Requirements
============

**System:** Unix-based system.

**Software:** `Node.js (>= 0.8.0 <0.11) <http://nodejs.org/>`_, `npm (> 1.0.0) <http://npmjs.org/>`_

.. _mojito_quickstart-install:

Installation Steps
==================

#. Globally install the Mojito CLI package v0.2.x (``mojito-cli``) from the npm registry 
   so that you can run Mojito commands. You may need to use ``sudo`` if 
   you run into permission errors.

   ``$ npm install mojito-cli@~0.2.0 -g``
#. Do the same for the Mojito CLI utility for creating applications and mojits (``mojito-cli-create``):

   ``$ npm install mojito-cli-create@0~0.1.1 -g``

#. Confirm that Mojito has been installed by running the help command.

   ``$ mojito help``

.. _mojito_quickstart-create_app:

Create a Mojito Application
===========================

#. ``$ mojito create app hello_world``
#. ``$ cd hello_world``
#. ``$ mojito create mojit myMojit``



.. _mojito_quickstart-modify_app:

Modify Your Application
=======================

To make the application return a string we want, replace the code in 
``mojits/myMojit/controller.server.js`` with the following:

.. code-block:: javascript

  YUI.add('mymojit', function(Y, NAME) {
  
    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
            ac.done('Hello, world. I have created my first Mojito app at ' + (new Date()) + '.');
        }

    };
  });

.. _mojito_quickstart-run_app:

Running the Application
=======================

#. From the ``hello_world`` application directory, start Mojito:

   ``$ node app.js``
#. Go to http://localhost:8666/@myMojit/index to see your application. You'll also find
   the Mojito documentation and instructions for creating the ``quickstartguide`` application.
#. Stop your application by pressing **Ctrl-C**.

For a more in-depth tutorial, please see `Tutorial: Creating Your First Application <mojito_getting_started_tutorial.html>`_. 
To learn more about Mojito, see the `Mojito Documentation <../>`_.

