/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MModelFoo', function(Y, NAME) {

/**
 * The MModel module.
 *
 * @module MModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.namespace('mojito.models').MModelFoo = {

        init: function(config) {
            this.config = config;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        getMessage: function(callback) {
            callback(null, 'Mojito is working');
        }

    };

}, '0.0.1', {requires: ['mojito']});
