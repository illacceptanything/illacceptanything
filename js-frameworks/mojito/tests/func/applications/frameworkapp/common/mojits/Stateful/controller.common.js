/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('Stateful', function(Y, NAME) {

/**
 * The Stateful module.
 *
 * @module Stateful
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

        pitch: function(ac) {
            this.ball = ac.params.getFromMerged('ball');
            ac.done();
        },

        'catch': function(ac) {
            ac.done({ball: this.ball}, 'json');
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
