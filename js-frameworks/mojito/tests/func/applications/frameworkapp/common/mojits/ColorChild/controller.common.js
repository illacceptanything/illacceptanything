/*
 * Copyright (c) 2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('ColorChild', function(Y, NAME) {

/**
 * The ColorChild module.
 *
 * @module ColorChild
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
            ac.models.get('model').getData(function (data) {
                ac.done({
                    id: ac.config.get('id'),
                    color: data.color
                });
            });
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-config-addon', 'mojito-models-addon', 'ColorChildModel']});
