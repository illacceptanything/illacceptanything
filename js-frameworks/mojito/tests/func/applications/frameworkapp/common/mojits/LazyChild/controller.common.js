/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('LazyChild', function(Y, NAME) {

/**
 * The LazyPants module.
 *
 * @module LazyPants
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
        hello: function(ac) {
            ac.done({time: new Date()});
        },

        index: function(ac) {
            ac.done();
        }

    };

}, '0.0.1', {requires: ['mojito']});
