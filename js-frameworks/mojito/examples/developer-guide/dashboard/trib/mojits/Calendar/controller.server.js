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
            ac.models.get('calendar').getData({}, function (err, data) {
                var title = "YUI calendar Info";
                //Y.log("calendar -index - model.getData:", "info", NAME);
                //Y.log(data, "info", NAME);
                //Y.log("data 0 :", "info", NAME);
                //Y.log(data[0], "info", NAME);
                //Y.log("data 1 :", "info", NAME);
                //Y.log(data[1], "info", NAME);

                // Add mojit specific CSS
                ac.assets.addCss('./index.css');
                if(err) {
                    ac.error(err);
                } else {
                    // Populate and render calendar template
                    ac.done({
                        title: title,
                        results: data
                    });
                }
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon']});
