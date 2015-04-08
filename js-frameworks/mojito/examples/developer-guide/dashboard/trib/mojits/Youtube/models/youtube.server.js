/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('youtube-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
        },
        getData: function (params, callback) {
            Y.log("youtube server getData called", "info", NAME);

            if (this._isCached()) {
                callback(null, Y.youtubeData);
            } else {
                var
                    feedURL = "https://gdata.youtube.com/feeds/base/users/yuilibrary/uploads",
                    query = "select id,title,link,published from feed(0,6) where url='{feed}' and link.rel='alternate'",
                    queryParams = {
                        feed: feedURL
                    },
                    cookedQuery = Y.Lang.sub(query, queryParams);

                Y.log("youtube cookedQuery: " + cookedQuery, "info", NAME);

                Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));
            }

        },
        onDataReturn: function (cb, result) {
            Y.log("youtube.server onDataReturn called", "info", NAME);
            var results = [], err = null;
            if (!result.error) {

                //Y.log("result: ", "info", NAME);
                //Y.log(result, "info", NAME);
                if (result && result.query && result.query.results && result.query.results.entry) {
                    results = result.query.results.entry;
                    Y.youtubeData = results;
                    Y.youtubeCacheTime = new Date().getTime();
                } else {
                    err = new Error({ error: { description: "The returned response was malformed." }});
                }
            } else {
                err = result;
            }
            // Return valid results or error in callback to controller.
            if (err) {
              cb(err);
            } else {
              cb(null, results);
            }
        },
        _isCached: function() {
            var updateTime = this.config.feedCacheTime * 60 * 1000;
            return Y.youtubeData && (new Date().getTime() - Y.youtubeCacheTime) < updateTime;
        }
    };

}, '0.0.1', {requires: ['yql', 'substitute']});
