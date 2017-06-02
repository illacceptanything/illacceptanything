/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickr5Model', function(Y, NAME) {

/**
 * The PagedFlickr5Model module.
 *
 * @module PagedFlickr5Model
 */

    /**
     * Constructor for the PagedFlickrModelFlickr class.
     *
     * @class PagedFlickr5Model
     * @constructor
     */
    Y.mojito.models[NAME] = {

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        getFlickrImages: function (search, start, count, callback) {         
             var photos = [], i,
                 url = [
                     '/static/PagedFlickr5/assets/BanffPark.jpg',
                     '/static/PagedFlickr5/assets/Calgary.jpg',
                     '/static/PagedFlickr5/assets/JasperPark.jpg',
                     '/static/PagedFlickr5/assets/RockMountain.jpg'
                 ];
             for (i = 0; i < url.length; i += 1) {
                     photos.push({
                         id: i,
                         title: 'picture number'+i,
                         url: url[i]
                     });
             }
             callback(photos);
         }

    };

}, '0.0.1', {requires: ['yql','jsonp-url']});
