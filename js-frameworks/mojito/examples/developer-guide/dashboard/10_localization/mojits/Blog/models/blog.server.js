/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('blog-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
        },
        getData: function (params, feedURL, callback) {
            Y.log("blogmojit getData called");
            Y.log(this.config);

            var query = "select title,link,pubDate, description, dc:creator from feed where url='{feed}' limit 5",
                queryParams = {
                    feed: feedURL
                },
                cookedQuery = Y.Lang.sub(query, queryParams);

            Y.log("blogmodel calling YQL");
            Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));
        },
        onDataReturn: function (cb, result) {
            Y.log("blog.server onDataReturn called");
            if (result.error === undefined) {

                //Y.log("result: ");
                //Y.log(result);

                var results = result.query.results.item;
                cb(results);
            } else {
                cb(result.error);
            }
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
