/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('TweetViewBinderIndex', function(Y, NAME) {

/**
 * The TweetViewBinderIndex module.
 *
 * @module TweetViewBinderIndex
 */

    /**
     * Constructor for the Binder class.
     *
     * @param mojitProxy {Object} The proxy to allow the binder to interact
     *        with its owning mojit.
     *
     * @class Binder
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
            this.node = node;
            var me = this,
                mp = this.mojitProxy;
            mp.listen('show-tweets', function(event) {
                me.node.setContent('Loading...');
                mp.refreshView({
                    rpc: true,
                    params: {
                        url: {
                            screenName: event.data.screenName
                        }
                    }
                })
            });
        },

        onRefreshView: function(node) {
            this.node = node;
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
