/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('main', function (Y, NAME) {

    /**
    * The main module.
    *
    * @module main
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
    * @param actionContext {Object} The action context that provides access
    *        to the Mojito API.
    */
        index: function (actionContext) {
            actionContext.done({title: "HTML Frame Configuration Example"});
        }

    };
}, '0.0.1', {requires: ['mojito']});
