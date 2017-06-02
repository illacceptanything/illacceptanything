/*
 * Copyright (c) 2012 Yahoo! Inc. All rights reserved.
 */
/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('RefreshParentBinderIndex', function(Y, NAME) {

/**
 * The parentBinderIndex module.
 *
 * @module parentBinderIndex
 */

    /**
     * Constructor for the parentBinderIndex class.
     *
     * @class parentBinderIndex
     * @constructor
     */
    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var me = this;
            this.node = node;
            this.handle = me.node.one('#refreshViewButton').on('click', function(){
                Y.log("***********************I am in the click function");
                me.mojitProxy.refreshView();
            }, this);
        },
        onRefreshView: function() {
            this.handle.detach();
            this.bind.apply(this, arguments);
        }

    };

}, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});
