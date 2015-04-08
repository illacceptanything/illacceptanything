/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('myMojitModelFoo', function(Y, NAME) {

/**
 * The TestMojitModelFoo module.
 *
 * @module TestMojit
 */

    /**
     * Constructor for the TestMojitModelFoo class.
     *
     * @class TestMojitModelFoo
     * @constructor
     */
    Y.mojito.models.TestMojitModelFoo = {

        init: function(config) {
            this.config = config;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        getData: function(callback) {
            callback({some:'data'});
        }

    };

}, '0.0.1', {requires: ['mojito']});
