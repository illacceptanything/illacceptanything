/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('rollups', function(Y) {

    Y.mojito.controller = {

        index: function(ac) {
            ac.done('Mojito is working.');
        }

    };

}, '0.0.1', {requires: []});
/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('rollupsBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
            this.node = node;
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('rollupsModelClient', function(Y) {

    Y.mojito.models.rollups = {

        init: function(config) {
            this.config = config;
        },

        getData: function(callback) {
            callback({some:'data'});
        }

    };

}, '0.0.1', {requires: []});
