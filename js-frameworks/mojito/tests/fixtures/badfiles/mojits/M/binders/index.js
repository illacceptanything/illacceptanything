/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MBinderIndex', function(Y, NAME) {

/**
 * The MBinderIndex module.
 *
 * @module MBinderIndex
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
            var self = this;
            this.node = node;
            node.on(['mouseover', 'mouseout'], function() {
                self.node.setAttribute('style', 'background-color: ' + randomColor());
            });
        }

    };

    function randomColor() {
        return '#'+Math.floor(Math.random()*16777215).toString(16);
    }

}, '0.0.1', {requires: ['mojito-client']});
