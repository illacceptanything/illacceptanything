/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CoverageClientBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
            var self = this;
            self.node = node;
            self.node.one('#coverage_button').on('click', function() {
                self.mojitProxy.invoke('myIndex',{},    
				function(error, result){
					if (error) Y.log("**********************I am getting an error: " + error);
                	//Y.log("******************This is the result: " + result);
                    //Y.Node.one('#Coverage_ClientResult').set('innerHTML', result);
					//collectClientCodeCoverageData
					var hostport = self.node.one('#coverage_host_details').get('value');
					Y.log("This is the hostport: " + hostport);
					var url = "http://" + hostport + "/CoverageClient/collectClientCodeCoverageData"; 
					Y.log("This is the WS to call: " + url);
					var params = {
						coverageData: result,
						application: "mojitoclient"
					};
	                var config = {
	                    timeout: 5000,
	                    headers: {
	                        'Cache-Control': 'max-age=0'
	                    }
	                };

	                Y.mojito.lib.REST.POST(url, params, config, function(err, response) {
	                    if (err) 
	                    {
	                    	Y.log("this is error: " + err);
	                    	var newNode = Y.Node.create(err);
	                    	Y.Node.one('#Coverage_ClientResult').replace(newNode);
	                    }
	                    else
	                    {
	                    	//Y.log("this is response: " + response.getBody());
	                    	//var newNode = Y.Node.create(response.getBody());
	                    	Y.Node.one('#Coverage_ClientResult').set('innerHTML', response.getBody());
	                    }
	                });
                });
			}, this);
        }
    };

}, '0.0.1', {requires: []});
