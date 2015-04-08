====================
Handlebars in Mojito
====================

.. _hb_mojito-overview:

Overview
========

Handlebars is a superset of `Mustache <http://mustache.github.com/mustache.5.html>`_, 
thus, Handlebars expressions include Mustache tags. Handlebars, however, also 
has some additional features such as registering help function and built-in block 
helpers, iterators, and access to object properties through the dot operator 
(i.e, ``{{house.price}}``).  We're just going to look at a few 
Handlebars expressions as an introduction. See the
`Handlebars documentation <http://handlebarsjs.com/>`_ for more information 
examples.

One of the things that we mentioned already is block helpers, which help you 
iterate through arrays. You could use the block helper ``#each`` shown below 
to iterate through an array of strings:

.. code-block:: html

   <ul>
     {{#each view_engines}}
     <li>{{this}}</li>
     {{/each}}
   </ul>

Another interesting block helper used in this example is ``#with``, which will 
invoke a block when given a specified context. For example, in the code 
snippet below, if the ``ul`` object is given, the property title is evaluated.

.. code-block:: html

   {{#with ul}}
     <h3>{{title}}</h3>
   {{/with}}

.. _using_hb-partials:

Partials
========

Handlebars partials are simply templates using Handlebars expressions that other
templates can include. Mojito allows you to have both global (shared by all mojits) or 
local (available only to one mojit) partials depending on the context. Global and local 
partials are used the same way in templates, but the location of the partials is 
different. Data that is available to templates is also available to partials. 

Now let's look at the file naming convention, location, and usage of partials
before finishing up with a simple example.

.. _hb_partials-file_naming:

File Naming Convention
----------------------

The file name for partials is similar to templates using Handlebars except 
``{partial_name}`` replaces ``{controller_function}``:
``{partial_name}.[{selector}].hb.html``

.. _hb_partials-location:

Location of Partials
--------------------

.. _partials_location-global:

Global Partials
###############
 
``{app_dir}/views/partials`` 

Thus, the global partial ``foo.hb.html`` in the application ``bar_app`` would be located at
``bar_app/views/partials/foo.hb.html``.

.. _partials_location-local:

Local Partials
###############

``{app_dir}/mojits/{mojit_name}/views/partials`` 

Thus, the local partial ``foo.hb.html`` in the mojit ``bar_mojit`` would be located at
``mojits/bar_mojit/views/partials/foo.hb.html``.

.. _hb_partials-use:

Using Partials in Templates
---------------------------

To use a partial, the template uses the following syntax: ``{{> partial_name}}``

To use the partial ``status.hb.html``, you would included the following
in a template: ``{{> status }}``

.. _hb_partials-example:

Example
-------

**/my_news_app/views/partials/global_news.hb.html**

.. code-block:: html

   <div>
      <h3>Global News</h3>
      {{global_news_stories}}
   </div>

**/my_news_app/mojits/news/views/partials/local_news.hb.html**

.. code-block:: html

   <div>
      <h3>Local News</h3>
      {{local_news_stories}}
   </div>

**/my_news_app/mojits/news/views/index.hb.html**

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h2>Today's News Stories</h2>
     {{> global_news}}
     {{> local_news}}
   </div>

.. _using_hb-helpers:

Helpers
=======

Handlebars comes with a set of simple monadic functions called 
`helpers <http://handlebarsjs.com/expressions.html#helpers>`_ that
you can call in Handlebars expressions. Some helpers are called 
`block helpers <http://handlebarsjs.com/block_helpers.html>`_ because
they are iterators. You can also create new helpers and register them
using the `Helpers addon <https://developer.yahoo.com/cocktails/mojito/api/classes/Helpers.common.html>`_. 
We'll take a look how in Mojito application to use both simple helper and block helpers
and then show you how to create and register your own helpers.

.. _hb-using_helpers:

Using Helpers
-------------

.. _using_helpers-basic:

Basic Helpers
#############

We are calling helpers that don't iterate *basic* helpers to distinguish them
from block helpers that do iterate. An example of a basic helper would be
``link``, which takes two arguments and outputs an HTML ``a`` tag.

To use the ``link`` helper in a Mojito application, your controller passes
data to the template that uses the ``link`` helper in a Handlebars expression.
In the example controller below, the method ``ac.done`` passes the object
``yahoo_link`` to the template:

.. code-block:: javascript

   ...
   index: function(ac) {
     var yahoo_link = { name: "Yahoo", url: "http://www.yahoo.com" };
     ac.done(yahoo_link);
   }
   ...

In the ``index.hb.html`` template, the ``link`` method can create an HTML ``a``
tag with the ``name`` and ``url`` properties:

.. code-block:: javascript

    {{link yahoo_link.name yahoo_link.url}}

This creates the following link: ``<a href="http://www.yahoo.com">Yahoo</a>``


Block Helpers
#############

As we've mentioned earlier, block helpers allow you to iterate through the properties
of an object or through items in an array. The syntax for using block helpers is 
different that using basic helpers. The pound sign "#" is prepended to the 
helper name and the helper must have a closing Handlebars expression.
Within the opening and closing Handlebars expressions for iterators, you can access
items of the list that you are iterating over.

Let's use of the block helper ``each`` to iterate over an array of objects 
and then use the properties of the objects with the ``link`` helper to create
a list of links.

In this example controller, we pass an array of objects with links and names of
Yahoo pages to ``ac.done``:

.. code-block:: javascript
   ...
   index: function(ac) {
     var yahoo_links = [
       { name: "Yahoo", url: "http://www.yahoo.com" },
       { name: "Yahoo Finance", url: "http://finance.yahoo.com" },
       { name: "Yahoo News", url: "http://news.yahoo.com" },
       { name: "Yahoo Movies", url: "http://movies.yahoo.com" }
      ];
      ac.done(yahoo_links);
    }
    ...

In the template, we can now use the block helper ``each`` to
create links with the objects and their properties ``name`` and ``url``:

.. code-block: html

   <ul>
   {{#each yahoo_links}}
     <li><a href="{{url}}">{{name}}</a></li>
   {{/each}}
   </ul>

.. _hb-creating_helpers:

Creating Custom Helpers
-----------------------

As an aid to those used Handlebars helpers, we'll first look at 
how Handlebars helpers are used in Node.js applications and then
contrast that with how they are used in Mojito applications.

.. _creating_helpers-node:

Node.js Applications
####################

To create custom Handlebars helpers in a Node.js application, you use the 
Handlebars method ``registerHelper`` to register your helper so that
it can be used in Handlebars expressions.

In the example Node.js script below, the ``makeLink`` 

.. code-block:: javascript

   #!/usr/local/bin/node

   var Handlebars = require('handlebars');
   var context = { title: "My New Post", url: "http://mywebsite.com/new-post" };
   var source = "<div>{{makeLink title url}}</div>";
   // Registering a Handlebars helper that can be used
   // in the Handlebars expression in the HTML (`source`).
   Handlebars.registerHelper('makeLink', function(text, url) {
     text = Handlebars.Utils.escapeExpression(text);
     url  = Handlebars.Utils.escapeExpression(url);
     var result = '<a href="' + url + '">' + text + '</a>';
     return new Handlebars.SafeString(result);
   });
   var template = Handlebars.compile(source);
   var html = template(context);
   // Output: <div><a href="http://mywebsite.com/new-post">My New Post</a></div>
   console.log(html); 

.. _creating_helpers-mojito:

Mojito Applications
####################

To use custom Handlebars helpers in a Mojito application, you also need to register
your helper, but instead of using ``registerHelper``, you use the 
`Helpers addon <https://developer.yahoo.com/cocktails/mojito/api/classes/Helpers.common.html>`_.
The ``Helpers`` addon has several methods for getting helpers, setting mojit-level
helpers, or exposing helpers so that they can shared with other mojits.

Let's take a quick look at the ``Helper`` addon, show how to use the addon methods
to register helpers, and finally provide you with an example that includes both the
controller and corresponding template.

.. _mojito-helpers_addon:

Helpers Addon
*************

As with other addons, you need to require the ``Helpers`` addon by adding the string
``mojito-helpers-addon`` in the ``requires`` array of your controller. You also access the
addon and its methods through the ``ActionContext`` object.

The ``Helper`` addon has the following three methods:

- ``expose`` - Exposes a parent mojit's helper function so that on the server side any 
  child mojit instance under a particular request can use the helper. On the client, any 
  child mojit instance on the page can use the helper. 
- ``get`` - Allows you to get a specify helper (if given an argument) or all the helpers
  if not given any arguments.
- ``set`` - Sets a helper function for a mojit instance. Other mojit instances will not
  have access to this helper function.

.. _helpers_addon-set_mojit:

Setting Helpers for a Mojit Instance
************************************

To register a helper for a mojit instance, you use the ``set`` method of the ``Helpers``
addon. In the example controller below, the ``set`` method registers the helper
``highlightModuleHelper`` that uses the YUI ``Highlight`` class to highlight strings.
The reason for setting the helper for this mojit instance is that it depends on 
a specific data structure passed to it.

.. code-block:: javascript

   ...
     function highlightModuleHelper(mods, highlighted_module) {
       var mod_names = [];
       for (var i = 0, l=mods.length; i<l; i++){
         mod_names.push(mods[i].name);
       }
       mod_names = mod_names.join(', ');
       return Y.Highlight.words(mod_names, highlighted_module, {
        caseSensitive:false
       }); 
     }
     index: function(ac) {
       var data = {
             modules: [
               {name: "event", user_guide: "http://yuilibrary.com/yui/docs/event/", title: "Event Utility"},
               {name: "node", user_guide: "http://yuilibrary.com/yui/docs/node/",  title: "Node Utility"},
               {name: "base", user_guide: "http://yuilibrary.com/yui/docs/base/", title: "Base" },
               {name: "test", user_guide: "http://yuilibrary.com/yui/docs/test/", title: "YUI Test"}, 
               {name: "cookie", user_guide: "http://yuilibrary.com/yui/docs/cookie/",  title: "Cookie Utility"},
               {name: "yql", user_guide: "http://yuilibrary.com/yui/docs/yql/", title: "YQL Query"} 
             ]
           };
       ac.helpers.set('highlightModule', highlightModuleHelper);
       ac.done({ yui_info: data, highlighted_module: ac.params.url('module') || "event"});
     }
   ...

In the ``index.hb.html`` template, the helper ``highlightModule`` highlights
takes as the arguments passed to it by ``ac.done`` and highlights the strings matching
the values assigned to ``highlighted_module``:

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h3>Highlighted Products:</h3>
     {{{highlightModule yui_info.modules highlighted_module }}
   </div>


.. _helpers_addon-set_global:

Exposing Helpers for Global Use
*******************************

To register a helper so that parent mojits can share them with their children, you use the 
``expose`` method of the ``Helpers`` addon. In the example controller below, the 
``expose`` method registers the helper ``toLinkHelper`` that creates links. It makes sense 
to expose this helper so that its child mojit instances can also use the helper to create 
links.

.. code-block:: javascript

   ...
     function toLinkHelper(title, url) {
        return "<a href='" + url + "'>" + title + "</a>";
     }
     index: function(ac) {
       var data = {
             modules: [
               {name: "event", user_guide: "http://yuilibrary.com/yui/docs/event/", title: "Event Utility"},
               {name: "node", user_guide: "http://yuilibrary.com/yui/docs/node/",  title: "Node Utility"},
               {name: "base", user_guide: "http://yuilibrary.com/yui/docs/base/", title: "Base" },
               {name: "test", user_guide: "http://yuilibrary.com/yui/docs/test/", title: "YUI Test"}, 
               {name: "cookie", user_guide: "http://yuilibrary.com/yui/docs/cookie/",  title: "Cookie Utility"},
               {name: "yql", user_guide: "http://yuilibrary.com/yui/docs/yql/", title: "YQL Query"} 
             ]
           };
       ac.helpers.expose('toLink',toLinkHelper);
       ac.done({ yui_info: data });
     }
   ...

Other mojits on the page that want to access the global Handlebars helper in their 
templates must require the ``Helpers`` addon in their controllers as shown below:

.. code-block:: javascript

   YUI.add('child', function(Y, NAME) {
     // This is a child mojit that wants to reference the global
     // Handlebars helper. Although no helper code is needed in the controller,
     // the controller must require the `Helpers` addon by adding the string
     // `mojito-helpers-addon` to the `requires` array for its template to use the helper.
   }, '0.0.1', {requires: ['mojito', 'mojito-helpers-addon', 'mojito-params-addon', 'highlight']});


In the template ``index.hb.html`` below, the Handlebars block helper ``each``
iterates through the objects contained in the array ``yui_info.modules``, and
then the custom helper ``toLink`` creates links with the values of the properties
``title`` and ``user_guide``:

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h3>YUI Modules</h3>
     <ul>
     {{#each yui_info.modules}}
       <li>{{{toLink title user_guide }}}</li>
     {{/each}}
     </ul>
   </div>


.. _helpers_addon-ex:

Example
*******

.. _helpers_ex-controller:

controller.server.js
^^^^^^^^^^^^^^^^^^^^

.. code-block:: javascript

   YUI.add('helper', function(Y, NAME) {

     function toLinkHelper(title, url) {
       return "<a href='" + url + "'>" + title + "</a>";
     }
     function highlightModuleHelper(mods, highlighted_module) {
       var mod_names = [];
       for (var i = 0, l=mods.length; i<l; i++){
         mod_names.push(mods[i].name);
       }
       mod_names = mod_names.join(', ');
       return Y.Highlight.words(mod_names, highlighted_module, {
         caseSensitive:false
       }); 
     }
     Y.namespace('mojito.controllers')[NAME] = {
       index: function(ac) {
         ac.helpers.set('toLink', toLinkHelper);
         ac.helpers.expose('highlightModule', highlightModuleHelper);
         var data = {
           modules: [
             {name: "event", user_guide: "http://yuilibrary.com/yui/docs/event/", title: "Event Utility"},
             {name: "node", user_guide: "http://yuilibrary.com/yui/docs/node/",  title: "Node Utility"},
             {name: "base", user_guide: "http://yuilibrary.com/yui/docs/base/", title: "Base" },
             {name: "test", user_guide: "http://yuilibrary.com/yui/docs/test/", title: "YUI Test"}, 
             {name: "cookie", user_guide: "http://yuilibrary.com/yui/docs/cookie/",  title: "Cookie Utility"},
             {name: "yql", user_guide: "http://yuilibrary.com/yui/docs/yql/", title: "YQL Query"} 
           ]
         };
         ac.done({ yui_info: data, highlighted_module: ac.params.url('module') || "event"});
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-helpers-addon', 'mojito-params-addon', 'highlight']});

.. _helpers_ex-template:

index.hb.html
^^^^^^^^^^^^^

.. code-block:: html

   <div id="{{mojit_view_id}}">
     <h3>YUI Modules</h3>
     <ul>
     {{#each yui_info.modules}}
       <li>{{{toLink title user_guide }}}</li>
     {{/each}}
     </ul>
     <h3>Highlighted Products:</h3>
     {{{highlightModule yui_info.modules highlighted_module }}
   </div>


