/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('iphonei18n_MojitModel', function (Y, NAME) {
    Y.namespace('mojito.models')[NAME] = {

        init: function(config) {
            this.config = config;
        },
        getFlickrImages: function(queryString, callback) {       
            var photos = [], i,
                url = [
                    '/static/usecase/assets/BanffPark.jpg',
                    '/static/usecase/assets/Calgary.jpg',
                    '/static/usecase/assets/JasperPark.jpg',
                    '/static/usecase/assets/RockMountain.jpg'
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
