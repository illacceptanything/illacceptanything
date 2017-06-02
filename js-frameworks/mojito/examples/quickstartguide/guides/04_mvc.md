# 4. MVC in Mojito #

## Intro ##

The MVC architecture in Mojito incorporates a clear separation of the controller, model, 
and view. The controller is pivotal in the sense that it controls all interactions in the 
MVC of Mojito.The controller retrieves data from the model and passes it to the view. 
Client requests for data are sent to the controller, which in turn fetches data from the 
model and passes the data to the client.

The controller, model, and view are found in the mojit of Mojito. The mojit is a single 
unit of execution of a Mojito application. An application may have one or more mojits, 
which are physically represented by a directory structure. Each mojit has one controller, 
any number or no models, and one or more views. When Mojito receives an HTTP request, an 
application invokes a mojit controller that can then execute, pass data to the view, or 
get data from the model. Let’s look now at each of the MVC components in more detail.

## Controllers ##

After an application has been configured to use a mojit, the mojit controller can either 
do all of the work or delegate the work to models and/or views. In the typical case, the 
mojit controller requests the model to retrieve data and then the controller serves that 
data to the views.

### Location ###

The controller is found in the `{app_name}/mojits/{mojit_name}` directory.

### Naming Convention ###

Controllers use the following naming convention, where `{affinity}` can have one of 
the following three difference affinities: `server`, `client`, or `common`.

`controller.{affinity}.js`

The affinity indicates where the controller is run. The `common` affinity means that the
controller can be run on either the client or server.

### Example ###

The example controller below shows you the basic skeleton. The controller
has a `YUI.add` statement to register the controller, a namespace statement,
and a `requires` array for requiring dependencies. 

`mojits/{mojit_name}/controller.{affinity}.js`


    YUI.add('{mojit_name}', function(Y, NAME) {
      // Module name is {mojit-name}
      // Constructor for the Controller class.
      Y.namespace('mojito.controllers')[NAME] = {
        /**
        * Method corresponding to the 'index' action.
        * @param ac {Object} The ActionContext object
        * that provides access to the Mojito API.
        */
        index: function(ac) {
          ac.done({data: "Here is a string"});
        },
        // Other controller functions
        someFunction: function(ac) {
          ac.done("Hello");
        },
      };
      // The requires array lists the YUI module dependencies
    }, '0.0.1', {requires: []});


## Models ##

Models are intended to closely represent business logic entities and contain code that 
access and persist data. Mojito lets you create one or more models at the 
application and mojit level that can be accessed from controllers.

### Location ###

Models are found in the `{app_name}/mojits/{mojit_name}/models` directory.

### Naming Convention ###

Models use the following naming convention and also have an affinity to determine
where they are executed:

`{model_name}.{affinity}.js`

The model name is an arbitrary string defined by the developer.

### Example ###

The example model below shows you the basic skeleton, which as you
can see is similar to the controller:  a `YUI.add` statement to register the model, 
a `namespace` statement, and a `requires` array for requiring dependencies. 


`mojits/{mojit_name}/models/{model_name}.{affinity}.js`

    YUI.add('{mojit_name}Model{Model_name}', function(Y, NAME) {
      // Models must register themselves in the
      // Namespace for models
      Y.namespace('mojito.models')[NAME] = {
        // Optional init() method is given the
        // mojit configuration information.
        init: function(config) {
          this.config = config;
        },
        // Model methods ideally are asynchronous, and
        // thus need some way of notifying the caller
        // when the method is done.
        someMethod: function(foo, bar, callback) {
          // ... get some data ...
          callback(data);
        }
      };
       // The requires array list the YUI module dependencies.
    }, '0.0.1', { requires:[] });


## Views ##

The views are HTML files that can include templates, such as Handlebars 
expressions, and are located in the `views` directory. The default rendering engine 
for templates is [Handlebars](http://handlebarsjs.com/), which
will also render Mustache tags. 

You can pass data from the controller to the templates with the `ac.done` method.
The property name of an object passed to `ac.done` can then be embedded
in the template in double brackets. For example, `ac.done( { name: "John" })`
can be captured in the template with `{{name}}`.

### Location ###

Templates are found in the `{app_name}/mojits/{mojit_name}/views` directory.

### Naming Convention ###

Templates use the following naming convention:

`{controller_function}.{selector}.{view_engine}.html`

* `{controller_function}` - The controller function that is being run by default
  uses the template with the same name, although you can specify a different template
  name in `ac.done`.
* `{selector}` - The selector is defined by the developer and is used to differentiate
  versions of the same template. For example, you could have a template for the iPhone
  that is identified with the selector `iphone`: `index.iphone.hb.html`
* `{view_engine}` - The engine that renders the template, which is Handlebars by default, so
  the default is `hb`: `index.hb.html`

### Example ###

Here we're going to show an example controller function and an example template together
to show how they are related.

In the `index` function of the `controller.server.js` below, when the method `ac.done` is
called, the data is passed to the template `index.hb.html`.

    ...
        index: function(ac) {
          // You can also pass in the name of the template that you
          // would like to render as the second argument. The default is to render 
          // the template with the same name as the function, so calling the 
          // 'index' function would render 'index.hb.html'.
          ac.done({data: "Here is a string"}, 'index');
        },
    ...

The template `index.hb.html` below has the Handlebars expression `data` that
will be replaced by the value passed to it from the controller: 

    <div id="{{mojit_view_id}}">
      {{data}}
    </div>

The value for the Handlebars expression `{{mojit_view_id}}` is supplied by
Mojito and allows binding of client code to DOM elements.

## Learn More ##

* [MVC in Mojito](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_mvc.html)

