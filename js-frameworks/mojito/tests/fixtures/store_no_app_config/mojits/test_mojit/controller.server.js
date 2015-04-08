/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('soloMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
            ac.done('Mojito is working.');
        }

    };

}, '0.0.1', {requires: ['mojito']});
