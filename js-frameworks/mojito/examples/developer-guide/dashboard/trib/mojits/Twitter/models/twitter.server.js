/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('twittersearch-model', function (Y, NAME) {
    Y.mojito.models[NAME] = {

        init: function (config) {
            this.config = config;
        },
        getData: function (count, q, oauth, cb) {
            var mock_yui_data, mock_mojito_data, Twitter, tweets;
            // Confirm OAuth keys have been passed
            Y.log("twitter: oauth", "info", NAME);
            Y.log(oauth, "info", NAME);
            if (oauth) {
                Twitter = require('simple-twitter');
                tweets = new Twitter(
                    oauth.consumer_key,
                    oauth.consumer_secret,
                    oauth.access_token_key,
                    oauth.access_token_secret
                );
                tweets.get("search/tweets", "?q=" + encodeURIComponent(q) + "&count=" + count,
                    function(error, data) {
                        if (error) {
                            return cb(error);
                        }
                        var resp = Y.JSON.parse(data);
                        Y.twitterData = resp;
                        Y.twitterCacheTime = new Date().getTime();
                        cb(null, resp);
                    }
                     );
              //  Use mock data if no OAuth keys have been provided
            } else {
                mock_yui_data = { statuses: [ { from_user: "YUI User 1", text: "Love the new YUI Pure!" },
                                          { from_user: "YUI User 2", text: "YUI Charts is off the charts!" },
                                          { from_user: "YUI User 3", text: "Mojito + YUI = developer goodness." },
                                          { from_user: "YUI User 4", text: "The YUI Gallery offers all kinds of cool modules!" },
                                          { from_user: "YUI User 5", text: "I'm anxious to try the YUI App Framework." }
                                       ]};
                mock_mojito_data = { statuses: [ { from_user: "Mojit User 1", text: "Mojits are self-contained MVC modules." },
                                          { from_user: "Mojito User 2", text: "The Data addon allows you to rehydrate data on the client!" },
                                          { from_user: "Mojito User 3", text: "Mojito + YUI = developer goodness." },
                                          { from_user: "Mojito User 4", text: "Mojito makes it easier to create pages for different devices." },
                                          { from_user: "Mojito User 5", text: "The Mojito CLI is now a separate package from Mojito." }
                                       ]};
                if ("@yuilibrary" === q) {
                    cb(null, mock_yui_data);
                } else {
                    cb(null, mock_mojito_data);
                }
            }
        },
        _isCached: function() {
            var updateTime = this.config.feedCacheTime * 60 * 1000;
            return Y.twitterData && (new Date().getTime() - Y.twitterCacheTime) < updateTime;
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-rest-lib', 'json']});
