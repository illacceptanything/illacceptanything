/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickr2', function(Y, NAME) {

    var PAGESIZE = 9;

    Y.namespace('mojito.controllers')[NAME] = {

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
                Y.log("config1 from controller----"+Y.JSON.stringify(ac.config.get('config1')));
                var data = {
                    images: images,
                    date: dateString,
                    greeting: ac.intl.lang("TITLE") || 'title',
                    prev: {
                        title: ac.intl.lang("PREV") || 'prev'
                    },
                    next: {
                        url: selfUrl(ac, { page: page+1 }),
                        title: ac.intl.lang("NEXT") || 'next'
                    },
                    config1: Y.JSON.stringify(ac.config.get('config1'))
                };
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
        return ac.url.make('flickr2', 'index', params);
    }


}, '0.0.1', {requires: [
    'mojito',
    'json',
    'mojito-config-addon',
    'mojito-models-addon',
    'mojito-url-addon',
    'mojito-params-addon',
    'mojito-intl-addon',
    'mojito-util',
    'PagedFlickr2Model'], lang: ['de', 'en-US']});
