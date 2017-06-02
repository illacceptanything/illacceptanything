/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('gallery-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
        },

        getData: function (params, tablePath, callback) {
            Y.log("gallery getData called", "info", NAME);

            if (this._isCached()) {
                callback(null, Y.galleryData);
            } else {
                var query = "use '{table}' as gallerylogs; select * from gallerylogs",
                    queryParams = {
                        table: tablePath
                    },
                    cookedQuery = Y.Lang.sub(query, queryParams);

                    //Y.log("cookedQuery: " + cookedQuery, "info", NAME);
                Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));
            }
        },

        onDataReturn: function (cb, result) {
            Y.log("onDataReturn called", "info", NAME);
            var itemLimit = 10, results = [], err = null; 
            if (!result.error) {

                Y.log("gallery onDataReturn result:", "info", NAME);
                Y.log(result, "info", NAME);

                if (result && result.query && result.query.results && result.query.results.json) {
                    results = result.query.results.json;
                    results.json = results.json.slice(0, itemLimit);
                    Y.galleryData = results;
                    Y.galleryCacheTime = new Date().getTime();
                } else {
                    err = new Error({ error: { description: "The returned response was malformed." }}); 
                }

            } else {
                // Response had an error.
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
            return Y.calendarData && (new Date().getTime() - Y.calendarCacheTime) < updateTime;
        }
    };


}, '0.0.1', {requires: ['yql', 'substitute']});
