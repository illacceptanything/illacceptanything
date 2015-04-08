/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/

YUI.add('mojito-view-renderer-server-tests', function(Y, NAME) {

    'use strict';

    var suite = new Y.Test.Suite(NAME),
        A = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert,
        ve;

    Y.namespace('mojito.addons');

    suite.add(new Y.Test.Case({

        name: 'mojito-view-renderer',

        setUp: function() {
            // resetting the available engines
            ve = Y.mojito.addons.viewEngines = {};
        },

        tearDown: function() {},

        'test structures': function () {
            A.isObject(Y.mojito.addons);
            A.isFunction(Y.mojito.ViewRenderer);
        },

        'test ctor': function () {
            ve.foo = function(options) {
                A.areSame(99, options.z);
            };

            var vr = new Y.mojito.ViewRenderer('foo', {z:99});

            A.isObject(vr);
            A.isFunction(vr.render);
        },

        'test mock render method': function () {
            ve.bar = function() {};

            ve.bar.prototype = {
                render: function(data, mojitType, tmpl, adapter, meta, more) {
                    A.areSame(6, arguments.length);
                    A.areSame(1, arguments[0]);
                    A.areSame(2, arguments[1]);
                    A.areSame(3, arguments[2]);
                    A.areSame(4, arguments[3]);
                    A.areSame(5, arguments[4]);
                    A.areSame(6, arguments[5]);
                }
            };

            var vr = new Y.mojito.ViewRenderer('bar', {z:99});
            vr.render(1,2,3,4,5,6);
        },

        'test view engine caching machanism': function() {
            var rendererCtorCalled = 0;

            ve.mockViewEngine = function() {
                rendererCtorCalled += 1;
            };

            var obj1 = new Y.mojito.ViewRenderer('mockViewEngine', {z:99});
            var obj2 = new Y.mojito.ViewRenderer('mockViewEngine', {z:99});

            A.areSame(1, rendererCtorCalled, 'renderer constructor should be called once and then cached by engine name');
        }

    }));

    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: ['mojito-view-renderer']});
