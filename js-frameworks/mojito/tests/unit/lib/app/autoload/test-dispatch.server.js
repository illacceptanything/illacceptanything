/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI.add('mojito-dispatcher-server-tests', function(Y, NAME) {

    var suite = new Y.Test.Suite(NAME),
        A = Y.Assert,
        dispatcher,
        store,
        command,
        adapter;

    suite.add(new Y.Test.Case({

        name: 'dispatch',

        'setUp': function() {
            dispatcher = Y.mojito.util.heir(Y.mojito.Dispatcher);
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
            dispatcher = null;
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
            errorTriggered = false;
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
            errorTriggered = false;
            dispatcher.init(store, tunnel);
            dispatcher.rpc(command, {
                error: function () {
                    A.fail('tunnel should be called instead');
                }
            });
            A.areSame(command, tunnelCommand, 'delegate command to tunnel');
        },

        'test dispatch with valid controller': function () {
            var tunnel,
                acCommand;

            errorTriggered = false;
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
            dispatcher._createActionContext = function (c) {
                acCommand = c;
            };
            dispatcher.dispatch(command, {
                error: function () {
                    A.fail('_createActionContext should be called instead');
                }
            });
            A.areSame(command, acCommand, 'AC should be created based on the original command');
        },

        'test dispatch with invalid controller': function () {
            var tunnel,
                adapterErrorCalled;

            errorTriggered = false;
            dispatcher.init(store, tunnel);
            // if the expandInstance calls with an error, the tunnel
            // should be tried.
            store.expandInstance = function (instance, context, callback) {
                callback(null, instance);
            };
            dispatcher._createActionContext = function (c) {
                A.fail('adapter.error should be called instead');
            };
            dispatcher.dispatch(command, {
                error: function () {
                    adapterErrorCalled = true;
                }
            });
            A.isTrue(adapterErrorCalled, 'adapter.error should be called for invalid controllers');
        },

        'test dispatch with invalid context': function () {
            var tunnel,
                adapterError,
                adapterErrorCalled;

            dispatcher.init(store, tunnel);
            // if the expandInstance calls with an error, the tunnel
            // should be tried.
            store.validateContext = function (context) {
                var error = new Error('Invalid context dimension key "foo"');
                error.code = 400; //bad request
                throw error;
            };
            dispatcher._createActionContext = function (c) {
                A.fail('adapter.error should be called instead');
            };
            dispatcher.dispatch(command, {
                error: function (error) {
                    adapterError = error;
                    adapterErrorCalled = true;
                }
            });
            A.isTrue(adapterErrorCalled, 'adapter.error should be called for invalid context');
            A.areEqual(400, adapterError.code, 'adapter.error should be called with custom error');
        },

        'test dispatch with rpc and tunnel': function () {
            var tunnel,
                tunnelCommand;

            tunnel = {
                rpc: function (c, a) {
                    tunnelCommand = c;
                }
            };

            dispatcher.init(store, tunnel);

            // enabling rpc
            command.rpc = true;

            dispatcher.dispatch(command, {
                error: function () {
                    A.fail('adapter.error should not be called, rpc tunnel should be issued');
                }
            });
            A.areSame(command, tunnelCommand, 'tunnel.rpc should be called with the corresponding command');
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

        }

    }));


    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: ['mojito-dispatcher']});
