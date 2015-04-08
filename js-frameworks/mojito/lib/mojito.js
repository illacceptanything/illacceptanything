/**
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint nomen:true, node:true*/

/**
Usage:

    var express = require('express'),
        libmojito = require('mojito'),
        app,
        mojito;

    app = express();
    libmojito.extend(app, {});

    app.use(libmojito.middleware());
    app.mojito.attachRoutes();
    ...


@module mojito
**/

'use strict';


var debug = require('debug')('mojito'),
    libmap = require('express-map'),
    libpath = require('path'),
    libdispatcher = require('./dispatcher'),
    liblogger = require('./logger'),
    libmiddleware = require('./middleware'),
    librouter = require('./router'),
    libstore = require('./store'),
    libextend = require('./util').extend,
    Mojito;


//  ----------------------------------------------------------------------------
//  Mojito Global
//  ----------------------------------------------------------------------------

function extend(app, options) {
    if (app['@mojito']) { return app; }

    Object.defineProperty(app, '@mojito', { value: Mojito });

    app.mojito = new Mojito(app, options);

    return app;
}

/**
Mojito extension for Express.

@protected
@class Mojito
@constructor
@param {express.application} app `express` application instance
@param {Object} options server options
    @param {Object} options.context
@uses *express, dispatcher, logger, middleware, router, store
**/
function Mojito(app, options) {

    this._app = app;
    this._config = {};
    this._options = options || {};
    this._options.context = this._options.context || {};
    this._options.root = this._options.root || process.cwd();
    this._options.mojitoRoot = __dirname;
    app.mojito = this;

    libextend(Mojito.prototype, liblogger);

    // extend express-map
    libmap.extend(app);

    this._init();
}


/**
Creates a new instance of mojito and attach to `app`.

@method _init
@protected

**/

Mojito.prototype._init = function () {
    var app = this._app,
        options = this._options,
        context = options.context,
        store;          // reference to resource store

    if (!app.mojito) {
        throw new Error('`app.mojito` was not initialized properly.');
    }

    context.runtime = 'server';

    store = libstore.createStore(options);

    this._configureAppInstance(app, store, options);
};


/**
Configures YUI with both the Mojito framework and all the YUI modules in the
application.

@method _configureYUI
@protected
@param {Object} Y YUI object to configure
@param {Object} store Resource Store which knows what to load
@return {Array} array of YUI module names
**/
Mojito.prototype._configureYUI = function (Y, store) {
    var my = this,
        modules,
        load,
        lang;

    modules = store.yui.getModulesConfig('server', false);
    Y.applyConfig(modules);

    load = Object.keys(modules.modules);

    // NOTE:  Not all of these module names are guaranteed to be valid,
    // but the loader tolerates them anyways.
    for (lang in store.yui.langs) {
        // Make sure that YUI has the module.
        // This prevents warning messages regarding missing modules.
        if (store.Y.Env.lang.hasOwnProperty(lang)) {
            load.push('lang/datatype-date-format_' + lang);
        }
    }
    return load;
};





/**
Adds Mojito framework components to the Express application instance.

@method _configureAppInstance
@protected
@param {express.application} app The Express application instance to Mojito-enable.
@param {ResourceStore} store
@param {Object} options
    @param {Object} options.context static context set at startup
    @param {String} options.mojitoRoot
    @param {String} options.root the app root dir
**/
Mojito.prototype._configureAppInstance = function (app, store, options) {
    var Y = store.Y,
        modules = [];

    modules = this._configureYUI(Y, store);

    // attaching all modules available for this application for the server side
    Y.applyConfig({ useSync: true });

    if (store.lazyMojits) {
        // If lazy loading mojits, only use the essential modules.
        // Other modules will be used during runtime as mojits are loaded.
        modules = ['mojito', 'mojito-util', 'mojito-hooks', 'mojito-dispatcher', 'mojito-hb', 'mojito-mu'];
    }
    if (store.lazyLangs) {
        // en is the default date-format lang, so it should always be loaded.
        modules.push('lang/datatype-date-format_en');
    }

    Y.use.apply(Y, modules);
    Y.applyConfig({ useSync: false });

    libextend(app.mojito, {
        store: store,
        Y: Y,
        context: options.context,
        options: options,
        _app: app, // reference to `app` from within `app.mojito`
        attachRoutes: librouter.attachRoutes
    });

    console.log('✔\tMojito ready to serve.');
};


//  ----------------------------------------------------------------------------
//  EXPORT(S)
//  ----------------------------------------------------------------------------


module.exports = {
    extend: extend,
    middleware: libmiddleware.middleware,
    dispatch: libdispatcher.dispatch,
    dispatcher: libdispatcher.dispatch // alias for `dispatch`
};

