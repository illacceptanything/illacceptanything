/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PostParamsBinderIndex', function(Y, NAME) {

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
            self.node.one('#postParamsButton').on('click', function() {
	   			Y.log("******************firstcall");
                self.mojitProxy.invoke('postParams',{
					params : {
                		body: {
                			name: "Nobody",
                			likes: "spinach"
                		}
                	}
				},    
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#PostParamsResult').set('innerHTML', result);
                });
			}, this);
            self.node.one('#postParamsSimpleButton').on('click', function() {
	   			Y.log("******************firstcall");
                self.mojitProxy.invoke('postParamsSimple',{
					params : {
                		body: {
                			nameSimple: "Nobody",
                			likesSimple: "spinach"
                		}
                	}
				},    
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	Y.log("******************This is the result: " + result);
                	Y.Node.one('#PostParamsResult').set('innerHTML', result);
                });
			}, this);
		}
    };

}, '0.0.1', {requires: ['node']});
