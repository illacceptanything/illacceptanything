/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('paramgrabber', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        
        index: function(ac) {
            var merged = ac.params.merged(),
                get = ac.params.url(),
                post = ac.params.body(),
                route = ac.params.route(),
                
                dataOut = {
                    desc: "Merged Parameters Example",
                    name: merged.name,
                    thing: merged.likes,
                    paramGroup: [
                        {
                            desc: "merged params",
                            params: paramArrayBuilder(merged)
                        },
                        {
                            desc: "get params",
                            params: paramArrayBuilder(get)
                        },
                        {
                            desc: "post params",
                            params: paramArrayBuilder(post)
                        },
                        {
                            desc: "route params",
                            params: paramArrayBuilder(route)
                        }
                    ]
                };
            
            ac.done(dataOut);

        }
        
    };
    
    function paramArrayBuilder(params) {
        var paramsArray = [];
        Y.Object.each(params, function(param, key) {
            if (param) {
                paramsArray.push({key: key, value: param});
            }
        });
        return paramsArray;
    }

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
