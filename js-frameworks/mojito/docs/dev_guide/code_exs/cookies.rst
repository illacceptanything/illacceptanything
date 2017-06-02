=============
Using Cookies
=============

**Time Estimate:** 10 minutes

**Difficulty Level:** Beginning

.. _code_exs_cookies-summary:

Summary
=======

This example shows how to read and write cookies in a Mojito application.

The following topics will be covered:

- using the `Params addon <../../api/classes/Params.common.html>`_ from the
  ``actionContext`` object
- getting and setting cookies from the mojit controller
- using the `Cookie addon <../../api/classes/Cookie.server.html>`_ and the
  `YUI Cookie module <https://developer.yahoo.com/yui/3/cookie/>`__ to get and set cookies

.. _code_exs_cookies-notes:

Implementation Notes
====================

To access many methods on the Mojito JavaScript library, you use
`ActionContext addons <../../api/classes/ActionContext.html>`_. In this code example, the
`Cookie addon <../../api/classes/Cookie.server.html>`_ is used to call the methods
``getCookie`` and ``setCookie`` to get and set cookies.

The ``index`` function in the ``controller.server.js`` below shows how to use ``cookie.get``
and ``cookie.set``.  The ``cookie.set`` method also allows you to pass a third parameter
that contains the domain, the path, and the expiration date of the cookie. For those
familiar with YUI 3, these methods for getting and setting cookies should be familiar as
Mojito uses the `YUI 3 Cookie Module <https://developer.yahoo.com/yui/3/api/Cookie.html>`_.

.. code-block:: javascript

   YUI.add('cookie', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {
       index: function(actionContext) {
         var requestCookieValue = actionContext.cookie.get('request_cookie');
         // Or use this API to set a session cookie
         // with default properties set by Mojito
         actionContext.cookie.set("response_cookie", "Hello from the server!");
         actionContext.done(
           {
             title: "Cookie Demo",
             request_cookie_value: requestCookieValue
           }
         );
       }
     };
   }, '0.0.1', {requires: ['mojito-cookie-addon']});

The code below from the ``index`` template interpolates the value of the variable
``{{request_cookie_value}}`` from the controller and uses the
`YUI Cookie module <https://developer.yahoo.com/yui/3/api/module_cookie.html>`__ to set and
get a cookie. To use the YUI Cookie module, first include the module with ``YUI().use``
and then call ``Y.Cookie.get`` and ``Y.Cookie.set``.

.. code-block:: html

   <div id="{{mojit_view_id}}" class="mojit">
     <h2>{{title}}</h2>
     <div>
       <p>This is a demo showing how to read read cookies from browser, and how to write
       cookies to browser from the Mojit.</p>
     </div>
     <div>
       <p>Value of request cookie sent by browser: {{request_cookie_value}}</p>
     </div>
     <div>
       <p>The response cookie written by the server should be displayed in browser as an alert.</p>
     </div>
   </div>
   <script type="text/javascript" src="http://yui.yahooapis.com/3.3.0/build/yui/yui-min.js"></script>
   <script type="text/javascript">YUI().use('cookie', 'node', function(Y) {
     // Create a request cookie and set its value
     Y.Cookie.set('request_cookie', 'request cookie value');
     function showResponseCookie() {
       var responseCookieValue = Y.Cookie.get('response_cookie');
       alert('Response Cookie Value: ' + responseCookieValue);
     }
     Y.on('domready', showResponseCookie);
     });
   </script>

.. _code_exs_cookies-setup:

Setting Up this Example
=======================

To set up and run ``using_cookies``:

#. Create your application.

   ``$ mojito create app using_cookies``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit Cookie``
#. To configure your application to use the ``HTMLFrameMojit`` and its child mojit
   ``Cookie``, replace the code in ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "frame": {
              "type": "HTMLFrameMojit",
              "config":{
                "child": {
                  "type": "Cookie"
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
      app.get('/example1', libmojito.dispatch('frame.example1'));

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
#. Change to ``mojits/Cookie``.
#. To set a cookie from your controller, replace the code in ``controller.server.js`` with
   the following:

   .. code-block:: javascript

      YUI.add('Cookie', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {

          index: function(actionContext) {
            var requestCookieValue = actionContext.cookie.get('request_cookie');
            // Or use this API to set a session cookie
            // with default properties set by Mojito
            actionContext.cookie.set("response_cookie", "Hello from the server!");
            actionContext.done(
              {
                title: "Cookie Demo",
                request_cookie_value: requestCookieValue
              }
            );
          }
        };
      }, '0.0.1', {requires: ['mojito-cookie-addon']});

#. To display the cookie values set in your controller, replace the code in
   ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <h2>{{title}}</h2>
        <div>
          <p>This is a demo showing how to read read cookies from browser, and how to write cookies to browser from the Mojit.</p>
        </div>
        <div>
          <p>Value of request cookie sent by browser: {{request_cookie_value}}</p>
        </div>
        <div>
          <p>The response cookie written by the server should be displayed in browser as an alert.</p>
        </div>
      </div>
      <script type="text/javascript" src="http://yui.yahooapis.com/3.3.0/build/yui/yui-min.js"></script>
      <script type="text/javascript">YUI().use('cookie', 'node', function(Y) {
        // Create a request cookie and set its value
        Y.Cookie.set('request_cookie', 'request cookie value');
        function showResponseCookie() {
          var responseCookieValue = Y.Cookie.get('response_cookie');
          Y.log('RESPONSE COOKIE VALUE: ' + responseCookieValue);
          alert('Response Cookie Value: ' + responseCookieValue);
        }
        Y.on('domready', showResponseCookie);
        });
      </script>

#. From the application directory, run the server.

   ``$ node app.js``
#. To view your application, go to the URL below:

   http://localhost:8666

.. _code_exs_cookies-src:

Source Code
===========

- `Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/using_cookies/mojits/CookieMojit/controller.server.js>`_
- `Using Cookie Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/using_cookies/>`_
