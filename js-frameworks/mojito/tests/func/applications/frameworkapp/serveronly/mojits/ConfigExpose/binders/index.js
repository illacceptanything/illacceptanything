/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ConfigExposeBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        }
    };

}, '0.0.1', {requires: ['mojito-client']});
