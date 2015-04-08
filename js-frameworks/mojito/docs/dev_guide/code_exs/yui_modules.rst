=====================
Including YUI Modules
=====================

**Time Estimate:** 15 minutes

**Difficulty:** Intermediate

.. _code_exs-incl_yui_mods-summary:

Summary
=======

This example shows how to include the YUI module 
`Storage Lite <http://yuilibrary.com/gallery/show/storage-lite>`_ in a Mojito 
application. The example uses the Storage Lite module to create a notepad 
application. Any text that you input into the application will persist between 
page views and browser sessions.

The following topics will be covered:

- adding YUI modules to the ``yui_modules`` directory
- accessing YUI modules from a mojit

.. _code_exs-incl_yui_mods-notes:

Implementation Notes
====================

.. _yui_mod_impl-add:

Adding YUI Modules
------------------

.. _yui_mod_impl_add-loc:

Location
########

To add YUI modules that all your mojits can access, place the modules in the 
``yui_modules`` directory under the application directory. For example, YUI 
modules in the ``hello_world`` application would be placed in 
``hello_world/yui_modules``.

.. _yui_mod_impl_add-naming:

File Naming Convention
######################

YUI modules must use the following naming convention, where where 
``{module_name}`` is an arbitrary string for identifying the module and 
``{affinity}`` is either ``common``, ``server``, or ``client``.

``{module_name}.{affinity}.js``

In this code example, code is being deployed to the client, so the affinity 
must be either ``common`` or ``client``.

.. _registering_module:

Registering Module
##################

To register a module so that it is available to your mojits, pass a string that 
identifies the module to the ``YUI.add`` method. From the skeleton of 
``storage-lite.client.js`` below, you can see that ``add`` method registers 
the module identified by the string ``'gallery-storage-lite'``.

.. code-block:: javascript

   YUI.add('gallery-storage-lite', function (Y) {
      ...
   }, '1.0.0', { requires: [ 'event-base', 'event-custom', 'event-custom-complex', 'json']});


.. _yui_mod_impl-using:

Using a YUI Module from Mojits
------------------------------

After registered YUI modules have been added to the ``yui_modules`` directory, you 
can load them into your mojit code by listing them as dependencies in the 
``requires`` array. In the binder ``index.js`` below, you can see that the 
Storage Lite module that we created and registered in :ref:`registering_module` 
is listed as a dependency in the ``requires`` array.

.. code-block:: javascript

   YUI.add('notepad-binder-index', function (Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mp = mojitProxy;
       },
       bind: function(node) {
         ...
       }
     };
     // See yui_modules/storage-lite.client.js
   }, '0.0.1', {requires: [  'gallery-storage-lite' ]});

In the ``bind`` method, ``Y.StorageLite.getItem`` and ``Y.StorageLite.setItem`` 
are used to get and set persistent data. Note that you must use the ``Y`` 
instance to access the module.

.. code-block:: javascript

   ...
     bind: function (node) {

       // Based on http://yuilibrary.com/gallery/show/storage-lite
       var keyname = 'storage-lite-example',
           notes = node.one('#notes');

       // Populate the textarea with the stored note text on page load.
       notes.set('value', Y.StorageLite.getItem(keyname) || '');

       // Save the contents of the textarea after each keystroke.
       notes.on('keyup', function () {
         Y.StorageLite.setItem(keyname, notes.get('value'));
       });

       // adding a classname to the notes element to facilitate func tests
       notes.addClass('ready');
     }
   ...

.. _code_exs-incl_yui_mods-setup:

Setting Up this Example
=======================

To set up and run ``yui_module``:

#. Create your application.

   ``$ mojito create app yui_module``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit Notepad``
#. To specify that your application use the ``Notepad`` mojit and be deployed 
   to the client, replace the code in ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "notepad": {
              "type": "HTMLFrameMojit",
              "config": {
                "deploy": true,
                "title": "Notepad Example",
                "child": {
                  "type": "Notepad"
                }
              }
            }
          }
        }
      ]


#. Update your ``app.js`` with the following to use Mojito's middleware, configure routing and the port, and 
   have your application listen for requests:

   .. code-block:: javascript

      'use strict';

      var debug = require('debug')('app'),
          express = require('express'),
          libmojito = require('mojito'),
          app;

          app = express();
          app.set('port', process.env.PORT || 8666);
          libmojito.extend(app);

          app.use(libmojito.middleware());

          app.get('/status', function (req, res) {
              res.send('200 OK');
          });
          app.get('/', libmojito.dispatch('notepad.index'));

          app.listen(app.get('port'), function () {
              debug('Server listening on port ' + app.get('port') + ' ' +
              'in ' + app.get('env') + ' mode');
          });
          module.exports = app;

#. Confirm that your ``package.json`` has the correct dependencies as show below. If not,
   update ``package.json``.

   .. code-block:: javascript

      "dependencies": {
          "debug": "*",
           "mojito": "~0.9.0"
      },
      "devDependencies": {
          "mojito-cli": ">= 0.2.0"
      },

#. From the application directory, install the application dependencies:

   ``$ npm install``

#. Create the ``yui_modules`` directory for storing the Storage Lite module.

   ``$ mkdir yui_modules``
#. Get the Storage Lite module and place it in the ``yui_modules`` directory.

   ``$ wget -O yui_modules/storage-lite.client.js https://raw.github.com/rgrove/storage-lite/master/src/storage-lite.js --no-check-certificate``
#. Change to ``mojits/Notepad``.
#. Replace the code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('notepad', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   
          index: function(ac) {
            ac.done();
          }
        };
      }, '0.0.1', {requires: ['mojito']});

#. To create the binder for getting user input and storing it with the 
   Storage Lite module, replace the code in ``binders/index.js`` with the following:

   .. code-block:: javascript

      YUI.add('notepad-binder-index', function (Y, NAME) {

        Y.namespace('mojito.binders')[NAME] = {
          init: function (mojitProxy) {
            this.mp = mojitProxy;
          },
          bind: function (node) {

            // Based on http://yuilibrary.com/gallery/show/storage-lite
            var keyname = 'storage-lite-example',
                notes = node.one('#notes');

            // Populate the textarea with the stored note text on page load.
            notes.set('value', Y.StorageLite.getItem(keyname) || '');

            // Save the contents of the textarea after each keystroke.
            notes.on('keyup', function () {
                Y.StorageLite.setItem(keyname, notes.get('value'));
            });

            // adding a classname to the notes element to facilitate func tests
            notes.addClass('ready');
          }
        };
      }, '0.0.1', {
        requires: [ 
          'gallery-storage-lite' //see yui_modules/storage-lite.client.js
        ]
      });

#. To display a form that allows users to input text, replace the code in 
   ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}">
        <h1>Storage Lite: Simple Notepad Example</h1>
        <form>
          <p>Anything you type in this textarea will
          be stored and persisted between page views and browser sessions using the 
          <a href="http://github.com/rgrove/storage-lite/">Storage Lite</a> YUI module by 
          Ryan Grove.</p>
          <p><textarea id="notes" cols="80" rows="8"></textarea>
          </p>
        </form>
      </div>

#. From the application directory, run the server.

   ``$ node app.js``
#. Go to the application at the URL below and enter some text into the form.

   http://localhost:8666/
#. Point to the same URL in a new tab. You should see the same text that you entered in 
   the form before.
#. Open the same URL in a new browser window. Once again, you should see the same text 
   that you entered earlier.


.. _code_exs-incl_yui_mods-src:

Source Code
===========

- `YUI Module App <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/yui_module/>`_
- `Mojit Binder <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/yui_module/mojits/Notepad/binders/index.js>`_


