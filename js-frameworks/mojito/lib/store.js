/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, node:true*/


//  ----------------------------------------------------------------------------
//  Prerequisites
//  ----------------------------------------------------------------------------

var libpath = require('path'),
    YUIFactory = require('./yui-sandbox.js');

//  ----------------------------------------------------------------------------
//  Store
//  ----------------------------------------------------------------------------

/**
 * A factory object providing methods for creating and managing ResourceStore
 * instances.
 */
var Store = {};


/**
 * Creates a new server-side resource store instance and returns it.
 * @method createStore
 * @param {object} options An object containing store options.
 * @param {string} options.string Path to application directory.
 * @param {object} options.context Runtime context to apply to all requests.
 * @param {object} options.appConfig Overrides for the application.json file.
 * @param {string "skip"|"initial"|"full"} options.preload Whether to preload
 *      the application. Defaults to "full". If you only care about appConfig
 *      and package.json you can use the 'skip' option which is faster.
 * @return Y.mojito.ResourceStore
 */
Store.createStore = function(options) {

    var store,
        Y,
        YUI,
        appConfig,
        yuiConfig;

    if (!options) {
        options = {};
    }
    if (!options.root) {
        options.root = process.cwd();
    }
    if (!options.context) {
        options.context = {};
    }
    if (!options.preload) {
        options.preload = 'full';
    }
    if (!options.context.runtime) {
        options.context.runtime = 'server';
    }

    // Create a sandboxed YUI instance. This is necessary to avoid shared
    // metadata with the main Mojito execution context and the execution context
    // of the ResourceStore instance.
    YUI = YUIFactory.getYUI();

    // Configure the prerequisites and load the resource store impl code.
    Y = YUI({
        useSync: true,
        modules: {
            'mojito': {
                fullpath: libpath.join(__dirname, 'app/autoload/mojito.common.js')
            },
            'mojito-util': {
                fullpath: libpath.join(__dirname, 'app/autoload/util.common.js')
            },
            'mojito-resource-store': {
                fullpath: libpath.join(__dirname, 'app/autoload/store.server.js')
            }
        }
    });
    Y.use('mojito-resource-store');

    store = new Y.mojito.ResourceStore({
        root: options.root,
        context: options.context,
        appConfig: options.appConfig,
        perf: options.perf
    });

    appConfig = store.getStaticAppConfig();

    Y.applyConfig((appConfig && appConfig.yui && appConfig.yui.config) || {});

    if ('initial' === options.preload) {
        store.preloadInitial();
    } else if ('skip' !== options.preload) {
        store.preload();
    }

    return store;
};


/**
 * Returns the application config.
 * @method getAppConfig
 * @param {string} dir The directory containing the application config.
 * @param {Object} context The runtime context.
 * @return {Object} The application config.
 */
Store.getAppConfig = function(dir, context) {
    var store;
    store = this.createStore({
        root:       dir,
        context:    context,
        preload:    'skip'
    });
    return store.getStaticAppConfig();
};


//  ----------------------------------------------------------------------------
//  EXPORT(S)
//  ----------------------------------------------------------------------------

module.exports = Store;

