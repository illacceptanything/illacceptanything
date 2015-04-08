/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true, unparam: true, todo: true*/
/*global YUI*/


/*
 * Baseline Mojito client testing harness.
 */
YUI.add('mojito', function(Y, NAME) {

    // TODO: why does need to be in sync with autoload/mojito.common.js
    // why don't we just attach that to every test?

    Y.namespace('mojito');
    Y.namespace('mojito.trans');
    Y.namespace('mojito.actions');
    Y.namespace('mojito.binders');
    Y.namespace('mojito.controllers');
    Y.namespace('mojito.models');
    Y.namespace('mojito.addons');
    Y.namespace('mojito.addons.ac');
    Y.namespace('mojito.addons.viewEngines');

    // internal mojito framework cache (this is probably legacy)
    YUI.namespace('_mojito._cache');

    // setting the stage for all data-scopes implementation
    YUI.namespace('Env.mojito');

});

/* AC ADDONS */
YUI.add('mojito-analytics-addon', function(Y, NAME) {});
YUI.add('mojito-assets-addon', function(Y, NAME) {});
YUI.add('mojito-composite-addon', function(Y, NAME) {});
YUI.add('mojito-config-addon', function(Y, NAME) {});
YUI.add('mojito-cookie-addon', function(Y, NAME) {});
YUI.add('mojito-deploy-addon', function(Y, NAME) {});
YUI.add('mojito-http-addon', function(Y, NAME) {});
YUI.add('mojito-models-addon', function(Y, NAME) {});
YUI.add('mojito-i13n-addon', function(Y, NAME) {});
YUI.add('mojito-intl-addon', function(Y, NAME) {});
YUI.add('mojito-meta-addon', function(Y, NAME) {});
YUI.add('mojito-params-addon', function(Y, NAME) {});
YUI.add('mojito-partial-addon', function(Y, NAME) {});
YUI.add('mojito-url-addon', function(Y, NAME) {});

/* RS ADDONS */
YUI.add('addon-rs-config', function(Y, NAME) {});
YUI.add('addon-rs-dispatch-helper', function(Y, NAME) {});
YUI.add('addon-rs-selector', function(Y, NAME) {});
YUI.add('addon-rs-url', function(Y, NAME) {});
YUI.add('addon-rs-yui', function(Y, NAME) {});

/* VIEW ENGINE ADDONS */
YUI.add('mojito-hb', function(Y, NAME) {});

/* AUTOLOAD */
YUI.add('mojito-action-context', function(Y, NAME) {});
YUI.add('mojito-dispatcher', function(Y, NAME) {
    // We need to grab the output handler while testing lib/mojito.js.
    var mock = {
        init: function(store) {
            return {
                dispatch: function(command, outputHandler) {
                    mock.outputHandler = outputHandler;
                }
            };
        }
    };
    Y.namespace('mojito').Dispatcher = mock;
});
YUI.add('mojito-mojit-proxy', function(Y, NAME) {});
YUI.add('mojito-output-handler', function(Y, NAME) {});
YUI.add('mojito-output-buffer', function(Y, NAME) {});
YUI.add('mojito-perf', function(Y, NAME) {});
YUI.add('mojito-hooks', function(Y, NAME) {

    Y.namespace('mojito').hooks = {
        hook: function () {},
        registerHook: function () {},
        enableHookGroup: function () {}
    };

});
YUI.add('mojito-resource-store', function(Y, NAME) {});
YUI.add('mojito-rest-lib', function(Y, NAME) {});
YUI.add('mojito-route-maker', function(Y, NAME) {});
YUI.add('mojito-client-store', function(Y, NAME) {});
YUI.add('mojito-tunnel-client', function(Y, NAME) {});
YUI.add('mojito-util', function(Y, NAME) {});
YUI.add('mojito-view-renderer', function(Y, NAME) {});


