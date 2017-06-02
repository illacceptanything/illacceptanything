# 12. Mojito Frame Mojits #

## Intro ##

Mojito comes with the built-in utility mojits that make developing applications easier. 
Mojito currently comes with the `HTMLFrameMojit` that constructs Web pages from the 
skeleton HTML to the styling and content and the `LazyLoadMojit` that allows you to 
lazily load mojit code. You can also create your own custom frame mojits.


## HTMLFrameMojit ##

The `HTMLFrameMojit` builds the HTML skeleton of a Web page. When you use 
`HTMLFrameMojit` the `<html>`, `<head>`, and `<body>` elements are automatically 
created and the content from child mojits are inserted into the `<body>` element.  
The `HTMLFrameMojit` can also automatically insert assets such as CSS and JavaScript 
files into either the `<head>` or `<body>` elements.

### Configuration ###

You use the `HTMLFrameMojit` by defining a mojit instance of the type
HTMLFrameMojit in the configuration object of `application.json`. The mojit
instance of type `HTMLFrameMojit`
must be the top-level mojit: the instance cannot have a parent instance, but may have 
one or more child instances.

In the example `application.json` below, `frame` is an instance of HTMLFrameMojit 
that has the child instance of the framed mojit. After the HTML skeleton is 
created, the HTMLFrameMojit will insert the value of the title property into 
the `<title>` element and the content created by the frame mojit into the `<body>` 
element.

    [
      {
        "settings": [ "master" ],
        "specs": {
          "frame" : {
            "type" : "HTMLFrameMojit",
            "config": {
              "title": "Title of HTML page",
              "child" : {
                 "type" : "framed"
              }
            }
          }
        }
      }
    ]

## LazyLoadMojit ##

`LazyLoadMojit` allows you to defer the loading of a mojit instance by first 
dispatching the `LazyLoadMoit` as a proxy to the client. From the client, 
`LazyLoadMojit` can then request Mojito to load the proxied mojit. This allows 
your Mojito application to load the page quickly and then lazily load parts of 
the page.

### Configuration ###

To use the `LazyLoadMojit`, you simply create a mojit instance of type
`HTMLFrameMojit` and then configure the deployment of the child mojits to be 
deferred with the property `defer: true`. This tells Mojito to lazy load the
child mojits onto the page.

As you can see from the example `application.json`, you can choose to lazy load
some child mojits (`myLazyMojit` in this case) and have others load immediately
such as `myActiveMojit`:

    [
      {
        "settings": [ "master" ],
        "specs": {
          "frame": {
            "type": "HTMLFrameMojit",
            "config": {
              "deploy": true,
              "child": {
                "type": "Container",
                "config": {
                  "children": {
                    "myLazyMojit": {
                      "type": "LazyPants",
                      "action": "hello",
                      "defer": true
                    },
                    "myActiveMojit": {
                      "type": "GoGetter",
                    }
                  }
                }
              }
            }
          }
        }
      }
    ]

## Learn More ##

* [Frame Mojits](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_frame_mojits.html)
* [Code Examples: Using the HTML Frame Mojit](http://developer.yahoo.com/cocktails/mojito/docs/code_exs/htmlframe_view.html)
* [Code Examples: Attaching Assets with HTMLFrameMojit](http://developer.yahoo.com/cocktails/mojito/docs/code_exs/framed_assets.html)
* [Creating Custom Frame Mojits](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_frame_mojits.html#creating-custom-frame-mojits)