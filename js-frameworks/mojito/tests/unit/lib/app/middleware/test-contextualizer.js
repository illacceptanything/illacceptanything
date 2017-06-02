/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-test-extra', 'test', function(Y) {
    var A = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert,
        cases = {},

        factory = require(Y.MOJITO_DIR +
            'lib/app/middleware/mojito-contextualizer.js'),

        handler,
        req,
        res,
        nextCalled;

    function nextFn() {
        nextCalled = true;
    };

    cases = {
        name: 'contextualizer middleware tests',

        setUp: function() {
            handler = factory({
                context: {},
                logger: function() {}
            });
            req = {
                app: {
                    mojito: {
                        context: { }
                    }
                },
                url: '/some/uri',
                headers: {'user-agent': null}
            };
            res = null;
            nextCalled = false;
        },

        'test this should not explode': function () {
            A.isTrue(true);
        },

        'contextualize no headers': function() {
            var req = {
                    url: '/amoduleid/anything',
                    headers: {}
                };

            handler(req, res, nextFn);

            A.isNotNull(req.context, 'No context was found in request');
            A.areSame('server', req.context.runtime, 'runtime has wrong value');
            A.isTrue(nextCalled, 'next() was not called');
        },

        'contextualize ua iphone': function() {
            req.headers['user-agent'] = 'iphone';
            handler(req, res, nextFn);
            A.areSame('iphone', req.context.device);
        },

        'contextualize ua ipod': function() {
            req.headers['user-agent'] = 'ipod';
            handler(req, res, nextFn);
            A.areSame('iphone', req.context.device);
        },

        'contextualize ua opera mini': function() {
            req.headers['user-agent'] = 'opera mini';
            handler(req, res, nextFn);
            A.areSame('opera-mini', req.context.device);
        },

        'contextualize ua android': function() {
            req.headers['user-agent'] = 'android';
            handler(req, res, nextFn);
            A.areSame('android', req.context.device);
        },

        'contextualize ua ie-mobile': function() {
            req.headers['user-agent'] = 'windows ce';
            handler(req, res, nextFn);
            A.areSame('iemobile', req.context.device);
        },
        'contextualize ua palm': function() {
            req.headers['user-agent'] = 'palm';
            handler(req, res, nextFn);
            A.areSame('palm', req.context.device);
        },
        'contextualize ua kindle': function() {
            req.headers['user-agent'] = 'kindle';
            handler(req, res, nextFn);
            A.areSame('kindle', req.context.device);
        },
        'contextualize ua blackberry': function() {
            req.headers['user-agent'] = 'blackberry';
            handler(req, res, nextFn);
            A.areSame('blackberry', req.context.device);
        },

        'test some bad/empty lang header values return default': function() {
            req.headers = {'accept-language': ''};
            handler(req, res, nextFn);
            A.areEqual('en', req.context.lang);

            req.headers = {'accept-language': '   '};
            handler(req, res, nextFn);
            A.areEqual('en', req.context.lang);

            req.headers = {'accept-language': null};
            handler(req, res, nextFn);
            A.areEqual('en', req.context.lang);

            // da<space>,
            req.headers = {'accept-language': 'da , en-gb;q=0.8, en;q=0.7'};
            handler(req, res, nextFn);
            A.areEqual('da', req.context.lang);
        },

        'test lang value spaces work': function() {
            //http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.4
            req.headers = {'accept-language': 'da, en-gb;q=0.8, en;q=0.7'};
            handler(req, res, nextFn);
            A.areEqual('da', req.context.lang);
        },

        'test de lang value returns de': function() {
            req.headers = {'accept-language': 'de'};
            handler(req, res, nextFn);
            A.areEqual('de', req.context.lang);
        },

        'test missing lang header value returns default': function() {
            req.headers = {};
            handler(req, res, nextFn);
            A.areEqual('en', req.context.lang);
        },

        'bug4368914 bad case in Accept-Language header': function() {
            req.headers = {'accept-language': 'en-us,en;q=0.7;de;q=0.3'};
            handler(req, res, nextFn);
            A.areEqual('en-US', req.context.lang);
        }

    };

    Y.Test.Runner.add(new Y.Test.Case(cases));
});
