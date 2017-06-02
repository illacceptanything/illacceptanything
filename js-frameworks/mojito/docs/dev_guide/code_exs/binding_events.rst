==============
Binding Events
==============

**Time Estimate:** 20 minutes

**Difficulty Level:** Advanced

.. _code_exs_events-summary:

Summary
=======

This example shows how to bind events to a mojit, configure code to run 
on the client, and make AJAX calls to the YQL Web service. The application 
listens for events and then makes AJAX calls to YQL to get Flickr photo 
information. 

The following topics will be covered:

- configuring the application to run on the client
- getting Flickr data from the model with YQL
- binding events through the ``mojitProxy`` object
- making AJAX calls to YQL from the binder

.. _events_summary-req:

Requirements
------------

You will need to `get a Flickr API key <http://www.flickr.com/services/api/keys/apply/>`_
to run this example.

.. _code_exs_events-notes:

Implementation Notes
====================

.. _events_notes-client:

Configuring the Application to Run on the Client
------------------------------------------------

Mojito lets you configure applications to run on either the server or client 
side. This example uses binders that are deployed to the client, so we need 
to configure Mojito to deploy the application to the client, where it will 
be executed by the browser.

To configure Mojito to run on the client, you simply set the ``"deploy"`` 
property to ``true`` in ``application.json`` as seen below.

.. code-block:: javascript

   [
     {
       "settings": [ "master" ],
       "specs": {
         "frame": {
           "type": "HTMLFrameMojit",
           "config": {
             "deploy": true,
             "child": {
               "type": "Pager"
             }
           }
         }
       }
     }
   ]

.. _events_notes-data:

Getting Data with YQL in the Model
----------------------------------

In the mojit model, the `YUI YQL Query Utility <https://developer.yahoo.com/yui/3/yql/>`_ 
is used to get Flickr photo information. To access the utility in your model, 
specify ``'yql'`` in the ``requires`` array as seen in the code snippet below:

.. code-block:: javascript

   YUI.add('pager-model', function(Y, NAME) {
     ...
     /* Code for pager-model */
     ...
   }, '0.0.1', {requires: ['yql']});

This code example uses the ``flickr.photos.search`` table to get information 
for photos that have a title, description, or tags containing a string. For 
example, the YQL statement below returns Flickr photo information for those 
photos that have a title, description, or tags containing the string "Manhattan". 

Copy the query below into the `YQL Console <https://developer.yahoo.com/yql/console/>`_,
replace ``{your_flickr_api_key}`` with your own Flickr API key, and then  click **TEST** 
to see the returned XML response.

``select * from flickr.photos.search where text="Manhattan" and api_key="{your_flickr_api_key}"``

The returned response contains photo information in the ``photo`` element. You extract the 
``farm``, ``server``, ``id``, and ``secret`` attributes from each photo element to create 
the photo URI as seen here:

``http://farm + {farm} + static.flickr.com/ + {server} + / + {id} + _ + {secret} + .jpg``


In the ``model.server.js`` of ``Pager`` mojit shown below, the ``YQL`` function uses the YQL 
statement above to get photo data, then parses the returned response to create the photo 
URIs. The model then wraps the photo information in an object and stores those objects in 
the ``images`` array that is sent to the controller through the ``callback`` function.


.. code-block:: javascript

   YUI.add('pager-model', function(Y, NAME) {
     /**
     * The pager-model module.
     * @module pager-model
     */
     /**
     * Constructor for the Model class.
     * @class Model
     * @constructor
     **/
     Y.namespace('mojito.models')[NAME] = {
       init: function(config) {
         this.config = config;
       },
       getData: function(query, start, count, callback) {
          var q = null;
         // Get Flickr API key: http://www.flickr.com/services/api/keys/apply/
         var API_KEY = "{your_flickr_api_key}";
         start = parseInt(start) || 0;
         count = parseInt(count) || 10;
         q = 'select * from flickr.photos.search(' + start + ',' + count + ')  where text="%' + query + '%" and api_key="' + API_KEY + '"';
         Y.YQL(q, function(rawData) {
           if (!rawData.query.results) {
             callback([]);
             return;
           }
           var rawImages = rawData.query.results.photo, rawImage = null,images = [], image = null, i = 0;
           for (; i<rawImages.length; i++) {
             rawImage = rawImages[i];
             image = {
               title: rawImage.title,
               location: 'http://farm' + rawImage.farm + '.static.flickr.com/' + rawImage.server + '/' + rawImage.id + '_' + rawImage.secret + '.jpg',
               farm: rawImage.farm,
               server: rawImage.server,
               image_id: rawImage.id,
               secret: rawImage.secret
             };
             if (!image.title) {
               image.title = "Generic Title: " + query;
             }
             images.push(image);
           }
           callback(images);
         });
       }
     };
   }, '0.0.1', {requires: [ 'yql']});


