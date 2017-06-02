/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('PartialMojitBinderIndex', function(Y, NAME) {

/**
 * The PartialMojitBinderIndex module.
 *
 * @module PartialMojitBinderIndex
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
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var self = this;
            self.node = node;
            self.node.one('#partialRenderButton').on('click', function() {
                Y.log("******************firstcall");
                self.mp.invoke('mytest',{},    
				function(error, result){
				    Y.log("HereBinder....."+result+error);
                	Y.Node.one('#PartialMojitResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#partialInvokeButton').on('click', function() {
                Y.log("******************secondcall");
                self.mp.invoke('myinvoke',{},    
				function(error, result){
				    Y.log("HereBinder...invoke.."+result+error);
                	Y.Node.one('#PartialMojitResult').set('innerHTML', result);
                });
			}, this);
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
