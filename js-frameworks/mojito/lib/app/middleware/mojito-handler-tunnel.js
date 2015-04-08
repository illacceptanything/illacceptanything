/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint nomen:true, node:true*/

/**
@module moijto-handler-tunnel-tunnel
**/

'use strict';

var parser = require('./mojito-handler-tunnel-parser'),
    rpc    = require('./mojito-handler-tunnel-rpc'),
    specs  = require('./mojito-handler-tunnel-specs'),
    type   = require('./mojito-handler-tunnel-type');

/**
 * Export a middleware aggregate.
 * @return {Object} The handler.
 */
module.exports = function () {

    var parserMW = parser(),
        rpcMW    = rpc(),
        specsMW  = specs(),
        typeMW   = type();

    return function middlewareMojitoHandlerTunnel(req, res, next) {
        var middleware = [
            parserMW,
            rpcMW,
            specsMW,
            typeMW
        ];

        function run() {
            var m = middleware.shift();

            if (!m) {
                req._tunnel = null;
                return next();
            }

            m(req, res, function (err) {
                if (err) {
                    return next(err);
                }
                run();
            });
        }

        run();
    };
};
