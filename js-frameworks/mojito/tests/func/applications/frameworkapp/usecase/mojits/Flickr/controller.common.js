/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('Flickr', function(Y, NAME) {

/**
 * The Flickr module.
 *
 * @module Flickr
 */

    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         * @method index
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         * @return {}       
         */
        index: function(ac) {
            ac.models.get('model').getFlickrImages('mojito', function(images) {
                ac.done({images: images});
            });
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-models-addon',
    'FlickrModel']});
