/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('child', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
            var color = ac.config.get('color');
            // store some analytics stuff
            ac.analytics.store({ colors: [color]});
            // store some more analytics stuff
            ac.analytics.store({ time: new Date().getTime()});
            ac.done({color: color});
        }

    };

}, '0.0.1', {requires: ['mojito-analytics-addon', 'mojito-config-addon']});
