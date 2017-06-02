/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('RESTLibBinderIndex', function(Y, NAME) {

/**
 * The RESTLibBinder module.
 *
 * @module RESTLibBinder
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
        	this.node = node;
        	var hostname = this.node.one('#hostname').get('innerHTML');
            this.node.one('#p_simpleWS').on('click', function(){
            	var url = "http://" + hostname + "/restWS/simpleWS"; 
                var params = {};
                var config = {
                    timeout: 5000,
                    headers: {
                        'Cache-Control': 'max-age=0'
                    }
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                    if (err) 
                    {
                    	//console.log("this is error: " + err);
                    	var newNode = Y.Node.create(err);
                    	node.one('#p_simpleWS').replace(newNode);
                    }
                    else
                    {
                    	//console.log("this is response: " + response.getBody());
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_simpleWS').replace(newNode);
                    }
                });
            }, this);
            
            this.node.one('#p_inspectResp').on('click', function(){
            	var url = "http://" + hostname + "/restWS/simpleWS"; 
                var params = {};
                var config = {
                    timeout: 5000,
                    headers: {
                        'Cache-Control': 'max-age=0'
                    }
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                    if (err) 
                    {
                    	//console.log("this is error: " + err);
                    	var newNode = Y.Node.create(err);
                    	node.one('#p_inspectResp').replace(newNode);
                    }
                    else
                    {
                    	//console.log("this is response: " + response.getHeaders());
                    	var myResponse = "<div id=\"\" class=\"mojit\">\n" + 
                    	"<p>Below are the fragments of the response:\n" + 
                    	"<ul>\n" + 
                    		"<li id=\"code\">Status Code: "+ response.getStatusCode() +"</li>\n" + 
                    		"<li id=\"msg\">Status Message: "+ response.getStatusMessage() +"</li>\n" + 
                    		"<li id=\"headers\">Headers: "+ response.getHeaders() +"</li>\n" + 
                            "<li id=\"specific_header\">Specific Header: "+ response.getHeader('Content-Type') +"</li>\n" + 
                    	"</ul>\n" + 
                    	"</p>\n" + 
                    	response.getBody() + 
                    "</div>";
                    	
                    	
                    	var newNode = Y.Node.create(myResponse);
                    	node.one('#p_inspectResp').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_wsTimeout').on('click', function(){
            	var url = "http://" + hostname + "/rest/myws"; 
                var params = {};
                var config = {
                    timeout: 1000,
                    headers: {
                        'Cache-Control': 'max-age=0'
                    }
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                    if (err) {
                        //console.log("This is the error: " + err.status)
                    	var newNode = Y.Node.create(err);
                    	node.one('#p_wsTimeout').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_wsTimeout').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_inspectErr').on('click', function(){
            	var url = "http://" + hostname + "/invalidURL";
                var params = {};
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                    if (err) {
                        //console.log("*********************This is the error text: ");
                        //console.log(err.responseText);
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_inspectErr').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_inspectErr').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_getParam').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printGETParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_getParam').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_getParam').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_getParamNegative').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printGETParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.POST(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_getParamNegative').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_getParamNegative').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_postParam').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printPOSTParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.POST(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_postParam').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_postParam').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_postParamNegative').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printPOSTParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_postParamNegative').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_postParamNegative').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_putParam').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printPUTParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.PUT(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_putParam').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_putParam').replace(newNode);
                    }
                });
            }, this);
            this.node.one('#p_deleteParam').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printDELETEParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.DELETE(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_deleteParam').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_deleteParam').replace(newNode);
                    }
                });
            }, this);
            /*this.node.one('#p_headParam').on('click', function(){
            	var url = "http://" + hostname + "/restWS/printHEADParams";
                var params = {
                    project: "Mojito",
                    sprint: "4"
                };
                var config = {
                    timeout: 5000,
                    headers: {}
                };
                
                Y.mojito.lib.REST.HEAD(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_headParam').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create("");
                    	node.one('#p_headParam').replace(newNode);
                    }
                });
            }, this);*/
            this.node.one('#p_headers').on('click', function(){
            	var url = "http://" + hostname + "/restWS/getParticularHeader";
                var params = {};
                var config = {
                    timeout: 5000,
                    headers: {
                        "myHeader": "somevalue",
                        "connection":"keep-alive",
                        "Keep-Alive": 200
                    }
                };
                
                Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                	if (err) {
                    	var newNode = Y.Node.create(err.responseText);
                    	node.one('#p_headers').replace(newNode);
                    }
                    else
                    {
                    	var newNode = Y.Node.create(response.getBody());
                    	node.one('#p_headers').replace(newNode);
                    }
                });
            }, this);
        }
    };

}, '0.0.1', {requires: ['mojito-rest-lib']});
