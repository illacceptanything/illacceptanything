YUI.add('twitter', function (Y, NAME) {

/**
 * The twitter module.
 *
 * @module twitter
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    'use strict';
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function (ac) {
            var view_type, q, title, oauth_keys, count = 10;
            view_type = ac.params.getFromRoute('view_type') || "yui";

            if (view_type === "yui") {
                q = ac.config.getDefinition('yuiquery', 'notfound');
                title = ac.config.getDefinition('yuititle', 'notitle');
            } else if (view_type === "mojito") {
                q = ac.config.getDefinition('mojitoquery', 'notfound');
                title = ac.config.getDefinition('mojitotitle', 'notitle');
            }

            // Get twitter API keys from your developer account (https://dev.twitter.com/apps) and
            // use the `oauth_keys` to hold your consumer key/secret and access token/secret.
            // If you leave `oauth_keys` undefined, your app will just use mocked data.
            // Get OAuth keys from definition.json to get real data.
            // oauth_keys = ac.config.getDefinition('oauth');
            ac.models.get('twitter').getData(count, q, oauth_keys, function (err, data) {
                if (err) {
                    ac.error(err);
                    return;
                }
                // add mojit specific css
                ac.assets.addCss('./index.css');
                ac.done({
                    title: "YUI twitter Mentions",
                    results: data.statuses
                });
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'mojito-params-addon', 'mojito-config-addon', 'mojito-helpers-addon']});