For a more detailed explanation about how to use YQL in your Mojito application, see 
`Calling YQL from a Mojit <calling_yql.html>`_. For more information about YQL, see the 
`YQL Guide <https://developer.yahoo.com/yql/guide>`_.

.. _events_notes-bind_events:

Binding Events
--------------

This section will discuss the basics of binding events in Mojito and then look at the 
binder used in this code example.

.. _bind_events-basics:

Binder Basics
#############

A mojit may have zero, one, or many binders within the ``binders`` directory. Each binder 
will be deployed to the browser along with the rest of the mojit code, where the 
client-side Mojito runtime will call it appropriately.  On the client, the binder has a 
proxy object (``mojitProxy``) for interacting with the mojit it represents as well as 
with other mojits on the page. Methods can be called from the ``mojitProxy`` object 
that allow binders to listen for and fire events.

The binder consists of a constructor, an initializer, and a bind function. The following 
describes each component and indicates when the ``mojitProxy`` object can be used.

- **constructor** - creates the namespace for your binder that wraps the initialization 
  code and binder.
- **initializer** - is passed the ``mojitProxy`` where it can be stored and used to listen 
  and fire events with other binders. The ``mojitProxy`` is the only gateway back into the 
  Mojito framework for your binder.
- **bind** - is a function that is passed a ``Y.Node`` instance that wraps the DOM node 
  representing this mojit instance. The DOM event handlers for capturing user interactions 
  should be attached in this function.

The skeleton of the ``binders/index.js`` file below illustrates the basic structure of the 
binder. For more information, see `Mojito Binders <../intro/mojito_binders.html>`_.

.. code-block:: javascript

   YUI.add('awesome-binder-index', function(Y, NAME) {
     // Binder constructor
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       // The bind function
       bind: function(node) {
         var thatNode = node;
       }
     };
   }, '0.0.1', {requires: ['mojito']});

.. _bind_events-pagemojitbinder:

Examining the Pager Binder
##########################

This code example uses the binder module ``page-binder-index`` to perform the following:

- attach ``onClick`` handlers to ``prev`` and ``next`` links
- invoke the ``index`` method of the controller through the ``mojitProxy`` object
- create an overlay with Flickr photo information received from YQL

The ``binders/index.js`` for this code example is long and fairly involved, so we will 
dissect and analyze the code.  Let's begin by looking at the ``bind`` function of 
``index.js``, which allows mojits to attach DOM event handlers.

In this code snippet of ``binders/index.js``, the ``bind`` function contains the nested 
``updateDOM`` function that updates node content and attaches event handlers. Using the 
``mojitProxy`` object, the nested ``flipper`` function calls the ``index`` function of 
the controller. The callback ``updateDOM`` is passed to ``index`` to update the content.

