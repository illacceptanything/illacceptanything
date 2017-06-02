/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('pagedflickr', function (Y, NAME) {

    "use strict";
    var PAGESIZE = 9;
    function selfUrl(ac, page) {
        // No real link for pages before page 1
        if (page < 1) { return '#'; }
        var params = ac.params.url();
        params.page = page; // provide the page we want to createa  URL to
        return ac.url.make('flickr', 'index', Y.QueryString.stringify(params));
    }

    Y.namespace('mojito.controllers')[NAME] = {

        index: function (ac) {
            var page = ac.params.merged('page'),
                start;

            // a little paranoia about inputs
            page = parseInt(page, 10);
            if ((!page) || (page < 1)) {
                page = 1;
            }

            // The "page" parameter is base-1, but the model's "start"
            // parameter is base-0.
            start = (page - 1) * PAGESIZE;

            ac.models.get('model').getFlickrImages('mojito', start, PAGESIZE, function(err, images) {

                var dateString, data;

                // on model error, fail fast
                if (err) {
                    return ac.error(err);
                }

                dateString = ac.intl.formatDate(new Date());
                data = {
                    images: images,
                    date: dateString,
                    greeting: ac.intl.lang("TITLE") || 'title',
                    prev: {
                        url: selfUrl(ac, page - 1),
                        title: ac.intl.lang("PREV") || 'prev'
                    },
                    next: {
                        url: selfUrl(ac, page + 1),
                        title: ac.intl.lang("NEXT") || 'next'
                    }
                };
                ac.done(data);
            });
        }
    };
}, '0.0.1', {requires: [
    'mojito-intl-addon',
    'mojito-models-addon',
    'mojito-params-addon',
    'mojito-url-addon',
    'mojito-util',
    'pagedflickr-model'
], lang: ['de', 'en-US']});
