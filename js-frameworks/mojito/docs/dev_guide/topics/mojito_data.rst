==============
Data in Mojito
==============

This chapter will first discuss how your applications can get 
data and then look at how mojits can share that data with
binders, other mojits, as well as how to re-hydrate data.

.. _mojito_data-getting:

Getting Input and Cookie Data
=============================

.. _mojito_data-intro:

Introduction
------------

Mojito provides addons for accessing data from query string and routing 
parameters, cookies, and the POST request body.

This section will provide an overview of the following addons that allow you 
to access data:

- `Params addon <../../api/classes/Params.common.html>`_
- `Cookies addon <../../api/classes/Cookie.server.html>`_

To see examples using these addons to get data, see 
`Using Query Parameters <../code_exs/query_params.html>`_ and 
`Using Cookies <../code_exs/cookies.html>`_.


.. _mojito_data-params:

Getting Data from Parameters
----------------------------

The methods in the Params addon are called from the ``params`` namespace. 
As a result, the call will have the following syntax where ``ac`` is the 
ActionContext object: ``ac.params.*``

.. _mojito_data-params_get:

GET
###

The GET parameters are the URL query string parameters. The Params addon 
creates JSON using the URL query string parameters. The method ``getFromUrl`` 
allows you to specify a GET parameter or get all of the GET parameters. You 
can also use the alias ``url`` to get URL query string parameters.

For example, for the URL ``http://www.yahoo.com?foo=1&bar=2``, the Params 
addon would create the following object:

.. code-block:: javascript

   {
     foo: 1,
     bar: 2
   }


.. _data_params-get_single:

Single Parameter
****************

To get the value for a specific parameter, you pass the key to the ``getFromUrl`` 
method, which returns the associated value.

In the example controller below, the value for the ``name`` query string 
parameter is retrieved:

