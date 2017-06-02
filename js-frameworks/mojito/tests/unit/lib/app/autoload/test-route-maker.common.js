/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint nomen:true, node:true*/
/*global YUI*/

YUI().use('mojito-route-maker', 'mojito-util', 'test', 'dump', function (Y) {
    var suite = new Y.Test.Suite('mojito-route-maker-tests'),
        A = Y.Assert,
        OA = Y.ObjectAssert,
        annotationsFixture,
        routesFixture,
        routeMaker;

    function setupFixtures() {
        var annotations,
            routes,
            fns;

        fns = [
            function (req, res, next) { next(); }
        ];


        // The route configuration should conform to 
        // express-map#getRouteMap() 
        routes = {
            'admin.help': {
                path: '/admin/help',
                callbacks: [ fns ],
                keys: [],
                regexp: /^\/admin\/help\/?$/i,
                annotations: {
                    name: 'admin.help',
                    aliases: ['admin.help', 'get#admin.help'],
                    dispatch: {
                        call: 'admin.help',
                        params: { },
                        options: { }
                    }
                }
            },
            'get#admin.help': {
                path: '/admin/help',
                callbacks: [ fns ],
                keys: [],
                regexp: /^\/admin\/help\/?$/i,
                annotations: {
                    name: 'admin.help',
                    aliases: ['admin.help', 'get#admin.help'],
                    dispatch: {
                        call: 'admin.help',
                        params: { },
                        options: { }
                    }
                }
            },
            'admin.:action': {
                path: '/admin/:action',
                callbacks: [ fns ],
                keys: [
                    { name: 'action', optional: false }
                ],
                regexp: /^\/admin\/(?:([^\/]+?))\/?$/i,
                annotations: {
                    name: 'admin.:action',
                    aliases: ['admin.:action', 'get#admin.support'],
                    dispatch: {
                        call: 'admin.support',
                        params: { },
                        options: { }
                    }
                }
            },
            'get#admin.support': {
                path: '/admin/:action',
                callbacks: [ fns ],
                keys: [
                    { name: 'action', optional: false }
                ],
                regexp: /^\/admin\/(?:([^\/]+?))\/?$/i,
                annotations: {
                    name: 'admin.:action',
                    aliases: ['admin.:action', 'get#admin.support'],
                    dispatch: {
                        call: 'admin.support',
                        params: { },
                        options: { }
                    }
                }
            },

            ':type.support': {
                path: '/:type/support',
                callbacks: [ fns ],
                keys: [
                    { name: 'type', optional: false }
                ],
                regexp: /^\/(?:([^\/]+?))\/support\/?$/i,
                annotations: {
                    name: ':type.support',
                    aliases: [':type.support', 'get#{type}.support'],
                    dispatch: {
                        call: 'admin.support',
                        params: { },
                        options: { }
                    }
                }
            },
            'get#{type}.support': {
                path: '/:type/support',
                callbacks: [ fns ],
                keys: [
                    { name: 'type', optional: false }
                ],
                regexp: /^\/(?:([^\/]+?))\/support\/?$/i,
                annotations: {
                    name: ':type.support',
                    aliases: [':type.support', 'get#{type}.support'],
                    dispatch: {
                        call: 'admin.support',
                        params: { },
                        options: { }
                    }
                }
            },

            ':foo.:bar': {
                path: '/:foobar/:foo',
                callbacks: [ fns ],
                keys: [
                    { name: 'foobar', optional: false },
                    { name: 'foo', optional: false }
                ],
                regexp: /^\/(?:([^\/]+?))\/(?:([^\/]+?))\/?$/i,
                annotations: {
                    name: ':foo.:bar',
                    aliases: [':foo.:bar', 'get#foobar.foo'],
                    dispatch: {
                        call: 'foobar.foo',
                        params: { },
                        options: { }
                    }
                }
            },
            'get#foobar.foo': {
                path: '/:foobar/:foo',
                callbacks: [ fns ],
                keys: [
                    { name: 'foobar', optional: false },
                    { name: 'foo', optional: false }
                ],
                regexp: /^\/(?:([^\/]+?))\/(?:([^\/]+?))\/?$/i,
                annotations: {
                    name: ':foo.:bar',
                    aliases: [':foo.:bar', 'get#foobar.foo'],
                    dispatch: {
                        call: 'foobar.foo',
                        params: { },
                        options: { }
                    }
                }
            },
            ':type.:action': {
                path: '/:type/:action',
                callbacks: [ fns ],
                keys: [
                    { name: 'type', optional: false },
                    { name: 'action', optional: false }
                ],
                regexp: /^\/(?:([^\/]+?))\/(?:([^\/]+?))\/?$/i,
                annotations: {
                    name: ':type.:action',
                    aliases: [':type.:action', 'post#admin.submit'],
                    dispatch: {
                        call: 'admin.submit',
                        params: { },
                        options: { }
                    }
                }
            },
            'post#admin.submit': {
                path: '/:type/:action',
                callbacks: [ fns ],
                keys: [
                    { name: 'type', optional: false },
                    { name: 'action', optional: false }
                ],
                regexp: /^\/(?:([^\/]+?))\/(?:([^\/]+?))\/?$/i,
                annotations: {
                    name: ':type.:action',
                    aliases: [':type.:action', 'post#admin.submit'],
                    dispatch: {
                        call: 'admin.submit',
                        params: { },
                        options: { }
                    }
                }
            }
        };

        annotations = {
            "/admin/help": {
                name: "admin.help",
                names: [ "/admin/help", "get#admin.help" ]
            },
            "/admin/:action": {
                name: "admin.:action",
                names: ["admin.:action", "get#admin.support"]
            },
            "/:type/:action": {
                name: ":type.:action",
                names: [":type.:action", "post#admin.submit"]
            },
            "/:type/support": {
                name: ":type.support",
                names: [":type.support", "get#type.support"]
            },
            "/:foobar/:foo": {
                name: ":foo.:bar",
                names: [ ":foo.:bar", "get#foobar.foo"]
            }
        };
        annotationsFixture = annotations;
        routesFixture = routes;
    }

    suite.add(new Y.Test.Case({
        name: 'make() tests',

        _should: {
            error: {
                // 'test route maker use case #4 (-ve test)': true
                // 'test route maker where there is no match': true
            }
        },

        setUp: function () {
            setupFixtures();
            routeMaker = new Y.mojito.RouteMaker(routesFixture, annotationsFixture, true);
        },
        tearDown: function () {
        },

        // verify: make() returns "null" if not "query" is not found
        'test route maker where there is no match': function () {
            var url = routeMaker.make('admin.doesnotexist', 'delete', {});
            A.areEqual(null, url, 'wrong url for admin.doesnotexist');
        },

        'test linkTo use case #1': function () {
            A.isFunction(routeMaker.linkTo);

            var url = routeMaker.linkTo(':type.support', {
                type: 'admin'
            });
            A.areEqual('/admin/support', url, 'wrong url');
        },

        'test linkTo use case #2': function () {
            var url = routeMaker.linkTo('get#{type}.support', {
                type: 'yahoo'
            });
            A.areEqual('/yahoo/support', url, 'wrong url');
        },

        'test linkTo use case #3': function () {
            var url = routeMaker.linkTo(':foo.:bar', {
                foobar: 'hello',
                foo: 'world'
            });
            A.areEqual('/hello/world', url, 'wrong url');

            url = routeMaker.linkTo('get#foobar.foo', {
                foobar: 'hello',
                foo: 'world'
            });
            A.areEqual('/hello/world', url, 'wrong url');
        },

        // test missing context params
        'test linkTo use case #4': function () {
            var url = routeMaker.linkTo(':foo.:bar', {
                foobar: 'hello'
            });
            A.areEqual('/hello/undefined', url, 'wrong url');
        },


        // exact match
        // path: '/admin/help'
        // call: 'admin.help'
        'test route maker use case #1': function () {
            A.isFunction(routeMaker.make);

            var url = routeMaker.make('admin.help', 'get', {});
            A.areEqual('/admin/help', url, '#1.1 admin.help: bad URL');

            url = routeMaker.make('admin.help?foz=baz&foo=bar&', 'get', {});
            A.areEqual('/admin/help?foz=baz&foo=bar', url, '#1.2 admin.help: bad query');

            url = routeMaker.make('admin.help?foo=bar&foz=baz&', 'get', {src: 'TEST'});
            A.areEqual('/admin/help?foz=baz&foo=bar', url, '#1.3 admin.help: bad query');

            url = routeMaker.make('admin.help?', 'get', {src: 'TEST'});
            A.areEqual('/admin/help?src=TEST', url, '#1.4 admin.help: bad query');

        },
        //
        // path: '/admin/:action'
        // call: 'admin.support'
        'test route maker use case #2': function () {
            A.isFunction(routeMaker.make);
            var url = routeMaker.make('admin.support', 'get', {action: 'contactus'});
            A.areEqual('/admin/contactus', url, '#2.1 admin.support bad URL');

            url = routeMaker.make('admin.support?action=contactus', 'get', {foo: 'bar'});
            A.areEqual('/admin/contactus', url, '#2.2 admin.support bad URL');

            url = routeMaker.make('admin.support?foo=bar', 'get', {action: 'contactus'});
            A.areEqual(null, url, '#2.3 admin.support bad URL');

            // query params sorted in reverse
            url = routeMaker.make('admin.support?action=contactus&foo=bar&foz=baz', 'get', {});
            A.areEqual('/admin/contactus?foz=baz&foo=bar', url, '#2.4 admin.support bad URL');
        },

        // a variation of #2 above
        //
        // path: '/:type/support'
        // call: '/:type/support'
        'test route maker use case #3': function () {
            A.isFunction(routeMaker.make);
            var url = routeMaker.make('admin.support', 'get', {type: 'community'});
            // Expecting "null" because call=admin.support is being used in 
            // multiple entries, and there is not much that can be done to
            // reverse lookup the matching math
            A.areEqual(null, url, '#3.1 admin.support: bad URL');
            // A.areEqual('/community/support', url, '#3.1 admin.support: bad URL');

            url = routeMaker.make('admin.support', 'get', {type: 'community'});
            A.areEqual(null, url, '#3.2 admin.support: bad URL');
            // A.areEqual('/community/support', url, '#3.2 admin.support: bad URL');
        },

        //
        // path: /:type/:action
        // call: 'admin.submit'
        'test route maker use case #4 (+ve test)': function () {
            A.isFunction(routeMaker.make);
            var url = routeMaker.make('admin.submit', 'post', {type: 'community', action: 'submission'});
            A.areEqual('/community/submission', url, '#4.1 admin.submit: bad URL');

            url = routeMaker.make('admin.submit', 'post', {type: 'community', action: 'submission', foo: 'bar'});
            A.areEqual('/community/submission?foo=bar', url, '#4.2 admin.submit: bad URL');

            url = routeMaker.make('admin.submit', 'post', {type: 'community', action: 'submission', foo: 'bar', zoo: 'boo'});
            A.areEqual('/community/submission?zoo=boo&foo=bar', url, '#4.3 admin.submit: bad URL');

            url = routeMaker.make('admin.submit?type=community&action=submission&foo=bar&zoo=boo', 'post', {KEY: 'VALUE'});
            A.areEqual('/community/submission?zoo=boo&foo=bar', url, '#4.4 admin.submit: bad URL');
        },

        // path: /:type/:action
        // call: 'admin.submit'
        // method should be POST, not GET
        'test route maker use case #4 (-ve test)': function () {
            A.isFunction(routeMaker.make);
            var url = routeMaker.make('admin.submit', 'get', {type: 'community', action: 'submission'});
            A.areEqual(null, url, 'admin.submit: bad URL');
        },


        // make sure :foo and :foobar are replaced correctly
        //
        // path: '/:foobar/:foo'
        // call: 'foobar.foo'
        // method: 'GET'
        //
        'test route maker use case #5': function () {
            var url = routeMaker.make('foobar.foo', 'get', { foo: 'world', foobar: 'hello' });
            A.areEqual('/hello/world', url, '#5.1 foobar.foo: bad URL');

            url = routeMaker.make('foobar.foo', 'get', { foo: 'world', foobar: 'hello', x: 'y' });
            A.areEqual('/hello/world?x=y', url, '#5.2 foobar.foo: bad URL');
        },


        // find()
        //
        // find('/admin/help', 'get')
        'test find(/admin/help, get) use case #1': function () {
            var route = routeMaker.find('/admin/help', 'get');
            A.isNotNull(route, 'find(/admin/help, get) not found');
            A.areEqual('admin.help',
                       route.call,
                       'call value does not match admin.help');
        },
        // find('/admin/help', 'post')
        'test find(/admin/help, post) use case #2': function () {
            var route = routeMaker.find('/admin/help', 'post');
            A.isNull(route, 'find(/admin/help, post) should return null');
        },
        // find('/admin/support', 'get') matches get#admin.support
        'test find(/admin/support, get) use case #3': function () {
            var route = routeMaker.find('/admin/support', 'get');
            A.isNotNull(route, 'find(/admin/support, get) not found');
            A.areEqual('admin.support',
                       route.call,
                       'call value does not match admin.support');
            A.areEqual('support', route.params.action, 'action does not match');
        },
        // find('/foo/support', 'get') matches get#{type}.support
        'test find(/foo/support, get) use case #4': function () {
            var route = routeMaker.find('/foo/support', 'get');
            A.isNotNull(route, 'find(/foo/support, get) not found');
            A.areEqual('foo.support',
                       route.call,
                       'call value does not match foo.support');
            A.areEqual('foo', route.params.type, ':type does not match');
        },
        // find('/yahoo/rocks', 'get') matches get#foobar.foo
        'test find(/yahoo/rocks, get) use case #4': function () {
            var route = routeMaker.find('/yahoo/rocks', 'get');
            A.isNotNull(route, 'find(/yahoo/rocks, get) not found');
            A.areEqual('foobar.foo',
                       route.call,
                       'call value does not match foobar.foo');
            A.areEqual('yahoo', route.params.foobar, 'foobar does not match');
            A.areEqual('rocks', route.params.foo, 'bar does not match');
        }

    }));

    Y.Test.Runner.add(suite);
});

