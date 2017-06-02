# 5. Mojito Command-Line Utility #

## Intro ##

The following sections cover the basic functionality of the Mojito command-line
utility. You can always run `mojito help` to see the list of available commands
and options. 

You need to run the commands from a Mojito application directory except for the
command `create`. 

## Starting Applications ##

Use the following to start the server and run the application.

`$ mojito start [<port>] [--context "key1:value1,key2:value2,key3:value3"]`

The port number specified in the command above overrides the port number in 
the application configuration file, `application.json`. The default port 
number is 8666. 

## Testing ##

* To run tests for an application:

  `$ mojito test app`

* To run the unit tests for a specific mojit:

  `$ mojito test mojit <mojit-path> [<mojit-module>]`


## Sanitizing Code ##

Output is written to `{CWD}/artifacts/framework/jslint/`, where `{CWD}` 
is the current working directory.

- To run JSLint on the Mojito framework code:

  `$ mojito jslint mojito`


- To run JSLint on an application, including all of its (owned) mojits:

  `$ mojito jslint app <app-name>`

  Output is written to `{app-dir}/artifacts/jslint/`.

- To run JSLint on a specific mojit:

  `$ mojito jslint mojit <mojit-path>`

  Output is written to `{app-dir}/artifacts/jslint/mojits/{mojit-name}`/.

## Generating Documentation ##

* To generate documentation for an application, including all of its (owned) 
  mojits, run the following from the application directory:

  `$ mojito docs app`

  Output is written to `{app-dir}/artifacts/docs/`.

## Learn More ##

* [Mojito Command Line](http://developer.yahoo.com/cocktails/mojito/docs/reference/mojito_cmdline.html)




