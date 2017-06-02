/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CM_LayoutBinderIndex', function(Y, NAME) {

/**
 * The CM_LayoutBinderIndex module.
 *
 * @module CM_LayoutBinderIndex
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
            self.node.one('#composite_mojit_button').on('click', function() {
                self.mojitProxy.invoke('myIndex',{},    
				function(error, result){
                	//if (error) console.log("**********************I am getting an error: " + error);
                	//Y.log("******************This is the result: " + result);
                    Y.Node.one('#CompositeMojitResult').set('innerHTML', result);
                });
			}, this);			
        }
    };

}, '0.0.1', {requires: ['node']});
