/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('myMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {

            var data = {
                    msg: 'Mojito is Working.'
                };
            
            ac.done(data);
        }

    };

}, '0.0.1', {requires: ['mojito']});
