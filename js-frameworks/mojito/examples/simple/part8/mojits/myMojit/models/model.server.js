/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('myMojitModel', function(Y, NAME) {

    Y.namespace('mojito.models')[NAME] = {

        get: function(callback) {

            var data = {
                    msg: 'Mojito is Working.'
                };
                
            callback(data);
        }
    };

}, '0.0.1', {requires: ['mojito']});
