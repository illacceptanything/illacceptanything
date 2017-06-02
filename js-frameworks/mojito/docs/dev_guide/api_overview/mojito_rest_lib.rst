============
REST Library
============

Mojito has a library to make it easier to make a REST calls to Web services from your 
model. For implementation details, see 
`Class Y.mojito.lib.REST <../../api/classes/Y.mojito.lib.REST.html>`_ in the Mojito API 
documentation.

.. _mojito_rest_lib-incl:

Including Library
=================

To use the REST library, include the string 'mojito-rest-lib' in the ``requires`` array, 
which instructs YUI to load the library. Once the library is loaded,  you can use 
`Y.mojito.lib.REST <../../api/classes/Y.mojito.lib.REST.html>`_ to make REST calls..

.. code-block:: javascript

   YUI.add('MyModel', function(Y, NAME) {
     ...
     // Make the REST call.
     Y.mojito.lib.REST.GET("http://example.com");
       ...
     // Ask YUI to load the library w/ 'mojito-rest-lib'.
   }, '0.0.1', {requires: ['mojito', 'mojito-rest-lib']});


.. _mojito_rest_lib-ex:

Example
=======

In the model for the ``recipeSearch`` mojit below, the REST library is used to make a 
GET call to the Recipe Puppy API.

.. code-block:: javascript

   YUI.add('recipesearch-model', function(Y, NAME) {
      Y.namespace('mojito.models')[NAME] = {
       recipeSearch: function(count, cb) {
         var url = 'http://www.recipepuppy.com/api/';
         var params = {
           i:"onions,garlic",
           q:"omelet",
           p:1 
         };
         var config = {
           timeout: 5000,
           headers: {
             'Cache-Control': 'max-age=0'
           }
         };
         Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
           if (err) {
             return cb(err);
           }
           cb(null, response);
         });
       }
     };
   }, '0.0.1', {requires: ['mojito', 'mojito-rest-lib']});


