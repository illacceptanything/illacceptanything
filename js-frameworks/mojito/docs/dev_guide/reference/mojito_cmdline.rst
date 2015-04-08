===================
Mojito Command Line
===================

Mojito comes with a command-line tool that provides a number of key 
capabilities for the developer, from generating code skeletons, to 
running tests and test coverage, to cleaning up and documenting the 
code base.

All commands except ``mojito create`` must be run from within the application
directory.


.. _mj_cmdlne-help:

Help
====

To show top-level help for this command line tool:

``$ mojito help``

To show help for a specific command:

``$ mojito help <command>``

.. _mj_cmdlne-create_code:

Creating Code from Archetypes
=============================

Archetypes are used to create skeletons for the different types of artifacts 
in a Mojito application. The skeletons only contain stripped down boilerplate 
code that is easier to create using the command-line tool rather than by hand.

- To create a skeleton for a Mojito application:

  ``$ mojito create app [<archetype-name>] <path/to/app-name>``

  This will create an empty application (i.e., one with no mojits) with the name 
  provided. The application is created in a directory named ``<app-name>`` within 
  the specified directory. If no archetype name is provided, the default archetype 
  is used.

- From the application directory, use the following command to create a skeleton 
  for a mojit:

  ``$ mojito create mojit [<archetype-name>] <mojit-name>``

  This will create an empty mojit with the name provided. The command assumes it 
  is being executed within an application directory. Thus, the mojit is created 
  in a directory named ``<mojit-name>`` within a ``mojits`` subdirectory of the 
  application directory. For example, the mojit ``MyMojit`` would be created in 
  ``mojits/MyMojit``.

  As with application creation, if no archetype name is provided, the default 
  archetype is used. Depending upon the archetype, the skeleton may include any 
  or all of the controller, model, view, and binder.


.. _mj_cmdlne-archetype:

Mojito Archetypes
-----------------

Mojito offers the following three archetypes for applications and mojits.

- ``simple`` - The minimal configuration and code needed to run an application.
- ``default`` - This archetype is run if no command-line archetype option is 
  specified. It is a happy medium between ``simple`` and ``full``.
- ``demo`` - This archetype is only availabe for applications, but let's you 
  create demo applications. The built-in demo application is the quickstart 
  application that serves documentation and serves as an example application.
- ``full`` - Provides the most comprehensive configuration and code for 
  applications.

.. _archetype-custom:

Custom Archetypes
#################
 
You can also customize archetypes and then use them to generate code. 
For example, if you customized the template code that Mojito uses (or create your own)
to generate boilerplate code, such as an application or a mojit,
you could have Mojito use your archetype to generate code with the following command:

``$ mojito create mojit <archetype-path> <generated_code-path>``

For example: ``$ mojito create app ./my_archetypes/mobile_web_app ./my_apps/micro_blog``


.. _mj_cmdlne-testing:

Testing
=======

Unit tests are run using YUI Test invoked using the Mojito command-line tool. 
Test output is written to the console and also to the file 
``{CWD}/artifacts/test/result.xml``, where ``{CWD}`` is the current working directory. 
Note when you run tests, the output may overwrite the results of past tests. To avoid this,
you can use the long option ``--directory`` or an abbreviation such as ``--dir`` to
to specify where to write test results.


- To run tests for an application:

  ``$ mojito test [app] [.]``

- To run the unit tests for a specific mojit, use one of the following:

  - ``$ mojito test mojit <mojit-path>``
  - ``$ mojito test mojit <mojit-name>``

- To run the unit tests for one or more modules of a mojit, use one of the following:

  - ``$ mojito test mojit <mojit-path> --testname <mojit-module1> --testname <mojit-module2> --testname <mojit-module3>``
  - ``$ mojito test mojit <mojit-path> <mojit-module1>,<mojit-module2>,<mojit-module3>``


   If one or more mojit modules (i.e., the YUI modules for a portion of the mojit) are 
   specified, only the tests for those modules will be run. Otherwise all tests 
   for the mojit will be run.

- To specify the output directory for test results:


  - ``$ mojito test [app] --directory <test_results-path>``
  - ``$ mojito test mojit <mojit-path> --dir <test_results-path>``

To run functional and unit tests for the Mojito framework,
you would use the test framework `Yahoo Arrow <https://github.com/yahoo/arrow>`_.
Follow the instructions in `Running Mojito’s Built-In Tests <../topics/mojito_testing.html#running-mojito-s-built-in-tests>`_
to run the framework tests for Mojito.

.. _mj_cmdlne-code_coverage:

Code Coverage
-------------

Code coverage is invoked in the same way as unit testing, but with the added 
option ``--coverage`` or ``-c``. To run code coverage tests, you need to have 
Java installed. You can specify where to write the coverage results  using the option
``--directory`` or ``--dir``. Coverage results are written to  
the directory ``{CWD}/artifacts/test/coverage/`` by default. You can also specify the 
path to write results with the long option ``--directory`` or an abbreviation such as 
``--dir`` (see example below).


- To run code coverage for a Mojito application:

  ``$ mojito test [app] --coverage``

