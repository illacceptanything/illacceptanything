/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('clientcookie', function(Y, NAME) {

/**
 * The clientcookie module.
 *
 * @module clientcookie
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            ac.done();
        },

        setCookies: function(ac) {
            var opts = {
                domain: 'mojito-cookie-test.edu',
                path: '/',
                expires: new Date(2035, 1, 1)
            };
            ac.cookie.set('browser-state', 'PERFECT', opts);
            ac.cookie.set('the-man', 'MATT', opts);
            ac.done();
        }

    };

}, '0.0.1', {requires: ['mojito-cookie-addon']});
