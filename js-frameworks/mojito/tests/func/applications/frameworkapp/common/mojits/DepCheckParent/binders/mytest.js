/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('DepCheckParentMytest', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
            this.mp = mojitProxy;
            var self = this;
            //Y.log(this.mp);
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
