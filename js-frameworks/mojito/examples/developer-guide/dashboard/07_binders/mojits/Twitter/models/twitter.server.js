YUI.add('twittersearch-model', function (Y, NAME) {
    Y.mojito.models[NAME] = {

        init: function (config) {
            this.config = config;
        },
        getData: function (count, q, oauth, cb) {
            // Confirm OAuth keys have been passed
            // You'll also need to add `simple-twitter: "~1.0.0"` to the `dependencies` object in
            // `package.json` and then run `npm install` from the application directory
            // to get the `simple-twitter` module that will call the Twitter Search API
            // If `oauth` is null, you'll be using the mocked data.
            Y.log(oauth, "info", NAME);
            if (oauth) {
                var twitter = require('simple-twitter'),
                    tweets = new twitter(
                        oauth.consumer_key,
                        oauth.consumer_secret,
                        oauth.access_token_key,
                        oauth.access_token_secret
                   );
                tweets.get("search/tweets", "?q="+ encodeURIComponent(q) + "&count=" + count,
                    function(error, data) {
                        if(error) {
                            return cb(error);
                        }
                        cb(null, Y.JSON.parse(data));
                    }
                );
              //  Use mock data if no OAuth keys have been provided
            } else {
                var mock_yui_data = { statuses: [ { from_user: "YUI User 1", text: "Love the new YUI Pure!" },
                                          { from_user: "YUI User 2", text: "YUI Charts is off the charts!" },
                                          { from_user: "YUI User 3", text: "Mojito + YUI = developer goodness." },
                                          { from_user: "YUI User 4", text: "The YUI Gallery offers all kinds of cool modules!" },
                                          { from_user: "YUI User 5", text: "I'm anxious to try the YUI App Framework." }
                                       ]};
                var mock_mojito_data = { statuses: [ { from_user: "Mojit User 1", text: "Mojits are self-contained MVC modules." },
                                          { from_user: "Mojito User 2", text: "The Data addon allows you to rehydrate data on the client!" },
                                          { from_user: "Mojito User 3", text: "Mojito + YUI = developer goodness." },
                                          { from_user: "Mojito User 4", text: "Mojito makes it easier to create pages for different devices." },
                                          { from_user: "Mojito User 5", text: "The Mojito CLI is now a separate package from Mojito." }
                                       ]};
                if ("@yuilibrary"==q) {
                    cb(null, mock_yui_data);
                } else {
                    cb(null, mock_mojito_data);
                }
            }
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-rest-lib', 'json']});
