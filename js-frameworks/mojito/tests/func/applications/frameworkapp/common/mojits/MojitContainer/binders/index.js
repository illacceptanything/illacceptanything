/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MojitContainerBinderIndex', function(Y, NAME) {

/**
 * The MojitContainerBinderIndex module.
 *
 * @module MojitContainerBinderIndex
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
            self.node = node;
            self.node.one('#myMojitsButton').on('click', function() {
	   			self.mojitProxy.invoke('myMojits',{},    
				function(error, result){
				    //Y.log("Here....."+result+error);
                	Y.Node.one('#MojitContainerResult').set('innerHTML', result);
                });
			}, this);
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
