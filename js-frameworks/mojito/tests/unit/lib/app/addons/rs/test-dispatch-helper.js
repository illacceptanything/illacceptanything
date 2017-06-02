/*
 * Copyright (c) 2011-2014, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use(
    'base',
    'oop',
    'mojito-resource-store',
    'addon-rs-yui',
    'addon-rs-dispatch-helper',
    'json',
    'test',
    function(Y) {

    var suite = new YUITest.TestSuite('mojito-addon-rs-dispatch-helper-tests'),
        libpath = require('path'),
        mojitoRoot = libpath.join(__dirname, '../../../../../../lib'),
        A = Y.Assert;


    suite.add(new YUITest.TestCase({

        name: 'dispatch-helper rs addon tests',


        'augment getMojitTypeDetails with AC addons': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/gsg5');
            var store = new Y.mojito.ResourceStore({ root: fixtures, mojitoRoot: mojitoRoot });
            store.preload();

            var details = store.getMojitTypeDetails('server', {}, 'PagedFlickr');

            // order matters
            A.areSame(5, details.acAddons.length, 'number of AC addons');
            A.areSame(JSON.stringify(['config','intl','params','url','models']), JSON.stringify(details.acAddons), 'correct order');
        }


    }));

    Y.Test.Runner.add(suite);

});