/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('BindersModel', function(Y, NAME) {

/**
 * The BindersModel module.
 *
 * @module BindersModel
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
	    getData: function(callback) {

	        callback({
	            message: 'Hello, chicken.',
	            imageurl: randomChicken()
	        });

	    },

	    getTaco: function(callback) {
	        callback({message: 'taco'});
	    }

    };

	function randomChicken() {

	    var chickens = [
	        '/static/Binders/assets/BanffPark.jpg',
            '/static/Binders/assets/Calgary.jpg',
            '/static/Binders/assets/JasperPark.jpg',
            '/static/Binders/assets/RockMountain.jpg'
	    ];

	    return chickens[Math.floor(Math.random()*10)];
	}

}, '0.0.1', {requires: ['mojito']});
