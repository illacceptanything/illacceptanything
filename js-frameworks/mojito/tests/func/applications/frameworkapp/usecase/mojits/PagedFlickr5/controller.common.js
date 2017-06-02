/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickr5', function(Y, NAME) {

/**
 * The PagedFlickr5 module.
 *
 * @module PagedFlickri5
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

            ac.models.get('model').getFlickrImages('mojito', start, PAGESIZE, function(images) {

                var dateString = ac.intl.formatDate(new Date());
                var data = {
                    date: dateString,
                    greeting: ac.intl.lang("TITLE"),
                    prev: {
                        title: ac.intl.lang("PREV")
                    },
                    next: {
                        url: selfUrl(ac, { page: page+1 }),
                        title: ac.intl.lang("NEXT")
                    }
                };

                Y.Array.each(images, function(image) {
                    image.detail_url = selfUrl(ac, { image: image.id });
                }, this);
                data.images = images;

                if (page > 1) {
                    data.prev.url = selfUrl(ac, { page: page-1 });
                    data.has_prev = true;
                }
                ac.done(data);

            });
        }
    };

    function selfUrl(ac, mods) {
        var params = Y.mojito.util.copy(ac.params.getFromMerged());
        for (var k in mods) {
            params[k] = mods[k];
        }
        return ac.url.make('flickr5', 'index', params);
    }

}, '0.0.1', {requires: [
    'mojito',
    'mojito-params-addon',
    'mojito-models-addon',
    'mojito-url-addon',
    'mojito-intl-addon',
    'mojito-util',
    'PagedFlickr5Model'], lang: ['de', 'en-US']});
