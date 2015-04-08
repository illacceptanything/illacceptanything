/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('parent-binder-static', function(Y, NAME) {

/**
 * The parent-binder-static module.
 *
 * @module parent-binder-static
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
            Y.log(this.mp);
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
            this.mp.listen('hover', function(data) {
                Y.log('heard hover from ' + data.source);
            });
        },

        handleClick: function(evt) {
            var event, message, child, self = this;
            if (evt.currentTarget.get('tagName') === 'BUTTON') {
                event = this.node.one('#event').get('value');
                message = this.node.one('#message').get('value');
                child = this.node.one('#child').get('value');
                this.mp.broadcast(event, {message: message}, {target: {slot: child}});
            }
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
