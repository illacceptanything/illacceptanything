/*
 * Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, stupid:true, node:true */
/*global YUI*/


/**
 * @module ResourceStoreAddon
 */

/**
 * @class RSAddonUrl
 * @extension ResourceStore.server
 */
YUI.add('addon-rs-url', function(Y, NAME) {

    'use strict';

    var libfs   = require('fs'),
        liburl  = require('url'),
        libpath = require('path'),
        libutil = require('../../../util.js'),
        existsSync = libfs.existsSync || libpath.existsSync,
        URL_PARTS = ['frameworkName', 'appName', 'prefix'],
        // TODO:  needs a more future-proof way to do this
        SHARED_STATIC_URLS = {
            'asset-ico-favicon':     '/favicon.ico',
            'asset-txt-robots':      '/robots.txt',
            'asset-xml-crossdomain': '/crossdomain.xml'
        };

    function RSAddonUrl() {
        RSAddonUrl.superclass.constructor.apply(this, arguments);
    }
    RSAddonUrl.NS = 'url';

    Y.extend(RSAddonUrl, Y.Plugin.Base, {

        /**
         * This methods is part of Y.Plugin.Base.  See documentation for that for details.
         * @method initializer
         * @param {object} config Configuration object as per Y.Plugin.Base
         * @return {nothing}
         */
        initializer: function(config) {
            this.host = config.host;
            this.appRoot = config.appRoot;
            this.mojitoRoot = config.mojitoRoot;
            this.beforeHostMethod('resolveResourceVersions', this.resolveResourceVersions, this);
            this.onHostEvent('resolveMojitDetails', this.onResolveMojitDetails, this);
            this.onHostEvent('loadConfigs', this.loadConfigs, this);

            this.loadConfigs();
        },

        loadConfigs: function () {
            var appConfig,
                p,
                part,
                defaults = {};

            appConfig = this.host.getStaticAppConfig();
            this.config = appConfig.staticHandling || {};

            defaults.frameworkName = 'mojito';
            defaults.appName = libpath.basename(this.appRoot);
            defaults.prefix = 'static';
            for (p = 0; p < URL_PARTS.length; p += 1) {
                part = URL_PARTS[p];
                if (this.config.hasOwnProperty(part)) {
                    this.config[part] = this.config[part].replace(/^\//g, '').replace(/\/$/g, '');
                } else {
                    this.config[part] = defaults[part] || '';
                }
            }
        },

        /**
         * Using AOP, this is called before the ResourceStore's version.
         * It computes the static handler URL for all resources in all the
         * mojits (as well as the mojit itself).
         * @method resolveResourceVersions
         * @return {nothing}
         */
        resolveResourceVersions: function() {
            var store = this.get('host'),
                m,
                mojits,
                mojitIsPublic;

            mojits = store.listAllMojits();
            mojits.push('shared');
            for (m = 0; m < mojits.length; m += 1) {
                mojitIsPublic = this._processMojitResource(mojits[m]);
                this._processResources(store.getMojitResourceVersions(mojits[m]), mojitIsPublic);
            }
        },

        /**
         * Called by `resolveResourceVersions` to compute the static handler URL for a mojit.
         * @method _processMojitResource
         * @param mojit the mojit type
         * @return {nothing}
         */
        _processMojitResource: function(mojit) {
            var store = this.get('host'),
                mojitRes = store.getResourceVersions({type: 'mojit', name: mojit, selector: '*'})[0],
                mojitControllerRess,
                packageJson,
                mojitIsPublic;

            if (mojitRes) {
                this._calcResourceURL(mojitRes);
            }

            // TODO: deprecate "public" mojits
            mojitIsPublic = false;
            if (mojitRes) {
                packageJson = libpath.join(mojitRes.source.fs.fullPath, 'package.json');
                packageJson = store.config.readConfigJSON(packageJson);
                if ('public' === (packageJson.yahoo &&
                                  packageJson.yahoo.mojito &&
                                  packageJson.yahoo.mojito['package'])) {
                    mojitIsPublic = true;
                }
            }

            return mojitIsPublic;
        },


        /**
         * Called by `resolveResourceVersions` to compute the static handler URL an array of resources.
         * @method _processMojitResources
         * @param ress the mojit's resources
         * @return {nothing}
         */
        _processResources: function(ress, mojitIsPublic) {
            var r,
                res,
                skip;

            for (r = 0; r < ress.length; r += 1) {
                res = ress[r];
                skip = false;
                if ('config' === res.type) {
                    skip = true;
                }

                // This is mainly used during `mojito build html5app`.
                // In that situation, the user mainly doesn't want to
                // publish each mojit's package.json.  However, Livestand
                // did need to, so this feature allowed them to opt-in.
                if ('config--package' === res.id && mojitIsPublic) {
                    skip = false;
                }

                if (skip) {
                    continue;
                }

                this._calcResourceURL(res);
            }
        },


        /**
         * This is called when the ResourceStore fires this event.
         * It precomputes the mojits' assets URL base, to be used later during
         * getMojitTypeDetails.
         * @method onResolveMojitDetails
         * @param {object} evt The fired event
         * @return {nothing}
         */
        onResolveMojitDetails: function (evt) {
            var env = evt.args.env,
                mojitRes = evt.args.mojitRes,
                details = evt.mojitDetails;
            details.assetsRoot = libutil.webpath(mojitRes.url, 'assets');
        },


        /**
         * Calculates the static handler URL for the resource.
         * @private
         * @method _calcResourceURL
         * @param {object} res the resource for which to calculate the URL
         * @return {nothing}
         */
        _calcResourceURL: function(res) {
            var fs = res.source.fs,
                relativePath = fs.fullPath.substr(fs.rootDir.length + 1),
                urlParts = [liburl.resolve('/', (this.config.prefix || 'static'))];

            // Don't clobber a URL calculated by another RS addon, or bother to
            // proceed for server affinity resources that don't need uris
            if (res.hasOwnProperty('url') || ('server' === res.affinity.affinity)) {
                return;
            }

            if (res.yui && res.yui.name) {
                // any yui module in our app will have a fixed path
                // that has to be really short to optimize combo
                // urls as much as possible. This url will also work
                // in conjuntion with "base", "comboBase" and "root"
                // from application.json->yui->config
                urlParts.push(res.yui.name + '.js');
                res.url = libutil.webpath(urlParts);
                return;
            }

            // FUTURE:  routes.json can specify URLs for static resources

            if ('shared' === res.mojit) {
                if (SHARED_STATIC_URLS[res.id]) {
                    res.url = SHARED_STATIC_URLS[res.id];
                    return;
                }
                if ('mojito' === res.source.pkg.name) {
                    if (this.config.frameworkName) {
                        urlParts.push(this.config.frameworkName);
                    }
                } else {
                    if (this.config.appName) {
                        urlParts.push(this.config.appName);
                    }
                }
            } else {
                if ('mojit' === res.type) {
                    urlParts.push(res.name);
                } else {
                    urlParts.push(res.mojit);
                }
            }

            if ('mojit' === res.type) {
                if ('shared' !== res.name) {
                    res.url = libutil.webpath(urlParts);
                }
                return;
            }

            urlParts.push(relativePath);
            res.url = libutil.webpath(urlParts);
        }


    });
    Y.namespace('mojito.addons.rs');
    Y.mojito.addons.rs.url = RSAddonUrl;

}, '0.0.1', { requires: ['plugin', 'oop']});
