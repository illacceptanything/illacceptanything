/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('contactus', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function (ac) {
            var vudata = {
                'company': ac.config.get("company"),
                'copyright': ac.config.get("copyright"),
                'depts': ac.config.getDefinition(ac.config.get("key"))
            };

            ac.done(vudata);
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon'
]});
