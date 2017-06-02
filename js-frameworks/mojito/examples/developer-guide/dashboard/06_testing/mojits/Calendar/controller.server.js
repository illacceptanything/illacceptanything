
/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('calendar', function (Y, NAME) {

/**
 * The calendar module.
 *
 * @module calendar
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
            ac.models.get('calendar').getData({}, function (data) {
                Y.log("calendar -index - model.getData:");
                Y.log(data);

                // add mojit specific css
                ac.assets.addCss('./index.css');

                // populate blog template
                ac.done({
                    title: "YUI Calendar Info",
                    results: data
                });
            });
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon']});
