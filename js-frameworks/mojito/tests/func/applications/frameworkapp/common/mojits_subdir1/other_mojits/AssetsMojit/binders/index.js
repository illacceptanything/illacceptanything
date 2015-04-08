/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('AssetsMojitBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
			var self = this;
            self.node = node;
            self.node.one('#assets_loc_button').on('click', function() {
                self.mojitProxy.invoke('assetsWithLocation',{},    
				function(error, result){
                	//if (error) Y.log("**********************I am getting an error: " + error);
                	//Y.log("******************This is the result: " + result);
                    Y.Node.one('#AssetsResult').set('innerHTML', result);
                });
			}, this);	
            self.node.one('#assets_default_button').on('click', function() {
                self.mojitProxy.invoke('assetsByDefault',{},    
				function(error, result){
                	//if (error) Y.log("**********************I am getting an error: " + error);
                	//Y.log("******************This is the result: " + result);
                    Y.Node.one('#AssetsResult').set('innerHTML', result);
                });
			}, this);	
			self.node.one('#assets_img_button').on('click', function() {
                self.mojitProxy.invoke('assetsImg',{},    
				function(error, result){
                	//if (error) Y.log("**********************I am getting an error: " + error);
                	//Y.log("******************This is the result: " + result);
                    Y.Node.one('#AssetsResult').set('innerHTML', result);
                });
			}, this);
        }

    };

}, '0.0.1', {requires: []});
