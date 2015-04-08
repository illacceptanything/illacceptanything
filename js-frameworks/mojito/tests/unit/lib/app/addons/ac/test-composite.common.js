/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the composite.common.js file functionality.
 */
YUI().use('mojito-composite-addon', 'test', "async-queue", function(Y) {
    Y.AsyncQueue.defaults.timeout = -1;
    var suite = new Y.Test.Suite("mojito-composite-addon tests"),
        q = new Y.AsyncQueue(),
        A = Y.Assert,
        OA = Y.ObjectAssert;

    suite.add(new Y.Test.Case({

        name: 'composite tests',

        'test no-arg done calls execute with children': function() {
            var command = {
                    instance: {
                        config: {
                            children: {
                                kid_a: { id: 'kid_a', type: 'kida' }
                            }
                        }
                    }
                },
                datamock = {bar: 'mock'},
                metamock = {assets: {foo: 'mock'}},
                adapter = Y.Mock(),
                ac = {
                    done: function(data, meta) {
                        doneCalled = true;
                        A.areEqual('mock', data.bar, "wrong data value");
                        A.areEqual('mock', metamock.assets.foo, "wrong meta value");
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                doneCalled = false;
            c.execute = function(cfg, cb, scope) {
                OA.areEqual(command.instance.config, cfg);
                cb.call(scope, datamock, metamock);
            };

            c.done();

            A.isTrue(doneCalled, "ac done function never called");
        },

        'test done throws error when no children in config': function() {
            var command = {instance: {config: {}}},
                adapter = null,
                ac = {_notify: function() {}},
                c = new Y.mojito.addons.ac.composite(command, adapter, ac);
            try {
                c.done();
                A.fail("composite done should fail without children");
            } catch (err) {
                A.areSame("Cannot run composite mojit children because there are " +
                    "no children defined in the composite mojit spec.", err.message);
            }
        },

        'test done throws error when children is an empty object': function() {
            var command = {instance: {config: {children: {}}}},
                c = new Y.mojito.addons.ac.composite(command, null, {});
            try {
                c.done();
                A.fail("composite done should fail when children key exists and is an empty object");
            } catch (err) {
                A.areSame("Cannot run composite mojit children because there are " +
                    "no children defined in the composite mojit spec.", err.message);
            }
        },

        'test execute dispatches each child': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = {
                    _dispatch: function(command, adapter) {
                        A.isObject(command, "bad command object to dispatch");
                        A.isNotUndefined(adapter, "bad adapter for dispatch");
                        var id = command.instance.id;
                        var meta = {};
                        meta[id] = id + '__meta';
                        adapter.done(id + '__data', meta);
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: {
                        kid_a: { id: 'kid_a', type: 'kida' },
                        kid_b: { id: 'kid_b', type: 'kidb' }
                    }
                },
                config1 = {
                    children: {
                        kid_c: { id: 'kid_c', type: 'kidc' }
                    }
                },
                config2 = {
                    children: {
                        kid_d: { id: 'kid_d', type: 'kidd' }
                    }
                },
                config3 = {
                    children: {
                        kid_e: { id: 'kid_e', type: 'kide' }
                    }
                },
                exeCbCalled = false,
                exeCbCalled1 = false,
                exeCbCalled2 = false,
                exeCbCalled3 = false;

            q.add(function() {
                c.execute(config3, function(data, meta) {
                    exeCbCalled3 = true;
                    A.isString(data.kid_e, "missing kid_e data");
                    A.areSame('kid_e__data', data.kid_e, "wrong kid_e data");
                    A.isString(meta.kid_e, "missing kid_e meta");
                    A.areSame('kid_e__meta', meta.kid_e, "wrong kid_e meta");
                    A.isUndefined(data.kid_a, "should not have info about kid_a");
                    A.isUndefined(meta.kid_a, "should not have info about kid_a");
                    A.isUndefined(data.kid_b, "should not have info about kid_b");
                    A.isUndefined(meta.kid_b, "should not have info about kid_b");
                    A.isUndefined(data.kid_c, "should not have info about kid_c");
                    A.isUndefined(meta.kid_c, "should not have info about kid_c");
                    A.isUndefined(data.kid_d, "should not have info about kid_d");
                    A.isUndefined(meta.kid_d, "should not have info about kid_d");
	            });
            });
            q.add(function() {
                c.execute(config, function(data, meta) {
                    exeCbCalled = true;
                    A.isString(data.kid_a, "missing kid_a data");
                    A.isString(data.kid_b, "missing kid_b data");
                    A.areSame('kid_a__data', data.kid_a, "wrong kid_a data");
                    A.areSame('kid_b__data', data.kid_b, "wrong kid_b data");
                    A.isString(meta.kid_a, "missing kid_a meta");
                    A.isString(meta.kid_b, "missing kid_b meta");
                    A.areSame('kid_a__meta', meta.kid_a, "wrong kid_a meta");
                    A.areSame('kid_b__meta', meta.kid_b, "wrong kid_b meta");

                    c.execute(config1, function(data, meta) {
                        exeCbCalled1 = true;
                        A.isString(data.kid_c, "missing kid_c data");
                        A.areSame('kid_c__data', data.kid_c, "wrong kid_c data");
                        A.isString(meta.kid_c, "missing kid_c meta");
                        A.areSame('kid_c__meta', meta.kid_c, "wrong kid_c meta");
                        A.isUndefined(data.kid_a, "should not have info about kid_a");
                        A.isUndefined(meta.kid_a, "should not have info about kid_a");
                        A.isUndefined(data.kid_b, "should not have info about kid_b");
                        A.isUndefined(meta.kid_b, "should not have info about kid_b");
                        A.isUndefined(data.kid_d, "should not have info about kid_d");
                        A.isUndefined(meta.kid_d, "should not have info about kid_d");
                        A.isUndefined(data.kid_e, "should not have info about kid_e");
                        A.isUndefined(meta.kid_e, "should not have info about kid_e");
		            });

		            c.execute(config2, function(data, meta) {
                        exeCbCalled2 = true;
                        A.isString(data.kid_d, "missing kid_d data");
                        A.areSame('kid_d__data', data.kid_d, "wrong kid_d data");
                        A.isString(meta.kid_d, "missing kid_d meta");
                        A.areSame('kid_d__meta', meta.kid_d, "wrong kid_d meta");
                        A.isUndefined(data.kid_a, "should not have info about kid_a");
                        A.isUndefined(meta.kid_a, "should not have info about kid_a");
                        A.isUndefined(data.kid_b, "should not have info about kid_b");
                        A.isUndefined(meta.kid_b, "should not have info about kid_b");
                        A.isUndefined(data.kid_c, "should not have info about kid_c");
                        A.isUndefined(meta.kid_c, "should not have info about kid_c");
                        A.isUndefined(data.kid_e, "should not have info about kid_e");
                        A.isUndefined(meta.kid_e, "should not have info about kid_e");
                    });

                    A.isUndefined(data.kid_c, "should not have info about kid_c");
                    A.isUndefined(meta.kid_c, "should not have info about kid_c");
                    A.isUndefined(data.kid_d, "should not have info about kid_d");
                    A.isUndefined(meta.kid_d, "should not have info about kid_d");
                    A.isUndefined(data.kid_e, "should not have info about kid_e");
                    A.isUndefined(meta.kid_e, "should not have info about kid_e");
                });
            });
            q.run();
            A.isTrue(exeCbCalled, "execute callback never called");
            A.isTrue(exeCbCalled1, "execute callback 1 never called");
            A.isTrue(exeCbCalled2, "execute callback 2 never called");
            A.isTrue(exeCbCalled3, "execute callback 3 never called");
        },

        'test templateData (new API)': function() {

            var command = {
                    instance: {
                        config: {
                            children: {
                                kid_a: { id: 'kid_a', type: 'kida' },
                                kid_b: { id: 'kid_b', type: 'kidb' }
                            }
                        }
                    }
                },
                adapter = Y.Mock(),
                ac = {
                    done: function(data, meta) {
                        doneCalled = true;
                        A.isString(data.foo);
                        A.areSame('fooval', data.foo, "template data didn't transfer");
                        A.areSame('kid_a__data', data.kid_a, "Missing core child data");
                        A.areSame('kid_b__data', data.kid_b, "Missing core child data");
                    },
                    _dispatch: function(command, adapter) {
                        var id = command.instance.id;
                        var meta = {};
                        meta[id] = id + '__meta';
                        adapter.done(id + '__data', meta);
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                doneCalled = false;

            c.done({
                foo: 'fooval'
            });

            A.isTrue(doneCalled, "ac done function never called");

        },

        'test run extra template data can be passed with no child or params': function() {

            var command = {
                    instance: {
                        config: {
                            children: {
                                kid_a: { id: 'kid_a', type: 'kida' },
                                kid_b: { id: 'kid_b', type: 'kidb' }
                            }
                        }
                    }
                },
                adapter = Y.Mock(),
                ac = {
                    done: function(data, meta) {
                        doneCalled = true;
                        A.isString(data.foo);
                        A.areSame('fooval', data.foo, "template data didn't transfer");
                        A.areSame('kid_a__data', data.kid_a, "Missing core child data");
                        A.areSame('kid_b__data', data.kid_b, "Missing core child data");
                    },
                    _dispatch: function(command, adapter) {
                        var id = command.instance.id;
                        var meta = {};
                        meta[id] = id + '__meta';
                        adapter.done(id + '__data', meta);
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                doneCalled = false;

            c.done({
                template: {
                    foo: 'fooval'
                }
            });

            A.isTrue(doneCalled, "ac done function never called");

        },

        'test error is thrown when children is an array': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = {},
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: [
                        { id: 'kid_a', type: 'kida' },
                        { id: 'kid_b', type: 'kidb' }
                    ]
                };

            try {
                c.execute(config);
                A.fail('Execution should have failed because of children array');
            } catch (err) {
                A.areSame("Cannot process children in the format of an array. 'children' must be an object.", err.message, 'wrong error message');
            }

        },

        'test proxied mojits are processed properly': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = {
                    _dispatch: function(command, adapter) {
                        A.isObject(command, "bad command object to dispatch");
                        A.isNotUndefined(adapter, "bad adapter for dispatch");
                        var id = command.instance.id;
                        var meta = {};
                        if (! id) {
                            id = command.instance.proxied.id;
                            A.areSame('ProxyHandler', command.instance.type, 'Wrong proxy type');
                        }
                        meta[id] = id + '__meta';
                        adapter.done(id + '__data', meta);
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: {
                        kid_a: { id: 'kid_a', type: 'kida' , proxy: { type: 'ProxyHandler'}},
                        kid_b: { id: 'kid_b', type: 'kidb' }
                    }
                },
                exeCbCalled = false;

            c.execute(config, function(data, meta) {
                exeCbCalled = true;
                A.isString(data.kid_a, "missing kid_a data");
                A.isString(data.kid_b, "missing kid_b data");
                A.areSame('kid_a__data', data.kid_a, "wrong kid_a data: " + data.kid_a);
                A.areSame('kid_b__data', data.kid_b, "wrong kid_b data");
                A.isString(meta.kid_a, "missing kid_a meta");
                A.isString(meta.kid_b, "missing kid_b meta");
                A.areSame('kid_a__meta', meta.kid_a, "wrong kid_a meta");
                A.areSame('kid_b__meta', meta.kid_b, "wrong kid_b meta");
            });

            A.isTrue(exeCbCalled, "execute callback never called");
        },

        'test defered mojits are processed properly': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = {
                    _dispatch: function(command, adapter) {
                        A.isObject(command, "bad command object to dispatch");
                        A.isNotUndefined(adapter, "bad adapter for dispatch");
                        var id = command.instance.id;
                        var meta = {};
                        if (! id) {
                            id = command.instance.proxied.id;
                            A.areSame('LazyLoad', command.instance.type, 'Wrong proxy type');
                        }
                        meta[id] = id + '__meta';
                        adapter.done(id + '__data', meta);
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: {
                        kid_a: { id: 'kid_a', type: 'kida' , defer: true},
                        kid_b: { id: 'kid_b', type: 'kidb' }
                    }
                },
                exeCbCalled = false;

            c.execute(config, function(data, meta) {
                exeCbCalled = true;
                A.isString(data.kid_a, "missing kid_a data");
                A.isString(data.kid_b, "missing kid_b data");
                A.areSame('kid_a__data', data.kid_a, "wrong kid_a data: " + data.kid_a);
                A.areSame('kid_b__data', data.kid_b, "wrong kid_b data");
                A.isString(meta.kid_a, "missing kid_a meta");
                A.isString(meta.kid_b, "missing kid_b meta");
                A.areSame('kid_a__meta', meta.kid_a, "wrong kid_a meta");
                A.areSame('kid_b__meta', meta.kid_b, "wrong kid_b meta");
            });

            A.isTrue(exeCbCalled, "execute callback never called");
        },

        'test null or undefined child should be skipped': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                countDispatched = 0,
                ac = {
                    _dispatch: function(command, adapter) {
                        A.isObject(command, "bad command object to dispatch");
                        A.isNotUndefined(adapter, "bad adapter for dispatch");
                        var id = command.instance.id;
                        var meta = {};
                        meta[id] = id + '__meta';
                        countDispatched += 1;
                        adapter.done(id + '__data', meta);
                    }, _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: {
                        kid_a: null,
                        kid_b: { id: 'kid_b', type: 'kidb' }
                    }
                },
                exeCbCalled = false;

            c.execute(config, function(data, meta) {
                exeCbCalled = true;
                A.areSame(1, countDispatched, "dispatched wrong number of children");
                A.isUndefined(data.kid_a, "unexpected kid_a data for null child");
                A.isString(data.kid_b, "missing kid_b data");
                A.areSame('kid_b__data', data.kid_b, "wrong kid_b data");
                A.isUndefined(meta.kid_a, "unexpected kid_a meta for null child");
                A.isString(meta.kid_b, "missing kid_b meta");
                A.areSame('kid_b__meta', meta.kid_b, "wrong kid_b meta");
            });

            A.isTrue(exeCbCalled, "execute callback never called");
        },

        'test parentMeta (new API)': function() {

            var command = {
                    instance: {
                        config: {
                            children: {
                                kid_a: { id: 'kid_a', type: 'kida' },
                                kid_b: { id: 'kid_b', type: 'kidb' }
                            }
                        }
                    }
                },
                adapter = Y.Mock(),
                ac = {
                    done: function(data, meta) {
                        doneCalled = true;
                        A.isString(meta.view.name, 'view.name should come from parentMeta');
                        A.areSame('fooFromParent', meta.view.name, "child meta should not overrule parent meta");
                        A.areSame(3, meta.assets.top.js.length, "assets from parent and childs should be merged");
                        A.isUndefined(meta.view.binder, "meta.view should be preserved from parent without deep merge");
                        A.areEqual(123, meta.binders.mojitid, "meta.binders map should be preserved from children");
                    },
                    _dispatch: function(command, adapter) {
                        var id = command.instance.id;
                        var meta = {
                            view: {
                                name: 'barFromChild',
                                binder: 'barFromChildOnly'
                            },
                            assets: {
                                top: {
                                    js: [id]
                                }
                            },
                            binders: {
                                mojitid: 123
                            }
                        };
                        meta[id] = id + '__meta';
                        adapter.done(id + '__data', meta);
                    },
                    _notify: function() {}
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                doneCalled = false;

            c.done({}, {
                view: {
                    name: "fooFromParent"
                },
                assets: {
                    top: {
                        js: ["one"]
                    }
                }
            });

            A.isTrue(doneCalled, "ac done function never called");
        },

        'test failures in child default': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = {
                    _dispatch: function(command, adapter) {
                        var id = command.instance.id;
                        if (id === 'kid_a') {
                            adapter.done(id + '__data', {});
                        } else {
                            adapter.error(id + '__error');
                        }
                    }
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: {
                        kid_a: { id: 'kid_a', type: 'kida' },
                        kid_b: { id: 'kid_b', type: 'kidb' }
                    }
                },
                data;

            c.execute(config, function(d, m) {
                data = d;
            });

            A.isString(data.kid_a, "missing kid_a data");
            A.areSame('kid_a__data', data.kid_a, "wrong kid_a data");

            A.isString(data.kid_b, "missing kid_b data");
            A.areSame('', data.kid_b, "kid_b data should be empty since it failed during dispatch");
        },

        'test propagateFailure in child': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = {
                    _dispatch: function(command, adapter) {
                        var id = command.instance.id;
                        if (id === 'kid_a') {
                            adapter.done(id + '__data', {});
                        } else {
                            adapter.error(id + '__error');
                        }
                    }
                },
                c = new Y.mojito.addons.ac.composite(command, adapter, ac),
                config = {
                    children: {
                        kid_a: { id: 'kid_a', type: 'kida', propagateFailure: true },
                        kid_b: { id: 'kid_b', type: 'kidb', propagateFailure: true }
                    }
                },
                data,
                err;

            adapter.error = function (e) {
                err = e;
            };
            c.execute(config, function(d, m) {
                data = true;
            });


            A.isUndefined(data, "when an error is propagated, ac.composite.execute callback should never be called");
            A.areSame('Failed composite because of first child failure of "kid_b"', err, "when an error is propagated, adapter.error should be called");
        },

        'test addChild response': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = Y.Mock(),
                c,
                child;

            Y.Mock.expect(ac, {
                method:"_dispatch",
                args: [Y.Mock.Value.Object, Y.Mock.Value.Object]
            });

            c = new Y.mojito.addons.ac.composite(command, adapter, ac);

            // valid child
            child = c.addChild('foo', {
                id: 'bar',
                type: 'baz'
            });

            A.areSame('bar', child.id, "original config should be returned");
            Y.Mock.verify(ac);
        },

        'test addChild invalid entries': function() {
            var command = {instance: {}},
                ac = Y.Mock(),
                c,
                child;

            c = new Y.mojito.addons.ac.composite(command, Y.Mock(), ac);

            // invalid child
            child = c.addChild('foo', null);
            A.isUndefined(child, "invalid child not honored");

            // invalid child with defer and proxy at the same time
            A.throwsError(Error, function(){
                child = c.addChild('foo', {
                    id: 'bar',
                    type: 'baz',
                    defer: true,
                    proxy: {}
                });
            });
        },

        'test addChild defer response': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = Y.Mock(),
                c,
                child;

            Y.Mock.expect(ac, {
                method:"_dispatch",
                args: [Y.Mock.Value.Object, Y.Mock.Value.Object]
            });

            c = new Y.mojito.addons.ac.composite(command, adapter, ac);
            child = c.addChild('foo', {
                id: 'bar',
                type: 'baz',
                defer: true
            });

            A.areSame('LazyLoad', child.type, "type should be replaced with LazyLoad");
            A.areSame('baz', child.proxied.type, "original type should be preserved thru proxied entry");
            Y.Mock.verify(ac);
        },

        'test addChild proxy response': function() {
            var command = {instance: {}},
                adapter = Y.Mock(),
                ac = Y.Mock(),
                c,
                child;

            Y.Mock.expect(ac, {
                method:"_dispatch",
                args: [Y.Mock.Value.Object, Y.Mock.Value.Object]
            });

            c = new Y.mojito.addons.ac.composite(command, adapter, ac);
            child = c.addChild('foo', {
                id: 'bar',
                type: 'baz',
                proxy: {
                    type: "superproxy"
                }
            });

            A.areSame('superproxy', child.type, "type should be replaced with the proxy one");
            A.areSame('baz', child.proxied.type, "original type should be preserved thru proxied entry");
            Y.Mock.verify(ac);
        }

    }));

    Y.Test.Runner.add(suite);

});
