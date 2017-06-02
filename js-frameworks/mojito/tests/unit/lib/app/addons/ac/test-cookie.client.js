/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the cookie.client.js file functionality.
 */
YUI({useBrowserConsole: true}).use(
    "mojito-cookie-addon",
    "test",
    function(Y) {

        var suite = new Y.Test.Suite("mojito-cookie-addon tests"),
            A = Y.Assert,
            Addon = Y.mojito.addons.ac.cookie,
            _Obj = Y.Mock.Value.Object,
            _Str = Y.Mock.Value.String,
            _Func = Y.Mock.Value.Function,
            addon;

            Y.Cookie = Y.Mock();

        suite.add(new Y.Test.Case({
            setUp: function () {
                addon = new Addon(null, null, null);
            },

            tearDown: function() {
                addon = null;
            },

            "test constructor": function() {
                A.isNotNull(addon);
            },

            "test addon api functions exist": function() {
                A.isFunction(addon.exists);
                A.isFunction(addon.get);
                A.isFunction(addon.getSub);
                A.isFunction(addon.getSubs);
                A.isFunction(addon.remove);
                A.isFunction(addon.removeSub);
                A.isFunction(addon.set);
                A.isFunction(addon.setSub);
                A.isFunction(addon.setSubs);
            },

            "test cookie exists": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'exists'
                });
                addon.exists();
                Y.Mock.verify(Y.Cookie);
            },

            "test cookie get": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'get'
                });
                addon.get();
                Y.Mock.verify(Y.Cookie);
            },
            
            "test cookie getSub": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'getSub'
                });
                addon.getSub();
                Y.Mock.verify(Y.Cookie);
            },

            "test cookie getSubs": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'getSubs'
                });
                addon.getSubs();
                Y.Mock.verify(Y.Cookie);
            },
            
            "test cookie remove": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'remove'
                });
                addon.remove();
                Y.Mock.verify(Y.Cookie);
            },

            "test cookie removeSub": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'removeSub'
                });
                addon.removeSub();
                Y.Mock.verify(Y.Cookie);
            },
            
            "test cookie set": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'set'
                });
                addon.set();
                Y.Mock.verify(Y.Cookie);
            },

            "test cookie setSub": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'setSub'
                });
                addon.setSub();
                Y.Mock.verify(Y.Cookie);
            },
            
            "test cookie setSubs": function() {
                Y.Mock.expect(Y.Cookie, {
                    method: 'setSubs'
                });
                addon.setSubs();
                Y.Mock.verify(Y.Cookie);
            }

        }));

        Y.Test.Runner.add(suite);
    }
);