- To run code coverage for a specific mojit:

  ``$ mojito test -c [mojit] <mojit-path>``

- To specify the output directory for test results:

  - ``$ mojito test [app] -c --directory <path>``
  - ``$ mojito test mojit -c <mojit-path> --dir <path>``

  The coverage results will be written to ``<path>/coverage/``.

.. _mj_cmdlne-start_server:

Starting the Server
===================

Use the following to start the server and run the application.

``$ node app.js``

.. note:: Mojito v0.8 and earlier used the Mojito CLI utility to start
          applications (``mojito start``). If you have an older application
          you need to create an ``app.js`` first and then use ``node app.js``
          to start the application.

.. _mj_cmdlne-js_lint:

Sanitizing Code
===============

Static code analysis is run using JSLint invoked using the Mojito command-line 
tool. By default, the JSLint error report is written to 
``{CWD}/artifacts/jslint/jslint.html``. You can also specify the directory to
write the error report to with the long option ``--directory`` or an abbreviation such 
as ``--dir``.

- To run JSLint on the Mojito framework code:

  ``$ mojito jslint mojito``

- To run JSLint on an application, including its mojits:

  ``$ mojito jslint app .``

- To run JSLint on a specific mojit:

  ``$ mojito jslint mojit <mojit-path>``

- To run JSLInt on all the files in a path:

  ``$ mojito jslint [<path>]``

- To write the error report to a specific directory:

  ``$ mojito jslint app . --dir <path>``

.. _mj_cmdlne-document_code:

Documenting Code
================

API documentation is generated using `YUI Doc <https://developer.yahoo.com/yui/yuidoc/>`_, 
which is invoked using the Mojito command-line tool. Documentation output is 
written to files in the locations specified below. Because it's based on YUI Doc,
you can start a server that displays the documentation with the option ``--server`` and 
specify a port with ``--port``. You can also specify the output directory with the 
the option ``--directory`` or an abbreviation such as ``--dir``.

- To generate documentation for the Mojito framework itself:

  ``$ mojito docs mojito``

  Output is written to ``{CWD}/artifacts/docs/mojito/``, where ``{CWD}`` is 
  the current working directory.

- To generate documentation for an application, including all of its (owned) 
  mojits, run the following from the application directory:

  ``$ mojito docs app``

  Output is written to ``{app-dir}/artifacts/docs/``.

- To generate documentation for a specific mojit, run one of the following:

  - ``$ mojito docs mojit <mojit-path>``
  - ``$ mojito docs mojit <mojit-name>``

  Output is written to ``{app-dir}/artifacts/docs/mojits/{mojit-name}/``.

- To start a server for the documentation:

  ``$ mojito docs app --server [--port <port_number>]``

.. _mj_cmdlne-version_info:

Version Information
===================

- To display the version of the ``mojito-cli`` package:

  ``$ mojito version``

- To show the version of an application and the locally installed version of Mojito: 

  ``$ mojito version app``

- To show the version for a mojit, run the following from the application 
  directory:

  ``$ mojito version mojit <mojit-name>``

.. note:: Showing the version of the application and mojit requires that they have a 
          ``package.json`` file.

.. _mj_cmdline-dependency:

Dependency Graphs (Deprecated)
==============================

The command below generates the Graphviz file ``{CWD}/artifacts/gv/yui.client.dot`` 
(``{CWD}`` represents the current working directory) that describes the YUI module 
dependencies.

``$ mojito gv``

The ``mojito gv`` command has the following options:

- ``--client`` - inspects the files that have ``client`` and ``common`` as the affinity. 
  The default is just to inspect files that have ``server`` and ``common`` as the affinity. 
  For example, using the ``--client`` option, the file ``controller.client.js`` and 
  ``controller.common.js`` will be inspected.
- ``--framework`` - also inspects the Mojito framework files.

.. note:: To render the Graphviz files into GIF images, you need the `Graphviz - Graph 
          Visualization Software <http://www.graphviz.org/Download..php>`_.


No Longer Supported
===================

As of Mojito v0.9, several command-line features are no longer supported.
The following sections discuss what's not supported and offer possible solutions.

.. _mj_cmdline-mojito_start:

mojito start
------------

Applications are no longer started with ``mojito start``. You start applications
with ``node`` and the boot file ``app.js``:

``$ node app.js``

.. _mj_cmdlne-build_sys:

mojito build
------------

As of Mojito v0.9, the ``build`` command for creating HTML5 applications 
from Mojito applications is no longer available.

.. _mj_cmdline-context:

Specifying Context: --context
-----------------------------

You can no longer specify the base context with the ``--context`` option. 
To specify a base context in Mojito v0.9 and later, you pass a ``context`` object
to the ``extend`` method in the file ``app.js``. In the example snippet from 
``app.js`` below, the application when started will use the base context ``environment:staging``:

.. code-block:: javascript

   var express = require('express'),
       libmojito = require('mojito'),
       app = express();

   libmojito.extend(app, {
       context: {
           runtime: 'server',
           environment: 'staging'
       }
   });

Learn more about contexts in `Using Context Configurations <../topics/mojito_using_contexts.html>`_.

