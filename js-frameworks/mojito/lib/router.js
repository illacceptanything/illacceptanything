/*
 * Copyright (c) 2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE.txt file for terms.
 */

/*jslint nomen:true, node:true*/

/**
Attaches the Mojito specific router to the Express application instance.

Example usage:

    var libpath = require('path'),
        express = require('express'),
        mojito = require('mojito'),
        app;

    app = express();

    app.use(mojito.middleware());
    ...
    // or specify app specific paths (preferred)
    app.mojito.attachRoutes(libpath.join(__dirname, 'config', 'routes.json'));
    // or specify multiple paths
    app.mojito.attachRoutes([
        libpath.join(__dirname, 'config', 'routes01.json')
        libpath.join(__dirname, 'config', 'routes02.json')
    );
    ...
    // or let mojito to mount all routes defined in `routes.json`
    app.mojito.attachRoutes();
    ...

@module mojito
@submodule router
**/

"use strict";

var libfs = require('fs'),
    libpath = require('path'),
    liburl = require('url'),
    libycb = require('ycb'),
    libyaml = require('js-yaml'),
    debug = require('debug')('mojito:router'),
    dispatcher = require('./dispatcher'),
    readConfig = require('./util').readConfig;


/**
Reads `routes` configuration either in JSON or YAML format.

@param {String} fullPath the absolute path to the config file
@return {Object} the parsed configuration
**/
function readConfigYCB(fullPath) {
    var obj,
        ycb,
        ycbDims = [{}];

    obj = readConfig(fullPath);
    obj = ycbDims.concat(obj);
    ycb = libycb.read(obj, {});

    return ycb;
}


/**
Normalizes the `routes.json` configuration.

@param {String} name
@param {Object} route the route object from `routes.json`
@return {Object} normalized route object
**/
function buildRoute(name, route) {

    var i,
        verbObj;

    if (!route.name) {
        route.name = name;
    }
    if (!route.verbs) {
        route.verbs = ['GET'];
    }

    // Checking route.verbs is changed from an array to an object by the
    // building process, so routes that have already been computed are
    // not recomputed.
    if (route.verbs.length && route.path && route.call) {
        // Here we convert the verb array to a map for easy use later on
        verbObj = {};
        for (i in route.verbs) {
            if (route.verbs.hasOwnProperty(i)) {
                verbObj[route.verbs[i].toUpperCase()] = true;
            }
        }
        route.verbs = verbObj;
    }

    return route;
}

/**
@return {Object} normalized routes object
**/
function getRoutes(fullPath) {
    var routes,
        name,
        out = {};

    routes = readConfigYCB(fullPath);
    for (name in routes) {
        if (routes.hasOwnProperty(name)) {
            out[name] = buildRoute(name, routes[name]);
        }
    }
    return out;
}

/**
@class router
@static
**/
module.exports = {
    /**
    Only used for unit tests
    @protected
    **/
    buildRoute: buildRoute,
    getRoutes: getRoutes,
    readConfigYCB: readConfigYCB,

    /**
    Reads the `routes` configuration, and mounts the paths as express
    routes. Supported formats include YAML and JSON.

    If no `routesFiles` is specified, it will fall back to the resource
    store `appConfig.routesFiles` value.

    @method attachRoutes
    @public
    @param {String|Array} routeFiles optional absolute paths to `routes.*`
    **/
    attachRoutes: function (routesFiles) {
        var my = module.exports,
            app = this._app,     // express app
            mojito = app.mojito, // the mojito instance
            store = mojito.store,
            appRoot = mojito.options.root,
            encodeRouteName = mojito.Y.mojito.util.encodeRouteName;

        if (typeof routesFiles === 'undefined') {
            routesFiles = store.getStaticAppConfig().routesFiles;
        }
        if (!Array.isArray(routesFiles)) {
            routesFiles = [routesFiles];
        }

        function registerRoutes(routes) {
            Object.keys(routes).forEach(function (name) {
                var route = routes[name];
                route.annotations = route.annotations || {};
                // By default the "client" annotation is considered true, unless specified as false.
                // This means that routes are exposed to the client unless the "client" annotation
                // is set to false.
                route.annotations.client = route.annotations.client !== false;
                app.annotate(route.path, route.annotations);
                Object.keys(route.verbs).forEach(function (verb) {
                    debug('[%s %s] installing handler', route.call, route.path);
                    verb = verb.toLowerCase();

                    app[verb](route.path, dispatcher.dispatch(route.call, route.params));
                    app.map(route.path, name);
                    app.map(route.path, encodeRouteName(verb, route.call));
                });
            });
        }

        routesFiles.forEach(function (routesFile) {
            var routes;
            // relative paths are relative to the application
            routesFile = libpath.resolve(appRoot, routesFile);
            debug('loading routes config: %s', routesFile);
            routes = my.getRoutes(routesFile);
            registerRoutes(routes);
        });

    }

};

