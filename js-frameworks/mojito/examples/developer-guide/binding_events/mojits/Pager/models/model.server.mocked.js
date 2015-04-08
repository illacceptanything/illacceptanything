/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('iphonepager-model', function (Y, NAME) {
    var photos = [];
    Y.namespace('mojito.models')[NAME] = {
        init: function(config) {
            this.config = config;
        },
        getData: function (search, start, count, callback) {
            var id, title,
                url = [
                    '/static/PagerMojit/assets/pic.com/1234/1001_banffpark.jpg',
                    '/static/PagerMojit/assets/pic.com/1234/1002_calgary.jpg',
                    '/static/PagerMojit/assets/pic.com/1234/1003_jasperpark.jpg',
                    '/static/PagerMojit/assets/pic.com/1234/1004_rockmountain.jpg'
                ];
            photos.length = 0;
            for (i = 0; i < url.length; i += 1) {
                id = url[i].match(/com\/(\d+)\/(\d+)_([0-9a-z]+)\.jpg$/);
                title = url[i].split("/");
                photos.push({
                    id: id[2],
                    title: title[6],
                    location: url[i]
                });
            }
            callback(photos);
        },
        getContent: function (imageId, callback) { 
            for (i = 0; i < photos.length; i += 1) {
                id = photos[i].id;
                if(imageId === id) {
                    var raw = {
                        query: {
                            results: {
                                photo: photos[i]
                            }
                        }
                    }
                    callback(raw);
                }
            } 
        }
    };
}, '0.0.1', { requires: ['mojito', 'yql']});
