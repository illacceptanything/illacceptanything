/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('yuitweets-model', function(Y, NAME) {

    Y.namespace("mojito.models")[NAME] = {

        getTweets: function(cb) {
            var tweets = [ { "title": "YUI Tweet One", "pubData": new Date().toString(), "creator": "YUI Engineer" } ];
            return cb(null, tweets);
        }
    };
}, '0.0.1', {requires: ['jsonp']});
