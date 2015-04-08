==============
Mojito Binders
==============

.. _mojito_binders-overview:

Overview
========

Each mojit you create can have some specific code called binders that is only 
deployed to the browser. The code can perform the following three functions:

- allow event handlers to attach to the mojit DOM node
- communicate with other mojits on the page
- execute actions on the mojit that the binder is attached to

A mojit may have zero, one, or many binders within the ``binders`` directory. 
Each binder will be deployed to the browser along with the rest of the mojit 
code, where the client-side Mojito runtime will call it appropriately. The view 
used to generate output determines which binder is used. Thus, if the ``simple`` 
view is used, the binder ``simple.js`` is used. This can be overridden by setting  
``view.binder`` in the ``meta`` argument to 
`ac.done <../../api/classes/ActionContext.html#method_done>`_. 
If no binder matches the view, then no binder is used.

.. _mojito_binders-app_reqs:

Application Requirements for Using Binders
==========================================

To use binders, your application is required to have the following: 

- The top-level mojit instance defined in ``application.json`` is of type 
  ``HTMLFrameMojit`` or your own frame mojit. See 
  `HTMLFrameMojit <../topics/mojito_frame_mojits.html#htmlframemojit>`_ 
  for an introduction and example configuration.
- Your application is configured to deploy code to the client with the 
  ``deploy`` property in ``application.json``. See 
  `Configuring Applications to Be Deployed to Client <../intro/mojito_configuring.html
  #configuring-applications-to-be-deployed-to-client>`_ for more information.
- The template files (e.g., ``index.hb.html``) have 
  containers (``div`` elements) that have the ``id`` attribute assigned the 
  value ``{{mojit_view_id}}``. For example: ``<div id={{mojit_view_id}}>``. The
  attribute value ``{{mojit_view_id}}`` allows binders to attach themselves to 
  the DOM.

  
See `Binding Events <../code_exs/binding_events.html>`_ for a documented 
example that uses binders.

.. _mojito_binders-anatomy:

Anatomy of the Binder
=====================

A binder essentially has the two essential functions ``init`` and ``bind``. 
The ``init`` function initializes the binder and contains the ``mojitProxy`` 
object. The ``bind`` function allows the binder to be attached to the DOM.

The example binder below shows the basic structure of a binder. The binder 
is for the ``Awesome`` mojit and contains the ``init`` and ``bind`` 
functions that initialize and allow the binder code to be attached to the DOM.
The module and class naming syntax of mojit controllers, models, and binders 
follow that of YUI, so the module name for the mojit ``Awesomet`` is 
``awesome-binder-index`` and the class name is ``AwesomeBinderIndex``. 

.. code-block:: javascript

   YUI.add('awesome-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         this.mojitProxy = mojitProxy;
       },
       bind: function(node) {
       }
     };
   }, '0.0.1', {requires: ['node']});

An instance of the binder above will be created whenever the ``index`` function 
of the mojit ``Awesome`` is executed, and its corresponding DOM node is attached to 
a client page. Mojito will select that DOM node and pass it into the ``bind``
function. This allows you to write code to capture UI events and interact with 
Mojito or other mojit binders.

.. _binders_anatomy-init:

init
----

The ``init`` method is called with an instance of a mojit proxy specific for 
this mojit binder instance. The mojit proxy can be used at this point to listen 
for events. It is typical to store the mojit proxy for later use as well. The
mojit proxy is the only gateway back into the Mojito framework for your binder.

.. _binders_anatomy-bind:

bind
----

The ``bind`` method is passed a ``Y.Node`` instance that wraps the DOM node 
representing this mojit instance within the DOM. It will be called after all 
other binders on the page have been constructed and their ``init`` methods 
have been called. The mojit proxy can be used at this point to broadcast 
events. Users should attach DOM event handlers in ``bind`` to capture user 
interactions.

For Mojito to reference the DOM node representing the mojit instance and pass 
it to the ``bind`` function, the root element of the mojit's template must 
have the ``id`` attribute with the Handlebars expression ``{{mojit_view_id}}``. 
Mojito will render ``{{mojit_view_id}}`` into a unique ID that can be used to 
select the DOM node.

For example, the root element ``<div>`` in the template below has the ``id`` 
attribute with the value ``{{mojit_view_id}}``. This ``id`` lets Mojito 
reference the ``Y.Node`` instance wrapping the DOM node representing the 
mojit instance within the DOM. If this ``<div>`` element does not have this 
``id`` value, no node will be passed to the ``bind`` function.

