/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*global YUI, require, __dirname*/


YUI().use('test', function (Y) {
    var A = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert,
        sub = Y.Lang.sub,
        libpath = require('path'),
        mockery = require('mockery'),
        suite = new Y.Test.Suite('lib/dispatcher tests'),
        libmiddleware,
        req,
        res,
        next;

    suite.add(new Y.Test.Case({
        'setUp': function () {
            libmiddleware = require('../../../lib/middleware');

            req = {};
            res = {};
            next = function () {};
        },
        'tearDown': function () {
            libmiddleware = null;

            req = null;
            res = null;
            next = null;
        },

        'test module.exports': function () {
            A.isFunction(libmiddleware.middleware);
        },

        'test build-in middleware registration': function () {
            A.isFunction(libmiddleware.middleware);
            var mid = libmiddleware.middleware;

            A.isFunction(mid['mojito-handler-static']);
            A.isFunction(mid['mojito-parser-body']);
            A.isFunction(mid['mojito-parser-cookies']);
            A.isFunction(mid['mojito-contextualizer']);
            A.isFunction(mid['mojito-handler-tunnel']);
        },

        'test middleware() function': function () {
            var midFn = libmiddleware.middleware();

            A.isFunction(midFn);
        }

    }));

    Y.Test.Runner.add(suite);
});

