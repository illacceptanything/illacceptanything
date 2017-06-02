/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-partial-addon', 'test', function (Y) {

    var suite = new Y.Test.Suite('mojito-partial-addon tests'),
        Assert = Y.Assert,
        ObjectAssert = Y.ObjectAssert,
        Mock = Y.Mock,
        ViewRenderer = Y.mojito.ViewRenderer,
        ac,
        adapter;

    suite.add(new Y.Test.Case({

        name: 'render() tests',

        'setUp': function () {
            ac = {};
            adapter = {
                page: {
                    staticAppConfig: {
                        viewEngine: 'page static app config view engine'
                    }
                }
            };
        },

        'tearUp': function () {
            Y.mojito.ViewRenderer = ViewRenderer;
        },

        'test callback called with error when view is not found': function() {
            var command = {
                    instance: {
                        views: {}
                    }
                };

            var mockCallback = Mock();
            Mock.expect(mockCallback, {
                method: 'callback',
                args: ['View "missingView" not found']
            });

            var addon = new Y.mojito.addons.ac.partial(command, adapter, ac);
            addon.render(null, 'missingView', mockCallback.callback);

            Mock.verify(mockCallback);
        },

        'test correctly invokes named view engine': function() {
            var data = { key: 'value' },
                command = {
                    instance: {
                        type: 'myInstanceType',
                        views: {
                            myView: {
                                engine: 'myViewEngine',
                                'content-path': 'myContentPath'
                            }
                        }
                    }
                };

            var addon = new Y.mojito.addons.ac.partial(command, adapter, ac);

            var mockRenderer = Mock();
            Mock.expect(mockRenderer, {
                method: 'render',
                args: [data, command.instance,  Mock.Value.Object, Mock.Value.Object, Mock.Value.Object],
                run: function (data, type, mojitView, adapter) {
                    Assert.areEqual('myContentPath', mojitView['content-path']);
                    adapter.done('renderdone');
                }
            });

            var mockCallback = Mock();
            Mock.expect(mockCallback, {
                method: 'callback',
                args: [null, 'renderdone', Mock.Value.Any]
            });

            Mock.expect(Y.mojito, {
                method: 'ViewRenderer',
                args: ['myViewEngine', 'page static app config view engine'],
                returns: mockRenderer
            });

            addon.render(data, 'myView', mockCallback.callback);

            Mock.verify(Y.mojito, 'Y.mojito.ViewRenderer was never called');
            Mock.verify(mockRenderer, 'renderer.render was never called');
            Mock.verify(mockCallback, 'callback from addon.render() was not called');
        },

        'test passes a valid adapter to the view engine': function() {
            var command = {
                    instance: {
                        views: {
                            myView: {}
                        }
                    }
                };

            var mockCallback = Mock();
            Mock.expect(mockCallback, {
                method: 'callback',
                args: [null, 'flushdone', Mock.Value.Any]
            });

            Y.mojito.ViewRenderer = function() {
                this.render = function(a, b, c, adapter, d) {
                    adapter.flush('flush');
                    adapter.done('done');
                };
            };

            var addon = new Y.mojito.addons.ac.partial(command, adapter, ac);
            addon.render(null, 'myView', mockCallback.callback);

            Mock.verify(mockCallback);
        }

    }));

    suite.add(new Y.Test.Case({

        name: 'invoke() tests',

        'setUp': function () {
            ac = {};
            adapter = {
                page: {
                    staticAppConfig: {
                        viewEngine: 'page static app config view engine'
                    }
                },
                hook: {}
            };
        },

        'test populates command object for dispatch': function() {
            var command = {
                    instance: {
                        base: 'myBase',
                        type: 'myType'
                    }
                },
                options = {
                    params: { key: 'value' }
                };

            var mockDispatch = Mock();
            Mock.expect(mockDispatch, {
                method: 'dispatch',
                args: [
                    Mock.Value(function(arg) {
                        Assert.areEqual(arg.instance.base, 'myBase', 'Expected instance.base of the command the addon was created with');
                        Assert.areEqual(arg.instance.type, 'myType', 'Expected instance.type of the command the addon was created with');
                        Assert.areEqual(arg.action, 'myAction', 'Expected the action passed to invoke()');
                        Assert.areEqual(arg.context, 'myContext', 'Expected the ac.context the addon was created with');
                        Assert.areSame(arg.params, options.params, 'Expected the options.params passed to invoke()');
                    }),
                    Mock.Value.Object
                ]
            });

            ac.command = command;
            ac.context = 'myContext';
            ac._dispatch = mockDispatch.dispatch;

            var addon = new Y.mojito.addons.ac.partial(command, adapter, ac);
            addon.invoke('myAction', options, null);

            Mock.verify(mockDispatch);
        },

        'test passes a valid adapter to dispatch': function() {
            var command = {
                instance: {}
            };

            var mockCallback = Mock();
            Mock.expect(mockCallback, {
                method: 'callback',
                args: [null, 'flushdone', Mock.Value(function(arg) {
                    Assert.areEqual(arg.k1, 'flush', '');
                    Assert.areEqual(arg.k2, 'done', '');
                })]
            });

            var mockDispatch = Mock();
            Mock.expect(mockDispatch, {
                method: 'dispatch',
                args: [Mock.Value.Any, Mock.Value(function(arg) {
                    arg.flush('flush', { k1: 'flush', http: { headers: {} } });
                    arg.done('done', { k2: 'done', http: { headers: {} } });
                })]
            });

            var addon = new Y.mojito.addons.ac.partial(command, adapter, { _dispatch: mockDispatch.dispatch });
            addon.invoke(null, { params: {} }, mockCallback.callback);

            Mock.verify(mockDispatch);
            Mock.verify(mockCallback);
        },

        'test ac.params.getAll() is used when options.params not passed': function() {
            var command = {
                instance: {}
            };

            var mockParams = Mock();
            Mock.expect(mockParams, {
                method: 'getAll',
                returns: {}
            });

            var addon = new Y.mojito.addons.ac.partial(command, adapter, {
                _dispatch: function() {},
                params: mockParams
            });
            addon.invoke(null, /* expects function to indicate no params */ function() {});

            Mock.verify(mockParams);
        }

    }));


    Y.Test.Runner.add(suite);

});
