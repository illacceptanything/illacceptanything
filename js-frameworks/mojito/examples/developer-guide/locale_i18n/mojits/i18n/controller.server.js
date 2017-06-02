/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('i18n', function (Y, NAME) {
    Y.namespace('mojito.controllers')[NAME] = {
        index: function (ac) {
            // Default.
            ac.done({
                title: ac.intl.lang("TITLE"),
                today: ac.intl.formatDate(new Date())
            });
        }
    };
}, '0.0.1', { requires: ['mojito-intl-addon']});
