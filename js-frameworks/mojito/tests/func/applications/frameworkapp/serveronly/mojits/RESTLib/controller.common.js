/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('RESTLib', function(Y, NAME) {

/**
 * The RESTLib module.
 *
 * @module RESTLib
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
	        index: function(ac) {
		        //console.log(ac.http.getRequest());
	        	var data = {
	        		hostname: ac.http.getRequest().headers.host
	        	};
	        	ac.done(data);
        		//ac.done();
	        },

	        simpleWSCall: function(ac) {
	        	var hostPort = getHostNameAndPort(ac);
	            ac.models.get('model').callWSGET(hostPort, function(error, response){
	                if (!error)
	                {
	                    //console.log("This is the response: " + response);
	                	ac.http.setHeader('content-type', 'text/html');
	                    ac.done(response.getBody());
	                }
	                else
	                {
	                    //console.log("This is error: " + JSON.stringify(error));
	               		//ac.http.setStatusCode(error.status);
	                    ac.done(error.responseText);
	                }

	            });
	        },

	        inspectResponse: function(ac) {
	        	var hostPort = getHostNameAndPort(ac);
	            //console.log("***************************HostPort: " + hostPort);
	            ac.models.get('model').callWSGET(hostPort, function(error, response){
	                if (!error)
	                {
	                    //console.log(response.getHeaders());
	                    var responseParts = {
	                        statusCode: response.getStatusCode(),
	                        statusMsg: response.getStatusMessage(),
	                        body: response.getBody(),
	                        headers: JSON.stringify(response.getHeaders()),
	                        header_powered_by: response.getHeader('x-powered-by'),
	                        header_content_type: response.getHeader('content-type'),
	                        header_encoding: response.getHeader('transfer-encoding'),
	                        header_not_exist: response.getHeader('custom-header')
	                    };
	                    ac.done(responseParts);
	                }
	                else
	                {
	               		//ac.http.setStatusCode(error.status);
	                    ac.done(error.responseText);
	                }
	            });
	        },
	        WSTimeout: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            ac.models.get('model').callTimeoutWS(hostPort, function(error, response){
	                if (!error)
	                {
	                    ac.done(response);
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    //console.log("This is the error: " + JSON.stringify(error));
	                }

	            });
	        },

	        inspectError: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            ac.models.get('model').callInvalidWS(hostPort, function(error, response){
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    ac.done(response);
	                }
	                else
	                {
	               		//ac.http.setStatusCode(error.status);
	                	console.log(error);
	                	var err = new Error("This is my error message");
	                	err.code = 404;
	                	ac.error(err);
	                    //ac.done(error.responseText);
	                    //console.log("This is the error: " + JSON.stringify(error));
	                }

	            });
	        },

	        testGETParam: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            var sprintNumToPass = ac.params.getFromMerged("sprint_num");
	            var negativeTest = ac.params.getFromMerged("negative_test");
	            var outFunction = function(error, response)
	            {
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    ac.done(response);
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    //console.log("This is the error: " + JSON.stringify(error));
	                }
	            };

	            if (negativeTest === "true")
	            {
	                ac.models.get('model').wsWithGETParamsNeg(hostPort, sprintNumToPass, outFunction);
	            }
	            else
	            {
	                ac.models.get('model').wsWithGETParams(hostPort, sprintNumToPass, outFunction);
	            }
	        },

	        testPOSTParam: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            var sprintNumToPass = ac.params.getFromMerged("sprint_num");
	            var negativeTest = ac.params.getFromMerged("negative_test");
	            var outFunction = function(error, response)
	            {
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    ac.done(response);
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    //console.log("This is the error: " + JSON.stringify(error));
	                }
	            };

	            if (negativeTest === "true")
	            {
	                ac.models.get('model').wsWithPOSTParamsNeg(hostPort, sprintNumToPass, outFunction);
	            }
	            else
	            {
	                ac.models.get('model').wsWithPOSTParams(hostPort, sprintNumToPass, outFunction);
	            }
	        },

	        testPUTParam: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            var sprintNumToPass = ac.params.getFromMerged("sprint_num");
	            //var negativeTest = ac.params.getFromMerged("negative_test");
	            var outFunction = function(error, response)
	            {
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    var statusCode = response.getStatusCode();
	                    ac.done("<p id=\"status\">" + statusCode + "</p>" + response.getBody());
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    console.log("This is the error: " + JSON.stringify(error));
	                }
	            };

	            ac.models.get('model').wsWithPUTParams(hostPort, sprintNumToPass, outFunction);

	            /*if (negativeTest === "true")
	            {
	                ac.models.get('model').wsWithPUTParamsNeg(hostPort, sprintNumToPass, outFunction);
	            }
	            else
	            {
	                ac.models.get('model').wsWithPUTParams(hostPort, sprintNumToPass, outFunction);
	            }*/
	        },

	        testDELETEParam: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            var sprintNumToPass = ac.params.getFromMerged("sprint_num");
	            //var negativeTest = ac.params.getFromMerged("negative_test");
	            var outFunction = function(error, response)
	            {
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    var statusCode = response.getStatusCode();
	                    ac.done("<p id=\"status\">" + statusCode + "</p>" + response.getBody());
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    console.log("This is the error: " + JSON.stringify(error));
	                }
	            };

	            ac.models.get('model').wsWithDELETEParams(hostPort, sprintNumToPass, outFunction);
	        },

	        testHEAD: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            var sprintNumToPass = ac.params.getFromMerged("sprint_num");
	            var outFunction = function(error, response)
	            {
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    var statusCode = response.getStatusCode();
	                    var headerInfo = response._resp.headers.new_header;
	                    ac.done("<p id=\"status\">"+ statusCode +"</p><p id=\"header\">new_header = " + headerInfo + "</p>" + response.getBody());
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    console.log("This is the error: " + JSON.stringify(error));
	                }
	            };

	            ac.models.get('model').wsWithHEAD(hostPort, sprintNumToPass, outFunction);
	        },

	        testHeaders: function(ac)
	        {
	        	var hostPort = getHostNameAndPort(ac);
	            var outFunction = function(error, response)
	            {
	                if (!error)
	                {
	                	ac.http.setHeader('content-type', 'text/html');
	                    ac.done(response);
	                }
	                else
	                {
	                    ac.done(error.responseText);
	                    console.log("This is the error: " + JSON.stringify(error));
	                }
	            };
	            ac.models.get('model').wsWithHeadersSettings(hostPort, outFunction);

	        },

	        //My WebServices

	        simpleWS: function(ac) {
	        	//console.log("I am here");
	        	//ac.http.setHeader('content-type', 'text/html');
	        	//ac.done("<p id=\"output\">This is a very simple web service</p>");
	        	ac.done({output: "This is a very simple web service"}, {view: {name: "wsOutput"}});
	        },

	        myWS: function(ac){
	            ac.models.get('model').myTimeConsumingWS(function (output){
	            	ac.http.setHeader('content-type', 'text/html');
	                ac.done(output);
	            });
	        },

	        printGETParams: function(ac){
	            var project = ac.params.getFromUrl("project");
	            var sprint = ac.params.getFromUrl("sprint");
	            var method = ac.http.getRequest().method;

	            var output = "<p id=\"output\">(METHOD: " + method + ") This is sprint " + sprint + " for the project " + project + "</p>";
	            ac.http.setHeader('content-type', 'text/html');
	            ac.done(output);
	        },

	        printPOSTParams: function(ac){
	            var project = ac.params.getFromBody("project");
	            var sprint = ac.params.getFromBody("sprint");
	            var method = ac.http.getRequest().method;

	            var output = "<p id=\"output\">(METHOD: " + method + ") This is sprint " + sprint + " for the project " + project + "</p>";
	            ac.http.setHeader('content-type', 'text/html');
	            ac.done(output);
	        },

	        printPUTParams: function(ac){
	            var project = ac.params.getFromBody("project");
	            var sprint = ac.params.getFromBody("sprint");
	            var method = ac.http.getRequest().method;

	            var output = "<p id=\"output\">(METHOD: " + method + ") This is sprint " + sprint + " for the project " + project + "</p>";
	            ac.http.setHeader('content-type', 'text/html');
	            ac.done(output);
	        },

	        printDELETEParams: function(ac){
	            var project = ac.params.getFromMerged("project");
	            var sprint = ac.params.getFromMerged("sprint");
	            var method = ac.http.getRequest().method;

	            //var output = "<p id=\"output\">(METHOD: " + method + ") This is sprint " + sprint + " for the project " + project + "</p>";
	            var output = "<p id=\"output\">(METHOD: " + method + ")</p>";
	            ac.http.setHeader('content-type', 'text/html');
	            ac.done(output);
	        },

	        printHEADParams: function(ac){
	            /*var project = ac.params.getFromMerged("project");
	            var sprint = ac.params.getFromMerged("sprint");

	            var output = "<p id=\"output\">This is sprint " + sprint + " for the project " + project + "</p>";
	            */
	            ac.http.addHeader('new_header','dummy_value');
	            ac.done();
	        },

	        getParticularHeader: function(ac){
	            var reqObj = ac.http.getRequest();
	            var headers = reqObj.headers;
	            ac.http.setHeader('content-type', 'text/html');
	            ac.done("<p id=\"something\">" + JSON.stringify(headers) + "</p>" + "<p id=\"my_header\">" + headers.myheader + "</p>" + "<p id=\"connection\">" + headers.connection + "</p>");
	        }
	    };

	    function getHostNameAndPort(ac) {
	    	var hostPort;
	    	/*fromClient = ac.params.getFromMerged('fromClient');
	    	if (fromClient === "true")
	    	{
	    		hostPort = ac.params.getFromMerged('hostPort');
	    	}
	    	else
	    	{
	    		var reqObj = ac.http.getRequest();
	    		hostPort = reqObj.headers.host;
	    	}*/
	    	var reqObj = ac.http.getRequest();
			hostPort = reqObj.headers.host;
			
	    	return hostPort;
	    }
	
}, '0.0.1', {requires: [
    'mojito',
    'mojito-models-addon',
    'mojito-http-addon',
    'mojito-params-addon',
    'RESTLibModel']});
