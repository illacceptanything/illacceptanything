/*
 * Copyright (c) 2012 Yahoo! Inc. All rights reserved.
 */
YUI.add('OtherFeatures', function(Y, NAME) {

/**
 * The OtherFeatures module.
 *
 * @module OtherFeatures
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
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            var data = {
                title: "Testing Mojito Features"
            };
            ac.done(data);
        }

    };

}, '0.0.1', {requires: ['mojito']});
