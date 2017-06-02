/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the cookie.server.js file functionality.
 */
YUI().use('mojito-cookie-addon', 'test', function(Y) {

    var suite = new Y.Test.Suite('mojito-analytics-addon server tests'),
        A = Y.Assert;

    suite.add(new Y.Test.Case({

        'test set cookie': function() {
            var headerAdded = false,
                c = new Y.mojito.addons.ac.cookie(null, null, {
                    http: {
                        addHeader: function(name, cookie) {
                            headerAdded = true;
                            A.areSame('Set-Cookie', name, 'bad cookie header name');
                            A.areSame('key=val', cookie, 'wrong cookie value');
                        },
                        getRequest: function() {}
                    }
                });

            c.set('key', 'val');

            A.isTrue(headerAdded, 'cookie not set');
        },

        'test set cookie with options': function() {
            var headerAdded = false,
                c = new Y.mojito.addons.ac.cookie(null, null, {
                    http: {
                        addHeader: function(name, cookie) {
                            headerAdded = true;
                            A.areSame('Set-Cookie', name, 'bad cookie header name');
                            A.areSame('key=val; Path=path; Domain=domain', cookie, 'wrong cookie value');
                        },
                        getRequest: function() {}
                    }
                });

            c.set('key', 'val', {path: 'path', domain: 'domain'});

            A.isTrue(headerAdded, 'cookie not set');
        },

        'test get cookie by key': function() {
            var c = new Y.mojito.addons.ac.cookie(null, null, {
                    http: {
                        getRequest: function() {
                            return {
                                cookies: {one: 1}
                            };
                        }
                    }
                }),
                value = c.get('one');

            A.areSame(1, value, 'bad cookie value');
        },

        'test get all cookies': function() {
            var c = new Y.mojito.addons.ac.cookie(null, null, {
                    http: {
                        getRequest: function() {
                            return {
                                cookies: 'COOKIES'
                            };
                        }
                    }
                }),
                value = c.get();

            A.areSame('COOKIES', value, 'bad cookie value');
        }

    }));

    Y.Test.Runner.add(suite);

});