.. code-block:: javascript

   YUI.add('params', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       getNameParam: function(actionContext) {
         var nameParam = actionContext.params.getFromUrl('name');
         actionContext.done(
           {
             name: nameParam
           },
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});


.. _data_params-get_all:

All Parameters
**************

To get all of the query string parameters, you call ``getFromUrl`` or its alias 
``url`` without passing a key as a parameter.

In this example controller, all of the query string parameter are stored in 
the ``qs_params`` array, which ``ac.done`` makes available in the template.

.. code-block:: javascript

   YUI.add('prams', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       ...
       getAllParams: function(actionContext) {
         var qs_params = [];
         var allParams = actionContext.params.getFromUrl();
         Y.Object.each(allParams, function(param, key) {
           qs_params.push({key: key, value: param});
         });
         actionContext.done(
           {
             query_string: qs_params
           },
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});


.. _mojito_data-params_post:

POST
####

The POST parameters come from the HTTP POST request body and often consist of 
form data. As with query string parameters, the ``Params`` addon has the method 
``getFromBody`` that allows you to specify a single parameter or get all of 
the POST body parameters.

.. _data_params-post_single:

Single
******

To get a parameter from the POST body, call ``getFromBody`` with the key as the 
parameter. You can also use the alias ``body`` to get a parameter from the POST 
body.

In the example controller below, the POST body parameter ``name`` is retrieved 
and then uses the ``done`` method to make it accessible to the template.

.. code-block:: javascript

   YUI.add('params', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       getPostName: function(actionContext) {
         var postName = actionContext.params.getFromBody('name');
         actionContext.done(
           {
             posted_name: postName
           });
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});


.. _data_params-post_all:

All
***

To get all of the parameters from the POST body, call ``getFromBody`` or ``body`` 
without any parameters.

In the example controller below, ``getFromBody`` gets all of the POST body 
parameters, which are then stored in an array and made accessible to the view 
template.

.. code-block:: javascript

   YUI.add('params', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       ...
       getAllParams: function(actionContext) {
         var post_params = [];
         var allPostParams = actionContext.params.getFromBody();
         Y.Object.each(allPostParams, function(param, key) {
           post_params.push({key: key, value: param});
         });
         actionContext.done(
           {
             posted_params: post_params
           }
         )
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});


.. _mojito_data-routing:

Routing
-------

Routing parameters are mapped to routing paths, actions, and HTTP methods. 
You can use the routing parameters to provide data to mojit actions when 
specific routing conditions have been met.

.. _data_routing-set:

Setting Routing Parameters
##########################

The routing parameters are set in ``app.js``. See
`Adding Routing Parameters <../intro/mojito_routing.html#appjs-routing-params>`_
to learn how to set routing parameters.

.. _data_routing-get:

Getting Routing Parameters
##########################


The Params addon has the method ``getFromRoutes`` that allows you to specify 
a single parameter or get all of the routing parameters. You can also use 
the alias ``route`` to get routing parameters.

.. _data_routing-get_single:

Single
******

To get a routing parameter, call ``getFromRoute`` with the key as the 
parameter.

In the example controller below, the routing parameter ``coupon`` is used 
to determine whether the user gets a coupon.

.. code-block:: javascript

   YUI.add('coupon', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(actionContext) {
         var sendCoupon = actionContext.params.getFromRoute('coupon');
         var name = actionContext.params.getFromBody("name");
         if(sendCoupon){
            // Display coupon to user
             var coupon = sendCoupon;
         }
         actionContext.done(
           {
             name: name ? name : "Dear customer";
             coupon : coupon ? coupon : "";
           });
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});



.. _data_routing-get_all:

All
***

To get all of the routing parameters, call ``getFromRoute`` or ``route`` without 
any arguments.

In the example controller below, all of the routing routing parameters to create 
a URL.

.. code-block:: javascript

   YUI.add('link', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(actionContext) {
         var routeParams = actionContext.params.getFromRoute();
         var submitUrl = actionContext.url.make("myMojit", 'submit', routeParams);
         actionContext.done(
           {
             url: submitUrl
           });
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon', 'mojito-url-addon']});

.. _mojito_data-get_all:

Getting All Parameters
----------------------

The Params addon also has the method ``getFromMerged`` that lets you get one or 
all of the GET, POST, and routing parameters. Because all of the parameters are 
merged into one collection, one parameter might be overridden by another with 
the same key. You can also use the alias ``merged`` to
get one or all of the GET, POST, and routing parameters.

Thus, the parameter types are given the following priority:

#. routing parameters
#. GET parameters
#. POST parameters


For example, if each parameter type has a ``foo`` key, the ``foo`` routing 
parameter will override both the GET and POST ``foo`` parameters.

.. _mojito_data-get_single:

Single
######

To get one of any of the different type of parameters, call ``getFromMerged`` 
or ``merged`` with the key as the parameter.

In the example controller below, the ``name`` parameter is obtained using 
``getFromMerged``.

.. code-block:: javascript

   YUI.add('mergedparams', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       getPostName: function(actionContext) {
         var mergedName = actionContext.params.getFromMerged('name');
         actionContext.done(
           {
             name: mergedName
           });
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});


.. _mojito_data-get_all:

All
###

To get all of the GET, POST, and routing parameters, call ``getFromMerged`` or 
``merged`` without any arguments.

.. code-block:: javascript

   YUI.add('mergedparams', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       ...
       getAllParams: function(actionContext) {
         var all_params = [];
         var allParams = actionContext.params.getFromMerged();
         Y.Object.each(allParams, function(param, key) {
           all_params.push({key: key, value: param});
         });
         actionContext.done(
           {
             params: all_params
           }
         )
       }
     }
   }, '0.0.1', {requires: ['mojito-params-addon']});

.. _mojito_params_addon-aliases:

Params Addon Method Aliases
---------------------------

We have looked at the methods of the ``Params`` addon for getting query string
parameter, query string parameters, and HTTP body data. For simplicity,
the ``Params`` addon also provides the aliases below
for the methods that we have covered thus far.

+---------------------+--------------+
| Method              | Alias        | 
+=====================+==============+
| ``getAll``          | ``all``      | 
+---------------------+--------------+
| ``getFromBody``     | ``body``     |
+---------------------+--------------+
| ``getFromFiles``    | ``files``    |
+---------------------+--------------+
| ``getFromMerged``   | ``merged``   |
+---------------------+--------------+
| ``getfromRoute``    | ``route``    | 
+---------------------+--------------+
| ``getFromUrl``      | ``url``      |
+---------------------+--------------+


.. _mojito_data-cookie:

Cookies
-------

The `Cookies addon <../../api/classes/Cookie.server.html>`_ offers methods for 
reading and writing cookies. The API of the Cookie addon is the same as 
the `YUI 3 Cookie Utility <http://yuilibrary.com/yui/docs/api/classes/Cookie.html>`_. 
For a code example showing how to use the Cookies addon, 
see `Using Cookies <../code_exs/cookies.html>`_.

.. _data_cookie-get:

Getting Cookie Data
###################

The method ``cookie.get(name)`` is used to get the cookie value associated 
with ``name``. In the example controller below, the cookie value 
for ``'user'`` is obtained and then used to pass user information to the 
template.

.. code-block:: javascript

   YUI.add('cookie', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(actionContext) {
         var user = actionContext.cookie.get('user');
           actionContext.done(
             {
               user: user && users[user] ? users[user] : ""
             }
           );
         }
       }
     }
   }, '0.0.1', {requires: ['mojito-cookie-addon']});

