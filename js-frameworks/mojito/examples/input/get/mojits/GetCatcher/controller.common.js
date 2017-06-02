/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('getcatcher', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {

        allParams: function(ac) {
            var params = ac.params.url(),
                paramsArray = [];
            Y.Object.each(params, function(param, key) {
                paramsArray.push({key: key, value: param});
            });
            ac.done({
                desc: 'All params',
                get: paramsArray
            });
        },

        paramsByValue: function(ac) {
            var fooVal = ac.params.url('foo'),
                existsString = fooVal ? "YES" : "NO";
            ac.done({
                desc: 'Params by key',
                exists: existsString
            });
        }

    };
    
}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
