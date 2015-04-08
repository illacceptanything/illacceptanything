/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('device', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        /* Method corresponding to the 'index' action. 
        * 
        * @param ac {Object} The action context that 
        * provides access to the Mojito API.
        */
        index: function (ac) {
            ac.done({title: 'Device Views'});
        }
    };
}, '0.0.1', {requires: []});
