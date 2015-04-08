/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('twitter', function(Y, NAME) {

    var cache = {};

    Y.namespace("mojito.models")[NAME] = {

        getTweetsFor: function(screenName, callback) {
            var tweets = [ { title: "A Tweet", "created_at": new Date().toString(), text: "Here's a tweet!", creator: screenName }];
            callback(null, tweets);
        }
    };

}, '0.0.1', {requires: ['jsonp']});
