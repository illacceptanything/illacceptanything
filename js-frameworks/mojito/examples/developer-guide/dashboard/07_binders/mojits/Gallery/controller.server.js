/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('gallery', function (Y, NAME) {

/**
 * The gallery module.
 *
 * @module gallery
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
            var view_type = "yui", tablePath = "store://owgYr7PT7CWIOWMaWs9Stb", title = "YUI Gallery Pushes";

            ac.models.get('gallery').getData({}, tablePath, function (data) {
                // add mojit specific css
                ac.assets.addCss('./index.css');

                // populate youtube template
                ac.done({
                    title: title,
                    results: data
                });
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon']});
