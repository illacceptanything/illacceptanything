/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the tunnel-client.common.js file functionality.
 */
YUI({useBrowserConsole: true}).use(
    "mojito-tunnel-client",
    "test",
    "json-parse",
    function(Y) {

        var suite = new Y.Test.Suite("mojito-tunnel-client tests");

        suite.add(new Y.Test.Case({
            setUp: function () {
                this.appConfig = {
                    tunnelPrefix: '/tunnel',
                    tunnelTimeout: 10000
                };
                this.tunnelClient = new Y.mojito.TunnelClient(this.appConfig);
            },

            "test constructor": function () {
                var tunnelClient = this.tunnelClient;

                Y.Assert.areEqual(this.appConfig, tunnelClient._appConfig);
            },

            "test tunnelUrl override": function () {
                var appConfig = this.appConfig,
                    tunnelClient = this.tunnelClient,
                    tunnelUrl = '/tunnel;_ylt=A0oGdV8GMC1RcBgAQNhXNyoA',
                    command = {
                        _tunnelUrl: tunnelUrl
                    };

                tunnelClient._makeRequest = function (url) {
                    Y.Assert.isString(url);
                    Y.Assert.areEqual(tunnelUrl, url);
                };

                tunnelClient.rpc(command);
            },

            "test rpc success": function () {
                var appConfig = this.appConfig,
                    tunnelClient = this.tunnelClient,
                    adapter = Y.Mock(),
                    command = {},
                    response = {
                        responseText: Y.JSON.stringify({
                            data: {
                                html: '<div></div>',
                                meta: {
                                    http: {
                                        code: 200
                                    }
                                }
                            }
                        })
                    };

                tunnelClient._makeRequest = function (url, config) {
                    Y.Assert.isString(url);
                    Y.Assert.areEqual(appConfig.tunnelPrefix, url);
                    Y.Assert.isObject(config);
                    Y.Assert.areEqual('POST', config.method);
                    Y.Assert.areEqual(Y.JSON.stringify(command), config.data);
                    Y.Assert.isFunction(config.on.success);
                    Y.Assert.isFunction(config.on.failure);
                    Y.Assert.areEqual(config.on.scope, tunnelClient);
                    Y.Assert.areEqual(tunnelClient, config.context);
                    Y.Assert.areEqual(appConfig.tunnelTimeout, config.timeout);
                    Y.Assert.isObject(config.headers);
                    Y.Assert.areEqual('application/json', config.headers['Content-Type']);
                    config.on.success(null, response);
                };

                Y.Mock.expect(adapter, {
                    method: 'done',
                    args: [Y.Mock.Value.String, Y.Mock.Value.Object],
                    run: function (html, meta) {
                        Y.Assert.areEqual('<div></div>', html);
                        Y.Assert.areEqual(200, meta.http.code);
                    }
                });
                tunnelClient.rpc(command, adapter);
                Y.Mock.verify(adapter);
            },

            "test rpc failure": function () {
                var appConfig = this.appConfig,
                    tunnelClient = this.tunnelClient,
                    adapter = Y.Mock(),
                    command = {},
                    response = {
                        responseText: Y.JSON.stringify({
                            data: {
                                html: '<div></div>',
                                meta: {
                                    http: {
                                        code: 500
                                    }
                                }
                            }
                        })
                    };

                tunnelClient._makeRequest = function (url, config) {
                    Y.Assert.isString(url);
                    Y.Assert.areEqual(appConfig.tunnelPrefix, url);
                    Y.Assert.isObject(config);
                    Y.Assert.areEqual('POST', config.method);
                    Y.Assert.areEqual(Y.JSON.stringify(command), config.data);
                    Y.Assert.isFunction(config.on.success);
                    Y.Assert.isFunction(config.on.failure);
                    Y.Assert.areEqual(config.on.scope, tunnelClient);
                    Y.Assert.areEqual(tunnelClient, config.context);
                    Y.Assert.areEqual(appConfig.tunnelTimeout, config.timeout);
                    Y.Assert.isObject(config.headers);
                    Y.Assert.areEqual('application/json', config.headers['Content-Type']);
                    config.on.failure(null, response);
                };

                Y.Mock.expect(adapter, {
                    method: 'error',
                    args: [Y.Mock.Value.String],
                    run: function (html) {
                        Y.Assert.areEqual('<div></div>', html);
                    }
                });
                tunnelClient.rpc({}, adapter);
                Y.Mock.verify(adapter);
            }
        }));

        Y.Test.Runner.add(suite);
    }
);
