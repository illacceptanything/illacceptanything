/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('Container', function(Y, NAME) {

/**
 * The Container module.
 *
 * @module Container
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
            ac.composite.done();
        },
        lazy: function(ac) {
            ac.composite.done();
        }

    };

}, '0.0.1', {requires: ['mojito','mojito-composite-addon']});
