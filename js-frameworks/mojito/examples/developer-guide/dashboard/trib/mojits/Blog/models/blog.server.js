
/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('blog-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
        },
        getData: function (params, feedURL, callback) {
            Y.log("blogmojit getData called", "info", NAME);
            Y.log(this.config, "info", NAME);

            var query = "select title,link,pubDate, description, dc:creator from feed where url='{feed}' limit 5",
                queryParams = {
                    feed: feedURL
                },
                cookedQuery = Y.Lang.sub(query, queryParams);

            Y.log("blog cookedQuery: " + cookedQuery, "info", NAME);

            if (this._isCached()) {
                //Y.log("blogData! skip YQL", "info", NAME);
                //Y.log(Y.blogData, "info", NAME);

                callback(null, Y.blogData);
            } else {
                Y.namespace("blogData", "info", NAME);
                Y.log("blogmodel calling YQL", "info", NAME);
                Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));
            }

        },
        onDataReturn: function (cb, result) {
            Y.log("blog.server onDataReturn called", "info", NAME);
            var results = [], err = null;
                
            if (!result.error) {

                if (result && result.query && result.query.results && result.query.results.item) {
                    results = result.query.results.item;
                    Y.blogData = results;
                    Y.blogCacheTime = new Date().getTime();
                } else {
                    err = new Error({ error: { description: "The returned response is malformed." }});
                }
                Y.log("result.query.results.item = results: ", "info", NAME);
                Y.log(results, "info", NAME);
            } else {
                // Results had an error.
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
            return Y.blogData && (new Date().getTime() - Y.blogCacheTime) < updateTime;
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