.. code-block:: javascript

   ...
     bind: function(node) {
       var thatNode = node;
       // Define the action when user click on prev/next.
       var flipper = function(event) {
       var target = event.target;
       // Get the link to the page.
       var page = parsePage(target.get('href'));
       var updateDOM = function(markup) {
         thatNode.set('innerHTML', markup);
         thatNode.all('#nav a').on('click', flipper, this);
         thatNode.all('#master ul li a').on('mouseover', showOverlay, this);
         thatNode.all('#master ul li a').on('mouseout', showOverlay, this);
       };
       this.mojitProxy.invoke('index',
         {
           params: {page: page},
         }, updateDOM
       );
     };
   ...


The event handler for mouseovers and mouseouts are handled by the ``showOverlay`` 
function, which creates the overlay containing photo information. In the code snippet 
below, ``showOverlay`` makes an AJAX call to YQL to get photo data that is placed in an 
unordered list for the overlay.

.. code-block:: javascript

   ...
     bind: function(node) {
       ...
       var showOverlay = function(event) {
         var target = event.target;
         var href = target.get('href');
         var imageId = parseImageId(href);
         if (target.hasClass('overlayed')) {
           target.removeClass('overlayed');
           thatNode.one('#display').setContent('');
         } else {
           Y.log('HREF: ' + href);
           Y.log('IMAGE ID: ' + imageId);
           target.addClass('overlayed');
           // Query for the image metadata
           var query = 'select * from flickr.photos.info where photo_id="' + imageId + '" and api_key="' + {your_flickr_api_key} + '"';
           thatNode.one('#display').setContent('Loading ...');
           Y.YQL(query, function(raw) {
             if (!raw.query.results.photo) {
               Y.log('No results found for photoId: ' + imageId);
               return;
             }
             var props = raw.query.results.photo;
             var snippet = '<ul style="list-style-type: square;">';
             for (var key in props) {
               if (typeof(props[key]) == 'object') {
                 continue;
               }
               snippet += '<li>' + key + ': ' + props[key] + '</li>';
             }
             snippet += '</ul>';
             thatNode.one('#display').setContent(snippet);
           });
         }
       };
        ...
     }
   ...

Thus far, we've looked at the event handlers, but not the actual binding of the handlers 
to nodes. At the end of the ``bind`` function, you'll see three important lines 
(shown below) that bind the ``flipper`` and ``showOutlay`` functions to handle click and 
mouseover events.

.. code-block:: javascript

   ...
     bind: function(node) {
     ...
       // Bind all the image links to showOverlay
       thatNode.all('#master ul li a').on('mouseover', showOverlay, this);
       thatNode.all('#master ul li a').on('mouseout', showOverlay, this);
       // Bind the prev + next links to flipper
       thatNode.all('#nav a').on('click', flipper, this);
     }
   ...

After a little analysis, the full ``binders/index.js`` below should be easier to 
understand. The binder attaches event handlers to nodes, invokes a function in the 
controller, and updates the content in the template. The binder also has a couple of 
helper functions for parsing and requires the IO and YQL modules, which are specified in 
the ``requires`` array.

.. code-block:: javascript

   YUI.add('pager-binder-index', function(Y, NAME) {
     var API_KEY = '{your_flickr_api_key}';
     function parseImageId(link) {
       var matches = link.match(/com\/(\d+)\/(\d+)_([0-9a-z]+)\.jpg$/);
       return matches[2];
     }
     function parsePage(link) {
       var matches = link.match(/page=(\d+)/);
       return matches[1];
     }

     /**
     * The pager-binder-index module.
     * @module pager-binder-index
     */
     /**
     * Constructor for the Binder class.
     *
     * @param mojitProxy {Object} The proxy to allow
     * the binder to interact with its owning mojit.
     * @class Binder
     * @constructor
     */
     Y.namespace('mojito.binders')[NAME] = {
       /**
       * Binder initialization method, invoked
       * after all binders on the page have
       * been constructed.
       */
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       /**
       * The binder method, invoked to allow the mojit
       * to attach DOM event handlers.
       * @param node {Node} The DOM node to which this
       * mojit is attached.
       */
       bind: function(node) {
         var thatNode = node;
         Y.log('NODE: ' + Y.dump(this.node));
         // define the action when user click on prev/next
         var flipper = function(event) {
           var target = event.target;
           // get the link to the page
           var page = parsePage(target.get('href'));
           Y.log('PAGE: ' + page);
           var updateDOM = function(markup) {
             thatNode.set('innerHTML', markup);
             thatNode.all('#nav a').on('click', flipper, this);
             thatNode.all('#master ul li a').on('mouseover', showOverlay, this);
             thatNode.all('#master ul li a').on('mouseout', showOverlay, this);
           };
           this.mojitProxy.invoke('index',
             {
               params: {page: page}
             }, updateDOM
           );
         };
         var showOverlay = function(event) {
           var target = event.target;
           var href = target.get('href');
           var imageId = parseImageId(href);
           if (target.hasClass('overlayed')) {
             target.removeClass('overlayed');
             thatNode.one('#display').setContent('');
           } else {
             Y.log('HREF: ' + href);
             Y.log('IMAGE ID: ' + imageId);
             target.addClass('overlayed');
             // Query for the image metadata
             var query = 'select * from flickr.photos.info where photo_id="' + imageId + '" and api_key="' + API_KEY + '"';
             thatNode.one('#display').setContent('Loading ...');
             Y.YQL(query, function(raw) {
               if (!raw.query.results.photo) {
                 Y.log('No results found for photoId: ' + imageId);
                 return;
               }
               var props = raw.query.results.photo;
               var snippet = '<ul style="list-style-type: square;">';
               for (var key in props) {
                 if (typeof(props[key]) == 'object') {
                   continue;
                 }
                 snippet += '<li>' + key + ': ' + props[key] + '</li>';
               }
               snippet += '</ul>';
               thatNode.one('#display').setContent(snippet);
             });
           }
         };
         // Bind all the image links to showOverlay
         thatNode.all('#master ul li a').on('mouseover', showOverlay, this);
         thatNode.all('#master ul li a').on('mouseout', showOverlay, this);
         // Bind the prev + next links to flipper
         thatNode.all('#nav a').on('click', flipper, this);
       }
     };
   }, '0.0.1', {requires: ['yql', 'io', 'dump']});


.. _events_notes-paging:

Using Paging
------------

The paging for this code example relies on the application configuration to set route 
paths and the controller to create links to access previous and next pages.

The root path is configured in ``app.js`` with the following:

.. code-block:: javascript

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

The controller for ``Pager`` performs several functions:

- uses the ``Params`` addon to get the ``page`` parameter from the query string
- calculates the index of the first photo on the page
- calls the ``getData`` function in the model to get photo data
- creates URLs for the **next** and **prev** links

The `Params addon <../../api/classes/Params.common.html>`_ allows you to access variables 
from the query string parameters, the POST request bodies, or the routing systems URLs. 
In this code example, you use the ``getFromMerged`` method, which merges the parameters 
from the query string, POST request body, and the routing system URLs to give you access 
to all of the parameters. In the code snippet taken from ``controller.server.js`` below, 
the ``getFromMerged`` method is used to get the value for the ``page`` parameter and then 
calculate the index of the first photo to display:

.. code-block:: javascript

   ...
      index: function(actionContext) {
         var page = actionContext.params.getFromMerged('page');
         var start;
         page = parseInt(page) || 1;
         if ((!page) || (page<1)) {
           page = 1;
         }
         // Page param is 1 based, but the model is 0 based
         start = (page - 1) * PAGE_SIZE;
      ...
      }
   ...

To get the photo data, the controller depends on the model to call YQL to query the 
Flickr API. Using ``actionContext.get({model_name})`` lets you get a reference to the 
model. The file naming convention for models is ``{model_name}.{affinity}.{selector}.js``.
The ``{affinity}`` can have the values ``server``, ``client``, or ``common``. The ``{selector}``
is defined by the ``selector`` property in ``application.json``, but does not need to be defined.
The ``{model_name}`` is an arbitrary string defined by the user. So, for example, 
our model in this example is ``model.server.js``, so ``{model_file_name}`` is ``model``.

The example controller below calls the ``getData`` from the model 
with ``actionContext.models.get('model').getData``, which 
will get the returned data from YQL in the callback function. To use methods from models, 
you need to require the model in the ``requires`` array of the controller. 

.. code-block:: javascript

   ...
       index: function(actionContext) {
       ...
         // Data is an array of images
         actionContext.models.get('model').getData('mojito', start, PAGE_SIZE, function(data) {
           Y.log('DATA: ' + Y.dump(data));
           var theData = {
           data: data, // images
           hasLink: false,
           prev: {
             title: "prev" // opportunity to localize
           },
           next: {
             link: createLink(actionContext, {page: page+1}),
               title: "next"
             },
             query: 'mojito'
           };
           if (page > 1) {
             theData.prev.link = createLink(actionContext, {page: page-1});
             theData.hasLink = true;
           }
           actionContext.done(theData);
         });
       }
       ...
     };
   }, '0.0.1', {requires: [
     'mojito', 
     'mojito-models-addon', 
     'mojito-params-addon', 
     'pager-model',
     'dump'
   ]});


