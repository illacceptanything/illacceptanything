/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MojitProxyMojitMytestIndex', function(Y, NAME) {

/**
 * The MojitProxyMojitMytestIndex module.
 *
 * @module MojitProxyMojitMytestIndex
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
            value = this.mp.getFromUrl('test');
            Y.log("**********test key value*********"+value);
            self.node.one('#thisvalue').setContent(value);
            id = this.mp.getId();
            Y.log("**********id**********"+id);
            self.node.one('#thisid').setContent(id);
            var data = {
		        data: "abc"
		    }
            self.mp.render(data, "sub", function(error, markup){
		       Y.log("Here....."+markup);
        	   self.node.one("#thisdata").append(markup);
            });
        }
    };

}, '0.0.1', {requires: ['mojito-client', 'node-base']});
