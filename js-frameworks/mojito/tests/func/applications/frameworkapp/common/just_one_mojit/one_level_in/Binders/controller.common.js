/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('Binders', function(Y, NAME) {

/**
 * The Binders module.
 *
 * @module Binders
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
            ac.data.set('id', ac.config.get('id'));
            ac.done();
        },
        myIndex: function(ac) {
            //Y.log("In the myIndex");
            var model = ac.models.get('model');
            ac.data.set('id', ac.config.get('id'));
            ac.data.set('config_data', ac.config.get('config_data'));
            model.getData(function(data) {
                model.getTaco(function(taco) {
                    data.version = ac.config.get('version');
                    //console.log("VERSION: " + data.version);
                    data.extra = ac.config.get('extra');
                    data.taco = taco;
                    ac.done(data);
                 });
             });
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-models-addon',
    'mojito-data-addon',
    'BindersModel']});
