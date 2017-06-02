/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, regexp:true*/
/*global YUI*/


YUI.add('YqlWeatherModel', function(Y, NAME) {

    Y.mojito.models[NAME] = {

        fetch: function(location, callback) {
            var error = null,
                data = {
                    temp: "72",
                    text: "clear"
                };
            
            callback(error, data);
        }
    };

}, '0.0.1', {requires: ['mojito', 'yql', 'jsonp-url']});