The URLs for the **prev** and **next** links are created by passing the mojit instance, 
the method, and the query string parameters.

The code snippet below creates the query string parameters with the 
`YUI QueryString module <http://yuilibrary.com/yui/docs/api/modules/querystring.html>`_. 
If the query string created by ``Y.QueryString.stringify`` is "page=2" , 
the ``createLink`` method would return the URL ``{domain_name}:8666/?page=2``.

.. code-block:: javascript

   ...
     function createLink(actionContext, params) {
       var mergedParams = Y.mojito.util.copy(actionContext.params.getFromMerged());        
       for (var k in params) {
         mergedParams[k] = params[k];
       }
       return "/?" + Y.QueryString.stringify(mergedParams));
     }
   ...

Stitching the above code snippets together, we have the ``controller.server.js`` below. 
The ``index`` function relies on the model for data and the ``createLink`` function to 
create URLs for the **next** and **prev** links.

.. code-block:: javascript

   YUI.add('pager', function(Y, NAME) {
     /**
     * The pager module.
     * @module pager */
     var PAGE_SIZE = 10;
     /**
     * Constructor for the Controller class.
     * @class Controller
     * @constructor
     */
     Y.namespace('mojito.controllers')[NAME] = {   

       index: function(actionContext) {
         var page = actionContext.params.getFromMerged('page');
         var start;
         page = parseInt(page) || 1;
         if ((!page) || (page<1)) {
           page = 1;
         }
         // Page param is 1 based, but the model is 0 based
         start = (page - 1) * PAGE_SIZE;
         // Data is an array of images
         actionContext.models.get('model').getData('mojito', start, PAGE_SIZE, function(data) {
           Y.log('DATA: ' + Y.dump(data));
           var theData = {
             data: data, // images
             hasLink: false,
             prev: {
               title: "prev" // opportunity to localize
             },
             next: {
               link: createLink(actionContext, {page: page+1}),
               title: "next"
             },
             query: 'mojito'
           };
           if (page > 1) {
             theData.prev.link = createLink(actionContext, {page: page-1});
             theData.hasLink = true;
           }
           actionContext.done(theData);
         });
       }
     };
     // generate the link to the next page based on:
     // - mojit id
     // - action
     // - params
     function createLink(actionContext, params) {
       var mergedParams = Y.mojito.util.copy(actionContext.params.getFromMerged());        
       for (var k in params) {
         mergedParams[k] = params[k];
       }
       return actionContext.url.make('frame', 'index', Y.QueryString.stringify(mergedParams));
     }
   }, '0.0.1', {requires: [
     'mojito', 
     'mojito-models-addon', 
     'mojito-params-addon', 
     'pager-model',
     'dump'
   ]});

