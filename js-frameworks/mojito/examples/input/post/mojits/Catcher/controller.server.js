/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('Catcher', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        
        index: function(ac) {
            var name = ac.params.body('name'),
                thing = ac.params.body('likes');
            ac.done({
                desc: "Here's the POST data!",
                name: name,
                thing: thing
            });
        }
        
    };
        
}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
