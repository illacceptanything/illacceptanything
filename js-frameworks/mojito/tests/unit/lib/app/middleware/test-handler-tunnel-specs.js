/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*global YUI, require*/
/*jslint nomen:true*/

YUI().use('mojito-test-extra', 'test', function (Y) {
    'use strict';

    var A  = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert,

        factory = require(Y.MOJITO_DIR + 'lib/app/middleware/mojito-handler-tunnel-specs'),
        expandInstanceInvoked = false,
        error,
        req,
        res,
        sendData,
        nextCallCount,
        middleware,
        store,
        config;


    Y.Test.Runner.add(new Y.Test.Case({
        name: 'tunnel handler specs tests',

        setUp: function () {
            nextCallCount = 0;

            store = {
                expandInstanceForEnv: function (env, instance, context, callback) {
                    expandInstanceInvoked = true;
                }
            };

            config = {
                store: store
            };

            req = {
                url: '/tunnel',
                _tunnel: {
                    specsReq: {
                        type: 'MojitX',
                        name: 'default'
                    }
                },
                app: {
                    mojito: {
                        store: store
                    }
                }
            };

            res = {
                writeHead: function () {},
                end: function (data) {
                    sendData = data;
                }
            };
        },

        tearDown: function () {
            expandInstanceInvoked = false;

            nextCallCount = 0;
            sendData      = undefined;
            store         = null;
            config        = null;
            req           = null;
            res           = null;
            middleware    = null;
            error         = undefined;
        },

        'handler should exit early if not specs request': function () {
            req._tunnel.specsReq = null;
            middleware = factory();
            middleware(req, res, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() handler should have been called');
            A.isFalse(expandInstanceInvoked, 'should not have attempted to expand the instance');
        },

        'handler should error if "type" is missing': function () {
            req._tunnel.specsReq.type = null;
            middleware = factory();
            middleware(req, res, function (err) {
                error = err;
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() handler should have been called');
            A.isNotUndefined(error, 'next() handler should have received an error');
            A.areSame(404, res.statusCode, 'status code should have been set to 404');
        },

        'handler should error if "name" is missing': function () {
            req._tunnel.specsReq.name = null;
            middleware = factory();
            middleware(req, res, function (err) {
                error = err;
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() handler should have been called');
            A.isNotUndefined(error, 'next() handler should have received an error');
            A.areSame(404, res.statusCode, 'status code should have been set to 404');
        },

        'handler should error if expandInstanceForEnv errors': function () {
            config.store.expandInstanceForEnv = function (env, instance, context, callback) {
                callback(new Error('you have 10 seconds to eat that tomato'));
            };
            middleware = factory();
            middleware(req, res, function (err) {
                error = err;
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() handler should have been called');
            A.areSame(500, res.statusCode, 'status code should have been set to 500');
            A.isNotUndefined(error, 'next() handler should have received an error');
            A.isUndefined(sendData, 'data should not have been sent');
        },

        'test handler response for success': function () {
            var data = 'good job, here is your dessert!';
            config.store.expandInstanceForEnv = function (env, instance, context, callback) {
                callback(null, data);
            };
            middleware = factory();
            middleware(req, res, function (err) {
                error = err;
                nextCallCount += 1;
            });

            A.areSame(0, nextCallCount, 'next() handler should not have been called');
            A.isUndefined(error, 'next() handler should not have received an error');
            A.areEqual(JSON.stringify(data), sendData, 'data should have been sent');
        }
    }));
});
