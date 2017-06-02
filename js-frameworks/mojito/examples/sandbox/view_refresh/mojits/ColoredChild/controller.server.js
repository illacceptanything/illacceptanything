/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('ColoredChild', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
//            ac.assets.addCss('./style.css');

            var bg = ac.params.url('background') || ac.config.get('background');

            ac.done({
                time: new Date(),
                bg: bg
            });
        },
        
        alternative: function(ac) {
            ac.done({time: new Date()});
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon', 'mojito-config-addon', 'mojito-assets-addon']});
