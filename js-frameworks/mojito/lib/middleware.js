/**
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint nomen:true, node:true*/

/**
By default, Mojito does not add its built-in middleware, in case the
application would like to need customization. Users have to explicity
request Mojito to add them.

    app.use(libmojito.middleware());

The most common usage is:

    var express = require('express'),
        libmojito = require('mojito'),
        app;

    app = express();
    libmojito.extend(app);
    app.use(libmojito.middleware());

But if you want to have more control over the middleware that you want
to put in place, you could dome something like this:

    var express = require('express'),
        libmojito = require('mojito'),
        app;

    app = express();
    libmojito.extend(app);
    app.use(myCustomMiddleware());
    app.use(libmojito.middleware['mojito-handler-static']());
    app.use(libmojito.middleware['mojito-parser-body']());
    app.use(libmojito.middleware['mojito-parser-cookies']());
    app.use(libmojito.middleware['mojito-contextualizer']());
    app.use(myCustomContextualizerMiddleware());
    app.use(libmojito.middleware['mojito-handler-tunnel']());
    app.use(anotherCustomMiddleware());

@module mojito
@submodule middleware
**/

'use strict';

var debug = require('debug')('mojito:middleware'),
    libpath = require('path'),
    /**
    An ordered list of the middleware module names to load for a standard
    Mojito server instance.
    **/
    MOJITO_MIDDLEWARE = [
        'mojito-handler-static',
        'mojito-parser-body',
        'mojito-parser-cookies',
        'mojito-contextualizer',
        'mojito-handler-tunnel'
    ];

/**
Registers the default middleware that ships with Mojito.

The most common usage is:

    var express = require('express'),
        libmojito = require('mojito'),
        app;

    app = express();
    libmojito.extend(app);
    app.use(libmojito.middleware());

@method middleware
@public
@return {Function} middleware
**/
function middleware() {

    var handlers = [];

    MOJITO_MIDDLEWARE.forEach(function (m) {
        debug('adding middleware %s to be exec', m);
        handlers.push(middleware[m]());
    });

    return function (req, res, next) {

        function run(index) {
            if (index < handlers.length) {
                handlers[index](req, res, function (err) {
                    if (err) {
                        return next(err);
                    }
                    index = index + 1;
                    run(index);
                });
            } else {
                next();
            }
        }

        run(0);
    };
}

// this routine will add all default middleware (from `MOJITO_MIDDLEWARE`)
// onto `module.exports.middleware.<name-of-middleware>`, so they can be
// used directly when needed.
MOJITO_MIDDLEWARE.forEach(function (midName) {
    debug('attach mid %s to middleware', midName);
    middleware[midName] = require('./app/middleware/' + midName);
});

module.exports = {
    middleware: middleware
};
