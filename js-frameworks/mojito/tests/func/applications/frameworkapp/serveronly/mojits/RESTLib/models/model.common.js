/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('RESTLibModel', function(Y, NAME) {

/**
 * The RESTLibModel module.
 *
 * @module RESTLibModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    Y.mojito.models[NAME] = {

        init: function(mojitSpec) {
            this.spec = mojitSpec;
        },

        /**
         * Method that will be invoked by the mojit controller to obtain data.
         *
         * @param callback {Function} The callback function to call when the
         *        data has been retrieved.
         */
        callWSGET: function(hostport, cb) {
            var url = "http://" + hostport + "/restWS/simpleWS"; 
            var params = {};
            var config = {
                timeout: 5000,
                headers: {
                    'Cache-Control': 'max-age=0'
                }
            };

            Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                if (err) {
                    cb(err);
                    return;
                }
                cb(null, response);
            });
        },

        callTimeoutWS: function(hostport, cb){
            var url = "http://" + hostport + "/rest/myws"; 
            var params = {};
            var config = {
                timeout: 1000,
                headers: {
                    'Cache-Control': 'max-age=0'
                }
            };

            Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        callInvalidWS: function(hostport, cb){
            var url = "http://" + hostport + "/invalidURL";
            var params = {};
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        wsWithGETParams: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printGETParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        wsWithGETParamsNeg: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printGETParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.POST(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        wsWithPOSTParams: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printPOSTParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.POST(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        wsWithPOSTParamsNeg: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printPOSTParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        wsWithPUTParams: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printPUTParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.PUT(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response);
            });
        },

        wsWithDELETEParams: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printDELETEParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.DELETE(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response);
            });
        },

        wsWithHEAD: function(hostport, sprintNum, cb){
            var url = "http://" + hostport + "/restWS/printHEADParams";
            var params = {
                project: "Mojito",
                sprint: sprintNum
            };
            var config = {
                timeout: 5000,
                headers: {}
            };

            Y.mojito.lib.REST.HEAD(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                
                cb(null, response);
            });
        },

        wsWithHeadersSettings: function(hostport, cb){
            var url = "http://" + hostport + "/restWS/getParticularHeader";
            var params = {};
            var config = {
                timeout: 5000,
                headers: {
                    "myHeader": "somevalue",
                    "connection":"keep-alive",
                    "Keep-Alive": 200
                }
            };

            Y.mojito.lib.REST.GET(url, params, config, function(err, response) {
                if (err) {
                    return cb(err);
                }
                cb(null, response.getBody());
            });
        },

        myTimeConsumingWS: function(cb){
            sleep(3000);
            cb("I am back after sleeping for 3 seconds.");
        }
    };

    function sleep(milliseconds) {
        var start = new Date().getTime();
        var i = 0;
        for (i = 0; i < 1e7; i++) {
            if ((new Date().getTime() - start) > milliseconds){
                break;
            }
        }
    }


}, '0.0.1', {requires: ['mojito-rest-lib']});
