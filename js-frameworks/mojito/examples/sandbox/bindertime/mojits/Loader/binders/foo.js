/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('loader-binder-foo', function(Y, NAME) {

/**
 * The loader-binder-foo module.
 *
 * @module loader-binder-foo
 */

    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
            Y.log(mojitProxy.id + ' initialized', 'debug', NAME);
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
            var nodeId = node.get('id');
            var binderId = this.mojitProxy._viewId;
            var instanceId = this.mojitProxy._instanceId;
            Y.log('mojit ' + instanceId + ' binder ' + binderId + ' bound to node: #' + nodeId, 'debug', NAME);
            if (nodeId !== binderId) {
                throw new Error("bad node binding to binder!");
            }
            this.node.append("<p>mojit " + instanceId + ' / ' + nodeId + " bound</p>");
        },

        _updateId: function(msg) {
            var nodeId = this.node.get('id');
            msg = msg || 'bound';
            this.node.one("p").set('innerHTML', nodeId + ' ' + msg);
        },

        handleClick: function(evt) {
            this.node.one('div').set('innerHTML', "clicked on " + new Date());
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
