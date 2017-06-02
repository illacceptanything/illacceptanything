/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('container-binder-index', function(Y, NAME) {

/**
 * The container-binder-index module.
 *
 * @module container-binder-index
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
            var me = this;
            this.mp = mojitProxy;
            this.mp.listen('lazy-load', function() {
                me.node.append('<p>LAZY LOAD COMPLETE</p>');
                me.mp.broadcast('update-style', {
                    background: 'red',
                    border: 'dashed white 30px',
                    padding: '20px'
                });
            });
            this.mp.listen('lazy-load-complete', function(evt) {
                me.node.append('<p>' + Y.JSON.stringify(evt.data, null, 2) + '</p>');
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
        },

        onChildDestroyed: function(evt) {
            Y.log('my child was destroyed: ' + evt.id, 'info', NAME);
        }

    };

}, '0.0.1', {requires: ['mojito-client', 'json']});
