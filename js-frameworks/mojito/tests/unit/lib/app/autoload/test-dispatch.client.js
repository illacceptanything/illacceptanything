/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI.add('mojito-dispatcher-client-tests', function(Y, NAME) {

    var suite = new Y.Test.Suite(NAME),
        A = Y.Assert,
        Mock = Y.Mock,
        dispatcher = Y.mojito.Dispatcher,
        store,
        command,
        adapter;

    suite.add(new Y.Test.Case({

        name: 'dispatch',

        'setUp': function() {
            store = {
                getStaticAppConfig: function() {
                    return { yui: {} };
                },
                getRoutes: function() {
                },
                validateContext: function() {
                },
                expandInstance: function(instance, context, cb) {
                    cb(null, {
                        type: instance.type,
                        id: 'xyz123',
                        instanceId: 'xyz123',
                        'controller-module': 'dispatch',
                        yui: {
                            config: {},
                            langs: [],
                            requires: [],
                            sorted: ['mojito', 'mojito-action-context'],
                            sortedPaths: {}
                        }
                    });
                }
            };

            command = {
                action: 'index',
                instance: {
                    type: 'M'
                },
                context: {
                    lang: 'klingon',
                    langs: 'klingon'
                }
            };

            adapter = {};
        },

        'tearDown': function() {
            store = null;
            command = null;
            adapter = null;
        },

        'test rpc with tunnel': function () {
            var tunnel,
                tunnelCommand;

            tunnel = {
                rpc: function (c, a) {
                    tunnelCommand = c;
                }
            };
            dispatcher.init(store, tunnel);
            dispatcher.rpc(command, {
                error: function () {
                    A.fail('tunnel should be called instead');
                }
            });
            A.areSame(command, tunnelCommand, 'delegate command to tunnel');
        },

        'test rpc without tunnel available': function () {
            var tunnel,
                errorTriggered,
                tunnelCommand;

            tunnel = null;
            errorTriggered = false;
            dispatcher.init(store, tunnel);
            dispatcher.rpc(command, {
                error: function () {
                    errorTriggered = true;
                }
            });
            A.isTrue(errorTriggered, 'if tunnel is not set, it should call adapter.error');
        },

        'test dispatch with command.rpc=1': function () {
            var tunnel,
                tunnelCommand;

            tunnel = {
                rpc: function (c, a) {
                    tunnelCommand = c;
                }
            };
            command.rpc = 1;
            dispatcher.init(store, tunnel);
            dispatcher.dispatch(command, {
                error: function () {
                    A.fail('tunnel should be called instead');
                }
            });
            A.areSame(command, tunnelCommand, 'delegate command to tunnel');
        },

        'test dispatch with invalid mojit': function () {
            var tunnel,
                tunnelCommand;

            tunnel = {
                rpc: function (c, a) {
                    tunnelCommand = c;
                }
            };
            dispatcher.init(store, tunnel);
            // if the expandInstance calls with an error, the tunnel
            // should be tried.
            store.expandInstance = function (instance, context, callback) {
                callback({error: 1});
            };
            dispatcher.dispatch(command, {
                error: function () {
                    A.fail('tunnel should be called instead');
                }
            });
            A.areSame(command, tunnelCommand, 'delegate command to tunnel');
        },

        'test dispatch with valid controller': function () {
            var tunnel,
                useCommand,
                _useController = dispatcher._useController;

            dispatcher.init(store, tunnel);
            // if the expandInstance calls with an error, the tunnel
            // should be tried.
            store.expandInstance = function (instance, context, callback) {
                instance.controller = 'foo';
                Y.mojito.controllers[instance.controller] = {
                    fakeController: true
                };
                callback(null, instance);
            };
            dispatcher._useController = function (c) {
                useCommand = c;
            };
            dispatcher.dispatch(command, {
                error: function () {
                    A.fail('_useController should be called instead');
                }
            });
            A.areSame(command, useCommand, '_useController should be issued to attach modules.');

            // restoring references
            dispatcher._useController = _useController;
        },

        'test dispatch with invalid controller': function () {
            var tunnel,
                useCommand,
                acCommand,
                _createActionContext = dispatcher._createActionContext,
                _useController = dispatcher._useController;

            dispatcher.init(store, tunnel);
            // if the expandInstance calls with an error, the tunnel
            // should be tried.
            store.expandInstance = function (instance, context, callback) {
                instance.controller = 'foo';
                Y.mojito.controllers[instance.controller] = null;
                callback(null, instance);
            };
            dispatcher._useController = function (c) {
                useCommand = c;
            };
            dispatcher._createActionContext = function (c) {
                A.fail('_createActionContext should be called instead');
            };
            dispatcher.dispatch(command, {
                error: function () {
                    A.fail('_useController should be called instead');
                }
            });
            A.areSame(command, useCommand, '_useController should be called based on the original command');

            // restoring references
            dispatcher._createActionContext = _createActionContext;
            dispatcher._useController = _useController;
        },

        'test instance caching workflow': function () {
            var tunnel,
                useCommand,
                _useController = dispatcher._useController;

            dispatcher.init(store, tunnel);
            // if the expandInstance calls with an error, the tunnel
            // should be tried.
            store.expandInstance = function (instance, context, callback) {
                instance.controller = 'foo';
                Y.mojito.controllers[instance.controller] = {
                    foo: function () {
                        // synthetic controller
                    },
                    bar: function () {
                        // synthetic controller
                    }
                };
                callback(null, instance);
            };
            dispatcher._useController = function (c) {
                useCommand = c;
            };
            dispatcher.dispatch({
                action: 'foo',
                instance: {
                    instanceId: 123,
                    type: 'M'
                }
            }, {
                error: function () {
                    A.fail('_useController should be called instead');
                }
            });
            A.areSame(123, useCommand.instance.instanceId, 'instanceId should be preserved during the first round.');

            // triggering the second round
            useCommand.instance.cacheFlag = true;
            useCommand = null;
            dispatcher.dispatch({
                action: 'bar',
                instance: {
                    instanceId: 123,
                    type: 'M'
                }
            }, {
                error: function () {
                    A.fail('_useController should be called instead');
                }
            });
            A.areSame(123, useCommand.instance.instanceId, 'instanceId should be preserved during the second round.');
            A.isTrue(useCommand.instance.cacheFlag, 'command.instance should be re-use if the instanceId is the same.');

            // restoring references
            dispatcher._useController = _useController;
        },

        'test _createActionContext': function () {
            var config,
                ac,
                failure;

            dispatcher.init(store, null);

            Y.mojito.ActionContext = function (c) {
                config = c;
                ac = this;
            };

            // testing proper AC
            dispatcher._createActionContext(command, {
                error: function () {
                    A.fail('ActionContext should be created');
                }
            });
            A.isObject(ac, 'AC object should be created based');
            A.areSame(command, config.command, 'command should be propagated to ActionContext instance');

            // testing failure AC
            Y.mojito.ActionContext = function (c) {
                throw new Error('manually triggering an error to test invalid ac initialization');
            };
            dispatcher._createActionContext(command, {
                error: function () {
                    failure = true;
                }
            });
            A.isTrue(failure, 'adapter.error should be executed if AC fails to initialize');

        },

        'test _useController on demand': function () {
            var config,
                ac,
                _createActionContext = dispatcher._createActionContext,
                used,
                failure,
                adapter = {
                    error: function () {
                        A.fail('_createActionContext should be called instead');
                    }
                };

            dispatcher._createActionContext = Mock();

            Mock.expect(dispatcher, {
                method: '_createActionContext',
                args: [command, adapter],
                returns: []
            });

            dispatcher.init(store, null);

            YUI.add('fooControllerName', function () {
                Y.mojito.controllers.fooControllerName = {};
            });

            command.instance.controller = 'fooControllerName';

            // testing proper AC
            dispatcher._useController(command, adapter);
            Mock.verify(dispatcher);

            // restoring
            dispatcher._createActionContext = _createActionContext;
        }

    }));


    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: ['mojito-dispatcher']});
