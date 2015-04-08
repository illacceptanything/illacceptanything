/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('GlobalMojitActiions', function(Y) {

    Y.mojito.actions = {

        myExternalAction: function(ac) {
            ac.done('This is the action from an external actions file');
        }
    };

}, '0.0.1', {requires: []});