.. _data_cookies-write:

Writing Data to Cookies
#######################

The method ``cookie.set(name, value)`` is used to set a cookie with the a 
given name and value.  The following example controller sets a cookie 
with the name ``'user'`` if one does not exist.

.. code-block:: javascript

   YUI.add('cookie', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = { 
       index: function(actionContext) {
         var user = actionContext.cookie.get('user');
         if(!user){
           actionContext.cookie.set('user',(new Date).getTime());
         }
         actionContext.done(
           {
             user: user
           }
         );
        }
     }
   }, '0.0.1', {requires: ['mojito-cookie-addon']});

.. _mojito_data-sharing:

Sharing Data
============

Overview
--------

After a mojit gets data, it may need to share that data with binders, 
templates, or other mojits. Mojito provides the ``Data`` addon that allows your 
mojit controllers and binders to share and access data. Data can also be 
refreshed in templates using the ``Data`` addon and Handlebars expressions. 


.. _mojito_data_sharing-how:

How Is Data Shared?
###################

Data is passed from the server to the client and vice versa using a remote 
procedural call (RPC) through a tunnel. During this transmission, the state of 
the data is preserved. 

When content is sent to the client as part of the page, the data and templates 
are rendered on the server and sent to the client through the tunnel. After the 
initial rendering, each time the mojit instance invokes an action that triggers 
an RPC through the tunnel, data is serialized and sent to the server, where the 
instance is recreated with the data. If the action changes the data model, the 
new data is then sent back through the tunnel to the 
client to update the data model.


Benefits 
########

The data sharing model used in Mojito is extremely flexible, allowing you to share data
in your application in many ways. We'll look at how data is shared from the perspective
of controllers and binders, which have access to the ``Data`` addon.

Controllers
***********

Controllers can share data in the following ways:

- rehydrate data of its template or all the templates on the page
- share data with its binders or all the other mojit binders on the page

Binders
*******

Binders can do the following to share/access data:

- invoke an action to update data for another binder that is listening for changes in that 
  data to update the view
- rehydrate data in templates
- share data with other binders on the page
 

Potential Issues
################

When using the ``Data`` addon to share data, you should be aware of a couple of potential
issues. When multiple RPCs are made (e.g., multiple clicks on the same button), there is no
guarantee of the order of execution of the requests, which means you might get stale
data. Also, if your data model contains a lot of information, the payload of the RPC
will negatively affect performance and security as data is transmitted back and forth
between the client and server.

.. _mojito_data_sharing-data_addon:

Data Addon
----------

The ``Data`` addon is available through the ``ActionContext`` and ``mojitProxy`` objects
from the controller and binders respectively. Because the addon has two different 
functions, allowing data to be shared from the server to the client and allowing one
mojit to share data with other mojits, the addon has the two objects ``data`` and 
``pageData``. 

Both  ``data`` and ``pageData`` are fully functional `Y.Model instances 
on the client <http://yuilibrary.com/yui/docs/model/>`_, which enables listening for 
data changes and refreshing data on the page.

.. _data_addon_obj_table:

.. csv-table:: Data Addon Objects
   :header: "Object", "Available Methods", "Scope", "Description"

   "pageData", "get, set", "Page-Level", "Allows you to share data with other mojits on a page."
   "data", "get, set", "Mojit-Level", "Allows a controller to share data with its binders and templates."

