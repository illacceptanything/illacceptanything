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
