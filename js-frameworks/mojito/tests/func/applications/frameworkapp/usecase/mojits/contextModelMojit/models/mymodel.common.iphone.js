/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('IphonecontextModelMojitModel', function(Y, NAME) {

/**
 * The contextModelMojitModel module.
 *
 * @module contextModelMojit
 */

    /**
     * Constructor for the contextModelMojitModel class.
     *
     * @class contextModelMojitModel
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
             callback(null, {message: 'Hello Iphone Data'});
         }

    };

}, '0.0.1', {requires: []});
