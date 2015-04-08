/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


/*
 * Test suite for the analytics.common.js file functionality.
 */
YUI().use('mojito-config-addon', 'test', function(Y) {

    var suite = new Y.Test.Suite('mojito-config-addon tests'),
        Addon = Y.mojito.addons.ac.config,
        A = Y.Assert,
        O = Y.ObjectAssert;

    function create(configs, definitions) {
        return new Addon({
            instance: {
                config: configs,
                definition: definitions
            }
        }, {
            page: {
                appConfig: 'page app config'
            }
        });
    }

    suite.add(new Y.Test.Case({

        name: 'config tests',

        'test get(key) //one config': function() {
            var cf = {a: 1, b: 2},
                ac = create(cf, null);

            A.areSame(ac.get('a'), cf.a);
            A.areNotSame(ac.get('a'), String(cf.a));

            A.areSame(ac.get('b'), cf.b);
            A.areNotSame(ac.get('b'), String(cf.b));
        },

        'test get() //all configs': function() {
            var cf = {a: 1, b: 2 },
                ac = create(cf);

            O.ownsKeys(ac.get(), cf);

            cf.newfoo = 'newbar';
            A.areSame(3, Y.Object.size(cf));
        },

        'test getDefinition(key) //one def': function() {
            var df = {a: 1, b: 2},
                ac = create(null, df);

            A.areSame(ac.getDefinition('a'), df.a);
            A.areNotSame(ac.getDefinition('a'), String(df.a));

            A.areSame(ac.getDefinition('b'), df.b);
            A.areNotSame(ac.getDefinition('b'), String(df.b));
        },

        'test getDefinition() //all defs': function() {
            var df = {a: 1, b: 2},
                ac = create(null, df);

            O.ownsKeys(ac.getDefinition(), df);

            df.newfoo = 'newbar';
            A.areSame(3, Y.Object.size(df));
        },

        'test get(foo) returns falsey values': function() {
            //[Bug: 5439377] configuration does not accept null nor false values
            var cf = {c1: 0, c2: false, c3: null, c4: undefined},
                df = {d1: 0, d2: false, d3: null, d4: undefined},
                ac = create(cf, df);

            O.ownsKeys(cf, ac.get());
            A.areSame(cf.c1, ac.get('c1'));
            A.areSame(cf.c2, ac.get('c2'));
            A.areSame(cf.c3, ac.get('c3'));
            A.areSame(cf.c4, ac.get('c4'));
            A.isUndefined(ac.get('c4'));
            A.isUndefined(ac.get('UNDEFINED'));

            A.isNotUndefined(ac.get('c1'));
            A.isNotUndefined(ac.get('c2'));
            A.isNotUndefined(ac.get('c3'));
        },

        'test getDefinition(foo) returns falsey values': function() {
            //[Bug: 5439377] configuration does not accept null nor false values
            var cf = {c1: 0, c2: false, c3: null, c4: undefined},
                df = {d1: 0, d2: false, d3: null, d4: undefined},
                ac = create(cf, df);

            O.ownsKeys(df, ac.getDefinition());
            A.areSame(df.d1, ac.getDefinition('d1'));
            A.areSame(df.d2, ac.getDefinition('d2'));
            A.areSame(df.d3, ac.getDefinition('d3'));
            A.areSame(df.d4, ac.getDefinition('d4'));
            A.isUndefined(ac.getDefinition('d4'));
            A.isUndefined(ac.getDefinition('UNDEFINED'));

            A.isNotUndefined(ac.getDefinition('d1'));
            A.isNotUndefined(ac.getDefinition('d2'));
            A.isNotUndefined(ac.getDefinition('d3'));
        },

        'test getAppConfig()': function() {
            var ac = create();

            A.areSame('page app config', ac.getAppConfig(), 'the app config should come from adapter.page.appConfig');
        }

    }));

    Y.Test.Runner.add(suite);

});
