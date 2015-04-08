/*
 * Copyright (c) 2012 Yahoo! Inc. All rights reserved.
 */
/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('page', function(Y, NAME) {

/**
 * The page module.
 *
 * @module page
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
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            
			var x,
				count = ac.params.getFromUrl('count') || 1,
                children = ac.config.get('children'),
				cfg = {
					"children" : {}
				};

            // if children passed in, then use that
            if (children != undefined) {
                cfg.children = children;
            // otherwise generate children dynamically
            } else {
                for (x=1; x <= count; x++) {
                    cfg.children['mojit' + x] = { "type" : "weather" };
                }
            }

			ac.composite.execute(cfg, function executeChildren(data, meta) {
				ac.done(Y.Object.values(data).join(''), meta);
			});

        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-composite-addon',
    'mojito-config-addon',
    'mojito-params-addon'
]});
