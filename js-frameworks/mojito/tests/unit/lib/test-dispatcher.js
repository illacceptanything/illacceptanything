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
        dispatcher,
        req,
        res,
        next;

    suite.add(new Y.Test.Case({
        'setUp': function () {
            dispatcher = require('../../../lib/dispatcher');

            req = {
                app: {
                    mojito: {
                        Y: {
                            Lang: {
                                sub: sub
                            }
                        }
                    },
                    routes: {}
                },
                url: '/',
                query: {},
                params: {},
                context: { runtime: 'server' }
            };
            res = {
            };
            next = function () {};
        },
        'tearDown': function () {
            dispatcher.resetRoutesConfig();
        },

        'test resetRoutesConfig': function () {
            A.isFunction(dispatcher.resetRoutesConfig);
        },

        // verify:
        // - handleRequest() is called()
        'test dispatch': function () {
            A.isFunction(dispatcher.dispatch);

            var cb,
                mid,
                handleRequestCalled = false,
                fn;

            cb = function () { };

            fn = dispatcher.handleRequest;
            dispatcher.handleRequest = function (req, res, next) {
                handleRequestCalled = true;
            };

            req.app.routes = {
                get: [{
                    path: '/path',
                    method: 'get',
                    regexp: /^\/path\/?/,
                    keys: [ ],
                    params: { },
                    callbacks: [ cb ]
                }]
            };
            req.url = '/admin';
            req.query = { foo: 'bar' };
            req.params = { foz: 'baz' };
            req.context = { runtime: 'server' };

            mid = dispatcher.dispatch('admin.help');
            A.isFunction(mid, 'dispatch() should return middleware()');
            A.areEqual(3, mid.length, 'wrong # of args');

            mid(req, res, next);

            A.areEqual('help', req.command.action, 'wrong action');
            OA.areEqual({ base: 'admin' }, req.command.instance, 'wrong instance');
            OA.areEqual(req.context, req.command.context, 'wrong command.context');
            OA.areEqual(req.query, req.command.params.url, 'wrong params.url');
            OA.areEqual(req.params, req.command.params.route, 'wrong params.route');
            A.areEqual(true, handleRequestCalled, 'handleRequest was not called');

            dispatcher.handleRequest = fn;
        },

        // verify that parametrized calls are replaced
        'test dispatch() with parametrized calls': function () {
            var mid,
                fn;

            fn = dispatcher.handleRequest;
            dispatcher.handleRequest = function () {};

            req.url = '/:type/:action';
            req.query = { foo: 'bar' };
            req.params = { type: 'admin', action: 'help' };
            req.context = { runtime: 'server' };

            mid = dispatcher.dispatch('{type}.{action}', { foo: 'bar' });

            A.isFunction(mid);

            mid(req, res, next);

            dispatcher.handleRequest = fn; // restore asap

            A.areEqual('admin',
                       req.command.instance.base,
                       'mojit base mismatch');
            A.areEqual('help',
                       req.command.action,
                       'mojit action mismatch');

        },

        // test for Anonymous mojit
        'test dispatch() with parametrized calls for anonymous mojit': function () {
            var mid,
                fn;

            fn = dispatcher.handleRequest;
            dispatcher.handleRequest = function () {};

            req.url = '/:type/:action';
            req.query = { foo: 'bar' };
            req.params = { type: '@Admin', action: 'help' };
            req.context = { runtime: 'server' };

            mid = dispatcher.dispatch('{type}.{action}', { foo: 'bar' });

            A.isFunction(mid);

            mid(req, res, next);

            dispatcher.handleRequest = fn; // restore asap

            A.areEqual('Admin',
                       req.command.instance.type,
                       'mojit type mismatch');
            A.areEqual('help',
                       req.command.action,
                       'mojit action mismatch');

        },


        // mock the request, store
        // verify:
        // - outputHandler.page.* is set
        // - next() is not called
        'test handleRequest when no errors': function () {
            A.isFunction(dispatcher.handleRequest);

            var nextCalled = false,
                dispatcherCalled = false;

            req = {
                command: { },
                context: { runtime: 'server' },
                app: {
                    getRouteMap: function () {
                        return { foobar: { A: 'B' } };
                    },
                    mojito: {
                        Y: {
                            Lang: {
                                sub: sub
                            },
                            log: function () { },
                            mojito: {
                                hooks: {
                                    enableHookGroup: function () { },
                                    hook: function (label, hook, req, res) {
                                        A.areEqual('AppDispatch', label, 'Y.mojito.hooks.hook() label mismatch');
                                    }
                                },
                                Dispatcher: {
                                    init: function (store) {
                                        return {
                                            dispatch: function (cmd, output) {
                                                dispatcherCalled = true;

                                                OA.areEqual({staticAppConfig: 'true'},
                                                            output.page.staticAppConfig,
                                                            'wrong output.page.staticAppConfig');
                                                OA.areEqual({appConfig: 'true'},
                                                            output.page.appConfig,
                                                            'wrong output.page.appConfig');
                                                OA.areEqual({ A: 'B' },
                                                            output.page.routes.foobar,
                                                            'wrong output.page.routes');
                                            }
                                        };
                                    }
                                }
                            }
                        },
                        store: {
                            getStaticAppConfig: function () {
                                return {staticAppConfig: 'true'};
                            },
                            getAppConfig: function (ctx) {
                                OA.areEqual({runtime: 'server'},
                                            ctx,
                                            'wrong ctx');
                                return { appConfig: 'true' };
                            }
                        }
                    }
                }
            };
            res = { };
            next = function () {
                nextCalled = true;
            };

            dispatcher.handleRequest(req, res, next);

            A.areEqual(false, nextCalled, 'next should not have been called');
            A.areEqual(true, dispatcherCalled, 'Y.mojito.Dispatcher.init should have been called');
        },

        // verify that next() is called
        'test handleRequest when no command': function () {
            A.isFunction(dispatcher.handleRequest);

            var nextCalled = false;

            next = function () {
                nextCalled = true;
            };

            dispatcher.handleRequest(req, res, next);

            A.areEqual(true, nextCalled, 'next() should have been called');
        }
    }));

    Y.Test.Runner.add(suite);
});

