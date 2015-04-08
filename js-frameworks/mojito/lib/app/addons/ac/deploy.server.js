/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node: true*/
/*global YUI*/


/**
 * @module ActionContextAddon
 */
YUI.add('mojito-deploy-addon', function (Y, NAME) {

    'use strict';

    var serialize = require('express-state/lib/serialize');

    /**
     * <strong>Access point:</strong> <em>ac.deploy.*</em>
     * Provides ability to create client runtime deployment HTML
     * @class Deploy.server
     */
    function Addon(command, adapter, ac) {
        this.instance = command.instance;
        this.adapter = adapter;
        this.scripts = {};
        this.ac = ac;
        this.rs = null;
    }


    Addon.prototype = {

        namespace: 'deploy',

        /**
         * Declaration of store requirement.
         * @method setStore
         * @private
         * @param {ResourceStore} rs The resource store instance.
         */
        setStore: function (rs) {
            this.rs = rs;
        },

        /**
         * Builds up the browser Mojito runtime.
         * @method constructMojitoClientRuntime
         * @param {AssetHandler} assetHandler asset handler used to add scripts
         *     to the DOM under construction.
         * @param {object} binderMap information about the binders that will be
         *     deployed to the client.
         */
        constructMojitoClientRuntime: function (assetHandler, binderMap) {
            var store = this.rs,
                contextServer = this.ac.context,
                contextClient,
                appConfigClient,
                yuiConfig = {},
                yuiConfigStr,
                viewId,
                i,
                clientConfig = {},
                clientConfigStr,
                initialModuleList = {},
                initializer, // script for YUI initialization
                pathToRoot,
                pageData = this.adapter && this.adapter.page && this.adapter.page.data,
                pageRoutes = this.adapter && this.adapter.page && this.adapter.page.clientRoutes;

            // Update the loader for this lang if there have been newly added lazy mojits/langs.
            store._updateLoader(this.ac.context.lang);

            contextClient = Y.mojito.util.copy(contextServer);
            contextClient.runtime = 'client';
            appConfigClient = store.getAppConfig(contextClient);
            yuiConfig = store.yui.getYUIConfig(contextClient);
            clientConfig.context = contextClient;

            // yui.config goes through a different channel (yuiConfig),
            // so we should remove it from the appConfigClient.
            if (appConfigClient.yui && appConfigClient.yui.config) {
                appConfigClient.yui.config = undefined;
            }

            // attaching seed files
            for (i = 0; i < yuiConfig.seed.length; i += 1) {
                assetHandler.addAsset('js', 'top', yuiConfig.seed[i]);
            }
            // once the seed files have been inserted in the dom, there
            // is not need to send the info to the client side.
            yuiConfig.seed = undefined;

            // adding the default module for the Y.use statement in the client
            initialModuleList['mojito-client'] = true;

            // add binders' dependencies
            for (viewId in binderMap) {
                if (binderMap.hasOwnProperty(viewId)) {
                    if (binderMap[viewId].name) {
                        initialModuleList[binderMap[viewId].name] = true;
                    }
                }
            }

            clientConfig.binderMap = binderMap;

            // we need the app config on the client for log levels (at least)
            clientConfig.appConfig = appConfigClient;

            // this is mainly used by html5app
            pathToRoot = this.ac.http.getHeader('x-mojito-build-path-to-root');
            if (pathToRoot) {
                clientConfig.pathToRoot = pathToRoot;
            }

            clientConfig.routes = pageRoutes;
            clientConfig.page = {
                data: pageData && pageData.toJSON ? pageData.toJSON() : undefined
            };

            // some cleanup before serializing all the data
            clientConfig.appConfig.routesFiles = undefined;
            clientConfig.appConfig.mojitsDirs = undefined;
            clientConfig.appConfig.appPort = undefined;

            yuiConfigStr = serialize(yuiConfig);
            clientConfigStr = serialize(clientConfig);

            initialModuleList = "'" + Y.Object.keys(initialModuleList).join("','") + "'";

            initializer = '<script type="text/javascript">\n' +
                '    YUI.applyConfig(' + yuiConfigStr + ');\n' +
                '    YUI().use(' + initialModuleList + ', function(Y) {\n' +
                '    window.YMojito = { client: new Y.mojito.Client(' +
                clientConfigStr + ') };\n' +
                '        });\n' +
                '</script>\n';

            // Add the boot script
            assetHandler.addAsset('blob', 'bottom', initializer);
        }

    };

    Y.namespace('mojito.addons.ac').deploy = Addon;

}, '0.1.0', {requires: [
    'mojito-util',
    'mojito-http-addon'
]});
