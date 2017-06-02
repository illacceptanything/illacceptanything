=======================================================
Startup Requirements for Mojito in Hosting Environments
=======================================================

This chapter discusses the startup files needed to launch Mojito applications 
in a hosting environment. Because different versions of Mojito use different 
startup files, you may need to modify or even remove certain startup files in 
your applications, so that they may be launched in hosting environments. We 
will look at what startup files are required for each version and provide 
code examples.

.. _startup_reqs-v0.9:

Mojito v0.9 and Later
=======================

Version 0.9 removes the ``server.js`` and requires the addition of ``app.js`` to 
boot applications. The ``app.js`` is structured much like the ``app.js`` of Express
applications with Mojito included to dispatch mojits, attach routing and application configurations,
and providing middleware. 

Although your ``app.js`` can have many forms, from using Express-like syntax for defining 
routing paths, adding middleware, etc., to using Mojito to do those same things, the 
example ``app.js`` below can be the starting point or template for your applications.

.. _startup_reqs-v0.9-appjs:

app.js
------

.. code-block:: javascript

   /*jslint node:true*/

   'use strict';

   var express = require('express'),
       libmojito = require('mojito'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.use(libmojito.middleware());
   app.mojito.attachRoutes();

   app.get('/status', function (req, res) {
       res.send('200 OK');
   });

   app.listen(app.get('port'), function () {
       console.log('Server listening on port ' + app.get('port') + ' ' +
                   'in ' + app.get('env') + ' mode');
   });
   module.exports = app;


.. _startup_reqs-v0.9-appjs:

server.js
---------

**OBSOLETE**

The ``server.js`` file is not created by version 0.9. Remove from any 
applications that are using versions 0.9 or greater.

.. _startup_reqs-v0.4.8:

Mojito v0.4.8 and Later
=======================

Version 0.4.8 solidifies the changes made in version 0.4.7, removing the
``index.js`` file from any application archetypes (the files used to create 
new applications) and the unnecessary commented-out code in the ``server.js``
file. As with applications created by version 0.4.7, you should remove 
the ``index.js`` file from any applications using version 0.4.8 or later 
versions and update your ``server.js`` file to match the one provided below.

.. _startup_reqs_v0.4.8-index:

index.js
--------

**OBSOLETE**

The ``index.js`` file is not created by version 0.4.7. Remove from any 
applications that are using versions 0.4.7 or greater.

.. _startup_reqs_v0.4.8-server:

server.js
---------

.. code-block:: javascript

   /*
   * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
   * Copyrights licensed under the New BSD License.
   * See the accompanying LICENSE file for terms.
   */

   /*jslint anon:true, sloppy:true, nomen:true*/

   process.chdir(__dirname);

   /*
   * Create the MojitoServer instance we'll interact with. Options can be passed
   * using an object with the desired key/value pairs.
   */
   var Mojito = require('mojito');
   var app = Mojito.createServer();

   // ---------------------------------------------------------------------------
   // Different hosting environments require different approaches to starting the
   // server. Adjust below to match the requirements of your hosting environment.
   // ---------------------------------------------------------------------------

   module.exports = app.listen();

.. _startup_reqs-v0.4.7:

Mojito v0.4.7
=============


Version 0.4.7 repaired an issue with a specific hosting container and replaced
the ``start`` method with a ``listen`` wrapper method and a ``getHttpServer`` method
to provide access to the Node.js ``http.Server`` instance being used. This
approach makes it possible for Mojito to support an even broader range of
hosting containers and startup requirements.

Applications running version 0.4.7 or greater no longer require an ``index.js``
file, although one is still provided. Such applications must use the ``server.js``
file shown below.

.. _startup_reqs_v0.4.7-index:

index.js
--------

**OBSOLETE**

Version 0.4.7 still creates the ``index.js`` file, but is not used. Remove the
``index.js`` file from any applications using version 0.4.7 or greater.

.. _startup_reqs_v0.4.7-server:

server.js
---------

.. code-block:: javascript

   /*
   * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
   * Copyrights licensed under the New BSD License.
   * See the accompanying LICENSE file for terms.
   */

   /*jslint anon:true, sloppy:true, nomen:true*/

   process.chdir(__dirname);

   /*
   * Create the MojitoServer instance we'll interact with. Options can be passed
   * using an object with the desired key/value pairs.
   */
   var Mojito = require('mojito');
   var app = Mojito.createServer();

   // ---------------------------------------------------------------------------
   // Different hosting environments require different approaches to starting the
   // server. Adjust below to match the requirements of your hosting environment.
   // ---------------------------------------------------------------------------

   /*
   * Manhattan
   *
   module.exports = function(config, token) {
     process.emit('application-ready', token, app.getHttpServer());
   };
   */

   /*
   * Localhost and others where the default port/host combinations work.
   * You can provide port, host, callback parameters as needed.
   */
   module.exports = app.listen();

.. _startup_reqs-v0.4.6:

Mojito v0.4.6
=============

**NOT RECOMMENDED**

Version 0.4.6 has been found not to work with at least one hosting container
due to changes in how a Mojito server instance is created and the
API of that instances. See :ref:`server.js <startup_reqs_v0.4.6-server>`
for details of the changes. We **recommend** using version 0.4.7 or
greater. 

.. _startup_reqs_v0.4.6-index:

index.js
--------

The ``index.js`` file does not change for version 0.4.6.


.. _startup_reqs_v0.4.6-server:

server.js
---------

For version 0.4.6, the ``server.js`` file changes due to changes in how a Mojito
server instance is created and the API of that instance. In this version of
Mojito, there is a ``start`` method on the Mojito server that is used to launch a
new server. Unfortunately, while this approach works, it retains some limitations
and created an issue with at least one hosting container.

.. code-block:: javascript

   /*
   * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
   * Copyrights licensed under the New BSD License.
   * See the accompanying LICENSE file for terms.
   */

   /*jslint anon:true, sloppy:true*/

   /**
   * Create and start a new Mojito server/application.
   */

   var Mojito = require('mojito');
   var app = Mojito.createServer();

   module.exports = app.start();


.. _startup_reqs_v0.4.6-npm:

npm start
---------

Version 0.4.6 is the first version of Mojito to support ``npm start`` in addition
to ``mojito start`` as a means for starting up a new Mojito server instance.

Mojito version 0.4.6 alters Mojito's startup logic to support ``npm start`` as a
common startup mechanism and expands the number of hosting containers Mojito was
compatible with.


.. _startup_reqs-v0.4.5:

Mojito v0.4.5 and Earlier Versions
==================================

Version 0.4.5 and earlier versions rely exclusively on ``mojito start`` to 
run a new Mojito server instance, which means the ``index.js`` and ``server.js`` 
files are both required. Applications using Mojito version 0.4.5 and prior 
versions should use ``index.js`` and ``server.js`` files matching those 
below.

.. _startup_reqs_v0.4.5-index:

index.js
--------

.. code-block:: javascript

   /*
   * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
   * Copyrights licensed under the New BSD License.
   * See the accompanying LICENSE file for terms.
   */


   /*jslint anon:true, sloppy:true, nomen:true, node:true*/

   process.chdir(__dirname);

   /**
   * @param {object} config The configuration object containing processing params.
   * @param {object} token Token used to identify the application.
   */
   module.exports = function(config, token) {
     var app = require('./server.js');

     // Signal the application is ready, providing the token and app references.
     process.emit('application-ready', token, app);
   };

.. _startup_reqs_v0.4.5-server:

server.js
---------


.. code-block:: javascript

   /*
   * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
   * Copyrights licensed under the New BSD License.
   * See the accompanying LICENSE file for terms.
   */

   /*jslint anon:true, sloppy:true*/

   /**
   * Returns a new Mojito server instance.
   */
   module.exports = require('mojito').createServer();