.. code-block:: html 

   <div id="{{mojit_view_id}}" class="container">
     <div class="logo-nav">
       <h1>Slide<em>board</em></h1>
     </div>
     <div id="toc" class="toc">
       <ul>
         {{{weather}}}
         {{#tiles}}
           <li><a href="{{link}}">{{name}}</a></li>
         {/tiles}}
       </ul>
     </div>
   </div>

.. _binders_anatomy-mojitProxy:

mojitProxy Object
-----------------

Each binder, when constructed by Mojito on the client, is given a proxy object 
for interactions with the mojit it represents as well as with other mojits on 
the page. This ``mojitProxy`` should be saved with ``this`` for use in the 
other parts of the binder.

From the ``mojitProxy``, you can access properties that use the interface and 
provides the information below:

**Mojit config** - the instance specification for the mojit linked to the binder 
and uses the following syntax:

::

   mojitProxy.config

**Mojit context** - environment information such as language, device, region, 
site, etc.

::

   mojitProxy.context
   
**Mojit children** - the children of the mojit, which is defined in ``application.json``.

::

   mojitProxy.children

**Mojit type** - the name of the mojit that attached the binder to the DOM.

::

   mojitProxy.type

.. _mojito_binders-refresh_views:

Refreshing Views
================

Often all you want your binder to do is to refresh its associated view. From 
the ``mojitProxy`` object, you can call the ``refreshView`` method to render 
a new DOM node for the current mojit and its children, as well as reattach 
all of the existing binders to their new nodes within the new markup. Because 
all binder instances are retained, state can be stored within a binder's scope.

.. _refresh_views-onRefreshView:

onRefreshView
-------------

After ``refreshView`` has been called and the DOM has been refreshed, an event is
triggered that calls the hook method ``onRefreshView``. You can use ``onRefreshView``
to do things such as detach an event or prepare for another user action by 
re-attaching an event.

.. _refresh_views-ex:

Example Usage
-------------

The code snippet below shows how to call the ``refreshView`` method with 
optional parameters. The ``refreshView`` method does not require a callback 
to manage the markup returned from the action invocation. After the DOM
has been refreshed, ``onRefreshView`` is called, and in this example, is
used to detach the event handler and re-attach (bind) the event to the node.

.. code-block:: javascript

   ...
     mojitProxy.listen('flickr-image-detail', function(payload) {
       var urlParams = Y.mojito.util.copy(mojitProxy.context);
       var routeParams = {
         image: payload.data.id
       };
       mojitProxy.refreshView({
         params: {
           url: urlParams,
           route: routeParams
         }
       });
     });

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },
        bind: function(node) {
            var urlParams = Y.mojito.util.copy(this.mojitProxy.context);
            var me = this;
            this.node = node;
            this.handle = me.node.one('#refreshViewButton').on('click', function(){
                Y.log("****I am in the click function****");
                me.mojitProxy.refreshView({
                  params: {
                    ur: urlParams 
                  }
                });
            }, this);
        },
        onRefreshView: function() {
            this.handle.detach();
            this.bind.apply(this, arguments);
        }


.. _mojito_binders-destroy_child:

Destroying Child Mojits
=======================

A mojit binder can attempt to destroy a child mojit on the page by calling the 
``destroyChild`` method from the ``mojitProxy`` object. The ``destroyChild`` 
method accepts one parameter that identifies the child mojit to be destroyed. 
That parameter can either be the ``slot`` or ``_viewId`` that identify the child 
mojit.

After being destroyed, the child's DOM node is detached, destroyed, and its 
binder life-cycle events (``unbind``, ``destroy``) are executed.

.. _destroy_child-ex:

Example Usage
-------------

The code snippet below uses the ``destroyChild`` method to remove the child 
nodes based on the ``_viewId``.

.. code-block:: javascript

   ...
     bind: function(node) {
       this.destroy = node.one("#destroyButton").on('click', function() {
         var childId = this.node.one('#' + this.mojitProxy._viewId).get('value');
         mojitProxy.destroyChild(childId);
       }, this);
     ...
     }
   ...

.. _mojito_binders-class_mojitProxy:

Class MojitProxy
================

See the `Class MojitProxy <../../api/classes/MojitProxy.html>`_ in the Mojito 
API Reference.

.. _class_mojitProxy-exs:

Binder Examples
---------------

The following example shows a typical binder. To see how to use binders in a 
working example, see the `Code Examples: Events <../code_exs/events.html>`_.

.. code-block:: javascript

   YUI.add('chicken-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy) {
         Y.log('Binder(' + mojitProxy.config.id + ')', 'debug', NAME);
         // Store object and ID for later use
         this.mojitProxy = mojitProxy;
         this.id = mojitProxy.config.id;
         // Listen for cluck events from other chickens
         this.mojitProxy.listen('cluck', function(evt) {         
           Y.log(this.id + ' heard cluck from ' + evt.source.id);
           if (this.node) {          
             this.node.append('<p>' + this.id + ' heard cluck from ' + evt.source.id + '</p>');
           }
         }, this);
       },
       bind: function(node) {
         Y.log('bind(' + this.id + ')', 'debug', NAME);
         this.node = node;
         node.on('click', function() {
           Y.log(this.id + ' clicked', 'debug', NAME);
           this.mojitProxy.broadcast('cluck');
         }, this);
       }
     }
   }, '0.0.1', {requires: ['node']});

This example binder shows how to use the methods ``refreshView`` and ``destroyChild``.

.. code-block:: javascript

   YUI.add('parent-binder-index', function(Y, NAME) {
     Y.namespace('mojito.binders')[NAME] = {
       init: function(mojitProxy)   {
         this.mojitProxy = mojitProxy;
         this.myid = Y.guid();
       },
       bind: function(node) {
         var mp = this.mojitProxy;
         var id = this.myid;
         this.node = node;
         this.buttonClickHandler = node.one('#' + mp._viewId + '_ParentRefresh').on('click', function() {
           mp.refreshView(function(data, meta) {
             Y.log('refresh complete', 'warn', NAME);
           });
         });
         this.destroyHandler = node.one('#' + mp._viewId + '_destroyButton').on('click', function() {
         var childId = this.node.one('#' + mp._viewId + '_destroyInput').get('value');
           mp.destroyChild(childId);
         }, this);
         this.moHandler = node.one('h3').on('mouseover', function() {
           Y.log('parent: ' + id, 'info', NAME);
         });
       },
       onRefreshView: function(node, element) {
         Y.log(this.myid + ' refreshed', 'info', NAME);
         this.buttonClickHandler.detach();
         this.destroyHandler.detach();
         this.moHandler.detach();
         this.bind(node, element);
       },
       destroy: function() {
         console.error(this.myid + ' destroyed!');
       }
     };
   }, '0.0.1', {requires: ['mojito-client']});


