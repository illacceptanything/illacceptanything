/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('AppLevelMojitModel', function(Y) {

    Y.mojito.models.AppLevelMojit = {

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

}, '0.0.1', {requires: ['arraysort']});
