/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('myMojitModel', function(Y, NAME) {

    Y.namespace('mojito.models').message = {

        get: function(callback) {

            var data = {
                    msg: 'Mojito is Working.'
                };
                
            callback(data);
        }
    };

}, '0.0.1', {requires: ['mojito']});
