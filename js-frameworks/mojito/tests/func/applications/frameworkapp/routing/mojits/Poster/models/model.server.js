/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('PosterModel', function(Y, NAME) {

/**
 * The PosterModel module.
 *
 * @module PosterModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models.Poster = {

        init: function(mojitSpec) {
            this.spec = mojitSpec;
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