YUI.add('mojito-test', function(Y, NAME) {

    function EasyMock() {

        var mock = Y.Test.Mock();

        mock.expect = function() {
            Y.Array.each(arguments, function(expectation) {
                Y.Test.Mock.expect(mock, expectation);
            });
            return mock;
        };

        mock.verify = function() {
            Y.Test.Mock.verify(mock);
        };

        return mock;
    }


    function createMockAddon(source, name) {
        source._addons.push(name);
        source[name] = new EasyMock();
    }


    function createMockModel(source, name) {
        source.models[name] = new EasyMock();
    }


    function createMockExtra(source, ns, name) {
        var mock = new EasyMock();

        if (!source[ns]) {
            source[ns] = {};
        }
        if (!source._extras[ns]) {
            source._extras[ns] = {};
        }
        source._extras[ns][name] = mock;
        source[ns][name] = mock;
    }


    function MockActionContext(opts) {
        var mock = Y.Test.Mock();

        opts = opts || {};
        mock._addons = [];
        mock.models = {};
        mock._extras = {};

        if (opts.addons) {
            Y.Array.each(opts.addons, function(addon) {
                createMockAddon(mock, addon);
            });
        }
        if (opts.models) {
            Y.Array.each(opts.models, function(model) {
                createMockModel(mock, model);
            });
        }
        if (opts.extras) {
            Y.Object.each(opts.extras, function(extras, namespace) {
                if (Y.Lang.isArray(extras)) {
                    Y.Array.each(extras, function(extra) {
                        createMockExtra(mock, namespace, extra);
                    });
                } else {
                    createMockExtra(mock, namespace, extras);
                }
            });
        }

        mock.expect = function() {
            Y.Array.each(arguments, function(expectation) {
                Y.Test.Mock.expect(mock, expectation);
            });
            return mock;
        };
        mock.verify = function() {
            var i,
                j,
                mockAddon;

            Y.Test.Mock.verify(mock);
            for (i = 0; i < mock._addons.length; i += 1) {
                mockAddon = mock[mock._addons[i]];
                mockAddon.verify();
            }
            for (i in mock.models) {
                if (mock.models.hasOwnProperty(i)) {
                    mock.models[i].verify();
                }
            }
            for (i in mock._extras) {
                if (mock._extras.hasOwnProperty(i)) {
                    for (j in mock._extras[i]) {
                        if (mock._extras[i].hasOwnProperty(j)) {
                            mock._extras[i][j].verify();
                        }
                    }
                }
            }
        };
        return mock;
    }

    Y.mojito.MockActionContext = MockActionContext;
    Y.mojito.EasyMock = EasyMock;

}, '0.1.0', {requires: [
    'event',
    'node',
    'node-event-simulate'
]});


YUI.add('mojito-test-extra', function(Y, NAME) {
    var A = Y.Assert;

    // move to mojito-test, under Y.mojito namespace? i.e. Y.mojito.BASEDIR
    if (('undefined' !== typeof require) && ('function' === typeof require.resolve)) {
        // define the abs path to the mojito base dir on nodejs only
        Y.MOJITO_DIR = require('path').resolve(__dirname, '../../') + '/';
    }

    // path doesn't need to be given, mainly used during recursion
    Y.TEST_CMP = function (x, y, msg, path) {
        path = path || 'obj';
        var i;
        if (Y.Lang.isArray(x)) {
            A.isArray(x, path + ': ' + (msg || 'first arg should be an array'));
            A.isArray(y, path + ': ' + (msg || 'second arg should be an array'));
            A.areSame(x.length, y.length, path + ': ' + (msg || 'arrays are different lengths'));
            for (i = 0; i < x.length; i += 1) {
                Y.TEST_CMP(x[i], y[i], msg, path + '[' + i + ']');
            }
            return;
        }
        if (Y.Lang.isObject(x)) {
            A.isObject(x, path + ': ' + (msg || 'first arg should be an object'));
            A.isObject(y, path + ': ' + (msg || 'second arg should be an object'));
            A.areSame(Y.Object.keys(x).length, Y.Object.keys(y).length, path + ': ' + (msg || 'object keys are different lengths'));
            for (i in x) {
                if (x.hasOwnProperty(i)) {
                    Y.TEST_CMP(x[i], y[i], msg, path + '{' + i + '}');
                }
            }
            return;
        }
        A.areSame(x, y, path + ': ' + (msg || 'args should be the same'));
    };

}, '0.1.0', {requires: ['test']});
