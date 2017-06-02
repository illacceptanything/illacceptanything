/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('sender', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        "index": function (actionContext) {
            actionContext.done({title: 'List of images for testing'});
        }
    };
}, '0.0.1', {requires: ['mojito']});
