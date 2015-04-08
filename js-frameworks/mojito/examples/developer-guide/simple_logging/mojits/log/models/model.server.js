/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('log-model', function (Y, NAME) {

/**
 * The log-model module.
 *
 * @module log
 */

    /**
     * Constructor for the log-model class.
     *
     * @class log-model
     * @constructor
     */
     Y.namespace('mojito.models')[NAME] = {

        init: function (config) {
            Y.log('model: entering into init (..)' + config, "INFO");
            this.config = config;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        getData: function (callback) {
            callback({some: 'data'});
        }

    };

}, '0.0.1', {requires: []});
