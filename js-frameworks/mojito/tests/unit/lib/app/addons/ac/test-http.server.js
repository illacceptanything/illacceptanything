/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-http-addon', 'test', function (Y) {

    var suite = new Y.Test.Suite('mojito-http-addon tests'),
        A = Y.Assert,
        OA = Y.ObjectAssert;

    suite.add(new Y.Test.Case({

        name: 'all functions',

        'getRequest and getResponse use adapter request and response': function() {
            var addon = new Y.mojito.addons.ac.http(null, {
                req: 'req',
                res: 'res'
            }, null);

            A.areSame('req', addon.getRequest(), 'bad request from getRequest()');
            A.areSame('res', addon.getResponse(), 'bad response from getRequest()');
        },

        'addHeader call': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.addHeader('k1', 'v1');

            OA.areEqual(['v1'], addon._respHeaders.k1, 'after one addition, bad headers stored');

            addon.addHeader('k2', 'v2');
            OA.areEqual(['v1'], addon._respHeaders.k1, 'after two additions, previous header changed');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after two additions, bad headers stored');

            addon.addHeader('k1', 'v3');
            OA.areEqual(['v1', 'v3'], addon._respHeaders.k1, 'after setting same header with another val, was not properly stored');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after three additions, previous header changed');
        },

        'addHeaders call': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.addHeaders({
                'k1': 'v1',
                'k2': 'v2'
            });

            OA.areEqual(['v1'], addon._respHeaders.k1, 'after one addition, bad headers stored');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after one addition, bad headers stored');

            addon.addHeaders({
                'k1': 'v3',
                'k2': 'v4',
                'k3': 'v5'
            });
            OA.areEqual(['v1', 'v3'], addon._respHeaders.k1, 'after two additions, previous header was not updated');
            OA.areEqual(['v2', 'v4'], addon._respHeaders.k2, 'after two additions, previous header was not updated');
            OA.areEqual(['v5'], addon._respHeaders.k3, 'after two additions, bad headers stored');

        },

        'addHeader lower-cases all keys': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.addHeader('HappyFunBall', 'v1');

            OA.areEqual(['v1'], addon._respHeaders.happyfunball, 'after one addition, bad headers stored');
        },

        'addHeader security': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.addHeader('HappyFunBall\nxyz', 'v1\n\nv2');

            OA.areEqual(['v1'], addon._respHeaders.happyfunball, 'after one addition, bad headers stored');
        },

        'setHeader call': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.setHeader('k1', 'v1');

            OA.areEqual(['v1'], addon._respHeaders.k1, 'after one addition, bad headers stored');

            addon.setHeader('k2', 'v2');
            OA.areEqual(['v1'], addon._respHeaders.k1, 'after two additions, previous header changed');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after two additions, bad headers stored');

            addon.setHeader('k1', 'v3');
            OA.areEqual(['v3'], addon._respHeaders.k1, 'after setting same header with another val, was not properly stored');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after three additions, previous header changed');
        },

        'setHeader overrides values set by addHeader calls': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.addHeader('k1', 'v1');
            addon.addHeader('k2', 'v2');

            addon.setHeader('k1', 'v3');
            OA.areEqual(['v3'], addon._respHeaders.k1, 'after setting same header with another val, was not properly stored');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after three additions, previous header changed');
        },

        'setHeaders call': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.setHeaders({
                'k1': 'v1',
                'k2': 'v2'
            });

            OA.areEqual(['v1'], addon._respHeaders.k1, 'after one addition, bad headers stored');
            OA.areEqual(['v2'], addon._respHeaders.k2, 'after one addition, bad headers stored');

            addon.setHeaders({
                'k1': 'v3',
                'k2': 'v4',
                'k3': 'v5'
            });
            OA.areEqual(['v3'], addon._respHeaders.k1, 'after two additions, previous header was not updated');
            OA.areEqual(['v4'], addon._respHeaders.k2, 'after two additions, previous header was not updated');
            OA.areEqual(['v5'], addon._respHeaders.k3, 'after two additions, bad headers stored');
        },

        'setHeader lower-cases all keys': function() {
            var addon = new Y.mojito.addons.ac.http(null, null, null);

            addon.setHeader('HappyFunBall', 'v1');

            OA.areEqual(['v1'], addon._respHeaders.happyfunball, 'after one addition, bad headers stored');
        },

        'getHeader gets one header from the adapter request': function() {
            var addon = new Y.mojito.addons.ac.http(null, {
                req: {
                    headers: {
                        foo: 'fooval',
                        bar: 'barval'
                    }
                },
                res: 'res'
            }, null);

            A.areSame('fooval', addon.getHeader('foo'), 'bad header value for foo');
            A.areSame('barval', addon.getHeader('bar'), 'bad header value for bar');
            A.isUndefined(addon.getHeader('baz'), 'value should not be returned for a nonexistant header');
        },

        'getHeader keys are case-insensitive': function() {
            var addon = new Y.mojito.addons.ac.http(null, {
                req: {
                    headers: {
                        FoO: 'fooval',
                        bAr: 'barval'
                    }
                },
                res: 'res'
            }, null);

            A.areSame('fooval', addon.getHeader('foo'), 'bad header value for foo');
            A.areSame('fooval', addon.getHeader('Foo'), 'bad header value for foo');
            A.areSame('barval', addon.getHeader('bar'), 'bad header value for bar');
            A.areSame('barval', addon.getHeader('baR'), 'bad header value for bar');
        },

        'getHeaders gets all headers from the adapter request': function() {
            var theHeaders = {
                foo: 'fooval',
                bar: 'barval'
            };
            var addon = new Y.mojito.addons.ac.http(null, {
                req: {
                    headers: theHeaders
                },
                res: 'res'
            }, null);

            A.areSame(theHeaders, addon.getHeaders(), 'bad headers value');
        }

    }));

    suite.add(new Y.Test.Case({

        name: 'redirect',

        'redirect with code provided': function() {
            var doneCalled = false;
            var addon = new Y.mojito.addons.ac.http(null, null, {
                done: function(data, meta) {
                    var hdrs;
                    addon.mergeMetaInto(meta);
                    hdrs = meta.http.headers;
                    doneCalled = true;
                    A.areSame(302, meta.http.code, 'bad status code');
                    A.isNull(data, 'data should be null on redirect');
                    A.isArray(hdrs.location, 'no Location header');
                    A.areSame(1, hdrs.location.length, 'too many Location header values');
                    A.areSame('/some/uri', hdrs.location[0], 'bad redirect location');
                    A.isArray(hdrs['content-type'], 'no Content-Type header');
                    A.areSame(1, hdrs['content-type'].length, 'too many Content-Type header values');
                    A.areSame('text/html', hdrs['content-type'][0], 'bad content-type value');
                }
            });

            addon.redirect('/some/uri', 302);

            A.isTrue(doneCalled, 'redirect never called done');

        },

        'redirect uses default status code of 301': function() {
            var doneCalled = false;
            var addon = new Y.mojito.addons.ac.http(null, null, {
                done: function(data, meta) {
                    doneCalled = true;
                    A.areSame(302, meta.http.code, 'bad status code');
                }
            });

            addon.redirect('/some/uri', 302);

            A.isTrue(doneCalled, 'redirect never called done');
        }

    }));

    suite.add(new Y.Test.Case({

        name: 'header specific functions',

        'isXhr returns true when x-requested-with header set': function() {
            var addon = new Y.mojito.addons.ac.http(null, {
                req: {
                    headers: {
                        'x-requested-with': 'XmlHttpRequest'
                    }
                },
                res: 'res'
            }, null);

            A.isTrue(addon.isXhr(), "request was XHR, isXhr() should be true");
        },

        'isXhr returns true case insensitive': function() {
            var addon = new Y.mojito.addons.ac.http(null, {
                req: {
                    headers: {
                        'x-requested-with': 'xmlhttprequest'
                    }
                },
                res: 'res'
            }, null);

            A.isTrue(addon.isXhr(), "request was XHR, isXhr() should be true");
        },

        'isXhr returns false when x-requested-with header has different value': function() {
            var addon = new Y.mojito.addons.ac.http(null, {
                req: {
                    headers: {
                        'x-requested-with': 'NO WAY'
                    }
                },
                res: 'res'
            }, null);

            A.isFalse(addon.isXhr(), "request was XHR, isXhr() should be true");
        }


    }));

    Y.Test.Runner.add(suite);

});
