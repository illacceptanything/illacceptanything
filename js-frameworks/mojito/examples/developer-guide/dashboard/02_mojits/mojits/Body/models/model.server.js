/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('body-model', function(Y, NAME) {

/**
 * The body-model module.
 *
 * @module body
 */

    /**
     * Constructor for the BodyModel class.
     *
     * @class BodyModel
     * @constructor
     */
    Y.namespace('mojito.models')[NAME] = {

        init: function(config) {
            this.config = config;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {function(err,data)} The callback function to call when the
         *        data has been retrieved.
         */
        getData: function(callback) {
            callback(null, { some: 'data' });
        }

    };

}, '0.0.1', {requires: []});
