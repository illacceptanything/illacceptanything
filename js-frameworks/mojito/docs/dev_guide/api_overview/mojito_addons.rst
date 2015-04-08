=====================
Action Context Addons
=====================

The Action Context uses a mechanism called addons to provide functionality that lives both 
on the server and client. Each addon provides additional functions through a namespacing 
object that is appended to the ``ActionContext`` object. The ``ActionContext`` object
is available in each controller function, but controllers need to 
:ref:`require addons <addons-requiring>` before accessing addon methods.
See the `ActionContext Class <../../api/classes/ActionContext.html>`_ 
for the addon classes.

The Action Context addons allow you to do the following:

- access assets, such as CSS and JavaScript files
- get configuration information
- get and set cookies
- localize content
- access query and response parameters
- get and set HTTP headers
- create URLs


.. _mojito_addons-syntax:

Syntax
======

Using the ``ActionContext`` object ``ac``, you would call a ``{method}`` from an 
``{addon}`` with the following syntax:

``ac.{addon}.{method}``

For example, to get all of the query string parameters, you would use the ``Params`` addon 
with the ``url`` method as seen here:

``ac.params.url()``


.. _addons-requiring:

Requiring Addons
================

Prior to version 0.5.0, Mojito attached addons to the ``ActionContext`` object for 
every HTTP request and mojit instance. As a result, you were able to use
any of the Action Context addons by default.

In Mojito versions 0.5.0 and later, you need to explicitly require an addon before you
can use it. You require an addon by including an associated string in the 
``requires`` array of your controller. For example, in the controller below, 
the ``Params`` addon is required by adding the string ``'mojito-params-addon'`` 
to the ``requires`` array. 


.. code-block:: javascript

   YUI.add('Foo', function(Y, NAME) {
     Y.namespace('mojito.controllers')[NAME] = {
         index: function(ac) {
            var all_params = ac.params.all();
         }
     };
       // Require the addon by adding the param name to the requires array
   }, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});

The list below shows what strings are used to require addons.

- ``Assets`` addon - ``requires ['mojito-assets-addon']``
- ``Composite`` addon - ``requires ['mojito-composite-addon']``
- ``Config`` addon - ``requires ['mojito-config-addon']``
- ``Cookies`` addon - ``requires ['mojito-cookie-addon']``
- ``Http`` addon - ``requires ['mojito-http-addon']``
- ``Intl`` addon - ``requires ['mojito-intl-addon']``
- ``Params`` addon - ``requires ['mojito-params-addon']``
- ``Url`` addon - ``requires ['mojito-url-addon']``


.. note:: 
   To run older applications with Mojito v0.5.0 and later, you will need to
   modify your controllers so that the ActionContext addons that are being 
   used are required. The most common addons are ``Config``, ``Params``, ``Url``, 
   and ``Assets``.



.. _mojito_addons-exs:

Addon Examples
==============

The following code examples use the addons in parentheses:

- `Dynamically Adding CSS to Different Devices <../code_exs/dynamic_assets.html>`_  
  (``Assets``)
- `Using Cookies <../code_exs/cookies.html>`_ (``Cookie``)
- `Using Query Parameters <../code_exs/query_params.html>`_ (``Params``)
- `Generating URLs <../code_exs/generating_urls.html>`_ (``Url``)
- `Internationalizing Your Application <../code_exs/i18n_apps.html>`_ (``Intl``)
- `Using Multiple Mojits <../code_exs/multiple_mojits.html>`_ (``Composite``)


.. _mojito_addons-create:

Creating Addons
===============

Because customized addons are not part of the standard API, but an extension of the API, 
the instructions for creating addons can be found in 
`Creating New Addons <../topics/mojito_extensions.html#creating-new-addons>`_.

