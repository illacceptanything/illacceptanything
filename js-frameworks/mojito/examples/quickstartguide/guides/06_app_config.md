# 6. Mojito Application Configuration #

## Application Configuration File ##

Applications are configured in the JSON file `application.json` or the
YAML file `application.yaml`. You define mojit instances and configurations values
for different runtime environments called contexts.

## Simple Application ##

In this example `application.json`, we have the default context `master`
and the development context `"environment:development"`. In the `master`
context, we define the mojit instance `foo` of type `myMojit`. The mojit instance
`foo` also has access to the property `title`.

    [     
      {
        "settings": [ "master" ],
        "specs": {
          "foo": {
            "type": "myMojit"
             "config": {
               "title": "Creating the mojit instance 'foo' of type 'myMojit'."
             }
          }
       }
     },
     {
       "settings": [ "environment:development" ],
       "specs": {
         ...
       }
     },
     ...
    ]

## Application with Multiple Mojits ##

The `config` property is not only for defining key-value pairs,
but also for defining mojit relationships. In this example
`application.json`, the mojit instance `father` has two children.

    [
     {
       "settings": [ "master" ],
       "specs": {
         "father": {
           "type": "ParentMojit",
           "config": {
             "children": {
               "son": {
                 "type": "ChildMojit"
               },
               "daughter": {
                 "type": "ChildMojit"
               }
             }
           }
         }
       }
     }
    ]


## Learn More ##

* [Configuring Mojito](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_configuring.html)
* [Code Examples: Basic Configuring of Applications](http://developer.yahoo.com/cocktails/mojito/docs/code_exs/app_config.html)

