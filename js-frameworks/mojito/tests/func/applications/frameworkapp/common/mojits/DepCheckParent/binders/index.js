/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('DepCheckParentIndex', function(Y, NAME) {


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
            self.node.one('#DepCheckParentButton').on('click', function() {
                Y.log("******************firstcall");
                self.mp.invoke('mytest',{},    
				function(error, result){
				    Y.log("Here....."+result+error);
                	Y.Node.one('#DepCheckParentResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#MetaAddonButton').on('click', function() {
                Y.log("******************secondcall");
                self.mp.invoke('metachild',{},    
				function(error, result){
				    Y.log("Here....."+result+error);
                	Y.Node.one('#DepCheckParentResult').set('innerHTML', result);
                });
			}, this);
			self.node.one('#RetrieveDataButton').on('click', function() {
                Y.log("******************thirdcall");
                self.mp.invoke('retrievedata',{},    
				function(error, result){
				    Y.log("Here11111111111111111"+result+error);
                	Y.Node.one('#DepCheckParentResult').set('innerHTML', result);
                });
			}, this);
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
