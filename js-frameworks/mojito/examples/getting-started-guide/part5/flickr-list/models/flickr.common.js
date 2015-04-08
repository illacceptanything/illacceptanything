/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
/*jslint plusplus: true */
YUI.add('flickr-model', function (Y, NAME) {
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
                if (!rawYqlData || !rawYqlData.query || !rawYqlData.query.results) {
                    callback(rawYqlData);
                    return;
                }
                var rawPhotos = rawYqlData.query.results.photo,
                    rawPhoto = null,
                    photos = [],
                    photo = null,
                    i = 0;

                for (i = 0; i < rawPhotos.length; i++) {
                    rawPhoto = rawPhotos[i];
                    photo = {
                        id: rawPhoto.id,
                        title: rawPhoto.title,
                        url: buildFlickrUrlFromRecord(rawPhoto)
                    };
                    // some flickr photos don't have titles, so force them
                    if (!photo.title) {
                        photo.title = "[" + queryString + "]";
                    }
                    photos.push(photo);
                }
                callback(null, photos);
            });
        },

        getFlickrDetail: function (imageId, callback) {
            var q = 'select * from flickr.photos.info where photo_id="' + imageId + '" and api_key="' + API_KEY + '"';
            Y.YQL(q, function (rawYqlData) {
                if (!rawYqlData || !rawYqlData.query || !rawYqlData.query.results) {
                    callback("BAD YQL!");
                    return;
                }
                var photo = rawYqlData.query.results.photo;
                photo.urls.image = {
                    type: 'image',
                    content: buildFlickrUrlFromRecord(photo)
                };
                callback(null, photo);
            });
        }

    };


// TODO: remove 'jsonp-url' requirement when YUI fix for bug http://yuilibrary.com/projects/yui3/ticket/2530251 is deployed.
}, '0.0.1', {requires: ['yql', 'jsonp-url']});
