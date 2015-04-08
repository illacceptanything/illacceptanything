/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/

/*
 * Test suite for the mojito-client.client.js file functionality.
 */
YUI({useBrowserConsole: true}).use(
    'mojito-client',
    'test',
    function(Y) {

        var suite = new Y.Test.Suite("mojito-client.client tests"),
            A = Y.Assert;

        suite.add(new Y.Test.Case({

            setUp: function() {
                this.mojitoClient = new Y.mojito.Client();
            },

            "test constructor": function() {
                var client = this.mojitoClient;
                A.isObject(client);
            }
        }));

        Y.Test.Runner.add(suite);
    }
);