.. _data_addon-requiring:

Requiring Data Addon
####################

The Data addon is required liked other addons in the controller by
specifying the addon as a string in the ``required`` array:

.. code-block:: javascript

   }, '0.0.1', {requires: ['mojito-data-addon']}); 

.. note:: You don't need to require the addon in binders.


.. _mojito_data_sharing-server_client:

Mojits Sharing Data With Its Binders and Templates
--------------------------------------------------

As you saw in the :ref:`Data Addon Objects table`, the scope of the ``data`` object is 
limited to a mojit. In other words, the controller can use the ``data`` object to share 
data with its binder or templates, but not with other mojits. When you set data
with ``ac.data.set(key, value)``, the data is merged with the data passed to ``ac.done`` 
(this is a shallow merge).  The data is also serialized and rehydrated 
on the client when the page is rendered in the browser.

From the controller, you use ``ac.data.set`` to set or expose data that the binder 
can access with the ``mojitProxy`` object. The ``mojitProxy`` accesses the
set data with the ``data`` object as well with ``mojitProxy.data.get``. Templates can
have Handlebars expressions to inject the set data into a page.

The following example shows you how you would set data and then access it from the binder
or the template.

.. _server_client-ex:

Example
#######

The example below shows how a mojit controller can share stock price information 
with its binder code and templates. This example shows how to access the shared data
with both Handlebars expressions and using ``mojitProxy.pageData`` in the binder.
In reality, you would only need to use one of these methods, with the former (Handlebars
expressions) being the preferred way.

.. _server_client_ex-controller:

mojits/StockQuotes/controller.server.js
***************************************

.. code-block:: javascript

   YUI.add('stockquotes', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {
       index: function(ac) {
         // Model gets stock quote prices
         ac.models.get('stock').getData(function(err, data) {
           if (err) {
             ac.error(err);
             return;
           }
           // The data object allows the controller to set/expose the
           // variable stock_quotes that the binder and templates can access.
           ac.data.set('stock_quotes', data);
           ac.done({
             title: "Stock Quotes"
           });
         });
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-models-addon', 'stockquotes-model', 'mojito-data-addon']});

.. _server_client_ex-binder:

mojits/StockQuotes/binders/binder.js
************************************

.. code-block:: javascript

   YUI.add('stockquotes-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       bind: function(node) {
         // From the mojitProxy, you use the data object to get the 
         // value for stock_quotes that was set in the controller.
         var me = this,
             stock_quotes = this.mojitProxy.data.get('stock_quotes');
         this.node = node;
         var list = "<ul>";
         for (var s in stock_list) {
           list += "<li>" + s + ": $" + stock_list[s] + "</li>";
         }
         list += "</ul>";
         node.one('#stocks p').setHTML(list);
       }
     };
   }, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});



.. _server_client_ex-template:

mojits/StockQuotes/views/index.hb.html
**************************************

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h2>{{title}}</h2>
     <ul>
     <!-- The Handlebars block helper can iterate through the data
          made available through ac.data.set in the controller.
     -->
     {{#each stock_quotes}}
       <li>{{.}}</li>
      {{/each}}
     </ul>
     <!-- Binder attaches the stock prices to the div container -->
     <div id="stocks">
       <p></p>
     </div>
   </div>


.. _mojito_data_sharing-page_data:

Sharing Page Data
------------------

Page data simply means data that is scoped to the mojits within a page. 
The ``Data`` addon provides the ``pageData`` object, based on the 
`YUI Model API <http://yuilibrary.com/yui/docs/model/>`_, which has a ``set`` method
for setting or exposing data that other mojits on the page can access through the
``get`` method. 

The ``pageData`` object is unique to each request, but is the one store for all mojits 
of the request, allowing it to share data between mojits in a page. Binders 
can access page data with ``mojitProxy.pageData.get(name)``. Templates can use
Handlebars to access page data as well, so the page data set with ``ac.pageData.set('name', 'foo')`` 
from one mojit can be added the the template of another mojit with ``{{name}}``.

.. _page_data-page_obj:

page Object
###########

.. _page_obj-overview:

Overview
********

The ``page`` object contains all of the page data set by controllers. Templates
can use ``{{page}}`` to access all of the available page data. The ``page``
object is built on the server and then sent to the client, so the page data can
be shared and also *re-hydrate* the data on the page.

The ``pageData`` object serves as a mechanism for all mojits to access data in the 
``page`` object from the client or server. Both ``ac.pageData`` and ``mojitProxy.pageData`` 
provide access to the same page model.

.. _page_obj-creation:

How It's Created
****************

In the controller, when data is passed to ``ac.done`` or set through the ``data``
or ``pageData`` namespace, Mojito wraps that data in the object ``this.page``.
As a result of this wrapper, you can in fact also access data passed to ``ac.done`` or
set with the ``Data`` addon through ``this.page``. For example, the data passed to the template with either
``ac.done({ stock_list: ["YHOO", "GOOG", "CSCO"]})`` or 
``ac.pagedata.set('stock_list', ["YHOO", "GOOG", "CSCO"])`` can be accessed in the template
with ``{{stock_list}}`` or ``{{this.page.stock_list}}``.

.. _page_obj-creation:

Caveats
*******

Because Mojito creates this ``page`` object for you to share data, you **do not** want
to create your own ``page`` object and pass it to ``ac.done`` because it will
ovverride any page data set with ``pageData.set``.


.. _page_data-ex:

Example
#######

In this example, we're expanding on the idea of sharing stock price information.
The ``StockQuotes`` mojit shares the stock price quotes with other mojits
with the ``pageData``. 

As with previous example, we show how to access and attach shared data to the page with 
the binder and the template, but in your applications, you would normally only 
use one method, and the template approach is preferred.

.. _page_data_ex-controller:

mojits/StockQuotes/controller.server.js
***************************************

.. code-block:: javascript

   YUI.add('stockquotes', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {
       index: function(ac) {
         // Model gets stock quote prices
         ac.models.get('stock').getData(function(err, data) {
           if (err) {
             ac.error(err);
             return;
           }
           // The `pageData` object allows the controller to set/expose the
           // variable stock_quotes other mojits on the page can access.
           ac.pageData.set('stock_quotes', data);
           ac.done({
             title: "Stock Quotes"
           });
         });
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-models-addon', 'stockquotes-model', 'mojito-data-addon']});

.. _page_data_ex`-binder:

mojits/StockQuotes/binders/index.js
***********************************

.. code-block:: javascript

   YUI.add('stockquotes-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       bind: function(node) {
         var ticker = null;
         // From the mojitProxy, you use the data object to get the 
         // value for stock_quotes that was set in the controller.
         this.mojitProxy.pageData.on('change', function(e) {
            var ticker = e.changed;
         }
         this.node = node;
         var list = "<ul>";
         for (var s in stock_list) {
           list += "<li>" + s + ": $" + stock_list[s] + "</li>";
         }
         list += "</ul>";
         node.one('#stocks p').setHTML(list);
       }
     };
   }, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});


.. _page_data_ex2-binder:

mojits/StockTicker/binders/binder.js
************************************

In this binder, we are using an event handler to listen for updates to data. To listen
to changes to any data set, you can use ``mojitProxy.data.on('change', doSomething)``.
This example listens for changes to ``ticker_list``. 


.. code-block:: javascript

   YUI.add('stockticker-binder-index', function(Y, NAME) {

     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       bind: function(node) {
          // Listen for updates 
         this.mojitProxy.pageData.on('ticker_listChange', function(e){
           var ul = node.one("#ticker"),
               items = e.newVal;
           for (var i in items) {
             ul.append("<li>" + items[i] + "</li>");
           }
         });
       }
     };
   }, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});



.. _page_data_ex-template:

mojits/Ticker/views/index.hb.html
*********************************

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <!-- Here we iterate through the data
          made available through ac.pageData.set in the controller of
          another mojit.
          Note: `stock_quotes` is wrapped in `this.page`, Thus
          you could use `{{#each this.page.stock_quotes}}`, too.
     -->
       {{#each stock_quotes}}
         <a href="http://finance.yahoo.com/q?s={{.}}">{{.}}</a> |&nbsp;
        {{/each}}
   </div>

