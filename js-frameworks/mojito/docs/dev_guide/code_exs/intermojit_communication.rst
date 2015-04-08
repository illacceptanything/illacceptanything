=========================
Inter-Mojit Communication
=========================

**Time Estimate:** 15 minutes

**Difficulty Level:** Intermediate

.. _code_exs_intermojit-summary:

Summary
=======

This example shows how to configure mojits to communicate with each other 
through event binding.

The following topics will be covered:

- structuring your mojits for intercommunication
- implementing binders for each mojit to listen to and trigger events
- using the `Composite addon <../../api/classes/Composite.common.html>`_ 
  to execute code in child mojits

.. _code_exs_intermojit-notes:

Implementation Notes
====================

.. _impl_notes-app_config:

Application Configuration
-------------------------

The ``application.json`` for this example defines the hierarchy and 
relationship between the mojits of this application and configures the 
application to run on the client. In the ``application.json`` below, 
the ``HTMLFrameMojit`` is the parent of the ``Master`` mojit, 
which, in turn, is the parent of the mojits ``Sender`` and ``Receiver``. 
The ``"deploy"`` property of the ``"frame"`` object is assigned the value 
``"true"`` to configure Mojito to send code to the client for execution.

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
               "type": "Master",
               "config":{
                 "children": {
                   "sender": {
                     "type": "Sender"
                   },
                   "receiver": {
                     "type": "Receiver"
                   }
                 }
               }
             },
             "assets": {
               "top": {
                 "css":[
                   "/static/Master/assets/css/style.css",
                   "/static/Sender/assets/css/style.css",
                   "/static/Receiver/assets/css/style.css"
                 ]
               }
             }
           }
         }
       }
     }
   ]

.. _impl_notes-routes_config:
 
Routing Configuration
---------------------

In the snippet from ``app.js`` below, two route paths are defined . The route 
configuration for the root path specifies that the ``index`` method of 
the ``frame`` instance of ``HTMLFrameMojit`` be called when HTTP GET calls 
are received. Recall that the ``HTMLFrameMojit`` is the parent of the other 
mojits. Because the ``HTMLFrameMojit`` has no ``index`` function,  the ``index`` 
function in the controller of the child mojit ``Master`` is called instead.

.. code-block:: javascript

   var debug = require('debug')('app'),
       express = require('express'),
       libmojito = require('../../../'),
       app;

   app = express();
   app.set('port', process.env.PORT || 8666);
   libmojito.extend(app);

   app.get('/', libmojito.dispatch('frame.index'));
   app.get('/receiver/show', libmojito.dispatch('receiver.show'));

.. _impl_notes-master_mojit:

Master Mojit
------------

The ``Master`` mojit performs three major functions, each handled by a different 
file. The controller executes the ``index`` methods of the children mojits. The 
binder listens for events and then broadcasts those events to its children. 
Lastly, the ``index`` template displays the content created by the child 
mojits. We'll now take a look at each of the files to understand how they 
perform these three functions.

The ``controller.server.js`` below is very simple because the main purpose 
is to execute the ``index`` functions of the child mojits. The Action Context 
object ``actionContext`` is vital because it gives the ``Master`` mojit access 
to the child mojits through addons. The ``Master`` mojit can execute the 
``index`` functions of the child mojits by calling the ``done`` method from 
the ``Composite`` addon.

