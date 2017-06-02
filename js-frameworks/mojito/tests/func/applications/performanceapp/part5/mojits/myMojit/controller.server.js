/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('myMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {

            ac.models.get('model').get(function(data){
                ac.done(data);
            });
            
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-models-addon',
    'myMojitModel']});
