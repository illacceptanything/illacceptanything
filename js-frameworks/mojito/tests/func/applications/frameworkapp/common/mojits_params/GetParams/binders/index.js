/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('GetParamsBinderIndex', function(Y, NAME) {

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
            self.node.one('#allParamsButton').on('click', function() {
	   			Y.log("******************firstcall");
                self.mojitProxy.invoke('allParams',{
					params : {
                		url: {
                			foo: "123",
                			bar: "thisbar"
                		}
                	}
				},    
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#GetParamsResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#paramsByValueButton').on('click', function() {
				Y.log("******************secondcall");
                self.mojitProxy.invoke('paramsByValue',{
					params : {
                		url: {
                			foo: "notme"
                		}
                	}
				},
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#GetParamsResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#allParamsSimpleButton').on('click', function() {
	   			Y.log("******************firstcall");
                self.mojitProxy.invoke('allParamsSimple',{
					params : {
                		url: {
                			foo: "123",
                			bar: "thisbar"
                		}
                	}
				},    
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#GetParamsResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#paramsByValueSimpleButton').on('click', function() {
				Y.log("******************secondcall");
                self.mojitProxy.invoke('paramsByValueSimple',{
					params : {
                		url: {
                			foo: "notme"
                		}
                	}
				},
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#GetParamsResult').set('innerHTML', result);
                });
			}, this);
		}
    };

}, '0.0.1', {requires: ['node']});
