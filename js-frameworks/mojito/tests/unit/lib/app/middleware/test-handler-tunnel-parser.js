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

        factory = require(Y.MOJITO_DIR + 'lib/app/middleware/mojito-handler-tunnel-parser'),
        req,
        nextCallCount,
        middleware,
        store,
        config;


    Y.Test.Runner.add(new Y.Test.Case({
        name: 'tunnel handler parser tests',

        setUp: function () {
            nextCallCount = 0;

            store = {
                getAppConfig: function () {
                    return {};
                }
            };

            config = {
                store: store
            };

            req = {
                app: {
                    mojito: {
                        store: store
                    }
                },
                url: '/tunnel',
                headers: {
                    'x-mojito-header': 'tunnel'
                },
                method: 'POST'
            };
        },

        tearDown: function () {
            store           = null;
            config          = null;
            nextCallCount   = null;
            req             = null;
        },

        'test trailing slashes are removed from tunnel URIs': function () {
            store.getAppConfig = function () {
                return {
                    tunnelPrefix: '/spinach/'
                };
            };
            req.url = '/spinach';
            req.headers['x-mojito-header'] = 'nottunnel';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.isObject(req._tunnel.rpcReq, 'request should have been identified as a tunnel request');
            A.areSame(1, nextCallCount, 'next() should have been called');
        },

        'test multiple slashes are squashed into one': function () {
            config.store.getAppConfig = function () {
                return {
                    staticHandling: {
                        prefix: 'brusselsprouts//'
                    }
                };
            };
            req.url = '/brusselsprouts/MojitX/definition.json';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.isObject(req._tunnel.typeReq, 'request should have been identified as a tunnel request');
            A.areSame('MojitX', req._tunnel.typeReq.type, 'should have gotten the correct mojit type');
            A.areSame(1, nextCallCount, 'next() should have been called');
        },

        'test prefix defaults are used if custom prefixes are not declared': function () {
            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.isObject(req._tunnel.rpcReq, 'request should have been identified as an rpc request');
            A.areSame(1, nextCallCount, 'next() should have been called for the rpc request');

            req.url = '/static/MojitX/definition.json';
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.isObject(req._tunnel.typeReq, 'request should have been identified as a type request');
            A.areSame(2, nextCallCount, 'next() should have been called for the type request');
        },

        'test exit early if not tunnel request': function () {
            req.url = '/spinach';
            req.headers['x-mojito-header'] = 'brusselsprouts';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() should have been called');
            A.isUndefined(req._tunnel, 'next() should have been called immediately');
        },

        'test tunnel request paths are normalized correctly': function () {
            req.url = '/static/MojitX/specs/broccoli.json';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() should have been called');
            A.isObject(req._tunnel.specsReq, 'should have been identified as a specs request');

            req.url = '/tunnel/static/MojitX/specs/broccoli.json';
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(2, nextCallCount, 'next() should have been called');
            A.isObject(req._tunnel.specsReq, 'should have been identified as a specs request');
        },

        'test tunnel spec request is correctly parsed': function () {
            req.url = '/static/MojitX/specs/broccoli.json';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() should have been called');
            A.isObject(req._tunnel.specsReq, 'should have been identified as a specs request');
            A.areSame('MojitX', req._tunnel.specsReq.type, 'should have parsed out the mojit type correctly');
            A.areSame('broccoli', req._tunnel.specsReq.name, 'should have parsed out the file name correctly');
        },

        'test tunnel type request is correctly parsed': function () {
            req.url = '/static/MojitY/definition.json';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() should have been called');
            A.isObject(req._tunnel.typeReq, 'should have been identified as a type request');
            A.areSame('MojitY', req._tunnel.typeReq.type, 'should have parsed out the mojit type correctly');
        },

        'test compatibility with tunnelUrl option': function () {
            req.url = '/tunnel;_ylt=A0oGdV8GMC1RcBgAQNhXNyoA;_ylu=X3oDMTE5aWhtbjdhBHNlYwNvdi10b3AEY29sbwNzazEEdnRpZANTTUUwNDFfMTU0BHBvcwMx';
            req.headers['x-mojito-header'] = 'huggies';

            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() handler should have been called');
            A.isObject(req._tunnel.rpcReq, 'should have been identified as an rpc request');
        },


        'test tunnel rpc request is correctly parsed': function () {
            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() should have been called');
            A.isObject(req._tunnel.rpcReq, 'should have been identified as an rpc request');

            // Although 'wipes' is not a tunnel header, we should identify this
            // request as a tunnel request because the tunnelPrefix matches.
            req.headers['x-mojito-header'] = 'wipes';
            req.url = '/diapers';
            config.store.getAppConfig = function () {
                return {
                    tunnelPrefix: '/diapers'
                };
            };
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(2, nextCallCount, 'next() should have been called');
            A.isObject(req._tunnel.rpcReq, 'should have been identified as an rpc request');
        }
    }));
});
