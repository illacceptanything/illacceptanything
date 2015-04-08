/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('i18nMojitBinder', function(Y) {

/**
 * The i18nMojitBinder module.
 *
 * @module i18nMojitBinder
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

    Y.namespace('mojito.binders.i18nMojit');
    Y.mojito.binders.i18nMojit.index = binder;

}, '0.0.1', {requires: []});
