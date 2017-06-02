/*
 * Copyright (c) 2012 Yahoo! Inc. All rights reserved.
 */
/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('RefreshParent', function(Y, NAME) {

/**
 * The parent module.
 *
 * @module parent
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
            var parentConfig = ac.config.get();
			var childrenConfig = parentConfig.children;

            var childInfo = [];

            Y.Object.each(childrenConfig, function(childSpec, childName) {
                var info = {
                    name: childName,
                    specs: JSON.stringify(childSpec)
                };
                Y.log("**************ChildName: " + childName + " and child spec: " +  JSON.stringify(childSpec));
                childInfo.push(info);
            });

            ac.composite.done({
                childData: childInfo
            });
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-composite-addon'
]});
