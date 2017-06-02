/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('PartialMojit', function(Y, NAME) {

/**
 * The PartialMojit module.
 *
 * @module PartialMojit
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
        'index': function(ac) {
            ac.done();
        },
        
        'mytest': function(ac) {
            var myparam = ac.params.getFromUrl("name"),
                myparamExists = myparam ? "YES" : "NO";
            //console.log("**********************myparam" + myparam);
            if( myparamExists === "NO"){
                //console.log("*********************no myparam");
                myparam = "data not from url";
            }
            var data = {
		        mydata: myparam
		    }
            /*ac.partial.render(data, "mytest", function(error, mymarkup){
		       Y.log("HereController....."+mymarkup);
        	   ac.done(mymarkup);
            });*/
            //ac.done(data);
            ac.partial.render(data, "partials/sub", function(error, mymarkup){
		       Y.log("HereController....."+mymarkup);
        	   ac.done(mymarkup);
            })
        },
        'mytest1': function(ac){
            var myparam = ac.params.getFromUrl("name");
            var data = {
		        mydata: myparam
		    }
            ac.done(data);
        },
        'myinvoke': function(ac) {
            Y.log("**********************I am in myinvoke ");
            ac.partial.invoke('mytest1',{
				params : {
            		url: {
            			name: "data from url"
            		}
            	}
			},    
			function(error, result){
				if (error) Y.log("**********************I am getting an error: " + error);
            	Y.log("******************This is the result: " + result);
            	ac.done(result);
            });
        }
    };

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon', 'mojito-partial-addon']});
