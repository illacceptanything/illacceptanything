# Running the Code Examples

## Overview

Starting from Mojito 0.6.x, to run Mojito commands, you
need a global installation of the `mojito-cli` package and
a local installation of the `mojito` package. 

The `mojito-cli` package contains the `create` command that 
lets you create applications and mojits, but depends on the
local installation of the `mojito` package to run other Mojito CLI commands.

When you create an application with the `mojito-cli` package, it will
locally install the `mojito` package in your application for you. 
For existing applications, however, you will need to locally install 
`mojito` with before you can run the other commands.

## Instructions

1. Globally install the `mojito-cli` package. 
  
   `$ npm install -g mojito-cli`
1. Change to the application directory of one of the examples.
1. Locally install `mojito` in your application. (If you had created the application with `mojito create app <app_name>`, this would be done for you by `mojito-cli`.)
 
   `$ npm install`
1. Start your application.

   `$ mojito start`


