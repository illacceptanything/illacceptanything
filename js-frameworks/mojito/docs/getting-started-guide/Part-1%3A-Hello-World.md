
# Getting Started with Mojito, Part 1: Hello, World!

<div id="top"></div>

($Revision$ $Date$)

Mojito is built on top of YUI3 and run inside of Node.js.

## Installation

See the Mojito: Quickstart at https://developer.yahoo.com/cocktails/mojito/docs/quickstart.

## First Application:  Hello, World

This will create a simple application, configured to serve a single page. The page will have a controller to generate the output.

### Step 1:  Make the Application

   1. Cd to a directory in which you want to create your application directory.

   1. `mojito create app simple hello`

   1. `cd hello`

This created a directory for your application, and copied a file to define and start a server that serves your application. We specified the term "simple" before our app name ("hello") in order to create a very basic Mojito application structure. You'll learn more about more complex Mojito applications later.

The `application.json` file holds the application's configuration.
The `package.json` file helps define your application to Manhattan, and can be ignored for now.
The `mojits` directory contains your mojit definitions (currently empty).

### Step 2:  Make a Sample Mojit

A mojit is an MVC unit of execution used to generate output.

For example, you might have an `RSSMojit` which is used to display an RSS feed.
The mojit definition would have the code and views for fetching and rendering a feed, while the configuration would have which RSS URL to fetch, how many items to show, whether to show thumbnails, etc.

        $ mojito create mojit simple HelloMojit

The Mojito commandline tool was used above to create a canned mojit definition named `HelloMojit`. (There are other canned mojits, but we created the `simple` one which has just the minimum to get you started.) The canned mojit comes with a unit test, which we ran.


### Step 3: Configure the Mojit

Now there is a mojit available to be used within your application, but your application has not been configured to use it yet. You must add a mojit configuration within the `application.json` file in order to configure your Mojito app to use this new mojit. Edit `application.json` as seen below:

    [
        {
            "settings": [ "master" ],
            "specs": {
                "hello": {
                    "type": "HelloMojit"
                }
            }
        }
    ]


The line we added tells Mojito that we want to make an mojit instance of type `HelloMojit` available to be executed when the HTTP server receives a GET request for the path `/hello`.

### Step 4:  Start the Server

   1. `mojito start`
   1. visit [http://localhost:8666/hello/index](http://localhost:8666/hello/index)
   1. use `^C` to stop server

The `mojito start` initializes Mojito by running the `server.js` (or `index.js`) file.

### Step 5:  Modify the Sample Mojit

   1. Edit the mojit's controller at `mojits/HelloMojit/controller.server.js`
   1. Do Step 4 again to try out your changes.

As you can see, the "controller" is just a javascript object, and the "action" is just a method called on that object. The result of the method are communicated back to Mojito through the _**Action Context**_ object, which we'll describe in later parts of the tutorial.

### In Summary

While there is a lot that Mojito can do, getting started was hopefully fairly simple. There were only a few concepts to learn:  application, mojit controller, and action.

We'll build on these concepts, and introduce new ones, in the rest of this tutorial.

<hr/>
<div class="paginate right"><a href="/tutorials.GettingStarted-Part2">Part 2</a></div>
<div align="center"><a href="#top">top</a></div>
