/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('FlickrBrowser', function(Y, NAME) {

/**
 * The FlickrBrowser module.
 *
 * @module FlickrBrowser
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
            ac.composite.done();
        }

    };


}, '0.0.1', {requires: ['mojito-composite-addon']});
