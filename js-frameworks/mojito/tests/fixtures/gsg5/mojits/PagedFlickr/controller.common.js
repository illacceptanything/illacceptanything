/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickr', function(Y, NAME) {

/**
 * The PagedFlickr module.
 *
 * @module PagedFlickr
 */

    var PAGESIZE = 6;

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac) {

            Y.log(ac.params.getAll());

            var page = ac.params.getFromMerged('page'),
                start;

            // a little paranoia about inputs
            page = parseInt(page, 10);
            if ((!page) || (page < 1)) {
                page = 1;
            }

            // The "page" parameter is base-1, but the model's "start"
            // parameter is base-0.
            start = (page-1) * PAGESIZE;

            ac.models.get('flickr').getFlickrImages('mojito', start, PAGESIZE, function(err, images) {
               var dateString, data;

                // on model error, fail fast
                if (err) {
                    return ac.error(err);
                }

                dateString = ac.intl.formatDate(new Date());
                data = {
                    date: dateString,
                    greeting: ac.intl.lang("TITLE"),
                    prev: {
                        url: selfUrl(ac, 'flickr', { page: page-1 } ),
                        title: ac.intl.lang("PREV") || 'prev'
                    },
                    next: {
                        url: selfUrl(ac, 'flickr', { page: page+1 } ),
                        title: ac.intl.lang("NEXT") || 'next'
                    }
                };

                Y.Array.each(images, function(image) {
                    image.detail_url = selfUrl(ac, 'flickr', { image: image.id });
                }, this);
                data.images = images;

                if (page > 1) {
                    data.prev.url = selfUrl(ac, 'flickr', { page: page-1 });
                    data.has_prev = true;
                }
                ac.done(data);

            });
        }
    };

    function selfUrl(ac, mojitType, mods) {
        var params = Y.mojito.util.copy(ac.params.getFromMerged());
        for (var k in mods) {
            params[k] = mods[k];
        }
        return ac.url.make(mojitType, 'index', params);
    }

}, '0.0.1', {requires: [
    'mojito-intl-addon',
    'mojito-params-addon',
    'mojito-url-addon',
    'mojito-util',
    'ModelFlickr'
], lang: ['de', 'en-US']});
