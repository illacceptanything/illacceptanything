/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('Coverage', function(Y, NAME) {

/**
 * The Coverage module.
 *
 * @module Coverage
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
            var fs = require('fs');
	    	var mojitoVer = Y.mojito.version;
	    	var applicationName = ac.params.getFromUrl('application');
	    	
	    	var dir_structure = "/tmp/coverage/mojito/" + mojitoVer + "/data/" + applicationName;
	    	var fileName = "data.json";
	    	var coverageData = dir_structure + "/" + fileName;
	    	
	    	var exec = require('child_process').exec;
	        child = exec("mkdir -p " + dir_structure, function (error, stdout, stderr) {
	        	  response = stdout;
	        	  errorStr = stderr;
	        	  Y.log('This is the stderr: ' + stderr, "INFO");
	        	  if (error !== null) {
	        	      Y.log('exec error: ' + error);
	        	  }
	          	var coverageResult = _yuitest_coverage;
	        	var jsonData = JSON.stringify(coverageResult);
	        	fs.writeFile(coverageData, jsonData, function (err) {
	        	  if (err) throw err;
	        	  Y.log('File is saved!' + coverageData, "INFO");
	        	});
	        	ac.done("Executed the coverage mojit for " + applicationName + " application");
	        });
        }
    };

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
