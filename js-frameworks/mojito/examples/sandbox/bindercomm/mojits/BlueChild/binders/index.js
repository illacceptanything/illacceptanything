/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('bluechild-binder-index', function(Y, NAME) {

/**
 * The bluechild-binder-index module.
 *
 * @module bluechild-binder-index
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
            this.mp = mojitProxy;
            var self = this;
            this.mp.listen('notify', function(payload) {
                var data = payload.data,
                    source = payload.source,
                    msg = 'Recieved message "' + data.message + '" from ' + source;
                self.node.one('p').setContent(msg);
            });
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
