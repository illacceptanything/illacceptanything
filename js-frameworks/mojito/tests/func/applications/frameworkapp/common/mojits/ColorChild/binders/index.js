/*
 * Copyright (c) 2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('ColorChildBinderIndex', function(Y, NAME) {

/**
 * The ColorChildBinderIndex module.
 *
 * @module ColorChildBinderIndex
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
                self.node.one('p').setContent('ASPLODE!');
            });
            this.mp.listen('recover', function(payload) {
                self.node.one('p').setContent('<span style="font-size:20pt">bunny</span>');
            });
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
            var mp = this.mp;
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
