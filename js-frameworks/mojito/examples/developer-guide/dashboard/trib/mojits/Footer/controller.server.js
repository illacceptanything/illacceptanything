/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('footer', function (Y, NAME) {

/**
 * The footer module.
 *
 * @module footer
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
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function (ac) {
            ac.done({
                title: "Copyright 2013 Yahoo! Inc."
            });
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon']});
