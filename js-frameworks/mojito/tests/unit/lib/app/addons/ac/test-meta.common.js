/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-meta-addon', 'test', function(Y) {

    var suite = new Y.Test.Suite('mojito-meta-addon tests'),
        A = Y.Assert,
        OA = Y.ObjectAssert;

    suite.add(new Y.Test.Case({

        name: 'meta addon tests',

        'test stored values are retrievable': function() {
            var addon = new Y.mojito.addons.ac.meta();
            var retrieved;

            addon.store('foo', 'bar');
            addon.retrieve(function(val) {
                retrieved = val;
            });
            // faking what Mojito does when done() is called
            var meta = addon.mergeMetaInto({});

            OA.areEqual({foo:'bar'}, retrieved, 'wrong retrieved meta value');
        }

    }));

    Y.Test.Runner.add(suite);

});
