/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('FlickrDetail', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {

            var image = ac.params.getFromMerged('image') || '0';

            // a little paranoia about inputs
            if (!image.match(/^\d+$/)) {
                ac.assets.addCss('./message.css');
                ac.done({ type: 'error', message: ac.intl.lang('ERROR_BAD_IMAGE_ID') }, { view: { name:'message' } });
                return;
            }

            if ('0' === image) {
                ac.assets.addCss('./message.css');
                ac.done({ type: 'info', message: ac.intl.lang('INFO_NO_IMAGE_CHOSEN') }, { view: { name:'message' } });
                return;
            }

            ac.models.get('flickr').getFlickrDetail(image, function(err, details) {
                if (err) {
                    ac.error(new Error("YQL Error"));
                    return;
                }
                //Y.log(details);
                details.intl = {
                    DATE_POSTED:    ac.intl.lang('DATE_POSTED'),
                    TITLE:          ac.intl.lang('TITLE'),
                    TITLE_NONE:     ac.intl.lang('TITLE_NONE'),
                    DESCRIPTION:    ac.intl.lang('DESCRIPTION'),
                    DESCRIPTION_NONE: ac.intl.lang('DESCRIPTION_NONE'),
                    OWNER_USERNAME: ac.intl.lang('OWNER_USERNAME'),
                    TAGS:           ac.intl.lang('TAGS'),
                    TAGS_NONE:      ac.intl.lang('TAGS_NONE'),
                    URLS:           ac.intl.lang('URLS'),
                    URL_PHOTO_PAGE: ac.intl.lang('URL_PHOTO_PAGE'),
                    URL_IMAGE:      ac.intl.lang('URL_IMAGE')
                };
                details.intl.posted = ac.intl.formatDate(new Date(1000 * Number(details.dates.posted)));

                // The mustache library we're using is a little finicky.
                if (details.title) {
                    details.have_title = true;
                }
                if (details.description) {
                    details.have_description = true;
                }

                ac.assets.addCss('./index.css');
                ac.done(details);
            });
        }

    };

}, '0.0.1', {requires: ['mojito-intl-addon', 'mojito-assets-addon', 'mojito-params-addon', 'mojito-models-addon', 'ModelFlickr'], lang: ['de', 'en-US']});
