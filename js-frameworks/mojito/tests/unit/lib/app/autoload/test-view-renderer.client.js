/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/

YUI.add('mojito-view-renderer-client-tests', function(Y, NAME) {
    var suite = new Y.Test.Suite(NAME),
        A = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert,
        cases;

    suite.add(new Y.Test.Case({

        name: 'mojito-view-renderer',

        setUp: function() {
            Y.namespace('mojito.addons.viewEngines');
            // resetting engines
            Y.mojito.addons.viewEngines = {};
        },

        tearDown: function() {},

        'test instantiating a renderer engine': function () {
            var vr;

            vr = new Y.mojito.ViewRenderer('foo', {z:99});
            A.isObject(vr);
            // less restrictive initialization
            vr = new Y.mojito.ViewRenderer('bar');
            A.isObject(vr);
        },

        'test render method with valid engine': function () {
            var args;
            var ve = Y.namespace('mojito.addons.viewEngines');
            ve.foo = function(options) {
                A.areSame(99, options.z);
            };
            ve.foo.prototype.render = function(data, mojitType, tmpl, adapter, meta, more) {
                args = arguments;
            };

            var vr = new Y.mojito.ViewRenderer('foo', {z:99});
            vr.render(1,2,3,4,5,6);
            A.areSame(6, args.length);
            A.areSame(1, args[0]);
            A.areSame(2, args[1]);
            A.areSame(3, args[2]);
            A.areSame(4, args[3]);
            A.areSame(5, args[4]);
            A.areSame(6, args[5]);
        },

        'test render method with ondemand engine': function () {
            var args;

            YUI.add('mojito-fakebarengine', function (Y) {
                var ve = Y.namespace('mojito.addons.viewEngines');
                ve.fakebarengine = function(options) {
                    A.areSame(99, options.z);
                };
                ve.fakebarengine.prototype.render = function(data, mojitType, tmpl, adapter, meta, more) {
                    args = arguments;
                };
            });

            var vr = new Y.mojito.ViewRenderer('fakebarengine', {z:99});
            vr.render(1,2,3,4,5,6);
            this.wait(function () {
                A.areSame(6, args.length);
                A.areSame(1, args[0]);
                A.areSame(2, args[1]);
                A.areSame(3, args[2]);
                A.areSame(4, args[3]);
                A.areSame(5, args[4]);
                A.areSame(6, args[5]);
            }, 300);
        },


        'test render method with invalid ondemand engine': function () {
            var err;

            var vr = new Y.mojito.ViewRenderer('fakebazengine', {z:99});
            vr.render(1,2,3, {
                error: function () {
                    err = true;
                }
            },5,6);
            this.wait(function () {
                A.isTrue(err, 'an error should occurr when the engine is invalid');
            }, 300);
        }

    }));

    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: ['mojito-view-renderer']});
