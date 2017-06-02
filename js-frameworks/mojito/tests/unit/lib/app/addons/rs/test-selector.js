/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use(
    'mojito-test-extra',
    'base',
    'oop',
    'addon-rs-config',
    'addon-rs-selector',
    'test',
    function(Y) {
    
        var suite = new Y.Test.Suite('mojito-addon-rs-selector-tests'),
            libpath = require('path'),
            libycb = require('ycb'),
            mojitoRoot = libpath.join(__dirname, '../../../../../../lib'),
            fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            A = Y.Assert;


        function MockRS(config) {
            MockRS.superclass.constructor.apply(this, arguments);
        }
        MockRS.NAME = 'MockResourceStore';
        MockRS.ATTRS = {};
        Y.extend(MockRS, Y.Base, {
            initializer: function(cfg) {
                this._config = cfg || {};
                this.selectors = {
                    'devdroid': true,
                    'droid': true,
                    'shelves': true,
                    'right': true,
                    '*': true
                };
            },
            blendStaticContext: function() {
                return {};
            },
            validateContext: function() {},
            cloneObj: function(o) {
                return Y.clone(o);
            }
        });


        suite.add(new Y.Test.Case({

            name: 'selector rs addon tests',

            'read dimensions': function() {
                // from mojito
                var store = new MockRS({ root: fixtures });
                store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
                store.plug(Y.mojito.addons.rs.selector, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

                var have = store.selector.getPOSLFromContext({});
                var want = ['*'];
                Y.TEST_CMP(have, want);

                var have = store.selector.getPOSLFromContext({runtime:'client'});
                var want = ['right', '*'];
                Y.TEST_CMP(have, want);

                var have = store.selector.getPOSLFromContext({runtime:'server'});
                var want = ['shelves', '*'];
                Y.TEST_CMP(have, want);

                var have = store.selector.getPOSLFromContext({device:'android'});
                var want = ['droid', '*'];
                Y.TEST_CMP(have, want);

                var have = store.selector.getPOSLFromContext({runtime:'server', device:'android'});
                var want = ['shelves', 'droid', '*'];
                Y.TEST_CMP(have, want);

                var have = store.selector.getPOSLFromContext({device:'android', environment:'dev'});
                var want = ['devdroid', 'droid', '*'];
                Y.TEST_CMP(have, want);

                var have = store.selector.getPOSLFromContext({runtime:'server', device:'android', environment:'dev'});
                var want = ['shelves', 'devdroid', 'droid', '*'];
                Y.TEST_CMP(have, want);
            },


            'get all posls': function() {
                var store = new MockRS({ root: fixtures });
                store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
                store.plug(Y.mojito.addons.rs.selector, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

                var have = store.selector.getAllPOSLs();
                var want = [
                    [ 'shelves', '*' ],
                    [ 'shelves', 'devdroid', 'droid', '*' ],
                    [ 'shelves', 'droid', '*' ],
                    [ 'right', '*' ],
                    [ 'right', 'devdroid', 'droid', '*' ],
                    [ 'right', 'droid', '*' ]
                ];
                Y.TEST_CMP(want, have);
            },


            'list used dimensions': function() {
                var store = new MockRS({ root: fixtures });
                store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
                store.plug(Y.mojito.addons.rs.selector, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

                var have = store.selector._listUsedDimensions();
                var want = {
                    runtime: ['server', 'client'],
                    device: ['iphone', 'android'],
                    environment: ['dev']
                }
                Y.TEST_CMP(want, have);
            },


            'list used contexts': function() {
                var store = new MockRS({ root: fixtures });
                store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
                store.plug(Y.mojito.addons.rs.selector, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

                var have = store.selector._listUsedContexts();
                var want = [
                    { runtime: 'server', device: 'iphone', environment: 'dev' },
                    { runtime: 'server', device: 'iphone' },
                    { runtime: 'server', device: 'android', environment: 'dev' },
                    { runtime: 'server', device: 'android' },
                    { runtime: 'server', environment: 'dev' },
                    { runtime: 'server' },
                    { runtime: 'client', device: 'iphone', environment: 'dev' },
                    { runtime: 'client', device: 'iphone' },
                    { runtime: 'client', device: 'android', environment: 'dev' },
                    { runtime: 'client', device: 'android' },
                    { runtime: 'client', environment: 'dev' },
                    { runtime: 'client' }
                ];
                Y.TEST_CMP(want, have);
            }


        }));

        Y.Test.Runner.add(suite);
    
});
