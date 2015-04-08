/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('flickr-model', function (Y, NAME) {
    var API_KEY = '84921e87fb8f2fc338c3ff9bf51a412e';
    Y.namespace('mojito.models')[NAME] = {
        init: function (config) {
            this.config = config;
        },
        getData: function(callback) {
            callback({some: 'data'});
        },
        // Search for Flickr Images
        search: function (search, start, count, callback) {
            // Handle empty.
            if (null === search || 0 === search.length) {
                callback([]);
            }
            // Build YQL select. 
            start /= 1;
            count /= 1;
            var select = 'select * from ' + 'flickr.photos.search ' + '(' + (start || 0) + ',' + (count || 20) + ') ' + 'where ' + 'text="%' + (search || 'muppet') + '%" and api_key="' + API_KEY + '"';
            // Execute against YQL
            Y.YQL(select, function(rawYql) {
                // Handle empty response.
                if (null === rawYql || !rawYql.query.count || !rawYql.query.results) {
                    callback([]);
                }
                // Process data.
                var photos = [], item = null, i;
                // Force array.
                if (!rawYql.query.results.photo.length) {
                    rawYql.query.results.photo = [ rawYql.query.results.photo ];
                }
                // Assume array
                for (i = 0; i < rawYql.query.count; i += 1) {
                // Fix up the item.
                    item = rawYql.query.results.photo[i];
                    item.url = 'http://farm' + item.farm + '.static.flickr.com/' + item.server + '/' + item.id + '_' + item.secret + '.jpg';
                    item.title = (!item.title) ? search + ':' + i : item.title;
                    // Attach the result.
                    photos.push(
                        {
                            id: item.id,
                            title: item.title,
                            url: item.url
                        }
                    );
                }
                callback(photos);
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'yql']});
