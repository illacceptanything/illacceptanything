/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/

/*
 * Test suite for the store.client.js file functionality.
*/
YUI().use('mojito-client-store', 'test', 'querystring-stringify-simple', 'io', 'json', function (Y) {

    var suite = new Y.Test.Suite('mojito-client-store-tests'),
        A = Y.Assert,
        OA = Y.ObjectAssert;

    suite.add(new Y.Test.Case({

        name: 'Store tests',

        setUp: function () {
            this.store = new Y.mojito.ResourceStore({
                appConfig: {
                    foo: 1,
                    staticHandling: {
                        prefix: "mystaticprefix"
                    }
                },
                pathToRoot: '/root',
                context: {
                    env: 'dev'
                },
                routes: '/foo/bar'
            });
        },

        tearDown: function () {

        },

        'test buildUrl': function () {
            var self = this,
                tests = [
                    {
                        input: '/test/',
                        expectation: '/root/test/'
                    },
                    {
                        input: 'test',
                        expectation: '/root/test'
                    },
                    {
                        context: {
                            env: 'dev'
                        },
                        input: 'test',
                        expectation: '/root/test?env=dev'
                    },
                    {
                        context: {
                            env: 'dev',
                            test: 'test'
                        },
                        input: 'test',
                        expectation: '/root/test?env=dev&test=test'
                    }
                ];

            Y.Array.each(tests, function (test) {
                var output = this.store._buildUrl(test.input, test.context);
                A.areEqual(test.expectation, output, 'buildUrl did not create the correct url');
            }, this);
        },

        'test app config value': function() {
            var config = this.store.getAppConfig();
            A.areEqual(1, config.foo);
        },

        'test app static config value': function() {
            var config = this.store.getStaticAppConfig();
            A.areEqual(1, config.foo);
        },

        'test app static context': function() {
            var context = this.store.getStaticContext();
            A.areEqual('dev', context.env);
        },

        'test app routes': function() {
            var routes = this.store.getRoutes();
            A.areEqual('/foo/bar', routes);
        },

        'test validateContext': function() {
            var valid = this.store.validateContext(this.store.getStaticContext());
            A.areEqual(true, valid);
        },

        'test validateInstance positive': function() {
            var validbase = [1, 2, 3],
                valid;
            validbase.type = "sometype";
            valid = this.store._validateInstance(validbase);
            A.areEqual(true, valid);
        },

        'test validateInstance negtive': function() {
            var invalidbase = [1, 2, 3],
                valid = this.store._validateInstance(invalidbase);
            A.areEqual(false, valid);
        },

        'test expandInstanceForEnv1': function() {
            var instance = {
                type: 'test_mojit_1',
                config: {testKey4: 'other'}
            };
            try {
                this.store.expandInstanceForEnv("client", instance, {}, function(err, base) {});
            } catch (err) {
                A.fail("Got err: " + err.message);
            }
        },

        'test expandInstanceForEnv2': function() {
            var instance = {
                base: 'test_mojit_1',
                type: 'test_mojit_2',
                config: {testKey4: 'other'}
            };
            //mock _getSpec          
            this.store._getSpec = function(env, id, context, cb) {
                cb(null, instance);
            };
            this.store.expandInstanceForEnv("client", instance, {}, function(err, base) {
                A.areEqual("test_mojit_2", base.type);
                A.areEqual("test_mojit_1", base.base);
                A.areEqual("other", base.config.testKey4);
            });
        },

        'test expandInstanceForEnv3': function() {
            var context = this.store.getStaticContext(),
                mystore = this.store;
            A.throwsError('There was no info in the \"instance\" object', function() {
                mystore.expandInstanceForEnv("client", "mytype", context, function() {});
            });
        },

        'test expandInstanceForEnv4': function() {
            var instance = {
                base: 'test_mojit_1',
                config: {testKey4: 'other'}
            };
            //mock _getSpec          
            this.store._getSpec = function(env, id, context, cb) {
                cb(null, instance);
            };
            this.store.expandInstanceForEnv("client", instance, {}, function(err, base) {
                A.isNotNull(err.message);
                A.areEqual("Instance was not valid.", err.message.match("Instance was not valid."));
            });
        },

        'test expandInstance': function() {
            var instance = { base: 'testbase'},
                context = this.store.getStaticContext();
            //mock expandInstanceForEnv  
            this.store.expandInstanceForEnv = function(env, id, context, cb) {
                cb(null, instance);
            };
            this.store.expandInstance(instance, context, function(err, base) {
                A.areEqual("testbase", base.base);
            });
        },

        'test get type': function() {
            var context = this.store.getStaticContext();
            Y.io = function(url, config) {
                var id = "newid",
                    obj = {};
                obj.responseText = '{ "status": 200 }';
                config.on.complete('myid', obj);
            };
            try {
                this.store._getType("client", "mytype", context, function() {});
            } catch (err) {
                A.fail("Got err: " + err.message);
            }
        },

        'test get spec': function() {
            var context = this.store.getStaticContext();

            Y.io = function(url, config) {
                var id = "newid",
                    obj = {};
                obj.responseText = '{ "status": 200 }';
                config.on.complete('myid', obj);
            };
            try {
                this.store._getSpec("client", "myid", context, function() {});
            } catch (err) {
                A.fail("Got err: " + err.message);
            }
        }

    }));

    Y.Test.Runner.add(suite);

});
