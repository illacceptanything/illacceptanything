/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint nomen:true, node:true*/

/**
@module moijto-handler-tunnel-parser
**/

'use strict';

var liburl      = require('url'),
    libpath     = require('path'),
    libutil     = require('../../util.js'),

    RE_TRAILING_SLASHES = /\/+$/;

/**
 * Export a function which can parse tunnel requests.
 * @return {Object} The parser.
 */
module.exports = function () {
    var appConfig,
        app,
        store,
        staticPrefix,
        tunnelPrefix;

    return function middlewareMojitoHandlerTunnelParser(req, res, next) {

        if (!appConfig && req.app && req.app.mojito) {
            app = req.app;
            store = app.mojito.store;
            appConfig = store.getAppConfig({}) || {};

            staticPrefix = appConfig.staticHandling && appConfig.staticHandling.prefix;
            tunnelPrefix = appConfig.tunnelPrefix;

            // normalize() will squash multiple slashes into one slash.
            if (staticPrefix) {
                staticPrefix = staticPrefix.replace(RE_TRAILING_SLASHES, '');
                staticPrefix = libpath.normalize('/' + staticPrefix);
            }
            if (tunnelPrefix) {
                tunnelPrefix = tunnelPrefix.replace(RE_TRAILING_SLASHES, '');
                tunnelPrefix = libpath.normalize('/' + tunnelPrefix);
            }

            // normalizing the prefixes (this helps with windows runtime)
            staticPrefix = libutil.webpath(staticPrefix || '/static');
            tunnelPrefix = libutil.webpath(tunnelPrefix || '/tunnel');

        }

        var hasTunnelPrefix = req.url.indexOf(tunnelPrefix) === 0,
            hasTunnelHeader = req.headers['x-mojito-header'] === 'tunnel',
            name,
            type,
            path,
            parts;

        // If we are not tunneling get out of here fast!
        if (!hasTunnelPrefix && !hasTunnelHeader) {
            return next();
        }

        /**
        Tunnel examples

        RPC tunnel:
        /tunnel (or it could just have the tunnel header)

        Type tunnel:
        /static/{type}/definition.json
        /{tunnelPrefix}/{type}/definition.json // custom prefix
        /tunnel/static/{type}/definition.json  // according to a UT

        Spec tunnel:
        /static/{type}/specs/default.json
        /{staticPrefix}/{type}/specs/default.json  // custom prefix
        /tunnel/static/{type}/specs/default.json   // according to a UT
        **/

        path = liburl.parse(req.url).pathname;

        // Normalization step to handle `/{tunnelPrefix}`, `/{staticPrefix}`,
        // and `/{tunnelPrefix}/{staticPrefix}` URLs.
        path = path.replace(staticPrefix, '')
                   .replace(tunnelPrefix, '');

        parts = path.split('/');

        req._tunnel = {};

        // If there was a '/' in the path.
        if (parts.length > 1) {
            // Get the basename without the .json extension.
            name = libpath.basename(path, '.json');

            // Get the mojit type.
            type = parts[1];

            // "Spec" tunnel request
            if (parts[parts.length - 2] === 'specs') {
                req._tunnel.specsReq = {
                    type: type,
                    name: name
                };
            } else if (name === 'definition') {
                // "Type" tunnel request
                req._tunnel.typeReq = {
                    type: type
                };
            }
        } else if (hasTunnelPrefix && req.method === 'POST') {
            // "RPC" tunnel request
            req._tunnel.rpcReq = {};
        }

        return next();
    };
};
