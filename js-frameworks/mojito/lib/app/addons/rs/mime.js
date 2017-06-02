/*
 * Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, stupid:true*/
/*global YUI*/


/**
 * @module ResourceStoreAddon
 */

/**
 * @class RSAddonMime
 * @extension ResourceStore.server
 */
YUI.add('addon-rs-mime', function(Y, NAME) {

    var libfs = require('fs'),
        libpath = require('path'),
        libmime = require('mime'),
        existsSync = libfs.existsSync || libpath.existsSync,
        URL_PARTS = ['frameworkName', 'appName', 'prefix'];

    function RSAddonMime() {
        RSAddonMime.superclass.constructor.apply(this, arguments);
    }
    RSAddonMime.NS = 'mime';

    Y.extend(RSAddonMime, Y.Plugin.Base, {

        /**
         * This methods is part of Y.Plugin.Base.  See documentation for that for details.
         * @method initializer
         * @param {object} config Configuration object as per Y.Plugin.Base
         * @return {nothing}
         */
        initializer: function(config) {
            this.beforeHostMethod('addResourceVersion', this.addResourceVersion, this);
        },


        /**
         * Using AOP, this is called after the ResourceStore's version.
         * It computes the static handler URL for all resources in all the
         * mojits (as well as the mojit itself).
         * @method addResourceVersion
         * @param {object} res the resource
         * @return {nothing}
         */
        addResourceVersion: function(res) {
            var mimetype,
                charset;

            mimetype = libmime.lookup(res.source.fs.fullPath);
            charset = libmime.charsets.lookup(mimetype);
            res.mime = {
                type: mimetype,
                charset: charset
            };
        }


    });
    Y.namespace('mojito.addons.rs');
    Y.mojito.addons.rs.mime = RSAddonMime;

}, '0.0.1', { requires: ['plugin', 'oop']});