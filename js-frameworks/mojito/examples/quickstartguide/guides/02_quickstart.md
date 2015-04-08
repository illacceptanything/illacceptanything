# 2. Mojito Quickstart #

## Prerequisites ##

- **System:** Unix-based system.
- **Software:** [Node.js (>= 0.6.0 < 0.8)](http://nodejs.org/), [npm (> 1.0.0)](http://npmjs.org/)


## Installation Steps ##

1. Get Mojito from the [npm registry](https://npmjs.org/doc/registry.html) and globally install 
   the Mojito CLI package (`mojito-cli`) to run Mojito commands. You may need to use `sudo` 
   if you run into permission errors.
  

   `$ npm install mojito-cli -g`
2. Confirm that Mojito has been installed by running the **help** command.

   `$ mojito help`

## Create a Mojito Application  ##

1. `$ mojito create app hello_world`
1. `$ cd hello_world`
1. `$ mojito create mojit myMojit`

## Modify Your Application ##

To make the application return a string we want, replace the code in 
`mojits/myMojit/controller.server.js` with the following:


    YUI.add('myMojit', function(Y, NAME) {
      Y.namespace('mojito.controllers')[NAME] = {
        index: function(ac) {
          ac.done('Hello, world. I have created my first Mojito app at ' + (new Date()) + '.');
        }

      };
    });


## Running the Application ##

1. From the `hello_world` application directory, start Mojito:

   `$ mojito start`

1. Go to [http://localhost:8666/@myMojit/index/](http://localhost:8666/@myMojit/index) to see your application, which
   just displays a welcome message.

1. Stop your application by pressing **Ctrl-C**.

## Learn More ##

* [Mojito: Getting Started](http://developer.yahoo.com/cocktails/mojito/docs/getting_started/)
* [Mojito Documentation](http://developer.yahoo.com/cocktails/mojito/docs/)
