/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MojitProxyMojit', function(Y, NAME) {

/**
 * The MojitProxyMojit module.
 *
 * @module MojitProxyMojit
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
        'index': function(ac) {
            ac.done();
        },
        
        'refreshtest': function(ac) {
            ac.done({title: 'Testing ac.refreshView'})
        },
        
        'pauseresumetest': function(ac) {
            ac.done({title: 'Testing ac.pause And ac.resume'})
        },
        
        'mytest': function(ac) {
            ac.error(new Error("something is not right"));
            ac.done();
        }
    };

}, '0.0.1', {requires: ['mojito']});
