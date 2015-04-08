/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('BroadcastBinderDestroyDyno', function(Y, NAME) {

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
            this.mp = mojitProxy;
            var self = this;
            //Y.log(this.mp);
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var mp = this.mp;
            this.mp.listen('hover', function(payload) {
                var source = payload.source;
                var children = mp.getChildren();
                var child = findChild(source, children);
                var order = child.config.order;
                if(child.config.order == "1"){
                    mp.destroyChild(child.viewId);
                }
            });
        },
        
        handleClick: function(evt) {   
            var event, message, child;
            if (evt.currentTarget.get('tagName') === 'BUTTON') {
                this.mp.destroyChildren();
            }
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

}, '0.0.1', {requires: ['mojito-client']});
