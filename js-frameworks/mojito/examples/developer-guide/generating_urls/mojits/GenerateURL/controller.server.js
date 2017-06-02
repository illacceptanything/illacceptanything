/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('generateurl', function(Y, NAME) {
    Y.namespace('mojito.controllers')[NAME] = {
        init: function(config) {
            this.config = config;
        },
        index: function(actionContext) {
            var url = actionContext.url.make('mymojit', 'contactus', '');
            actionContext.done({contactus_url: url});
        },
        contactus: function(actionContext) {
            var currentTime = actionContext.intl.formatDate(new Date());
            actionContext.done({currentTime: currentTime});
        }
    };
}, '0.0.1', {requires: [
    'mojito',
    'mojito-url-addon',
    'mojito-intl-addon'
]});
