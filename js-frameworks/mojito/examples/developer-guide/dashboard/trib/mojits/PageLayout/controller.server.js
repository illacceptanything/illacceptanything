/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('pagelayout', function (Y, NAME) {

/**
 * The pagelayout module.
 *
 * @module pagelayout 
 */
    // Handlerbars helper for creating links
    function createLink(title, url, path, css) {
        return "<a href='" + url + path + "'" + " class='" + css + "'>" + title + "</a>";
    }
    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        init: function (config) {
            this.config = config;
        },

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function (ac) {
            // Register helper for use for all mojits on the page
            ac.helpers.expose('linker', createLink);

            var view_type = ac.params.getFromRoute('view_type') || "yui";

            if (view_type === "yui") {
                ac.composite.done({
                    title: ac.intl.lang("YUITitle"),
                    button_text: "See Mojito Dashboard",
                    other: "/mojito"
                });
            } else if (view_type === "mojito") {
                ac.composite.done({
                    title: ac.intl.lang("MojitoTitle"),
                    button_text: "See YUI Dashboard",
                    other: "/"
                });
            }
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'mojito-composite-addon', 'mojito-params-addon', 'mojito-helpers-addon', 'mojito-intl-addon']});
