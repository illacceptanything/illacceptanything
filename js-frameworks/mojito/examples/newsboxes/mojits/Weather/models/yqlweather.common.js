/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, regexp:true*/
/*global YUI*/


YUI.add('YqlWeatherModel', function(Y, NAME) {

    Y.mojito.models[NAME] = {

        fetch: function(location, callback) {
            var self = this,
                query =
                    'SELECT item.condition, item.description' +
                    ' FROM weather.forecast WHERE location' +
                    ' IN(SELECT id FROM weather.search WHERE query=@loc)' +
                    ' LIMIT 1',
                param = {
                    loc: location,
                    env: 'store://datatables.org/alltableswithkeys',
                    format: 'json'
                };

            function afterYql(response) {
                var data = {},
                    error = null,
                    item = response.query &&
                        response.query.results &&
                            response.query.results.channel &&
                                response.query.results.channel.item;

                if (!error || !item.condition || !item.description) {
                    data = {
                        temp: item.condition.temp,
                        text: item.condition.text,
                        pict: item.description.match(/l\.yimg\.com.+\.gif/)[0]
                    };
                } else {
                    error = 'no data for ' + location;
                }

                callback(error, data);
            }

            Y.YQL(query, afterYql, param);
        }
    };

}, '0.0.1', {requires: ['mojito', 'yql', 'jsonp-url']});

/*
http://query.yahooapis.com/v1/public/yql?loc=san+francisco,california&env=store://datatables.org/alltableswithkeys&format=json&q=SELECT+item.condition,item.description+FROM+weather.forecast+WHERE+location+IN(SELECT+id+FROM+weather.search+WHERE+query=@loc)+LIMIT+1
*/
