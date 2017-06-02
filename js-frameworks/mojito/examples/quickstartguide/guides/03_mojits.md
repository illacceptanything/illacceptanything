# 3. Mojits #

## What is a Mojit? ##

The basic unit of composition and reuse in a Mojito application is a *mojit*.
Visually, you can think of a mojit as the rectangular area of a page that was 
constructed by a Mojito application. 

## Why Mojit? ##

There are (at least) two very commonly used names given to the basic portions
of a page, site, or application: namely, module and widget. Depending upon
the context, each of these terms will be interpreted in different ways by
different people. In the hope of alleviating misinterpretation, we have chosen
to create our own word: mojit (derived from module + widget and pronounced 
"mod-jit").


## Mojit Structure ##

Mojits are built using a variant of the MVC pattern. The controller forms the core 
functionality of the mojit, reacting to stimuli from the view 
and potentially from outside of the mojit. The model centralizes the representation and 
management of the mojit’s data. The view provides for presentation and user 
interaction.

## Mojit Definition and Mojit Instance ##

A mojit definition is a set of resources that collectively define a reusable unit of 
functionality known as a mojit. A mojit definition includes the mojit implementation 
(e.g., JavaScript code, template files, CSS, etc.).

A mojit instance can refer to a specification of all the information required to create a 
running instance of mojit functionality within an application. This specification 
comprises the identifier for a mojit definition together with the 
concrete configuration parameters that will be used to instantiate a particular instance 
at runtime. The mojit instance can also refer to an in-memory runtime instance of a 
mojit—part of the running application.


## Learn More ##

* [Mojits](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_mojits.html)
* [Mojito Applications: Mojits](http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_apps.html#mojits)
