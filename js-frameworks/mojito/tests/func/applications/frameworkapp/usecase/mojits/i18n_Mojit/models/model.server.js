/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('i18n_MojitModel', function(Y, NAME) {


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
                 var rawPhotos = rawYqlData.query.results.photo,
                     rawPhoto = null,
                     photos = [],
                     photo = null,
                     i = 0;

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
