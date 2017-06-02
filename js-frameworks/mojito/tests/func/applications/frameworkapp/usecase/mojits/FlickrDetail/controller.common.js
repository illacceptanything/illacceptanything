/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('FlickrDetail', function(Y, NAME) {

/**
 * The FlickrDetail module.
 *
 * @module FlickrDetail
 */

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
            var image = ac.params.getFromMerged('image') || '0';

            // a little paranoia about inputs
            if (!image.match(/^\d+$/)) {
                ac.done({ type: 'error', message: ac.intl.lang('ERROR_BAD_IMAGE_ID') }, { name:'message' });
                return;
            }

            if ('0' === image) {
                ac.done({ type: 'info', message: ac.intl.lang('INFO_NO_IMAGE_CHOSEN') }, { name:'message' });
                return;
            }

            ac.models.get('model').getFlickrDetail(image, function(details) {
                if (!details) {
                    ac.done({ type: 'error', message: ac.intl.lang('ERROR_NO_DETAILS') }, { name:'message' });
                    return;
                }
                Y.log(details);
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
                details.title = details.title || false;
                if (details.title) {
                    details.have_title = true;
                }
                details.description = details.description || false;
                if (details.description) {
                    details.have_description = true;
                }
                details.tags = details.tags || false;

                ac.done(details);
            });
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-params-addon',
    'mojito-models-addon',
    'mojito-intl-addon',
    'FlickrDetailModelFlickr'], lang: ['de', 'en-US']});
