/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('youtube-model-yql', function(Y, NAME) {

/**
 * The youtube-model-yql module.
 *
 * @module youtube-model-yql
 */

    /**
     * Constructor for the YoutubeModelYQL class.
     *
     * @class YoutubeModelYQL
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
         getData: function (params, callback) {
             Y.log("youtube server getData called");
             var
                feedURL = "https://gdata.youtube.com/feeds/base/users/yuilibrary/uploads",
                query = "select id,title,link,published from feed(0,6) where url='{feed}' and link.rel='alternate'",
                queryParams = {
                    feed: feedURL
                },
                cookedQuery = Y.Lang.sub(query, queryParams);

             Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));

         },
         onDataReturn: function (cb, result) {
            Y.log("youtube.server onDataReturn called");
            if (result.error === undefined) {

                var results = {};

                if (result && result.query && result.query.results && result.query.results.entry) {
                    results = result.query.results.entry;

                } else {
                    results = null;
                }
                cb(results);
            } else {
                cb(result.error);
            }
        }
    };
}, '0.0.1', {requires: []});
