/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ConfigMojit', function(Y, NAME) {

/**
 * The ConfigMojit module.
 *
 * @module ConfigMojit
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
			ac.done();
		},
        myIndex: function(ac) {
			Y.log(ac.config.get('configArray1'));
			var output = {
				
				//application.json	
				configValue: JSON.stringify(ac.config.get('config1')), //Key from app.json
				configArrayValue: JSON.stringify(ac.config.get('configArray1')), //Array from application.json
				configNested1: JSON.stringify(ac.config.get('config2.config2Key1')), //Nested config from app.json
				configNested2: JSON.stringify(ac.config.get('config2.config2Key2')), //Nested config from app.json
				configNested3: JSON.stringify(ac.config.get('config2.config2Key2.config2Key2Key1')), //Nested config from app.json
				completeConfig: JSON.stringify(ac.config.get()), //Complete config from app.json
				noMatchConfig: JSON.stringify(ac.config.get('something_unknown', 'Config not found')),
				noMatchConfigArray: ac.config.get('something_unknown', '["I", "am", "an", "array"]'),
				
				//default.json
				defaultFileValue: JSON.stringify(ac.config.get('key1')), //Key from default.json
				commonKeyValue: JSON.stringify(ac.config.get('commonKey1')), // Common key from app and default.json
				defaultArray: JSON.stringify(ac.config.get('defaultArray')), //Array
				defaultNested1: JSON.stringify(ac.config.get('nestedConfig.subConfig2')), //Nested

				
				//definition.json
				definitionValue: JSON.stringify(ac.config.getDefinition('def1')), //Key from defn.json
				defArrayValue: JSON.stringify(ac.config.getDefinition('defArray1')), //Array from defn.json
				defNested1: JSON.stringify(ac.config.getDefinition('nested.subset2')), //Nested
				defNestedArray: JSON.stringify(ac.config.getDefinition('nested.subset2.subsubsetArray')[0]),
				noMatchDefinition: JSON.stringify(ac.config.getDefinition('something_unknown', 'definition not found')),
				noMatchDefinitionArray: ac.config.getDefinition('something_unknown', '["I", "am", "an", "array"]'),
				noMatchDefinitionJson: ac.config.getDefinition('something_unknown', '{one: {two: "I am two", three: "I am three"}}')
				
				
			};
            //console.log(ac.config.get('key1'));
			ac.done(output);
        }

    };

}, '0.0.1', {requires: [ 'mojito', 'mojito-config-addon' ]});
