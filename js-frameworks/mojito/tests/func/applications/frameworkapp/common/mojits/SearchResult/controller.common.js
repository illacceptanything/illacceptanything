/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SearchResult', function(Y, NAME) {

/**
 * The SearchResult module.
 *
 * @module SearchResult
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
            ac.done(ac.config.get());
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-config-addon']});
