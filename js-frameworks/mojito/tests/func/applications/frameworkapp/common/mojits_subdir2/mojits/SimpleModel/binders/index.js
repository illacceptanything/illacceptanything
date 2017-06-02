/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('SimpleModelBinderIndex', function(Y, NAME) {

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
            self.node.one('#simpleModelButton').on('click', function() {
                self.mojitProxy.invoke('simpleModel',{},    
				function(error, result){
                	Y.Node.one('#SimpleModelResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#configModelButton').on('click', function() {
                self.mojitProxy.invoke('configModel',{},    
				function(error, result){
                	Y.Node.one('#SimpleModelResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#rpcModelButton').on('click', function() {
			    var rpcvalue = gup("ifrpc") || false;
			    isRPC = (rpcvalue === "true");
			    Y.log("Running with rpc value***********"+isRPC);
			    var myresult = "this is not from error"; 
                self.mojitProxy.invoke('rpcModel',{rpc: isRPC},   
				function(error, result){
				    if (error) {
                        Y.log("Here**********error********"+ error);
                        myresult = error;
                    }
                    Y.Node.one('#SimpleModelResult').set('innerHTML', myresult);
                });
			}, this);
		}
    };
    
    var gup = function( name ){
        name = name.replace(/[\[]/,"\\\[").replace(/[\]]/,"\\\]");
        var regexS = "[\\?&]"+name+"=([^&#]*)";
        var regex = new RegExp( regexS );
        var results = regex.exec( window.location.href );
        if( results == null )
          return "";
        else
          return results[1];
      }

}, '0.0.1', {requires: ['node']});
