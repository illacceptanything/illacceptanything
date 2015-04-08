/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('clicklog-binder-index', function(Y, NAME) {

/**
 * The clicklog-binder-index index binder.
 * This binder looks for clicks on the clicker and uses mojitProxy.render() to
 * create UI about each click
 *
 * @module clicklog-binder-index
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
            this.clicks = 0;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var self = this;
            this.node = node;
            this.node.one('.clicker a').on('click', function(evt) {
                self.clicks++;
                evt.halt();
                var data = {
                    num: self.clicks,
                    date: new Date()
                }
                self.mojitProxy.render(data, 'entry', function(err, markup) {
                    self.node.one(".log").append(markup);
                });
            });
        }

    };

}, '0.0.1', {requires: ['mojito-client', 'node-base']});
