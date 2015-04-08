/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('i18nMojitModel', function(Y, NAME) {

/**
 * The i18nMojitModel module.
 *
 * @module i18nMojitModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models[NAME] = {

        init: function(mojitSpec) {
            this.spec = mojitSpec;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
         getFlickrImages: function(queryString, callback) {
             var API_KEY = '84921e87fb8f2fc338c3ff9bf51a412e';
             var q = 'select * from flickr.photos.search where text="%' + queryString + '" and api_key="' + API_KEY + '"';
             Y.YQL(q, function(rawYqlData) {
                 var rawPhotos,
                     rawPhoto = null,
                     photos = [],
                     photo = null,
                     i = 0;

                 rawPhotos = rawYqlData.query.results && rawYqlData.query.results.photo || [];

                 for (; i<rawPhotos.length; i++) {
                     rawPhoto = rawPhotos[i];
                     photo = {
                         title: rawPhoto.title,
                         url: buildFlickrUrlFromRecord(rawPhoto)
                     };
                     photos.push(photo);
                 }

                 callback(photos);

             });
         }
    };

    function buildFlickrUrlFromRecord(record) {
        return 'http://farm' + record.farm 
            + '.static.flickr.com/' + record.server 
            + '/' + record.id + '_' + record.secret + '.jpg';
    }

}, '0.0.1', {requires: ['mojito', 'yql']});
