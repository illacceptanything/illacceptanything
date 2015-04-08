/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the output-handler.client.js file functionality.
 */
YUI({useBrowserConsole: true}).use(
    "mojito-output-handler",
    "test",
    "json-parse", // even though this dependency is in output-handler, it only works with it here
    "node",
    function(Y) {

        var suite = new Y.Test.Suite("mojito-output-handler tests");

        suite.add(new Y.Test.Case({
            setUp: function () {
                this.viewId = 'yui_3_5_1_23_1342581977495_7';
                this.callback = function () {};
                this.mojitoClient = Y.Mock();
                this.outputHandler = new Y.mojito.OutputHandler(this.viewId, this.callback, this.mojitoClient);
            },

            "test constructor": function() {
                var outputHandler = this.outputHandler;
                Y.Assert.isObject(outputHandler);
                Y.Assert.areEqual(this.viewId, outputHandler.viewId);
                Y.Assert.areEqual(this.callback, outputHandler.callback);
                Y.Assert.areEqual('', outputHandler.buffer);
                Y.Assert.areEqual(this.mojitoClient, outputHandler.mojitoClient);
            },

            "test flush": function () {
                var outputHandler = this.outputHandler,
                    expectedData = {},
                    expectedMeta = {};
                outputHandler.done = function (data, meta) {
                    Y.Assert.areEqual(expectedData, data);
                    Y.Assert.areEqual(expectedMeta, meta);
                };
                outputHandler.flush(expectedData, expectedMeta);
            },

            "test done without meta": function () {
                var outputHandler = this.outputHandler,
                    inputData = 'test';

                outputHandler.callback = function (err, data, meta) {
                    Y.Assert.isNull(err);
                    Y.Assert.isString(data);
                    Y.Assert.areEqual(inputData, data);
                    Y.Assert.isUndefined(meta);
                };
                outputHandler.done(inputData);
            },

            "test done with binders and json data": function () {
                var self = this,
                    outputHandler = self.outputHandler,
                    inputData = '{"testKey": "testValue"}',
                    inputMeta = {
                        binders: {
                            'yui_3_5_1_2_1342738213108_12': {
                                action: 'index',
                                base: 'detail',
                                guid: 'yui_3_5_1_35_1342738202643_10',
                                instanceId: 'yui_3_5_1_35_1342738202643_10',
                                name: 'FlickrDetailBinderIndex',
                                type: 'FlickrDetail',
                                viewId: 'yui_3_5_1_2_1342738213108_12'
                            }
                        },
                        http: {
                            headers: {
                                'content-type': ['application/json; charset=utf-8']
                            }
                        },
                        view: {
                            binder: 'index',
                            cacheTemplates: true,
                            id: 'yui_3_5_1_2_1342738213108_12',
                            name: 'index'
                        },
                        assets: {}
                    };

                Y.Mock.expect(this.mojitoClient, {
                    method: 'attachBinders',
                    args: [Y.Mock.Value.Object, Y.Mock.Value.String, Y.Mock.Value.String],
                    run: function (binders, viewId, metaViewId) {
                        Y.Assert.isObject(binders);
                        Y.Assert.isObject(binders.yui_3_5_1_2_1342738213108_12);
                        Y.Assert.areEqual(self.viewId, viewId);
                        Y.Assert.areEqual(inputMeta.view.id, metaViewId);
                    }
                });
                outputHandler.callback = function (err, data, meta) {
                    Y.Assert.isNull(err);
                    Y.Assert.isObject(data);
                    Y.Assert.areEqual('testValue', data.testKey);
                    Y.Assert.isObject(meta);
                };
                outputHandler.done(inputData, inputMeta);
                this.wait(function () {
                    Y.Mock.verify(this.mojitoClient);
                });
            },

            "test done with assets": function () {
                var outputHandler = this.outputHandler,
                    inputData = 'test',
                    inputMeta = {
                        assets: {
                            bottom: {
                                js: ['index.js'],
                                css: ['index.css'],
                                blob: ['<meta id="blobTest" />']
                            },
                            top: {
                                js: ['common.js'],
                                css: []
                            }
                        }
                    };

                Y.Get = Y.Mock();
                Y.Mock.expect(Y.Get, {
                    method: 'css',
                    args: [Y.Mock.Value.Any, Y.Mock.Value.Object],
                    run: function (css, options) {
                        Y.Assert.isArray(css);
                        Y.Assert.isFunction(options.onEnd);
                    }
                });
                Y.Mock.expect(Y.Get, {
                    method: 'script',
                    args: [Y.Mock.Value.Any, Y.Mock.Value.Object],
                    count: 2,
                    run: function (js, options) {
                        Y.Assert.isArray(js);
                        Y.Assert.isFunction(options.onEnd);
                    }
                });
                outputHandler.callback = function (err, data, meta) {
                    Y.Assert.isNull(err);
                    Y.Assert.isString(data);
                    Y.Assert.areEqual(inputData, data);
                    Y.Assert.isObject(meta);
                };
                outputHandler.done(inputData, inputMeta);
                Y.Assert.isObject(Y.one('#blobTest'));
                Y.Mock.verify(Y.Get);
            },

            "test error": function () {
                var outputHandler = this.outputHandler,
                    expectedError = 'Test error';
                outputHandler.callback = function (err) {
                    Y.Assert.areEqual(expectedError, err);
                };
                outputHandler.error(expectedError);
            }
        }));

        Y.Test.Runner.add(suite);
    }
);