.. code-block:: javascript

   YUI.add('master', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {   
       "index": function(actionContext) {
         actionContext.composite.done();
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-composite-addon']});

The binder for the ``Master`` mojit listens for events from the ``Sender``
mojit. Once an event is received, the ``Master`` mojit then broadcasts 
that event to its child mojits. The child mojit ``Receiver`` will 
then intercept the  broadcasted events, which we look at later in
:ref:`impl_notes-receiver_mojit`.

So, how do mojits listen to events from other mojits or broadcast events? On 
the client, each mojit binder can use the ``mojitProxy`` object to interact 
with other mojits on the page. In the ``binders/index.js`` of the 
``Master`` mojit below, the ``mojitProxy`` object is used to listen to hyperlink 
events and then to broadcast an event to the child mojits. The first arguments 
passed to the ``listen`` and ``fire`` methods are the event types.

.. code-block:: javascript

   YUI.add('master-binder-index', function(Y, NAME) {
     Y.namespace("mojito.binders")[NAME]= {
       init: function(mojitProxy) {
         var mp = this.mp = this.mojitProxy = mojitProxy;             
         Y.log("mojitProxy.getChildren(): ");
         Y.log("Entering master-binder-index");
         this.mojitProxy.listen('fire-link', function(payload) {
           var c = mp.getChildren();
           var receiverID = c["receiver"].viewId;
           Y.log('intercepted fire-link event: ' + payload.data.url, 'info', NAME);
           mojitProxy.broadcast('broadcast-link', {url: payload.data.url},{ target: {viewId:receiverID }});
           Y.log('broadcasted event to child mojit: ' + payload.data.url, 'info', NAME);
         });
       },
       /**
       * The binder method, invoked to allow the
       * mojit to attach DOM event handlers.
       * @param node {Node} The DOM node to which
       * this mojit is attached.
       **/
       bind: function(node) {
         this.node = node;
       }
     };
   }, '0.0.1', {requires: ['mojito-client']});

In the ``application.json`` file discussed in :ref:`impl_notes-app_config`, 
four mojit instances were declared: ``frame``, ``child``, ``sender``, and 
``receiver``. Because the ``child`` instance of ``Master`` is the parent 
of the ``sender`` and ``receiver`` mojit instances, the controller can execute 
the code in the child mojit instances by calling ``actionContext.composite.done`` 
in the controller. As you can see below, the output from the ``sender`` and 
``receiver`` instances can be inserted into the template through Handlebars 
expressions.

.. code-block:: html

   <div id="{{mojit_view_id}}" class="mojit">
     <div id="header">
     This example demonstrates inter mojit communication on a page. The mojit on the left 
     side contains a list of image links. The mojit on the right side will display the 
     image whenever a link in the left mojit is clicked on.
     </div>
     <table>
       <tr>
         <td class="left">{{{sender}}}</td>
         <td class="right">{{{receiver}}}</td>
       </tr>
     </table>
   </div>

.. _impl_notes-sender_mojit:

Sender Mojit
------------

The ``Sender`` mojit listens for click events and then forwards them and 
an associated URL to the ``Master`` mojit. Because the controller for the 
``Sender`` mojit does little but send some text, we will only examine the 
binder and index template.

The binder for the ``Sender`` mojit binds and attaches event handlers to the 
DOM. In the ``binders/index.js`` below, the handler for click events uses 
the ``mojitProxy`` object to fire the event to the binder for the 
``Master`` mojit. The URL of the clicked link is passed to ``Master``.


.. code-block:: javascript

   YUI.add('sender-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mp = mojitProxy;
       },
       bind: function(node) {
         var mp = this.mp;
         this.node = node;
         // capture all events on "ul li a"
         this.node.all('ul li a').on('click', function(evt) {
           var url = evt.currentTarget.get('href');
           evt.halt();
           Y.log('Triggering fire-link event: ' + url, 'info', NAME);
           mp.broadcast('fire-link', {url: url});
         });
       }
     };
   }, '0.0.1', {requires: ['node','mojito-client']});

The ``index`` template for the ``Sender`` mojit has an unordered list of links 
to Flickr photos. As we saw in the binder, the handler for click events passes 
the event and the link URL to the ``Master`` mojit.

