/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('parent', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {

            ac.analytics.retrieve(function(analytics) {
                // do what you wish with the analytics data here, but
                // you cannot use it to build the data for ac.done()
                Y.log(analytics, 'info');
                // if you call ac.done(data, meta) here, an error is thrown
                // because you cannot call ac.done() twice
            });
            // the 'composite' addon's done() function will prep the done() call for you, and eventually call
            // ac.done() in the correct fashion for any composites you have defined 
            ac.composite.done();
        }

    };

}, '0.0.1', {requires: ['mojito-analytics-addon', 'mojito-composite-addon']});
