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
        libpath = require('path'),
        mockery = require('mockery'),
        suite = new Y.Test.Suite('lib/router tests'),
        mojitoRoot,
        router,
        app,
        mojito,
        routes01Mock = { admin: { path: '/admin', call: 'admin.help' } },
        routes02Mock = { help:  { path: '/help',  call: 'help.index' } };

    suite.add(new Y.Test.Case({
        'setUp': function () {
            var utilMock;

            utilMock = {
                readConfig: function (path) {
                    var o;
                    if (path.indexOf("routes01.json") > -1) {
                        o = [{
                            settings: [ "master" ],
                            admin: {
                                path: '/admin',
                                call: 'admin.help'
                            }
                        }];
                    } else if (path.indexOf("routes02.json") > -1) {
                        o = [{
                            settings: [ "master" ],
                            help: {
                                path: '/help',
                                call: 'help.index'
                            }
                        }];
                    } else {
                        o = { };
                    }
                    return o;
                }
            };

            mockery.enable({
                warnOnReplace: false,
                warnOnUnregistered: false,
                useCleanCache: true
            });
            mockery.registerMock('./util', utilMock);

            mojitoRoot = libpath.resolve(__dirname, '../../..');
            router = require('../../../lib/router');

            // fake express app instance
            app = {
                mojito: {
                    Y: { mojito: { util: { encodeRouteName: function (a, b) { return a + '#' + b; } } } },
                    options: {
                        // set to empty string because our util.readConfig()
                        // mock expect 'routes01.json'
                        root: ''
                    }
                },
                annotate: function () {}
            };
            router._app = app;
        },
        'tearDown': function () {
            mockery.deregisterMock('./util');
            mockery.disable();

            app = undefined;
            router._app = undefined;
        },

        'test readConfigYCB': function () {
            A.isFunction(router.readConfigYCB);

            var o;

            o = router.readConfigYCB('routes01.json');
            OA.areEqual(routes01Mock.admin,
                       o.admin,
                       'routes01.json did not match');
        },

        'test buildRoute': function () {
            A.isFunction(router.buildRoute);

            var o;

            o = router.buildRoute('foo', {
                path: '/foo',
                call: 'foo.bar'
            });

            A.areEqual('foo', o.name, 'missing name');
            A.areEqual(true, o.verbs.GET, 'missing verbs');
        },

        'test getRoutes': function () {
            A.isFunction(router.getRoutes);

            var o,
                fix;

            fix = {
                admin: {
                    name: "admin",
                    path: "/admin",
                    call: "admin.help",
                    verbs: { GET: true }
                }
            };

            o = router.getRoutes('routes01.json');

            A.areEqual(fix.admin.name, o.admin.name, 'wrong name');
            A.areEqual(fix.admin.path, o.admin.path, 'wrong path');
            A.areEqual(fix.admin.call, o.admin.call, 'wrong call');
            OA.areEqual(fix.admin.verbs, o.admin.verbs, 'wrong verbs');


        },

        'test attachRoutes': function () {
            A.isFunction(router.attachRoutes);

            var o,
                adminCalled = false,
                helpCalled = false,
                mapCalled = false,
                postCalled = false;

            router._app.map = function (path, name) {
                mapCalled = true;
                // verify that name and path matches
                if (path === '/admin') {
                    if (name.indexOf('#') === -1) {
                        A.areEqual('admin', name, 'name should be "admin"');
                    } else {
                        A.areEqual('get#admin.help', name, 'name should be "admin"');
                    }
                } else if (path === '/help') {
                    if (name.indexOf('#') === -1) {
                        A.areEqual('help', name, 'name should be "help"');
                    } else {
                        A.areEqual('get#help.index', name, 'name should be "help"');
                    }
                } else {
                    A.isTrue(false, 'invalid path: ' + path);
                }

            };

            router._app.get = function (path, fn) {
                if (path === "/admin") {
                    A.isFunction(fn, 'fn should be a function');
                    adminCalled = true;
                } else if (path === "/help") {
                    A.isFunction(fn, 'fn should be a function');
                    helpCalled = true;
                } else {
                    A.isFalse(true, 'unexpected path: ' + path);
                }
            };

            app.post = function () {
                postCalled = true;
            };

            o = router.attachRoutes([
                'routes01.json',
                'routes02.json'
            ]);

            A.areEqual(true, adminCalled, '/admin handler was not installed');
            A.areEqual(true, helpCalled, '/help handler was not installed');
            A.areEqual(false, postCalled, 'app.post() should not have been called');
        },

        // verify that routesFiles is retrieved from appConfig
        'test attachRoutes with no args returns empty array': function () {
            A.isFunction(router.attachRoutes);

            var fnCalled = false;

            router._app.mojito.store = {
                getStaticAppConfig: function () {
                    fnCalled = true;
                    return {
                        routesFiles: []
                    };
                }
            };
            router.attachRoutes();

            A.areEqual(true,
                       fnCalled,
                       'mojito.store.getStaticAppConfig() was not called');
        },

        // verify that routesFiles retrieved from appConfig is resolved to the
        // appDir
        'test attachRoutes routesFile is resolved': function () {
            A.isFunction(router.attachRoutes);

            var fnCalled = false,
                getRoutesCalled = false,
                originalGetRoutes;

            originalGetRoutes = router.getRoutes;
            router.getRoutes = function (routesFile) {
                getRoutesCalled = true;
                A.areEqual('/base/app/dir/routes01.json',
                           routesFile,
                           'routesFile mismatch');
                return [];
            };
            router._app.mojito.options.root = '/base/app/dir';
            router._app.mojito.store = {
                getStaticAppConfig: function () {
                    fnCalled = true;
                    return {
                        routesFiles: ['routes01.json']
                    };
                }
            };
            router.attachRoutes();

            A.areEqual(true,
                       getRoutesCalled,
                       'router.getRoutes() was not called');
            A.areEqual(true,
                       fnCalled,
                       'mojito.store.getStaticAppConfig() was not called');
            router.getRoutes = originalGetRoutes;
        },

        // verify getStaticAppConfig() is not called if attachRoutes is called
        // with empty array
        'test attachRoutes with empty array': function () {
            A.isFunction(router.attachRoutes);

            var fnCalled = false;

            router._app.mojito.store = {
                getStaticAppConfig: function () {
                    fnCalled = true;
                    return {
                        routesFiles: []
                    };
                }
            };
            router.attachRoutes([]);

            A.areEqual(false,
                       fnCalled,
                       'mojito.store.getStaticAppConfig() should not have been called');
        },

        // verify routesFiles is converted to an array if a filename is passed
        'test attachRoutes with string arg': function () {
            A.isFunction(router.attachRoutes);

            var fnCalled = false,
                originalGetRoutes;

            originalGetRoutes = router.getRoutes;
            router._app.mojito.options.root = '/base/app/dir';
            router.getRoutes = function (arg) {
                fnCalled = true;
                A.isString(arg, 'arg should be of type array');
                A.areEqual('/base/app/dir/routes.json',
                           arg,
                           'incorrect routes.json filename');
                return []; // HACK so that registerRoutes does nothing
            };
            router.attachRoutes('routes.json');

            A.areEqual(true,
                       fnCalled,
                       'router.getRoutes() should have been called');
            router.getRoutes = originalGetRoutes;
        }

    }));

    Y.Test.Runner.add(suite);
});

