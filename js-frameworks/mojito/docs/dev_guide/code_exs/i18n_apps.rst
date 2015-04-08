===================================
Internationalizing Your Application
===================================

**Time Estimate:** 15 minutes

**Difficulty Level:** Intermediate

.. _code_exs_intl-summary:

Summary
=======

This example shows how to use the i18n support built into Mojito that includes top-level 
defaults and the capability to override the default languages of countries.

The following topics will be covered:

- including the `YUI Internationalization utility <https://developer.yahoo.com/yui/3/intl/>`_ 
  in the mojit controller
- using the `Intl addon <../../api/classes/Intl.common.html>`_
- specifying the `BCP 47 <ftp://ftp.rfc-editor.org/in-notes/bcp/bcp47.txt>`_ language tags. 
  BCP 47 is currently the combination of `RFC 5646 <http://tools.ietf.org/html/rfc5646>`_ 
  and `RFC 4647 <http://tools.ietf.org/html/rfc4647>`_
- specifying the resource bundles for the YUI Internationalization utility

.. _code_exs_intl-notes:

Implementation Notes
====================

.. _intl_notes-res_bundles:

Resources Bundles for Languages
-------------------------------

Mojito uses the `YUI 3 Internationalization <https://developer.yahoo.com/yui/3/intl/#switchingLangs>`_ 
utility to support internationalization. To use the YUI Internationalization utility in 
Mojito, you create resource bundles in JSON that specify the keys and values for the 
strings that need localizing. 

.. _res_bundles-loc:

Location of Resource Bundles
############################

These resource bundles are JavaScript files that are placed 
in the ``lang`` directory of the mojit.

This code example has the following three resource bundles in ``lang`` directory of the 
``i18n`` mojit:

::

   /mojits/i18n/lang
               /i18n_en-US.js
               /i18n_en-AU.js
               /i18n_fr-FR.js

.. _res_bundles-naming:

Naming Conventions
##################

.. _res_bundles_naming-file:

File Names
**********

Resource bundle files use the following naming convention:

``{mojit_name}_{BCP 47 tag}.js``

.. _res_bundles_naming-yui:

YUI Module Name
***************

The YUI module name that is registered in the resource bundle file with ``YUI.add``
must have the following syntax: ``'lang/{mojit_name}_{BCP 47 Tag}.js'``

For example, the YUI module name of the resource bundle for US English 
of the mojit ``i18n`` would be ``'lang/i18n_en-US'``.

Example
#######

From the content of the ``i18n_en-US.js`` resource bundle below, you see that the ``add`` 
method specifies the module, the `BCP 47 <ftp://ftp.rfc-editor.org/in-notes/bcp/bcp47.txt>`_ 
language tag, and the ``TITLE`` key with its value. The YUI Internationalization utility 
is included by adding the string ``'intl'`` to the ``requires`` array. The
YUI module name also 

.. code-block:: javascript

   YUI.add("lang/i18n_en-US", function(Y, NAME) {
     Y.Intl.add(
       "i18n",  // associated mojit
       "en-US",    // BCP 47 language tag
       // key-value pairs for this module and language
       {
         TITLE: "Hello!",
       }
     );
   }, "3.1.0", {requires: ['intl']});

.. _intl_notes-using_addon:

Using the intl Addon
--------------------

In the ``controller.server.js`` file below, the ``intl.lang`` and ``intl.formData`` 
methods rely on the YUI Internationalization utility to select the language and format of 
the title and date. The YUI Internationalization utility uses the ``Intl.lookupBestLang`` 
method to determine the best language based on an application's request and a module's 
language support. You also need to include the 
`Intl addon <../../api/classes/Intl.common.html>`_ by adding the string 
'mojito-intl-addon' to the ``requires`` array.

.. code-block:: javascript

   YUI.add('i18n', function(Y, NAME) {/
     Y.namespace('mojito.controllers')[NAME] = {   

       index: function(ac) {
         // Default.
         ac.done(
           {
             title: ac.intl.lang("TITLE"),
             today: ac.intl.formatDate(new Date())
           }
         );
       }
     };
    }, '0.0.1', { requires: ['mojito-intl-addon']});

.. _intl_notes-run_client:

Configuring a Mojit to Run on Client
------------------------------------