.. _code_exs_events-setup:

Setting Up this Example
=======================

To set up and run ``binding_events``:

#. Create your application.

   ``$ mojito create app binding_events``
#. Change to the application directory.
#. Create your mojit.

   ``$ mojito create mojit Pager``
#. To configure you application to run on the client and use ``HTMLFrameMojit``, replace 
   the code in ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "frame": {
              "type": "HTMLFrameMojit",
              "config": {
                "deploy": true,
                "child": {
                  "type": "Pager"
                }
              }
            }
          }
        }
      ]

#. Confirm that your ``app.js`` has the following content:

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

#. Also, confirm that your ``package.json`` has the correct dependencies as show below. If not,
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
#. Change to ``mojits/Pager``.
#. To have the controller get data from the model and create links for paging, replace the 
   code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('pager', function(Y, NAME) {
        var PAGE_SIZE = 10;
        /**
        * Constructor for the Controller class.
        * @class Controller
        * @constructor
        */
          Y.namespace('mojito.controllers')[NAME] = {  

            index: function(actionContext) {
              var page = actionContext.params.getFromMerged('page');
              var start;
              page = parseInt(page) || 1;
              if ((!page) || (page<1)) {
                page = 1;
              }
              // Page param is 1 based, but the model is 0 based
              start = (page - 1) * PAGE_SIZE;
              // Data is an array of images
              actionContext.models.get('model').getData('mojito', start, PAGE_SIZE, function(data) {
                Y.log('DATA: ' + Y.dump(data));
                var theData = {
                  data: data, // images
                  hasLink: false,
                  prev: {
                    title: "prev" // opportunity to localize
                  },
                  next: {
                    link: createLink(actionContext, {page: page+1}),
                    title: "next"
                  },
                  query: 'mojito'
                };
                if (page > 1) {
                  theData.prev.link = createLink(actionContext, {page: page-1});
                  theData.hasLink = true;
                }
                actionContext.done(theData);
              });
            }
          };
          // Generate the link to the next page based on:
          // - mojit id
          // - action
          // - params
          function createLink(actionContext, params) {
            var mergedParams = Y.mojito.util.copy(actionContext.params.getFromMerged());
            for (var k in params) {
              mergedParams[k] = params[k];
            }
            return "/?" + Y.QueryString.stringify(mergedParams);
          }
      }, '0.0.1', {requires: [
        'mojito', 
        'mojito-models-addon', 
        'mojito-params-addon', 
        'pager-model',
        'dump'
      ]});


