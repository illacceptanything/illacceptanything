/*jslint anon:true, sloppy:true, nomen:true, indent: 4, white: false*/
/*global YUI*/
YUI.add('stats-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {

        init: function (config) {
            this.config = config;
            // Create cache object
            Y.yqlData = {};
        },
        getData: function (params, yqlTable, id, repo, callback) {
            Y.log(this.config, "info", NAME);
            if (this._isCached(repo)) {
                callback(null, Y.yqlData);
            } else {
                var itemLimit = "10",
                    query = "use '{table}' as github.events; select json.type, json.actor, json.payload from github.events where id='{id}' and repo='{repo}' limit {limit}",
                    queryParams = {
                        table: yqlTable,
                        limit: itemLimit,
                        id: id,
                        repo: repo
                    },
                    cookedQuery = Y.Lang.sub(query, queryParams);
                Y.log("github: yql.server getData called", "info", NAME);
                Y.log("github: cookedQuery:" + cookedQuery, "info", NAME);
                Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback, repo));
            }
        },
        onDataReturn: function (cb, repo, result) {
            Y.log("github: onDataReturn called", "info", NAME);
            var results = [], err = null;
            if (!result.error) {

                Y.log("github: result:", "info", NAME);
                Y.log(result, "info", NAME);
                if (result.query && result.query.results && result.query.results.json) {

                    results = result.query.results.json;
                    Y.yqlData[repo] = results;
                    Y.yqlCacheTime = new Date().getTime();
                } else {
                    err = new Error({ error: { description: "The returned response is malformed." }});
                } 
                Y.log("github: results.json:", "info", NAME);
                Y.log(results, "info", NAME);
            } else {
                // Results had an error.
                err = result;
            }
            // Return valid results or error in callback to controller.
            if (err) {
              Y.log(err, "info", NAME);
              cb(err);
            } else {
              cb(null, results);
            }
        },
        _isCached: function(repo) {
            var updateTime = this.config.feedCacheTime * 60 * 1000;
            if (Y.yqlData[repo]) {
                return Y.yqlData[repo] && (new Date().getTime() - Y.yqlCacheTime) < updateTime;
            }
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
