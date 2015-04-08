/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CookiesBinderIndex', function(Y, NAME) {

/**
 * The CookiesBinderIndex module.
 *
 * @module CookiesBinderIndex
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
            this.node = node;
            var self = this;
            var mojitProxy = this.mojitProxy;

            node.all('#async a').on('click', function(event) {

                Y.log('An async link is clicked', 'debug', NAME);

                event.halt();

                this.mojitProxy.broadcast('showCookies', {});
                
            }, this);

            this.mojitProxy.listen('showCookies', function(payload) {

                mojitProxy.invoke('showCookies', {
                    params: {},
                    scope: this
                }, function(err, markup) {
                    if (err) {
                        Y.log(err, 'error', NAME);
                    } else {
                        self.node.replace(markup);
                    }
                });                
            });

         }

    };

}, '0.0.1', {requires: ['mojito-client']});
