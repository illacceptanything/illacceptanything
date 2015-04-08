/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MojitProxyMojitBinderIndex', function(Y, NAME) {

/**
 * The MojitProxyMojitBinderIndex module.
 *
 * @module MojitProxyMojitBinderIndex
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
            self.node.one('#mojitProxyMojitButton').on('click', function() {
                Y.log("******************firstcall");
                self.mp.invoke('mytest',{},    
				function(error, result){
				    Y.log("Here..result..."+result);
				    Y.log("Here..error..."+error);
                	Y.Node.one('#MojitProxyMojitResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#mojitProxyRefreshButton').on('click', function() {
                Y.log("******************firstcall");
                self.mp.invoke('refreshtest', function(error, result){
				    Y.log("Here....."+result+error);
                	Y.Node.one('#MojitProxyMojitResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#mojitProxyPauseResumeButton').on('click', function() {
                Y.log("******************firstcall");
                self.mp.invoke('pauseresumetest', function(error, result){
				    Y.log("Here....."+result+error);
                	Y.Node.one('#MojitProxyMojitResult').set('innerHTML', result);
                });
			}, this);
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
