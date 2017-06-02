/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SimpleRouteBinder', function(Y) {

/**
 * The SimpleRouteBinder module.
 *
 * @module SimpleRouteBinder
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
    function Binder(mojitProxy) {
        this.mojitProxy = mojitProxy;
    }

    Binder.prototype = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function() {
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

    Y.namespace('mojito.binders.SimpleRoute');
    Y.mojito.binders.SimpleRoute.index = Binder;

}, '0.0.1', {requires: []});
