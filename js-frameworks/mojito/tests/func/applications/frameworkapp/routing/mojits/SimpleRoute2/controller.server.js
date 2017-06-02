/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SimpleRoute2', function(Y, NAME) {

/**
 * The SimpleRoute2 module.
 *
 * @module SimpleRoute2
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
            ac.done({displaytext: 'This is another simple mojit for testing routing - ' + type + " (" + id + ")"});
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-type-addon',
    'mojito-http-addon']});