#. To get Flickr photo information using YQL, create the file ``models/model.server.js`` 
   with the code below. Be sure to replace the ``'{your_flickr_api_key}'`` with your own
   Flickr API key.

   .. code-block:: javascript

      YUI.add('pager-model', function(Y, NAME) {
        var API_KEY = '{your_flickr_api_key}';
        /**
        * The pager-model module.
        * @module pager-model
        */
        /**
        * Constructor for the Model class.
        * @class Model
        * @constructor
        */
        Y.namespace('mojito.models')[NAME] = {

          getData: function(query, start, count, callback) {
             var q = null;
            // Get Flickr API key: http://www.flickr.com/services/api/keys/apply/
            var API_KEY = "{your_api_key}";
            start = parseInt(start) || 0;
            count = parseInt(count) || 10;
            q = 'select * from flickr.photos.search(' + start + ',' + count + ')  where text="%' + query + '%" and api_key="' + API_KEY+ '"';
            Y.YQL(q, function(rawData) {
              if (!rawData.query.results) {
                callback([]);
                return;
              }
              var rawImages = rawData.query.results.photo, rawImage = null,images = [], image = null, i = 0;
              for (; i<rawImages.length; i++) {
                rawImage = rawImages[i];
                image = {
                  title: rawImage.title,
                  location: 'http://farm' + rawImage.farm + '.static.flickr.com/' + rawImage.server + '/' + rawImage.id + '_' + rawImage.secret + '.jpg',
                  farm: rawImage.farm,
                  server: rawImage.server,
                  image_id: rawImage.id,
                  secret: rawImage.secret
                };
                if (!image.title) {
                  image.title = "Generic Title: " + query;
                }
                images.push(image);
              }
              callback(images);
            });
          }
        };
      }, '0.0.1', {requires: ['yql']});

