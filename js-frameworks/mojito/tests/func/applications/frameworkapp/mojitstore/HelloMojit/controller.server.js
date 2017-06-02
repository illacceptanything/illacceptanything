/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('HelloMojit', function(Y, NAME) {

/**
 * The HelloMojit module.
 *
 * @module HelloMojit
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
            ac.done('Mojito is working.');
        }

    };

}, '0.1.0', {requires: ['mojito']});
