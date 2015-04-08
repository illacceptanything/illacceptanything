/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
/*jslint plusplus: true */
YUI.add('pagedflickr-model', function (Y, NAME) {
    "use strict";
    var API_KEY = '84921e87fb8f2fc338c3ff9bf51a412e';
    function buildFlickrUrlFromRecord(record) {
        return 'http://farm' + record.farm
            + '.static.flickr.com/' + record.server
            + '/' + record.id + '_' + record.secret + '.jpg';
    }

    Y.namespace('mojito.models')[NAME] = {
        getFlickrImages: function (queryString, start, count, callback) {
            var q;
            start = parseInt(start, 10) || 0;
            count = parseInt(count, 10) || 10;
            // The YQL docs say that the second number is the end, but in practice
            // it appears to be the count.
            // http://developer.yahoo.com/yql/guide/paging.html#remote_limits
            q = 'select * from flickr.photos.search(' + start + ',' + count + ') where text="' + queryString + '" and api_key="' + API_KEY + '"';
            Y.YQL(q, function (rawYqlData) {
                Y.log(rawYqlData);
                if (!rawYqlData.query.results) {
                    callback("No YQL results!");
                    return;
                }
                var rawPhotos = rawYqlData.query.results.photo,
                    rawPhoto = null,
                    photos = [],
                    photo = null,
                    i = 0;

                // Sometimes YQL returns more than the requested amount.
                rawPhotos.splice(count);
                for (i = 0; i < rawPhotos.length; i++) {
                    rawPhoto = rawPhotos[i];
                    photo = {
                        title: rawPhoto.title,
                        url: buildFlickrUrlFromRecord(rawPhoto)
                    };
                    // some flickr photos don't have titles, so force them
                    if (!photo.title) {
                        photo.title = "[" + queryString + "]";
                    }
                    photos.push(photo);
                }
                Y.log('calling callback with photos');
                Y.log(photos);
                callback(null, photos);
            });
        }
    };
// TODO: remove 'jsonp-url' requirement when YUI fix for bug http://yuilibrary.com/projects/yui3/ticket/2530251 is deployed.
}, '0.0.1', {requires: ['yql', 'jsonp-url']});
