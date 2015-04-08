/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SearchMojitModel', function(Y, NAME) {

    Y.mojito.models[NAME] = {

        init: function(config) {
            this.config = config;
        },

        getData: function (query, zip, callback) {
            if (!query || !zip) {
                callback([]);
                return;
            }
            Y.YQL("select * from local.search where zip='" + zip + "' and query='" + query + "'", function(r) {
                if (r && r.query && r.query.results && r.query.results.Result) {
                    callback(r.query.results.Result);
                } else {
                    callback([]);
                }
            });
        }
    };
}, '0.0.1', {requires: ['yql']});
