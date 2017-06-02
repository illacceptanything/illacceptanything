/*
 * Copyright (c) 2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('ColorChildModel', function (Y, NAME) {

/**
 * The ColorChildModel module.
 *
 * @module ColorChildModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models[NAME] = {

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
            callback({color: this.color});
        }

    };

}, '0.0.1', {requires: []});
