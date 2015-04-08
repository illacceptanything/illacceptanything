/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ModelFlickr', function(Y) {

    Y.mojito.models.flickr = {

        getFlickrImages: function(queryString, start, count, callback) {
            var q;
            start = parseInt(start) || 0;
            count = parseInt(count) || 10;
            // The YQL docs say that the second number is the end, but in practice
            // it appears to be the count.
            // http://developer.yahoo.com/yql/guide/paging.html#remote_limits
            q = 'select * from flickr.photos.search(' + start + ',' + count + ') where text="' + queryString + '"';
                Y.YQL(q, function(rawYqlData) {
                    if (!rawYqlData || !rawYqlData.query || !rawYqlData.query.results) {
                        callback(rawYqlData);
                        return;
                    }
                    var rawPhotos = rawYqlData.query.results.photo,
                        rawPhoto = null,
                        photos = [],
                        photo = null,
                        i = 0;

                    for (; i < rawPhotos.length; i++) {
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

        getFlickrDetail: function(imageId, callback) {
            var q = 'select * from flickr.photos.info where photo_id="' + imageId + '"';
            Y.YQL(q, function(rawYqlData) {
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

    function buildFlickrUrlFromRecord(record) {
        return 'http://farm' + record.farm
                + '.static.flickr.com/' + record.server
                + '/' + record.id + '_' + record.secret + '.jpg';
    }

// TODO: remove 'jsonp-url' requirement when YUI fix for bug http://yuilibrary.com/projects/yui3/ticket/2530251 is deployed.
}, '0.0.1', {requires: ['yql', 'jsonp-url']});
