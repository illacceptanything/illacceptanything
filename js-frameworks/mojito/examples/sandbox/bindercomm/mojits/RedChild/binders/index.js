/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('redchild-binder-index', function(Y, NAME) {

/**
 * The redchild-binder-index module.
 *
 * @module redchild-binder-index
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
            this.mp = mojitProxy;
            var self = this;
            this.mp.listen('notify', function(payload) {
                var data = payload.data,
                    source = payload.source,
                    msg = 'Recieved message "' + data.message + '" from ' + source;
                self.node.one('p').setContent(msg);
            });
            this.mp.listen('explode', function(payload) {
                self.node.one('p').setContent('☣☢✺ ASPLODE! ✺☢☣');
            });
            this.mp.listen('recover', function(payload) {
                self.node.one('p').setContent('<span style="font-size:40pt">✌☺</span>');
            });
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var mp = this.mp;
            this.node = node;
            node.on('mouseover', function() {
                node.addClass('hover');
                mp.broadcast('hover');
            });
            node.on('mouseout', function() {
                node.removeClass('hover');
            });
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
