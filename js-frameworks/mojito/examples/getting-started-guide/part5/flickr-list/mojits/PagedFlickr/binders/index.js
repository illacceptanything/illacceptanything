/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('pagedflickr-binder-index', function (Y, NAME) {
    "use strict";

/**
 * The pagedflickr-binder-index module.
 *
 * @module pagedflickr-binder-index
 */

    /**
     * Constructor for the Binder class.
     *
     * @param mojitProxy {Object} The proxy to allow the binder to interact
     *        with its owning mojit.
     *
     * @class Binder
     * @constructor
     */
    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function (mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function (node) {
            node.all('.pic a').on('click', function (evt) {
                var url = evt.currentTarget.get('href'),
                    matches = url.match(/image\/(\d+)/),
                    imageID = matches[1];
                Y.log('on click ' + url, 'debug', NAME);
                if (imageID) {
                    evt.halt();

                    // Update our pagination links so when we round-trip back
                    // to the server, we persist the image choice.
                    node.all('#paginate a').each(function (pageLink) {
                        var pageUrl = pageLink.get('href');
                        pageLink.set('href', pageUrl.replace(/\/image\/\d+/, '/image/' + imageID));
                    });

                    Y.log('broadcast flickr-image-chosen ' + imageID, 'debug', NAME);
                    this.mojitProxy.broadcast('flickr-image-chosen', { id: imageID });
                }
            }, this);
        }

    };

}, '0.0.1', {requires: ['node', 'mojito-client']});
