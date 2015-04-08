/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('DepCheckModel', function(Y) {

/**
 * The DepCheckParentModel module.
 *
 * @module DepCheckParentModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models.DepCheckModel = {

        init: function(config) {
            this.config = config;
        },

        getData: function(callback) {
            var input = ["attic", "Aardvark", "1", "0", "Zoo", "zebra"];
                output = input.sort(Y.ArraySort.compare);
            Y.log("*****output*****"+output);
            callback(output);
        }
    };

}, '0.0.1', {requires: ['mojito', 'arraysort']});
