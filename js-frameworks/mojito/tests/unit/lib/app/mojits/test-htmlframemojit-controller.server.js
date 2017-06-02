/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, node:true*/

YUI().use('HTMLFrameMojit', 'test', function(Y, NAME) {

    'use strict';

    var suite = new Y.Test.Suite(NAME),
        A = Y.Test.Assert;

    suite.add(new Y.Test.Case({

        name: 'HTMLFrameMojit user tests',

        'test _renderChild': function() {
            var composite = Y.Mock();

            Y.Mock.expect(composite, {
                method: 'execute',
                args: [Y.Mock.Value.Object, Y.Mock.Value.Function],
                run: function (cfg, cb) {
                    A.isObject(cfg);
                    A.areEqual('foo', cfg.children.child, 'children structure incomplete');
                    cb(); // calling back as usually
                }
            });

            var ac = {
                config: {
                    get: function (name) {
                        return {
                            child: "foo",
                            assets: {}
                        }[name];
                    }
                },
                composite: composite
            };
            Y.mojito.controllers.HTMLFrameMojit._renderChild(ac, function () {});

            Y.Mock.verify(composite);
        },

        'test default action on child mojit': function() {

            var dispatchCalled,
                executeCalled,
                ac = {
                    action: 'index',
                    config: {
                        get: function (name) {
                            return {
                                child: {
                                    base: "child-1"
                                },
                                assets: {}
                            }[name];
                        }
                    },
                    composite: {
                        execute: function (cfg, callback) {
                            executeCalled = {
                                cfg: cfg
                            };
                        }
                    }
                };

            Y.mojito.controllers.HTMLFrameMojit.index(ac);
            A.isObject(executeCalled, 'ac.composite.execute was not executed');
            A.areEqual('index', executeCalled.cfg.children.child.action, 'the default action index was not set');
        },

        'test custom action on child mojit': function() {

            var dispatchCalled,
                executeCalled,
                ac = {
                    action: 'index',
                    config: {
                        get: function (name) {
                            return {
                                child: {
                                    base: "child-1",
                                    action: "custom"
                                },
                                assets: {}
                            }[name];
                        }
                    },
                    composite: {
                        execute: function (cfg, callback) {
                            executeCalled = {
                                cfg: cfg
                            };
                        }
                    }
                };

            Y.mojito.controllers.HTMLFrameMojit.index(ac);
            A.isObject(executeCalled, 'ac.composite.execute was not executed');
            A.areEqual('custom', executeCalled.cfg.children.child.action, 'the custom action was not honored');

        },

        'test index()': function() {

            var dispatchCalled,
                doneCalled,
                executeCalled,
                assetsAdded,
                ac = {
                    config: {
                        get: function (name) {
                            return {
                                child: {
                                    base: "child-1"
                                },
                                assets: {},
                                deploy: false
                            }[name];
                        }
                    },
                    composite: {
                        execute: function (cfg, callback) {
                            executeCalled = {
                                cfg: cfg
                            };
                            callback({}, {
                                assets: {
                                    bottom: {}
                                }
                            });
                        }
                    },
                    done: function(data, viewMeta) {
                        doneCalled = {
                            data: data,
                            viewMeta: viewMeta
                        };
                    },
                    assets: {
                        renderLocations: function() {
                            return {};
                        },
                        getAssets: function() {
                            return [];
                        },
                        addAssets: function(assets) {
                            assetsAdded = assets;
                        }
                    }
                };

            Y.mojito.controllers.HTMLFrameMojit.index(ac);
            A.isObject(executeCalled, 'ac.composite.execute was not called');
            A.areEqual('child-1', executeCalled.cfg.children.child.base,   'the child base config was not honored');

            A.isObject(doneCalled, 'ac.done was not called');

            A.isString(doneCalled.data.title, 'title is required');

            A.areEqual('index', doneCalled.viewMeta.view.name, 'the view name should always be index');

            A.isObject(assetsAdded, 'ac.assets.addAssets was not called');
            A.isObject(doneCalled.viewMeta.assets.bottom, 'assets coming from the child should be inserted in the correct position');
        },

        'test metas edge cases': function() {

            var dispatchCalled,
                doneCalled,
                executeCalled,
                assetsAdded,
                ac = {
                    config: {
                        get: function (name) {
                            return {
                                child: {
                                    base: "child-1"
                                },
                                assets: {},
                                deploy: false
                            }[name];
                        }
                    },
                    composite: {
                        execute: function (cfg, callback) {
                            callback({}, {
                                view: {
                                    name: "trying-to-overrule-parent-view"
                                },
                                http: {
                                    headers: {
                                        'content-type': 'trying-to-overrule-parent-content-type',
                                        'something-extra': 1
                                    }
                                },
                                metaFromChildGoesHere: true
                            });
                        }
                    },
                    done: function(data, viewMeta) {
                        doneCalled = {
                            data: data,
                            viewMeta: viewMeta
                        };
                    },
                    assets: {
                        renderLocations: function() {
                            return {};
                        },
                        getAssets: function() {
                            return [];
                        },
                        addAssets: function(assets) {
                        }
                    }
                };

            Y.mojito.controllers.HTMLFrameMojit.index(ac);
            A.isTrue(doneCalled.viewMeta.metaFromChildGoesHere, 'custom meta should be passed into done');
            A.areEqual('index', doneCalled.viewMeta.view.name, 'the view name should always be index');
            A.areEqual(1, doneCalled.viewMeta.http.headers['something-extra'], 'extra headers from children should be mixed in');
            A.areEqual('text/html; charset="utf-8"', doneCalled.viewMeta.http.headers['content-type'], 'the content-type cannot be overrule');
        }

    }));

    Y.Test.Runner.add(suite);

});
