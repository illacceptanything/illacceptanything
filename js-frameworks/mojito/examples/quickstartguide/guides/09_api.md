# 9. Overview of the Mojito API #

## Action Context ##

The Action Context (we'll call `ac`) is an essential element of the Mojito framework that 
gives you access to the frameworks features from within a controller function.

Controller functions always have access to the following methods through the Action 
Context:

* `ac.done(data,[{view_name}])` - sends data to the templates and optionally can
  specify a view.
* `ac.error(err)` - reports errors.

See the [ActionContext Class](http://developer.yahoo.com/cocktails/mojito/api/classes/ActionContext.html) 
for the methods available from ``ac``.

__ac.done__

Calling the `done` method allows you to pass data to templates and
then render those templates. The example controller 
below passes the object `data` to the template that
can then replace the Handlebars expression `{{data}}` with
the given value. 

    YUI.add('HelloMojit', function(Y, NAME) {
        Y.namespace('mojito.controllers')[NAME] = {
            index: function(ac) {
                var data = { "data":"data passed to the index template" };
                ac.done(data);
            }
        };
    }, '0.0.1', {requires: []});


## Action Context Addons ##

The Action Context uses a mechanism called addons to provide functionality that lives 
both on the server and client. Each addon provides additional functions through a 
namespacing object that is appended to the `ActionContext` object. 

The Action Context addons allow you to do the following:

*  access assets, such as CSS and JavaScript files
*  get configuration information
*  get and set cookies
*  localize content
*  access query and response parameters
*  get and set HTTP headers
*  create URLs


### Example Use of an Addon ###


Our example controller below uses the `Params` addon to get
the query string parameters:

    YUI.add('Foo', function(Y, NAME) {
      Y.namespace('mojito.controllers')[NAME] = {
        index: function(ac) {
          var all_params = ac.params.url();
          ac.done({ params: all_params });
        }
      };
      // Require the addon by adding the param name to the requires array
    }, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});

See the 
[Mojito API Documentation](http://developer.yahoo.com/cocktails/mojito/api "Mojito API Documentation")
for other Action Context addons.


## Creating Addons ##

Because customized addons are not part of the standard API, but an extension of the API, 
the instructions for creating addons can be found in 
[Creating New Addons](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_extensions.html#creating-new-addons).


## Learn More ##

* [Action Context](http://developer.yahoo.com/cocktails/mojito/docs/api_overview/mojito_action_context.html)
* [Action Context Addons](http://developer.yahoo.com/cocktails/mojito/docs/api_overview/mojito_addons.html)

