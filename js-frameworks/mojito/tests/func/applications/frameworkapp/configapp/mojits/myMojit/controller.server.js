/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('myMojit', function(Y, NAME) {

/**
 * The TestMojit module.
 *
 * @module TestMojit
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
            var data = {
                myenv:ac.config.get('myenv', '[myenv not found]'),
                mysubject:ac.config.get('mysubject', '[mysubject not found]'),
                mylang: ac.config.get('mylang', '[mylang not found]')
            }
            ac.done(data);
        }

    };

}, '0.0.1', {requires: ['mojito','mojito-config-addon']});