.. code-block:: html

   <div id="{{mojit_view_id}}" class="mojit">
     <h3>{{title}}</h3>
     <ul>
       <li><a href="http://farm6.static.flickr.com/5064/5632737098_f064e4193c.jpg">Image 1</a></li>
       <li><a href="http://farm6.static.flickr.com/5061/5632537388_ff1763af69.jpg">Image 2</a></li>
       <li><a href="http://farm6.static.flickr.com/5061/5631063565_bc0d4d6fa4.jpg">Image 3</a></li>
       <li><a href="http://farm6.static.flickr.com/5265/5630493861_508fd54a3f.jpg">Image 4</a></li>
       <li><a href="http://farm6.static.flickr.com/5187/5631076804_65eccc0ec0.jpg">Image 5</a></li>
       <li><a href="http://farm6.static.flickr.com/5303/5630492129_1a8cb2e35e.jpg">Image 6</a></li>
       <li><a href="http://farm6.static.flickr.com/5025/5631077466_f088b79d8e.jpg">Image 7</a></li>
       <li><a href="http://farm6.static.flickr.com/5104/5630493353_9b4aba1468.jpg">Image 8</a></li>
       <li><a href="http://farm6.static.flickr.com/5109/5630710610_cc076791cc.jpg">Image 9</a></li>
     </ul>
   </div>

.. _impl_notes-receiver_mojit:

Receiver Mojit
--------------

The ``Receiver`` mojit is responsible for capturing events that were broadcasted 
by ``Master`` mojit and then displaying the photo associated with the link that 
was clicked.

In the controller for ``Receiver`` mojit, the additional function ``show`` displays 
a photo based on the query string parameter ``url`` or a default photo. The ``show`` 
function gets invoked from the binder, which we'll look at next.

.. code-block:: javascript

   YUI.add('receiver', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {   
       index: function(actionContext) {
         actionContext.done({title: 'This is the receiver mojit'});
       },
       show: function(actionContext) {
         var url = actionContext.params.getFromMerged('url') || "http://farm1.static.flickr.com/21/35282840_8155ba1a22_o.jpg";
         actionContext.done({title: 'Image matching the link clicked on the left.', url: url});
       }
     };
   }, '0.0.1', {requires: ['mojito-params-addon']});

The binder for the ``Receiver`` mojit listens for broadcasted link events. In the 
``binders/index.js`` below, those broadcasted link events, which are the event type 
"broadcast-link", will come from the ``Master`` mojit. When the event is captured, the 
``mojitProxy`` object is used to invoke the ``show`` function and pass the photo URI.

.. code-block:: javascript

   YUI.add('receiver-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         var self = this;
         this.mojitProxy = mojitProxy;
         this.mojitProxy.listen('broadcast-link', function(payload) {
           Y.log('Intercepted broadcast-link event: ' + payload.data.url, 'info', NAME);
           // Fire an event to the mojit to reload
           // with the correct URL
           var params = {
             url: {
               url: payload.data.url
             }
           };
           mojitProxy.invoke('show', { params: params }, function(err, markup) {
             self.node.setContent(markup);
           });
         });
       },
       /**
       * The binder method, invoked to allow the
       * mojit to attach DOM event handlers.
       * @param node {Node} The DOM node to which
       * this mojit is attached.
       **/
       bind: function(node) {
         this.node = node;
       }
     };
   }, '0.0.1', {requires: ['mojito-client']});

.. _code_exs_intermojit-setup:

Setting Up this Example
=======================

To set up and run ``inter-mojit``:

#. Create your application.

   ``$ mojito create app inter-mojit``
#. Change to the application directory.
#. Create the mojits for the application.

   ``$ mojito create mojit Master``

   ``$ mojito create mojit Sender``

   ``$ mojito create mojit Receiver``
