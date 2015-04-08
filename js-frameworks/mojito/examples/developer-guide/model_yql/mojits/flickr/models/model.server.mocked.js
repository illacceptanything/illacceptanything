/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('iphoneflickr-model', function (Y, NAME) {
    Y.namespace('mojito.models')[NAME] = {

        init: function(config) {
            this.config = config;
        },
        getData: function (callback) {
            callback({some: 'data'});
        },
        // Search for Flickr Images
        search: function (search, start, count, callback) {         // Handle empty.
            if (null === search || 0 === search.length) {
                callback([]);
            }
            var photos = [], i,
                url = [
                    '/static/flickr/assets/BanffPark.jpg',
                    '/static/flickr/assets/Calgary.jpg',
                    '/static/flickr/assets/JasperPark.jpg',
                    '/static/flickr/assets/RockMountain.jpg'
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
}, '0.0.1', { requires: ['mojito', 'yql']});
