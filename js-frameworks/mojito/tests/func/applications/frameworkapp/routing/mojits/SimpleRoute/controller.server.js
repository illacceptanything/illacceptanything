/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SimpleRoute', function(Y, NAME) {

/**
 * The SimpleRoute module.
 *
 * @module SimpleRoute
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
            var config = ac.config.get();
			var type = ac.type;
			var id = config.id;
			//ac.http.setHeader('content-type', 'text/html');
	        ac.done({displaytext: 'This is a simple mojit for testing routing - ' + type + " (" + id + ")"});
        },

		myAction: function(ac) {
        	//ac.http.setHeader('content-type', 'text/html');
		    ac.done({displaytext: 'myAction output - This is another action'});
		}

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-type-addon',
    'mojito-http-addon']});