#. To configure your application to use the mojits you created, replace the code in 
   ``application.json`` with the following:

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
                  "type": "Master",
                  "config":{
                    "children": {
                      "sender": {
                        "type": "Sender"
                      },
                      "receiver": {
                        "type": "Receiver"
                      }
                    }
                  }
                },
                "assets": {
                  "top": {
                    "css":[
                      "/static/Master/assets/css/style.css",
                      "/static/Sender/assets/css/style.css",
                      "/static/Receiver/assets/css/style.css"
                    ]
                  }
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
          app.get('/receiver/show', libmojito.dispatch('receiver.show'));

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

#. Change to ``mojits/Master``.
#. To allow the ``Master`` to execute its children mojits, replace the code in 
   ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('master', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   
          "index": function(actionContext) {
            actionContext.composite.done();
          }
        };
      }, '0.0.1', {requires: ['mojito-composite-addon']});

#. To allow the ``Master`` mojit to capture events and refire them to its children mojits, 
   replace the code in ``binders/index.js`` with the following:

   .. code-block:: javascript

      YUI.add('master-binder-index', function(Y, NAME) {
        Y.namespace("mojito.binders")[NAME]= {
          init: function(mojitProxy) {
            var mp = this.mp = this.mojitProxy = mojitProxy;
            Y.log("mojitProxy.getChildren(): ");
            Y.log("Entering master-binder-index");
            this.mojitProxy.listen('fire-link', function(payload) {
              var c = mp.getChildren();
              var receiverID = c["receiver"].viewId;
              Y.log('intercepted fire-link event: ' + payload.data.url, 'info', NAME);
              mojitProxy.broadcast('broadcast-link', {url: payload.data.url},{ target: {viewId:receiverID }});
              Y.log('broadcasted event to child mojit: ' + payload.data.url, 'info', NAME);
            });
          },
          /**
          * The binder method, invoked to allow the
          * mojit to attach DOM event handlers.
          * @param node {Node} The DOM node to which
          * this mojit is attached.
          **/
          bind: function(node) {
            this.node = node;
          }
        };
      }, '0.0.1', {requires: ['mojito-client']});

#. Modify the ``index`` template to include output from the mojits ``Sender`` and 
   ``Receiver`` by replacing the code in ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <div id="header">
          This example demonstrates inter mojit communication on a page.
          The mojit on the left side contains a list of image links.
          The mojit on the right side will display the image whenever a link in the left 
          mojit is clicked on.
        </div>
        <table>
          <tr>
            <td class="left">{{{sender}}}</td>
            <td class="right">{{{receiver}}}</td>
          </tr>
        </table>
      </div>

#. Change to the ``Sender`` directory.

   ``$ cd ../Sender``
#. Replace the code in ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('sender', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   
          index: function(actionContext) {
            actionContext.done({title: 'List of images for testing'});
          }
        };
      }, '0.0.1', {requires: []});

#. To allow the ``Sender`` mojit to fire an event, replace the code in ``binders/index.js`` 
   with the following:

   .. code-block:: javascript

      YUI.add('sender-binder-index', function(Y, NAME) {
        Y.namespace('mojito.binders')[NAME] = {
          init: function(mojitProxy) {
            this.mp = mojitProxy;
          },
          bind: function(node) {
            var mp = this.mp;
            this.node = node;
            // capture all events on "ul li a"
            this.node.all('ul li a').on('click', function(evt) {
              var url = evt.currentTarget.get('href');
              evt.halt();
              Y.log('Triggering fire-link event: ' + url, 'info', NAME);
              mp.broadcast('fire-link', {url: url});
            });
          }
        };
      }, '0.0.1', {requires: ['node','mojito-client']});