When trying to deliver HTML pages with the language and date format preferred by the user, 
it's best to rely on the user's browser settings. YUI, when running on the client side, 
can detect the browser settings to select the default translation and date format. During 
server-side execution, however, the preferred language and date format is determined by 
the order of languages listed in the mojit controller.

Fortunately, Mojito lets you configure applications to run on either the server or client 
side. Because this code example illustrates how to localize your application, we want to 
configure Mojito to run the application on the client to improve the chances of serving 
content in the user's preferred language and date format.

To configure Mojito to run on the client, you simply set the ``"deploy"`` property to 
``true`` as seen in the ``application.json`` file below.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame" : {
           "type" : "HTMLFrameMojit",
             "config": {
             "deploy": true,
             "child" : {
               "type" : "i18n"
             }
           }
         }
       }
     }
   ]

.. _code_exs_intl-setup:

Setting Up this Example
=======================

To set up and run ``locale_i18n``:

#. Create your application.

   ``$ mojito create app locale_i18n``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit i18n``
#. To configure you application to have the mojit code run on the client, replace the 
   code in ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "frame" : {
              "type" : "HTMLFrameMojit",
              "config": {
                "deploy": true,
                "child" : {
                  "type" : "i18n"
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
          app.get('/', libmojito.dispatch('frame.index'));

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

#. Change to ``mojits/i18n``.
#. Replace the code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('i18n', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   

          index: function(ac) {
            // Default.
            ac.done(
              {
                title: ac.intl.lang("TITLE"),
                today: ac.intl.formatDate(new Date())
              }
            );
          }
        };
      }, '0.0.1', { requires: ['mojito-intl-addon']});

#. To add the resource bundle for American English, create the file ``lang/i18n_en-US.js`` 
   with the following:

   .. code-block:: javascript

      YUI.add("lang/i18n_en-US", function(Y, NAME) {
        Y.Intl.add(
          "i18n",  // associated mojit
          "en-US",    // BCP 47 language tag
          // key-value pairs for this module and language
          {
            TITLE: "Hello!"
          }
        );
      }, "3.1.0", {requires: ['intl']});

#. To add the resource bundle for French, create the file ``lang/i18n_fr-FR.js`` with the 
   following:

   .. code-block:: javascript

      YUI.add("lang/i18n_fr-FR", function(Y, NAME) {
        Y.Intl.add(
          "i18n",  // associated mojit
          "fr-FR",    // BCP 47 language tag
          // key-value pairs for this module and language
          {
            TITLE: "Tiens!"
          }
        );
      }, "3.1.0", {requires: ['intl']});

#. To add the resource bundle for Australian English, create the file 
   ``lang/i18n_en-AU.js`` with the following:

   .. code-block:: javascript

      YUI.add("lang/i18n_en-AU", function(Y, NAME) {
        Y.Intl.add(
          "i18n",  // associated mojit
          "en-AU",    // BCP 47 language tag
          // key-value pairs for this module and language
          {
            TITLE: "G'day!"
          }
        );
      }, "3.1.0", {requires: ['intl']});

#. To modify the index template to show a localized message, replace the code in 
   ``views/index.hb.html`` with the following:

   .. code-block:: javascript

      <div id="{{mojit_view_id}}"class="mojit">{{title}} -- {{today}}</div>

#. From the application directory, run the server.

   ``$ node app.js``
#. To view your application in the default language used by your browser, go to the URL:

   http://localhost:8666

#. Configure your browser to use French as the default language. To change the language 
   preferences of Firefox or Chrome, see the 
   `Firefox instructions <http://support.mozilla.com/en-US/kb/Options%20window%20-%20Content%20panel?s=change+preference+language&as=s#w_languages>`_ 
   and `Chrome instructions <http://www.google.com/support/chrome/bin/answer.py?hl=en&answer=95416&from=95415&rd=1>`_.

#. Now go to your `application URL <http://localhost:8666>`_ and see the page display 
   French.
#. To force the page to display a specific language and date format, you can also use the 
   query string parameter ``lang.`` The URL below uses the ``lang`` parameter to display 
   the page in Australian English:

   http://localhost:8666?lang=en-AU

.. _code_exs_intl-src:

Source Code
===========

- `Resource Bundles for Languages <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/locale_i18n/mojits/i18n/lang/>`_
- `Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/locale_i18n/mojits/i18n/controller.server.js>`_
- `Internationalization Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/locale_i18n/>`_


