/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CM_Footer', function(Y, NAME) {

/**
 * The CM_Footer module.
 *
 * @module CM_Footer
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
            var noOfTimes = 0;

            //ac.mojit.config.runFromServer = ac.params.getFromUrl('run_from_server') || 'false';
            //console.log("*********************************This is runFromServer: " + ac.mojit.config.runFromServer);


            if (ac.params.getFromMerged('times'))
            {
                noOfTimes = ac.params.getFromMerged('times');
                noOfTimes++;
            }

            //console.log("***********************This is from parent: " + ac.params.getFromUrl('fromParent'));
            ac.done({title: 'Refresh this module', no_of_times: noOfTimes});
        }
    };

}, '0.0.1', {requires: ['mojito','mojito-params-addon']});
