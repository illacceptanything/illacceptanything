/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('frame', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        init: function(config) {
            this.config = config;
        },
        index: function(actionContext) {
            actionContext.composite.done({title: "Parent Frame"});
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-composite-addon']});
