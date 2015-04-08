/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('PosterBinder', function(Y) {

/**
 * The PosterBinder module.
 *
 * @module PosterBinder
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
    var binder = {

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
        }

    };

    Y.namespace('mojito.binders.Poster');
    Y.mojito.binders.Poster.index = binder;

}, '0.0.1', {requires: []});
