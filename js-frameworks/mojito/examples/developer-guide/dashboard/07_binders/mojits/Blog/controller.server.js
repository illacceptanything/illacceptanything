/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('blog', function (Y, NAME) {

/**
 * The blog module.
 *
 * @module blog
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
            var view_type = "yui", feedURL = "http://www.yuiblog.com/blog/feed/", title = "YUI Blog Posts";


            ac.models.get('blog').getData({}, feedURL, function (data) {
                // add mojit specific css
                ac.assets.addCss('./index.css');

                // populate blog template
                ac.done({
                    title: title,
                    results: data
                });
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon']});
