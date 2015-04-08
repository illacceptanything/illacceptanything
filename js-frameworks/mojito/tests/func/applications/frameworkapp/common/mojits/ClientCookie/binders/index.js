/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ClientCookieBinderIndex', function(Y, NAME) {

/**
 * The ClientCookieBinder module.
 *
 * @module ClientCookieBinder
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
            self.node.one('#setClientCookieButton').on('click', function() {
                self.mojitProxy.invoke('setCookies',{},    
				function(error, result){
                	Y.Node.one('#ClientCookieResult').set('innerHTML', result);
                }, this);
            });
            self.node.one('#getClientCookieButton').on('click', function() {
                self.mojitProxy.invoke('catch',{},    
				function(error, result){
				    if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#ClientCookieResult').set('innerHTML', result);
                });
            }, this);
        }

    };

}, '0.0.1', {requires: []});
