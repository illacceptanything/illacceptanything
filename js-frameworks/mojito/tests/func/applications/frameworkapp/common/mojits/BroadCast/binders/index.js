/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('BroadCastBinderIndex', function(Y, NAME) {

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
            //Y.log(this.mp);
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
            self.node.one('#staticButton').on('click', function() {
                //Y.log("******************firstcall");
                self.mp.invoke('static',{},    
				function(error, result, meta) {
                    //Y.log("Here....."+result+error);
                	Y.Node.one('#BroadCastResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#dynamicButton').on('click', function() {
                //Y.log("******************firstcall");
                self.mp.invoke('dyno',{},    
				function(error, result){
				    //Y.log("Here....."+result+error);
                    Y.Node.one('#BroadCastResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#destroychildButton').on('click', function() {
                self.mp.invoke('destroychild',{},    
				function(error, result){
				    Y.log("Here222222....."+result+error);
				    //self.mp.destroyChildren();
                    Y.Node.one('#BroadCastResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#destroydynochildButton').on('click', function() {
                Y.log("******************firstcall22222");
                self.mp.invoke('destroydynochild',{},    
				function(error, result){
				    Y.log("Here222222....."+result+error);
                    Y.Node.one('#BroadCastResult').set('innerHTML', result);
                });
			}, this);
        },

    };

}, '0.0.1', {requires: ['mojito-client']});
