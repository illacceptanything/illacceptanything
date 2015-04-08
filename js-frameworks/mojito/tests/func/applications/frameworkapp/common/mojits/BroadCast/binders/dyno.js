/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ParentBinderDyno', function(Y, NAME) {

/**
 * The ParentBinderIndex module.
 *
 * @module ParentBinderIndex
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
            var mp = this.mp = mojitProxy;
            this.mp.listen('hover', function(payload) {
                var source = payload.source;
                var children = mp.getChildren();
                var child = findChild(source, children);
                var order = child.config.order;
                var surrounding = findChildrenAround(order, children);
                var i;
                for (i=0; i<surrounding.length; i++) {
                    mp.broadcast('explode',
                            {order: order},
                            {target: {viewId: surrounding[i].viewId}});
                }
                mp.broadcast('recover', {}, {target: {viewId: child.viewId}});
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
        }

    };

    function findChild(guid, children) {
        var n;
        for (n in children) {
            if (children[n].viewId === guid) {
                return children[n];
            }
        }
    }

    function findChildrenAround(order, children) {
        var n, c, out = [];
        for (n in children) {
            c = children[n];
            if (c.config.order === (order + 1) || c.config.order === (order - 1)) {
                out.push(c);
            }
        }
        return out;
    }
}, '0.0.1', {requires: ['mojito-client']});
