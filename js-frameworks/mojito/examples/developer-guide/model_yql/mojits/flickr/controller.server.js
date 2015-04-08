/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('flickr', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        index: function (ac) {
            // Use aliases to params addon
            // if they exist.
            var q, page, count, start, model;
            if (ac.params.hasOwnProperty('url')) {
                q = ac.params.url('q') || 'muppet';
                page = (ac.params.url('page') || 0);
                count = (ac.params.url('size') || 20);
            } else {
                q = ac.params.getFromUrl('q') || 'muppet';
                page = (ac.params.getFromUrl('page') || 0);
                count = (ac.params.getFromUrl('count') || 20);
            }
            start = page * count;
            model = ac.models.get('model');
            model.search(q, start, count, function (photos) {
                ac.done({
                    photos: photos,
                    page: page,
                    count: count,
                    start: start
                });
            });
        }
    };
}, '0.0.1', {requires: [
    'mojito',
    'mojito-params-addon',
    'mojito-models-addon',
    'flickr-model'
]});
