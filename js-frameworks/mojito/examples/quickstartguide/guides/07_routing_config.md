# 7. Routing in Mojito #

## Intro ##

Routing in Mojito is done in the configuration file `routes.json`.
In essence, in the the configuration, you map URL paths to mojit actions,
so that when a request is made to that path, the controller function 
that matches the mojit action name is executed.

We'll cover the default routing Mojito creates for mojit actions and then
show you how to configure single and multiple routes.

## Default Route 

When you create a mojit, Mojito will create the following route path
for you: `{domain_name}:{port}/@{mojit_name}/{action}`

For example, if you are locally running an application that has the
mojit `myMojit`, you can make an HTTP GET request to `http://localhost:8666/@myMojit/index`
to execute the `index` function of the mojit's controller. 

In general, you don't want to use the default routing except for testing, but 
you should know that Mojito is creating an anonymous instance of your 
mojit that allows it to execute a mojit action. Anonymous instances have
the `@` symbol prepended to the mojit name. 

## Single Route ##

To create a route, you need to create a mojit instance that can be mapped to a 
path. In the `application.json` below, the `hello` instance of type 
`HelloMojit` is defined.


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

The `hello` instance and a function in the `HelloMojit` controller can now 
be mapped to a route path in `routes.json` file. In the `routes.json` below, 
the `index` function is called when an HTTP GET call is made on the root path:


    [
      {
        "settings": [ "master" ],
        "hello index": {
          "verbs": ["get"],
          "path": "/",
          "call": "hello.index"
        }
      }
    ]

Instead of using the `hello` mojit instance defined in the `application.json` 
shown above, you can create an anonymous instance of `HelloMojit` for mapping 
an action to a route path. In the `routes.json` below,  an anonymous instance 
of `HelloMojit` is made by prepending "@" to the mojit type.


    [
      {
        "settings": [ "master" ],
        "hello index": {
          "verbs": ["get"],
          "path": "/",
          "call": "@HelloMojit.index",
          "params": { "first_visit": true }
        }
      }
    ]


## Multiple Routes ##

To specify multiple routes, you create multiple route objects that contain 
`verb`, `path`, and `call` properties in `routes.json` as seen here:


    [
      {
        "settings": [ "master" ],
        "root": {
          "verb": ["get"],
          "path": "/*",
          "call": "foo-1.index"
        },
        "foo_default": {
          "verb": ["get"],
          "path": "/foo",
          "call": "foo-1.index"
        },
        "bar_default": {
          "verb": ["get"],
          "path": "/bar",
          "call": "bar-1.index",
          "params": { "page": 1, "log_request": true }
        }
      }
    ]

The `routes.json` file above creates the following routes:

* `http://localhost:8666`
* `http://localhost:8666/foo`
* `http://localhost:8666/bar`
* `http://localhost:8666/anything`

Notice that the `routes.json` above uses the two mojit instances `foo-1` and 
`bar-1`; these instances must be defined in the `application.json` file before 
they can be mapped to a route path. Also, the wildcard used in `root` object 
configures Mojito to call `foo-1.index` when HTTP GET calls are made on any 
undefined path.

## Learn More ##

* [Routing](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_configuring.html#routing)
* [Using Parameterized Paths to Call a Mojit Action](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_configuring.html#using-parameterized-paths-to-call-a-mojit-action)
* [Using Regular Expressions to Match Routing Paths](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_configuring.html#using-regular-expressions-to-match-routing-paths)
* [Code Examples: Configuring Routing](http://developer.yahoo.com/cocktails/mojito/docs/code_exs/route_config.html)
