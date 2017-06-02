/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickrBinderIndex', function(Y, NAME) {

/**
 * The PagedFlickrBinder module.
 *
 * @module PagedFlickrBinder
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
        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            node.all('.pic a').on('click', function(evt) {
                var url = evt.currentTarget.get('href');
                Y.log('on click ' + url, 'debug', NAME);
                var matches = url.match(/image\/(\d+)/);
                var imageID = matches[1];
                if (imageID) {
                    evt.halt();

                    // Update our pagination links so when we round-trip back
                    // to the server, we persist the image choice.
                    node.all('#paginate a').each(function(pageLink) {
                        var pageUrl = pageLink.get('href');
                        pageLink.set('href', pageUrl.replace(/\/image\/\d+/, '/image/'+imageID));
                    });

                    Y.log('broadcast flickr-image-chosen ' + imageID, 'debug', NAME);
                    this.mojitProxy.broadcast('flickr-image-chosen', { id: imageID });
                }
            }, this);
        }

    };

}, '0.0.1', {requires: ['node', 'mojito-client']});
