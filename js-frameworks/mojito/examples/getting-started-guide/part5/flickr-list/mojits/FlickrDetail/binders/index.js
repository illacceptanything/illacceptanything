/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('flickrdetail-binder-index', function (Y, NAME) {

/**
 * The flickrdetail-binder-index module.
 *
 * @module flickrdetail-binder-index
 */
    "use strict";
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
            var self = this;
            this.mojitProxy = mojitProxy;
            this.mojitProxy.listen('flickr-image-detail', function (payload) {
                Y.log('on flickr-image-detail ' + payload.data.id, 'debug', NAME);
                var urlParams = Y.mojito.util.copy(mojitProxy.context),
                    routeParams = {
                        image: payload.data.id
                    };
                mojitProxy.refreshView({
                    params: {
                        url: urlParams,
                        route: routeParams
                    }
                });
            });
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function (node) {
            this.node = node;
        }

    };

}, '@VERSION@', {requires: ['node', 'mojito-client', 'mojito-util']});
