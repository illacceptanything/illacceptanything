/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the analytics.common.js file functionality.
 */
YUI().use('mojito-analytics-addon', 'test', function(Y) {

    var suite = new Y.Test.Suite("mojito-analytics-addon tests"),
        A = Y.Assert;

    suite.add(new Y.Test.Case({

        'test setMergeFunction': function() {
            var analyticsValue1 = {'foo': 'bar'},
                analyticsValue2 = {'foo': 'baz'},
                testStore = {},
                mockAc = {},
                retrieved,
                addon = new Y.mojito.addons.ac.analytics(null, null, mockAc);

            //  First, we do a store/retrieve cycle using the standard
            //  merge function (which is preconfigured by the analytics
            //  addon to be Y.mojito.util.metaMerge). It will *NOT*
            //  overlay values.

            mockAc.meta = Y.Mock();

            Y.Mock.expect(mockAc.meta, {
                method: 'store',
                args: [Y.Mock.Value.String, Y.Mock.Value.Object],
                callCount: 2,
                run: function(key, val) {
                    testStore[key] = val;
                }
            });
            Y.Mock.expect(mockAc.meta, {
                method: 'retrieve',
                args: [Y.Mock.Value.Function, undefined],
                run: function(cb) {
                    cb(testStore);
                }
            });

            addon.store(analyticsValue1);
            addon.store(analyticsValue2);

            addon.retrieve(function(val) {
                retrieved = val;
            });

            Y.Mock.verify(mockAc.meta);

            A.areEqual(retrieved.foo, 'bar', 'got wrong value');

            //  ---------------

            //  Then, we do a store/retrieve cycle using a custom
            //  merge function. It will *ALWAYS* overlay values.

            //  Reset the store and the mock
            testStore = {};
            mockAc = {};

            mockAc.meta = new Y.Mock();

            Y.Mock.expect(mockAc.meta, {
                method: 'store',
                args: [Y.Mock.Value.String, Y.Mock.Value.Object],
                callCount: 2,
                run: function(key, val) {
                    testStore[key] = val;
                }
            });
            Y.Mock.expect(mockAc.meta, {
                method: 'retrieve',
                args: [Y.Mock.Value.Function, undefined],
                run: function(cb) {
                    cb(testStore);
                }
            });

            addon = new Y.mojito.addons.ac.analytics(null, null, mockAc);
            addon.setMergeFunction(
                function(to, from, clobber) {
                    var k;
                    for (k in from) {
                        if (from.hasOwnProperty(k)) {
                            to[k] = from[k];
                        }
                    }
                    return to;
                }
            );

            addon.store(analyticsValue1);
            addon.store(analyticsValue2);

            addon.retrieve(function(val) {
                retrieved = val;
            });

            Y.Mock.verify(mockAc.meta);

            A.areEqual(retrieved.foo, 'baz', 'got wrong value');
        },

        'test stored analytics defers to meta addon for store and retrieve': function() {
            var analyticsValue = {foo: 'bar'},
                mockAc = {},
                retrieved,
                addon = new Y.mojito.addons.ac.analytics(null, null, mockAc);

            mockAc.meta = Y.Mock();

            Y.Mock.expect(mockAc.meta, {
                method: 'store',
                args: ['analytics', Y.Mock.Value.Object],
                run: function(key, val) {
                    A.isObject(val);
                    A.areEqual(analyticsValue.foo, val.foo, 'wrong value sent to meta.store');
                }
            });
            Y.Mock.expect(mockAc.meta, {
                method: 'retrieve',
                args: [Y.Mock.Value.Function, undefined],
                run: function(cb) {
                    cb({analytics: 'result'});
                }
            });

            addon.store(analyticsValue);
            addon.retrieve(function(val) {
                retrieved = val;
            });

            Y.Mock.verify(mockAc.meta);
            A.areEqual('result', retrieved, 'wrong retrieved meta value');
        }

    }));

    Y.Test.Runner.add(suite);

});
