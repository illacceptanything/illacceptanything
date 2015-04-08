/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('body', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        index: function (actionContext) {
            actionContext.done({title: "Body"});
        }
    };
}, '0.0.1', {requires: ['mojito']});
