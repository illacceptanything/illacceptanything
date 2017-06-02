/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('contextModelMojit', function(Y, NAME) {

/**
 * The contextModelMojit module.
 *
 * @module contextModelMojit
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
         index: function(ac) {

             ac.models.get('mymodel').getData(function(err, data) {
                 if (err) {
                     ac.error(err);
                     return;
                 }
                 ac.done({
                     message: data.message
                 });
             });
         }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-assets-addon',
    'mojito-models-addon',
    'contextModelMojitModel',
    'IphonecontextModelMojitModel'
]});
