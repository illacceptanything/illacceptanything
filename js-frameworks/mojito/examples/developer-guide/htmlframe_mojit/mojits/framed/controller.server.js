/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('framed', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        index: function (ac) {
            ac.done({app_name: 'Framed Mojit'});
        }
    };
}, '0.0.1', {requires: ['']});
