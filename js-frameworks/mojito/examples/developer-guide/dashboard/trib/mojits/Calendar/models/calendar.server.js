
/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('calendar-model-yql', function (Y, NAME) {
    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
        },
        getData: function (params, callback) {
            Y.log("getData called", "info", NAME);
            var
                feedURL = "https://www.google.com/calendar/feeds/fcde7kbrqnu7iccq9ofi9lqqf8%40group.calendar.google.com/public/basic",
                query = "select entry.title, entry.summary, entry.link from xml where url='{feed}' and entry.link.rel='alternate' limit 10",
                queryParams = {
                    feed: feedURL
                },
                cookedQuery = Y.Lang.sub(query, queryParams);
            Y.log("calendar cookedQuery: " + cookedQuery, "info", NAME);

            if (this._isCached()) {
                Y.log("calendarData! skip YQL", "info", NAME);
                callback(null, Y.calendarData);
            } else {
                Y.namespace("calendarData");
                Y.log("calendarModel calling YQL", "info", NAME);
                Y.YQL(cookedQuery, Y.bind(this.onDataReturn, this, callback));
            }
        },
        onDataReturn: function (cb, result) {
            Y.log("calendar.server onDataReturn called", "info", NAME);
            Y.log(result, "info", NAME);
            var results = [], err = null;
            if (!result.error) {

                Y.log("onDataReturn: calendar-model-yql...", "info", NAME);
                Y.log("result: ", "info", NAME);
                Y.log(result, "info", NAME);

                if (result && result.query && result.query.results && result.query.results.feed) {
                    results = result.query.results.feed;
                    //Y.log("results 0 summary . content", "info", NAME);
                    //Y.log(results[0].entry.summary.content, "info", NAME);
                    Y.Array.each(results, function (val, key, obj) {
                        Y.log(val.entry.summary.content);
                        var tempDate = val.entry.summary.content;
                        // Strip off 'br', 'When:'' and 'to' elements to get date
                        tempDate = tempDate.split("<")[0].split("When:")[1]; 
                        val.entry.summary.content = tempDate;
                        Y.log(val.entry.summary.content, "info", NAME);
                    });
                    Y.calendarData = results;
                    Y.calendarCacheTime = new Date().getTime();
                } else {
                    err = new Error({ error: { description: "Malformed response couldn't be parsed." }});
                }
            } else {
                // Results had an error
                err= result;
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
