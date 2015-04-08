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

        factory = require(Y.MOJITO_DIR + 'lib/app/middleware/mojito-handler-tunnel'),
        req,
        nextCallCount,
        middleware,
        store,
        config;


    Y.Test.Runner.add(new Y.Test.Case({
        name: 'tunnel handler tunnel tests',

        setUp: function () {
            nextCallCount = 0;
            config = {
                store: {
                    getAppConfig: function () {}
                }
            };
            req = {
                url: '/nodessertunlessyoueatallyourveggies',
                headers: {}
            };
        },

        tearDown: function () {
            nextCallCount   = null;
            config          = null;
            req             = null;
        },

        'handler should run all tunnel middleware if not a tunnel request': function () {
            middleware = factory(config);
            middleware(req, null, function () {
                nextCallCount += 1;
            });

            A.areSame(1, nextCallCount, 'next() handler should have been called');
            A.isNull(req._tunnel, 'should have cleaned up private variable');
        }
    }));
});
