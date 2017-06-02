/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('FlickrBrowserBinder', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
            this.mojitProxy.listen('flickr-image-chosen', function(event) {
                Y.log('on flickr-image-chosen ' + event.data.id, 'debug', NAME);
                mojitProxy.broadcast('flickr-image-detail', { id: event.data.id });
            });
        },

        bind: function(node) {}

    };

}, '0.0.1', {requires: ['node']});

