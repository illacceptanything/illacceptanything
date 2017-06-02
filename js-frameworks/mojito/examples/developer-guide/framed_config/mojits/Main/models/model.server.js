/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('main-model', function (Y, NAME) {

     Y.namespace('mojito.models')[NAME] = {

        getData: function (callback) {
            callback({some: 'data'});
        }

    };
}, '0.0.1', {requires: ['mojito']});
