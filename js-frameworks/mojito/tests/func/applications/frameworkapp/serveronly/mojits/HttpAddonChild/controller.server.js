/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('HttpAddonChild', function(Y, NAME) {

/**
 * The HttpAddonChild module.
 *
 * @module HttpAddonChild
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
        	console.log("Child set header");
            ac.http.addHeader('my_header', 'ByChild');
            ac.done({title: "Child Mojit adding a header"});
        }
    };

}, '0.0.1', {requires: ['mojito', 'mojito-http-addon']});
