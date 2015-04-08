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
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function (ac) {
            var q="@yuilibrary", oauth_keys, count=10;

            // Get Twitter API keys from your developer account (https://dev.twitter.com/apps) and 
            // use the `oauth_keys` to hold your consumer key/secret and access token/secret.
            // If you leave `oauth_keys` undefined, your app will just use mocked data.
            /*
             * oauth_keys = {
             *    "consumer_key": "xxxx",
             *    "consumer_secret": "xxxx",
             *    "access_token_key": "xxxx",
             *    "access_token_secret": "xxxx"
             * }
            */

            // Get OAuth keys from definition.json to get real data.
            // If `oauth_keys==null`, use mock data from model.
            ac.models.get('twitter').getData(count, q, oauth_keys, function (err, data) {
                if (err) {
                    ac.error(err);
                    return;
                }
                // add mojit specific css
                ac.assets.addCss('./index.css');
                ac.done({
                    title: "YUI Twitter Mentions",
                    results: data.statuses
                });
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'mojito-params-addon']});
