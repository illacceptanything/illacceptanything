/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('stateful-model', function(Y) {

/**
 * The stateful-model module.
 *
 * @module stateful-model
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models.Stateful = {

        init: function(config) {
            this.config = config;
            this.time = new Date().getTime() + '-model';
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        getData: function(callback) {
            callback(null, {modelId: this._getMyId()});
        },

        _getMyId: function() {
            return this.time;
        }

    };

}, '0.0.1', {requires: []});
