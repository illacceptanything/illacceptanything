/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('GlobalMojitModel', function(Y) {

/**
 * The GlobalMojitModel module.
 *
 * @module GlobalMojitModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models.GlobalMojit = {

        init: function(config) {
            this.config = config;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        myGlobalModelFunction: function(callback) {
            callback({some:'I am calling from the model of the global mojit.'});
        }

    };

}, '0.0.1', {requires: []});
