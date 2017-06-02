/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('BroadcastBinderDestroy', function(Y, NAME) {

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
            this.node = node;
            //var binderId = this.mp._viewId;
            //Y.log("**********binderId*********"+binderId);
            //var instanceId = this.mp._instanceId;
            //Y.log("**********instanceId*********"+instanceId);
            //this.mp.getChildre();
            this.mp.destroyChild("devil");
            //this.mp.getChildren();
        },

        handleClick: function(evt) {   
            var event, message, child;
            if (evt.currentTarget.get('tagName') === 'BUTTON') {
                event = this.node.one('#event').get('value');
                Y.log("**********Devent value*********"+event);
                message = this.node.one('#message').get('value');
                Y.log("**********Dmessage value*********"+message);
                child = this.node.one('#child').get('value');
                Y.log("**********Dchild value*********"+child);
                this.mp.destroyChild(child);
                if(child === 'all'){
                    Y.log("**********sending to all*********");
                    this.mp.broadcast(event, {message: message});
                }else{
                    Y.log("**********sending to *********" + child);
                    this.mp.broadcast(event, {message: message}, {target: {slot: child}});
                }
                //this.mp.destroyChild(child);
            }
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
