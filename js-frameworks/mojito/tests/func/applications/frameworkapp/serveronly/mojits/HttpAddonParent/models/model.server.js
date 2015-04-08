/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('HttpAddonParentModel', function(Y, NAME) {

/**
 * The HttpAddonParentModel module.
 *
 * @module HttpAddonParentModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models[NAME] = {

        init: function(mojitSpec) {
            this.spec = mojitSpec;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
    	callWS: function(hostport, isXhr, cb) {
    	    var xhrHeader = "";
    	    if (isXhr === "true")
    	    {
    	    	xhrHeader = "XMLHttpRequest";
    	    }

	        var url = "http://" + hostport + "/httpParent/checkingXhr"; 
	        var params = {};
	        var config = {
	            timeout: 5000,
	            headers: {
	                'Cache-Control': 'max-age=0',
	                'X-Requested-With': xhrHeader
	            }
	        };

	        Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
	            if (err) {
	                cb(err);
	                return;
	            }
	            cb(null, response);
	        });
    	}
    };

}, '0.0.1', {requires: ['mojito', 'mojito-rest-lib']});
