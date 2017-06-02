YUI.add('stats-model-yql', function(Y, NAME) {

    Y.mojito.models[NAME] = {

        init: function(config) {
            this.config = config;
        },
        getData: function(params, callback) {
            var yqlTable = 'https://raw.github.com/triptych/trib/master/src/yql/github.xml', 
                query = "use '{table}' as yahoo.awooldri.github.repo; select watchers,forks from yahoo.awooldri.github.repo where id='yql' and repo='yql-tables'",
                queryParams = {
                    table: yqlTable
                },
                cookedQuery = Y.substitute(query, queryParams);
                Y.log("getData called");
                Y.log("cookedQuery:" + cookedQuery);
                Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));
        },
        onDataReturn: function (cb, result) {
            Y.log("onDataReturn called");
            if (typeof result.error === 'undefined') {

                Y.log("result:");
                Y.log(result);
                var results = result.query.results.json;
                cb(results);
            } else {
                cb(result.error);
            }
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
