/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('i18nMojit', function(Y, NAME) {

/**
 * The i18nMojit module.
 *
 * @module i18nMojit
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
            ac.models.get('model').getFlickrImages('mojito', function(images) {
	            var dateString = ac.intl.formatDate(new Date());
	            var tempStr = ac.intl.lang("TITLE");
	            console.log("*********************TITLE: " + tempStr);
	            var data = {
	                images: images,
	                date: dateString,
	                greeting: ac.intl.lang("TITLE"),
	                order1: ac.intl.lang('chosecookie', {name: 'Mojito'}),
                    order2: ac.intl.lang('chosefruit', 'Bronx'),
                    order3: ac.intl.lang('chosecake', ['Zombie', "Earthquake"])
	            };
	            console.log("**************This is greeting: " + JSON.stringify(data));
            	ac.done(data);
			});
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-models-addon',
    'mojito-intl-addon',
    'i18nMojitModel'], lang: ['de', 'en-US']});
