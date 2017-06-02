/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add("simple-ctrl", function (Y, NAME) {
    Y.namespace("mojito.controllers")[NAME] = {
        index: function (ac) {
            ac.done(ac.config.get("message"));
        }
    };
}, '0.0.1', {
    requires: ['mojito-config-addon']
});
