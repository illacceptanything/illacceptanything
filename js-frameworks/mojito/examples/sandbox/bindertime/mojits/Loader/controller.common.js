/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('loader-controller', function(Y, NAME) {

/**
 * The loader module.
 *
 * @module loader
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

        foo: function(ac) {
            ac.done({
                name: 'foo',
                time: new Date().getTime()
            });
        }

    };

}, '0.0.1', {requires: []});