#. To create the binder for click events and invoke the ``index`` function of the 
   controller, replace the code in ``binders/index.js`` with the code below. Again,
   Be sure to replace the ``'{your_flickr_api_key}'`` with your own Flickr API key.

   .. code-block:: javascript

      YUI.add('pager-binder-index', function(Y, NAME) {
        var API_KEY = '{your_flickr_api_key}';
        function parseImageId(link) {
          var matches = link.match(/com\/(\d+)\/(\d+)_([0-9a-z]+)\.jpg$/);
          return matches[2];
        }
        function parsePage(link) {
          var matches = link.match(/page=(\d+)/);
          return matches[1];
        }

        /**
        * The pager-binder-index module.
        * @module pager-binder-index
        */
        /**
        * Constructor for the Binder class.
        *
        * @param mojitProxy {Object} The proxy to allow
        * the binder to interact with its owning mojit.
        * @class Binder
        * @constructor
        */
        Y.namespace('mojito.binders')[NAME] = {
          /**
          * Binder initialization method, invoked
          * after all binders on the page have
          * been constructed.
          */
          init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
          },
          /**
          * The binder method, invoked to allow the mojit
          * to attach DOM event handlers.
          * @param node {Node} The DOM node to which this
          * mojit is attached.
          */
          bind: function(node) {
            var thatNode = node;
            Y.log('NODE: ' + Y.dump(this.node));
            // define the action when user click on prev/next
            var flipper = function(event) {
              var target = event.target;
              // get the link to the page
              var page = parsePage(target.get('href'));
              Y.log('PAGE: ' + page);
              var updateDOM = function(markup) {
                thatNode.set('innerHTML', markup);
                thatNode.all('#nav a').on('click', flipper, this);
                thatNode.all('#master ul li a').on('mouseover', showOverlay, this);
                thatNode.all('#master ul li a').on('mouseout', showOverlay, this);
              };
              this.mojitProxy.invoke('index',
                {
                  params: {
                    url: {
                      page: page
                    }
                  }
                }, updateDOM
              );
            };
            var showOverlay = function(event) {
              var target = event.target;
              var href = target.get('href');
              var imageId = parseImageId(href);
              if (target.hasClass('overlayed')) {
                target.removeClass('overlayed');
                thatNode.one('#display').setContent('');
              } else {
                Y.log('HREF: ' + href);
                Y.log('IMAGE ID: ' + imageId);
                target.addClass('overlayed');
                // Query for the image metadata
                var query = 'select * from flickr.photos.info where photo_id="' + imageId + '" and api_key="' + API_KEY + '"';
                thatNode.one('#display').setContent('Loading ...');
                Y.YQL(query, function(raw) {
                  if (!raw.query.results.photo) {
                    Y.log('No results found for photoId: ' + imageId);
                    return;
                  }
                  var props = raw.query.results.photo;
                  var snippet = '<ul style="list-style-type: square;">';
                  for (var key in props) {
                    if (typeof(props[key]) == 'object') {
                      continue;
                    }
                    snippet += '<li>' + key + ': ' + props[key] + '</li>';
                  }
                  snippet += '</ul>';
                  thatNode.one('#display').setContent(snippet);
                });
              }
            };
            // Bind all the image links to showOverlay
            thatNode.all('#master ul li a').on('mouseover', showOverlay, this);
            thatNode.all('#master ul li a').on('mouseout', showOverlay, this);
            // Bind the prev + next links to flipper
            thatNode.all('#nav a').on('click', flipper, this);
          }
        };
      }, '0.0.1', {requires: ['yql', 'io', 'dump']});

#. To display links to photos and associated photo data in the rendered template, replace 
   the code in ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit" style="position: relative; width: 960px">
        <h3>Query Term: {{query}}</h3>
        <div id="nav" style="clear: both;">
        {{#hasLink}}
          {{#prev}}
          <a href="{{{link}}}">{{title}}</a>
          {{/prev}}
        {{/hasLink}}
        {{^hasLink}}
          {{#prev}}{{title}}{{/prev}}
        {{/hasLink}}
        {{#next}}
          <a href="{{{link}}}">{{title}}</a>
        {{/next}}
        </div>
        <div id="master" style="width: 30%; float: left;">
          <ul>
          {{#data}}
            <li><a href="{{location}}" data-id="{{image_id}}">{{title}}</a></li>
          {{/data}}
          </ul>
        </div>
        <div style="width: 50%; float: right">
        <!-- load image here dynamically -->
          <div id="display" style="margin: 0 auto;">
            &nbsp;
          </div>
        </div>
      </div>

#. From the application directory, run the server.

   ``$ node app.js``
#. To view your application, go to the URL:

   http://localhost:8666

.. _code_exs_events-src:

Source Code
===========

- `Application Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/binding_events/application.json>`_
- `Mojit Binder <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/binding_events/mojits/Pager/binders/index.js>`_
- `Binding Events Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/binding_events/>`_
