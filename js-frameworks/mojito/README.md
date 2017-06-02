<img src="http://www.blackducksoftware.com/files/images/Rookie_2012-125.png" alt="Black Duck OS 2012 Rookie of the Year" align="right" vspace="-50" />

# Yahoo! Mojito [![Build Status](https://secure.travis-ci.org/yahoo/mojito.svg?branch=develop)](http://travis-ci.org/yahoo/mojito)

Mojito is the JavaScript library implementing Cocktails, a JavaScript-based
on-line/off-line, multi-device, hosted application platform.




## Quick Start 

This quick start is intended for Mojito application developers. To contribute to the Mojito project,
see [Contributing Code to Mojito](https://github.com/yahoo/mojito/wiki/Contributing-Code-to-Mojito).


1. Install the Mojito command-line tool:

        $ npm install -g mojito-cli

1. Create an app:

        $ mojito create app hello
        $ cd hello

1. Create a mojit:

        $ mojito create mojit HelloMojit

1. Start the server:

        $ mojito start

1. Go to http://localhost:8666/@HelloMojit/index

1. Run Unit Tests:

        $ mojito test app .

1. Generate documentation:

        $ mojito docs app hello
        
## Mojito Git Branches

The default branch for the Mojito GitHub repository is `develop`. To clone the version of Mojito that is the same as 
the latest version of Mojito in the npm registry, explicitly request the `master` branch: `git clone https://github.com/yahoo/mojito.git --branch master`
Mojito application developers should work with the `master` branch.

Mojito contributors, however, should clone and make pull requests to the `develop` branch. Before you
start contributing, please read [Contributing Code to Mojito](https://github.com/yahoo/mojito/wiki/Contributing-Code-to-Mojito).


## Documentation

### General

* Mojito Home Page - http://developer.yahoo.com/cocktails/mojito
* Mojito Docs Navigation - http://developer.yahoo.com/cocktails/mojito/docs/
* Mojito FAQ - http://developer.yahoo.com/cocktails/mojito/docs/faq/
* Mojito Introduction - http://developer.yahoo.com/cocktails/mojito/docs/intro/
* Mojito Getting Started - http://developer.yahoo.com/cocktails/mojito/docs/getting_started/


### API Documentation

* Running the following command will generate API docs and locally save them to `./artifacts/docs/mojito/`
    `$ mojito docs mojito`
* View the Mojito API documentation on YDN: http://developer.yahoo.com/cocktails/mojito/api/

## Discussion/Forums

http://developer.yahoo.com/forum/Yahoo-Mojito

## Licensing and Contributions

Mojito is licensed under a [BSD license](https://github.com/yahoo/mojito/blob/master/LICENSE.txt). To contribute to the Mojito project, please see [Contributing](https://github.com/yahoo/mojito/wiki/Contributing-Code-to-Mojito). 

The Mojito project is a [meritocratic, consensus-based community project](https://github.com/yahoo/mojito/wiki/Governance-Model) which allows anyone to contribute and gain additional responsibilities.
