/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MojitProxyMojitRefreshtestIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
            this.mp = mojitProxy;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var self = this;
            self.node = node;
            this.handle = self.node.one('input').on('click', function(){
                Y.log("I am in the click function");
                self.mp.refreshView();
            }, this);
        },

        onRefreshView: function() {
            this.handle.detach();
            this.bind.apply(this, arguments);
        }
    };

}, '0.0.1', {requires: ['mojito-client', 'node-base']});
