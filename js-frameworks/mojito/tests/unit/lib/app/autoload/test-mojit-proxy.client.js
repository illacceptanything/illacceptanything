/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the mojito-client.client.js file functionality.
 */
YUI({useBrowserConsole: true}).use(
    "mojito-mojit-proxy",
    "test",
    function(Y) {

        var suite = new Y.Test.Suite("mojito-mojit-proxy tests");

        suite.add(new Y.Test.Case({
            _should: {
                error: {
                    "test destroyChild with invalid ID": true
                }
            },
            setUp: function () {
                this.mojitProxyConfig = {
                    action: 'testAction',
                    binder: 'testBinder',
                    base: 'testBase',
                    node: 'testNode',
                    element: 'testElement',
                    viewId: 'testViewId',
                    instanceId: 'testInstanceId',
                    client: 'testClient',
                    store: 'testStore',
                    type: 'testType',
                    config: {
                        'testKey': 'testValue'
                    },
                    context: {
                        'affinity': 'client'
                    },
                    pageData: {
                        toJSON: function () {
                            return {color: 'orange'};
                        }
                    }
                };
                this.mojitProxy = new Y.mojito.MojitProxy(this.mojitProxyConfig);
                this.exampleChild = {
                    guid: 'yui_3_5_1_23_1342581977495_9',
                    instanceId: 'yui_3_5_1_23_1342581977495_9',
                    type: 'Example',
                    viewId: 'yui_3_5_1_23_1342581977495_7'
                };
            },

            "test constructor": function() {
                var mojitProxy = this.mojitProxy;
                Y.Assert.isObject(mojitProxy);
            },

            "test broadcast": function () {
                var mojitProxy = this.mojitProxy;
                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'doBroadcast',
                    args: [Y.Mock.Value.String, Y.Mock.Value.String, Y.Mock.Value.Object, Y.Mock.Value.Object]
                });
                mojitProxy.broadcast('testEvent', {}, {});
                Y.Mock.verify(mojitProxy._client);
            },

            "test listen": function () {
                var mojitProxy = this.mojitProxy;
                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'doListen',
                    args: [Y.Mock.Value.String, Y.Mock.Value.String, Y.Mock.Value.Function]
                });
                mojitProxy.listen('testEvent', function () {});
                Y.Mock.verify(mojitProxy._client);
            },

            "test unlisten": function () {
                var mojitProxy = this.mojitProxy;
                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'doUnlisten',
                    args: [Y.Mock.Value.String, Y.Mock.Value.String]
                });
                mojitProxy.unlisten('testEvent');
                Y.Mock.verify(mojitProxy._client);
            },

            "test render": function () {
                var mojitProxy = this.mojitProxy;
                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'doRender',
                    args: [Y.Mock.Value.Object, Y.Mock.Value.Object, Y.Mock.Value.String, Y.Mock.Value.Function],
                    run: function (mp, data, view, cb) {
                        Y.Assert.areEqual(mojitProxy, mp);
                        Y.Assert.isObject(data);
                        Y.Assert.areEqual('testName', data.name);
                        Y.Assert.isObject(data.page);
                        Y.Assert.areEqual('orange', data.page.color);
                        Y.Assert.areEqual('index', view);
                        Y.Assert.isFunction(cb);
                    }
                });
                mojitProxy.render({
                    'name': 'testName'
                }, 'index', function () {});
                Y.Mock.verify(mojitProxy._client);
            },

            "test invoke without options": function () {
                var mojitProxyConfig = this.mojitProxyConfig,
                    mojitProxy = this.mojitProxy;

                mojitProxy._client = Y.Mock();
                mojitProxy.query = {}; // Avoid window calls

                Y.Mock.expect(mojitProxy._client, {
                    method: 'executeAction',
                    args: [Y.Mock.Value.Object, Y.Mock.Value.String, Y.Mock.Value.Function],
                    run: function (command, id, cb) {
                        Y.Assert.areEqual(mojitProxyConfig.base, command.instance.base, 'instance base does not match');
                        Y.Assert.areEqual(mojitProxyConfig.type, command.instance.type, 'instance type does not match');
                        Y.Assert.areEqual(mojitProxyConfig.instanceId, command.instance.instanceId, 'instance id does not match');
                        Y.Assert.areEqual('index', command.action, 'incorrect action');
                        Y.Assert.isObject(command.params.body, 'body params should be an object');
                        Y.Assert.isFalse(command.rpc, 'rpc should be false');
                    }
                });

                mojitProxy.invoke('index');
                Y.Mock.verify(mojitProxy._client);
            },

            "test invoke with options": function () {
                var mojitProxy = this.mojitProxy,
                    mojitProxyConfig = this.mojitProxyConfig;

                mojitProxy._client = Y.Mock();
                mojitProxy.query = {}; // Avoid window calls

                Y.Mock.expect(mojitProxy._client, {
                    method: 'executeAction',
                    args: [Y.Mock.Value.Object, Y.Mock.Value.String, Y.Mock.Value.Function],
                    run: function (command, id, cb) {
                        Y.Assert.areEqual(mojitProxyConfig.instanceId, command.instance.instanceId);
                        Y.Assert.areEqual('index', command.action);
                        Y.Assert.isObject(command.params.body);
                        Y.Assert.areEqual('testVal', command.params.body.testKey);
                        Y.Assert.isTrue(command.rpc);
                    }
                });

                mojitProxy.invoke('index', {
                    params: {
                        body: {
                            testKey: 'testVal'
                        }
                    },
                    rpc: true
                });
                Y.Mock.verify(mojitProxy._client);
            },

            "test invoke with tunnelUrl option": function () {
                var mojitProxy = this.mojitProxy,
                    mojitProxyConfig = this.mojitProxyConfig,
                    tunnelUrl = '/tunnel;_ylt=A0oGdV8GMC1RcBgAQNhXNyoA';

                mojitProxy._client = Y.Mock();
                mojitProxy.query = {}; // Avoid window calls

                Y.Mock.expect(mojitProxy._client, {
                    method: 'executeAction',
                    args: [Y.Mock.Value.Object, Y.Mock.Value.String, Y.Mock.Value.Function],
                    run: function (command, id, cb) {
                        Y.Assert.areSame(tunnelUrl, command._tunnelUrl);
                    }
                });

                mojitProxy.invoke('index', {
                    params: {
                        body: {
                            testKey: 'testVal'
                        }
                    },
                    tunnelUrl: tunnelUrl,
                    rpc: true
                });
                Y.Mock.verify(mojitProxy._client);
            },

            "test refreshView": function () {
                var mojitProxy = this.mojitProxy;
                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'refreshMojitView',
                    args: [Y.Mock.Value.Object, Y.Mock.Value.Object, Y.Mock.Value.Function],
                    run: function (mp, options, cb) {
                        Y.Assert.areEqual(mojitProxy, mp);
                        Y.Assert.isObject(options.params);
                        Y.Assert.isObject(options.params.body);
                        Y.Assert.areEqual('testVal', options.params.body.testKey);
                    }
                });
                mojitProxy.refreshView({
                    params: {
                        body: {
                            testKey: 'testVal'
                        }
                    }
                }, function () {});
                Y.Mock.verify(mojitProxy._client);
            },

            "test getFromUrl with key": function () {
                var mojitProxy = this.mojitProxy,
                    id;

                mojitProxy.query = {
                    id: 5
                };

                id = mojitProxy.getFromUrl('id');
                Y.Assert.areEqual(5, id);
            },

            "test getFromUrl without key": function () {
                var mojitProxy = this.mojitProxy,
                    params;

                mojitProxy.query = {
                    id: 5
                };

                params = mojitProxy.getFromUrl();
                Y.Assert.isObject(params);
                Y.Assert.areEqual(5, params.id);
            },

            "test getId": function () {
                var mojitProxyConfig = this.mojitProxyConfig,
                    id = this.mojitProxy.getId();
                Y.Assert.areEqual(mojitProxyConfig.viewId, id);
            },

            "test getChildren": function () {
                var mojitProxy = this.mojitProxy,
                    exampleChild = this.exampleChild,
                    children;

                mojitProxy._client = {
                    _mojits: {
                        testViewId: {
                            children: {
                                example: this.exampleChild
                            }
                        }
                    }
                };

                children = mojitProxy.getChildren();
                Y.Assert.isObject(children);
                Y.Assert.areEqual(exampleChild.guid, children.example.guid);
                Y.Assert.areEqual(exampleChild.instanceId, children.example.instanceId);
                Y.Assert.areEqual(exampleChild.type, children.example.type);
                Y.Assert.areEqual(exampleChild.viewId, children.example.viewId);
            },

            "test destroyChild with valid child id": function () {
                var exampleChild = this.exampleChild,
                    mojitProxy = this.mojitProxy;

                mojitProxy.getChildren = function () {
                    return {
                        example: exampleChild
                    };
                };

                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'destroyMojitProxy',
                    args: [Y.Mock.Value.String, Y.Mock.Value.Boolean],
                    run: function (id, retainNode) {
                        Y.Assert.areEqual(exampleChild.viewId, id);
                        Y.Assert.isTrue(retainNode);
                    }
                });

                mojitProxy.destroyChild('example', true);
            },

            "test destroyChild with valid viewId": function () {
                var exampleChild = this.exampleChild,
                    mojitProxy = this.mojitProxy;

                mojitProxy.getChildren = function () {
                    return {
                        example: exampleChild
                    };
                };

                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'destroyMojitProxy',
                    args: [Y.Mock.Value.String, Y.Mock.Value.Boolean],
                    run: function (id, retainNode) {
                        Y.Assert.areEqual(exampleChild.viewId, id);
                        Y.Assert.isTrue(retainNode);
                    }
                });

                mojitProxy.destroyChild(this.exampleChild.viewId, true);
            },

            "test destroyChild with invalid ID": function () {
                var exampleChild = this.exampleChild,
                    mojitProxy = this.mojitProxy;

                mojitProxy.getChildren = function () {
                    return {
                        example: exampleChild
                    };
                };

                mojitProxy.destroyChild('invalidViewId', true);
            },

            "test destroyChildren": function () {
                var exampleChild1 = this.exampleChild,
                    exampleChild2 = Y.merge(this.exampleChild, {
                        viewId: 'yui_3_5_1_23_1342581977495_8'
                    }),
                    mojitProxy = this.mojitProxy,
                    childrenDestroyed = [];

                mojitProxy.getChildren = function () {
                    return {
                        example1: exampleChild1,
                        example2: exampleChild2
                    };
                };

                mojitProxy.destroyChild = function (id, retainNode) {
                    childrenDestroyed.push(id);
                    Y.Assert.isTrue(retainNode);
                };

                mojitProxy.destroyChildren(true);

                Y.Assert.isTrue(-1 !== childrenDestroyed.indexOf('example1'), 'first example child was not destroyed');
                Y.Assert.isTrue(-1 !== childrenDestroyed.indexOf('example2'), 'second example child was not destroyed');
            },

            "test destroySelf": function () {
                var mojitProxy = this.mojitProxy,
                    mojitProxyConfig = this.mojitProxyConfig;
                mojitProxy._client = Y.Mock();
                Y.Mock.expect(mojitProxy._client, {
                    method: 'destroyMojitProxy',
                    args: [Y.Mock.Value.String, Y.Mock.Value.Boolean],
                    run: function (id, retainNode) {
                        Y.Assert.areEqual(mojitProxyConfig.viewId, id);
                        Y.Assert.isTrue(retainNode);
                    }
                });
                mojitProxy.destroySelf(true);
                Y.Mock.verify(mojitProxy._client);
            },

            "test _destroy": function () {
                var mojitProxy = this.mojitProxy,
                    mojitProxyConfig = this.mojitProxyConfig,
                    childrenDestroyed = false,
                    binderDestroyed = false,
                    binder = {
                        destroy: function () {
                            binderDestroyed = true;
                        }
                    },
                    nodeRemoved = false,
                    node = {
                        remove: function (b) {
                            nodeRemoved = true;
                        }
                    };
                mojitProxy._client = Y.Mock();
                mojitProxy._binder = binder;
                mojitProxy._node = node;
                mojitProxy.destroyChildren = function () {
                    childrenDestroyed = true;
                };
                Y.Mock.expect(mojitProxy._client, {
                    method: 'doUnlisten',
                    args: [Y.Mock.Value.String],
                    run: function (id) {
                        Y.Assert.areEqual(mojitProxyConfig.viewId, id);
                    }
                });
                mojitProxy._destroy(false);
                Y.Mock.verify(mojitProxy._client);
                Y.Assert.isTrue(childrenDestroyed, 'Children were not destroyed');
                Y.Assert.isTrue(binderDestroyed, 'Binder was not destroyed');
                Y.Assert.isTrue(nodeRemoved, 'Node was not removed');
            },

            "test _pause": function () {
                var mojitProxy = this.mojitProxy,
                    onPauseCalled = false,
                    binder = {
                        onPause: function () {
                            onPauseCalled = true;
                        }
                    };
                mojitProxy._binder = binder;
                mojitProxy._pause();
                Y.Assert.isTrue(onPauseCalled, 'onPause function was not called');
            },

            "test _resume": function () {
                var mojitProxy = this.mojitProxy,
                    onResumeCalled = false,
                    binder = {
                        onResume: function () {
                            onResumeCalled = true;
                        }
                    };
                mojitProxy._binder = binder;
                mojitProxy._resume();
                Y.Assert.isTrue(onResumeCalled, 'onResume function was not called');
            }
        }));

        Y.Test.Runner.add(suite);
    }
);
