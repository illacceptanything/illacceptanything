=========================================
Tutorial: Creating Your First Application
=========================================

.. _getting_started-prereq:

Prerequisite
============

Complete the `Mojito Quickstart <quickstart.html>`_, which instructs you
how to install Mojito and use basic commands for the Mojito command-line tool.

In this tutorial, you create a simple application that serves a single page and 
uses a controller to generate output. 

You will learn how to do the following:

- create an application
- create a mojit
- configure a mojit
- run an action (method) on the controller
- run unit tests for your application

.. _getting_started-make_app:

Create the Application
======================

#. Create the Mojito application ``minty_app``.

   ``$ mojito create app minty_app``

#. Change to your application directory.

   ``$ cd minty_app``

.. _getting_started-make_mojit:

Create the Sample Mojit
=======================

The name *mojit* is a fusion of the words module and widget. The mojit, 
however, is neither a module nor a widget. Instead, it is best understood as 
a unit of execution used to generate output. Mojits have an MVC structure and 
consist of two parts: the definition and the instance configuration.

The definition contains the controller and model code for the mojit, along with 
the views (and assets) used to render the output. The definition also 
contains unit tests for the code.

The instance configuration is what configures each instance of your mojit. For 
example, you might have an ``RSS`` mojit that is used to display an RSS feed. 
The mojit definition would have the code and views for fetching and rendering a 
feed, and the instance configuration would have the RSS URL 
to fetch, how many items to show, and whether to show thumbnails, etc.

Let's now begin by creating your mojit, but note that you won't be working with 
models or views in this tutorial.

#. Create the mojit for your ``minty_app`` application.

   ``$ mojito create mojit Hello``

   The `Mojito command-line tool <../reference/mojito_cmdline.html>`_ creates 
   a canned mojit named ``Hello``.

#. To configure your application to use the mojit ``Hello``, replace the code in 
   ``application.json`` with the following:

   .. code-block:: javascript

      [
        {
          "settings": [ "master" ],
          "specs": {
            "hello": {
              "type": "Hello"
            }
          }
        }
      ]

   Here you have defined the instance ``hello`` of the ``Hello`` mojit, 
   which will allow you to call the functions in the mojit controller.

#. To set up a new route for executing your mojit, define the routing 
   path in file ``app.js`` by adding the following above the ``app.listen`` 
   method:

   .. code-block:: javascript

      app.get('/', libmojito.dispatch('hello.index'));

   This ``app.js`` file defines the routing paths, the accepted HTTP 
   methods, and what action to take. The action is what method to call from 
   the mojit instance when a call is made on the defined path. 
   The ``app.js`` above configures Mojito to execute the ``index`` method 
   from the ``hello`` instance (defined in ``application.json``) when receiving 
   HTTP GET calls on the root path. The ``app.js`` is also used to 
   include middleware, set the port, and use contexts (runtime environment).

#. From the application directory, test your application. You will notice that 
   some tests are deferred.

   ``$ mojito test app .``

.. _getting_started-start_server:

Start the Server
================

#. Start the server.

   ``$ node app.js``

#. Open http://localhost:8666/ in a browser.
#. The Web page should display the following (you'll also see documentation links and 
   instructions for running the ``quickstartguide`` application)::

      status
             Mojito is working.
      data
             some: data

   The text was served by the controller, the ``controller.server.js`` file in the 
   ``minty_app/mojits/Hello`` directory. You will learn more about the controller in 
   :ref:`Modify the Sample Mojit <first_app-modify_mojit>`.

#. Stop the server by going back to your terminal pressing **^C**.


.. _first_app-modify_mojit:

Modify the Sample Mojit
=======================

You will now modify the controller, so that the ``index`` function called in the 
controller outputs different results.

#. Change to ``mojits/Hello``.
#. Edit ``controller.server.js`` and replace the string 'Mojito is working.' in 
   the code with 'Doing well, thanks.'. Your ``controller.server.js`` should look similar 
   to the following code:

   .. code-block:: javascript

      YUI.add('hello', function(Y, NAME) {

        /**
        * The hello module.
        *
        * @module hello
        **/

       /**
        * Constructor for the Controller class.
        *
        * @class Controller
        * @constructor
        */
        Y.namespace('mojito.controllers')[NAME] = {   

          /**
          * Method corresponding to the 'index' action.
          *
          * @param ac {Object} The ActionContext that provides access
          *        to the Mojito API.
          **/
          index: function(ac) {
            ac.models.get('model').getData(function(err, data) {
              if (err) {
                ac.error(err);
                return;
              }
              ac.assets.addCss('./index.css');
              ac.done({
                status: 'Doing well, thanks.',
                data: data
              });
            });
          }
        };
      }, '0.0.1', {requires: [
        'mojito',
        'mojito-models-addon', 
        'mojito-assets-addon'
      ]});


   As you can see the "controllers" are just an array of JavaScript objects, 
   and the "action" is just a method called on the controller object. 
   The result of the method are communicated back to Mojito through the 
   ``actionContext`` object. 

   Models in Mojito rely on HTTP-based services to get data and do not generally connect
   directly to a database through an ORM. We recommend `YQL <https://developer.yahoo.com/yql>`_
   to fetch data from Web services.

#. Change to the ``tests`` directory.

#. Edit ``controller.server-tests.js`` and replace the string 'Mojito is working.' 
   in the code with 'Doing well, thanks.'. Your ``controller.server-tests.js`` should 
   look similar to the  following code:

   .. code-block:: javascript

      YUI.add('hello-tests', function(Y) {

        var suite = new YUITest.TestSuite('hello-tests'),
            controller = null,
            A = YUITest.Assert;

        suite.add(new YUITest.TestCase({
        
          name: 'Hello user tests',
          setUp: function() {
            controller = Y.mojito.controllers["hello"];
          },
          tearDown: function() {
            controller = null;
          },
          'test mojit': function() {
            var ac,
                modelData,
                assetsResults,
                doneResults;
            modelData = { x:'y' };
            ac = {
              assets: {
                addCss: function(css) {
                  assetsResults = css;
                }
              },
              models: {
                get: function() {
                  return {
                    getData: function(cb) {
                      cb(null, modelData);
                    }
                  };
                }
              },
              done: function(data) {
                doneResults = data;
              }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.areSame('./index.css', assetsResults);
            A.isObject(doneResults);
            A.areSame('Doing well, thanks.', doneResults.status);
            A.isObject(doneResults.data);
            A.isTrue(doneResults.data.hasOwnProperty('x'));
            A.areEqual('y', doneResults.data['x']);       
          }
        }));
        YUITest.TestRunner.add(suite);
      }, '0.0.1', {requires: ['mojito-test', 'hello']});

   Mojito has the unit test given in ``controller.server-tests.js`` confirms 
   that the output from the action index is the same as the 
   string given in the assert statement.

#. From the application directory, run the application test.

   ``$ mojito test app .``
#. Restart the server and reopen http://localhost:8666/ in a browser to see the updated
   text::

      status
             Doing well, thanks.
      data
             some: data

#. Congratulations, now go try our `code examples <../code_exs/>`_ or check out 
   the `Mojito Documentation <../>`_.

