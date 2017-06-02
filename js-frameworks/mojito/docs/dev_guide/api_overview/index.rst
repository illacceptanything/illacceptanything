
===================
Mojito API Overview
===================

This section introduces some of the main features of the Mojito API. Please see the 
`Mojito API documentation <../../api/>`_ that has been built using 
`YUI Doc <http://yuilibrary.com/projects/yuidoc>`_ and is continuously updated. 

The API contains the following five modules:

- **ActionContext** - is a key module of the Mojito framework, giving you access to the 
  frameworks features from within a controller function.
- **Addons** - extensions that provide functionality that lives both on the server 
  and/or client. Each addon provides additional functions through a namespace that is 
  attached directly to the ``Action Context`` object and is available when required in a 
  controller.
- **CommonLibs** - is a utility library containing methods to handle cookies, access input 
  parameters, and make REST calls.
- **MojitoClient** - is the client-side Mojito runtime module containing methods that 
  allow inter-mojit communication through the ``mojitProxy`` object.
- **MojitServer** - is the module that provides access to the Mojito server.

.. toctree::
   :maxdepth: 2

   mojito_action_context
   mojito_addons
   mojito_rest_lib
   mojito_client_obj
