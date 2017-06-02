/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('master-binder-index', function (Y, NAME) {

    Y.namespace("mojito.binders")[NAME] = {

        init: function (mojitProxy) {
            var mp = this.mp = this.mojitProxy = mojitProxy;
            Y.log("Entering MasterMojitBinderIndex");
            this.mojitProxy.listen('fire-link', function(payload) {
                var c = mp.getChildren(),
                    receiverID = c.receiver.viewId;
                Y.log('intercepted fire-link event: ' + payload.data.url, 'info', NAME);
                mojitProxy.broadcast('broadcast-link',
                                     {url: payload.data.url}, { target: {viewId: receiverID }});
                Y.log('broadcasted event to child mojit: ' + payload.data.url, 'info', NAME);
            });
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function (node) {
            this.node = node;
        }

    };
}, '0.0.1', {requires: ['mojito-client']});
