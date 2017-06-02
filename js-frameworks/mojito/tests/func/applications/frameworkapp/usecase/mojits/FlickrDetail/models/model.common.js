/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('FlickrDetailModelFlickr', function(Y, NAME) {

/**
 * The FlickrDetailModelFlickr module.
 *
 * @module FlickrDetailModelFlickr
 */

    /**
     * Constructor for the FlickrDetailModelFlickr class.
     *
     * @class FlickrDetailModelFlickr
     * @constructor
     */
    Y.mojito.models[NAME] = {

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        getFlickrDetail: function(imageId, callback) {
            var API_KEY = '84921e87fb8f2fc338c3ff9bf51a412e';
            var q = 'select * from flickr.photos.info where photo_id="' + imageId + '" and api_key="' + API_KEY + '"';
            Y.YQL(q, function(rawYqlData) {
                if (!rawYqlData || !rawYqlData.query || !rawYqlData.query.results) {
                    callback();
                    return;
                }
                var photo = rawYqlData.query.results.photo;
                photo.urls.image = {
                    type: 'image',
                    content: buildFlickrUrlFromRecord(photo)
                };
                callback(photo);
            });
        }

    };

    function buildFlickrUrlFromRecord(record) {
        return 'http://farm' + record.farm 
            + '.static.flickr.com/' + record.server 
            + '/' + record.id + '_' + record.secret + '.jpg';
    }

}, '0.0.1', {requires: ['yql','jsonp-url']});