#. To provide an unordered list of image links to the ``index`` template of the 
   ``Master`` mojit, replace the code in ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="mojit">
        <h3>{{title}}</h3>
        <ul>
          <li><a href="http://farm6.static.flickr.com/5064/5632737098_f064e4193c.jpg">Image 1</a></li>
          <li><a href="http://farm6.static.flickr.com/5061/5632537388_ff1763af69.jpg">Image 2</a></li>
          <li><a href="http://farm6.static.flickr.com/5061/5631063565_bc0d4d6fa4.jpg">Image 3</a></li>
          <li><a href="http://farm6.static.flickr.com/5265/5630493861_508fd54a3f.jpg">Image 4</a></li>
          <li><a href="http://farm6.static.flickr.com/5187/5631076804_65eccc0ec0.jpg">Image 5</a></li>
          <li><a href="http://farm6.static.flickr.com/5303/5630492129_1a8cb2e35e.jpg">Image 6</a></li>
          <li><a href="http://farm6.static.flickr.com/5025/5631077466_f088b79d8e.jpg">Image 7</a></li>
          <li><a href="http://farm6.static.flickr.com/5104/5630493353_9b4aba1468.jpg">Image 8</a></li>
          <li><a href="http://farm6.static.flickr.com/5109/5630710610_cc076791cc.jpg">Image 9</a></li>
        </ul>
      </div>

#. Change to the ``Receiver`` directory.

   ``$ cd ../Receiver``
#. To display an image associated with a clicked link, replace the code in 
   ``controller.server.js`` with the following:

   .. code-block:: javascript

      YUI.add('receiver', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {   
          "index": function(actionContext) {
            actionContext.done({title: 'This is the receiver mojit'});
          },
          show: function(actionContext) {
            var url = actionContext.params.getFromMerged('url') || "http://farm1.static.flickr.com/21/35282840_8155ba1a22_o.jpg";
            actionContext.done({title: 'Image matching the link clicked on the left.', url: url});
          }
        };
      }, '0.0.1', {requires: ['mojito-params-addon']});

#. To allow the ``Receiver`` mojit to capture an event and invoke the ``show`` function in 
   the controller, replace the code in ``binders/index.js`` with the following:

   .. code-block:: javascript

      YUI.add('receiver-binder-index', function(Y, NAME) {
        Y.namespace('mojito.binders')[NAME] = {
          init: function(mojitProxy) {
            var self = this;
            this.mojitProxy = mojitProxy;
            this.mojitProxy.listen('broadcast-link', function(payload) {
              Y.log('Intercepted broadcast-link event: ' + payload.data.url, 'info', NAME);
              // Fire an event to the mojit to reload
              // with the correct URL
              var params = {
                url: {
                  url: payload.data.url
                }
              };
              mojitProxy.invoke('show', { params: params }, function(err, markup) {
                self.node.setContent(markup);
              });
            });
          },
          /**
          * The binder method, invoked to allow the
          * mojit to attach DOM event handlers.
          * @param node {Node} The DOM node to which
          * this mojit is attached.
          **/
          bind: function(node) {
            this.node = node;
          }
        };
      }, '0.0.1', {requires: ['mojito-client']});

#. Replace the code in ``views/index.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="Receiver">
        <div id="view" style="margin: auto auto;"></div>
      </div>

#. To create the template that displays the photo of the clicked link, create the file 
   ``views/show.hb.html`` with the following:

   .. code-block:: html

      <div id="{{mojit_view_id}}" class="Receiver">
        <h3>{{title}}</h3>
        <div id="view">
          <img src="{{url}}" width="200px" alt="Missing Image"/>
        </div>
      </div>

#. From the application directory, start the server.

   ``$ node app.js``
#. To view your application, go to the URL:

   http://localhost:8666

.. _code_exs_intermojit-src:

Source Code
===========

- `Application Configuration <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/application.json>`_
- `Master Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Master/controller.server.js>`_
- `Master Mojit Binder <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Master/binders/index.js>`_
- `Master Mojit Template <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Master/views/index.html>`_
- `Sender Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Sender/controller.js>`_
- `Sender Mojit Binder <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Sender/binders/binder.js>`_
- `Receiver Mojit Controller <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Receiver/controller.js>`_
- `Receiver Mojit Binder <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/mojits/Receiver/binders/binder.js>`_
- `Inter-Mojit Application <http://github.com/yahoo/mojito/tree/master/examples/developer-guide/inter-mojit/>`_

