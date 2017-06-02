/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MojitProxyMojitPauseResumetestIndex', function(Y, NAME) {

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
            self.node.one('#pauseButton').on('click', function(){
                Y.log("I am in the click function");
                YMojito.client.pause();
            }, this);
            self.node.one('#resumeButton').on('click', function(){
                Y.log("I am in the click function");
                YMojito.client.resume();
            }, this);
        }
    };

}, '0.0.1', {requires: ['mojito-client', 'node-base']});
