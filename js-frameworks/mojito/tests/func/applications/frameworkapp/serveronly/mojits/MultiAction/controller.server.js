/*
 * Copyright (c) 2012 Yahoo! Inc. All rights reserved.
 */
/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('MultiAction', function(Y, NAME) {

/**
 * The MultiAction module.
 *
 * @module MultiAction
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
        index: function(ac) { ac.done({data: "hello from index"}); },
        inst1: function(ac) { ac.done({data: "hello from inst1"}); },
        inst2: function(ac) { ac.done({data: "hello from inst2"}); },
    };

}, '0.0.1', {requires: ['mojito']});
