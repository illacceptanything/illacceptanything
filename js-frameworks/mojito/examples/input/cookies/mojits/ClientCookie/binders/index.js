/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('clientcookie-binder', function(Y) {

/**
 * The clientcookie-binder module.
 *
 * @module clientcookie-binder
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
            var self = this;
            mojitProxy.invoke('setCookies', function(err) {
                if (err) {
                    self.node.one('p').set('innerHTML', '<h1>COOKIE FAILURE</h1>' +
                            '<p>Go find Matt and tell him this:</p>' +
                            '<p>' + err.message + '</p>' +
                            '<p>' + err.stack + '</p>');
                } else {
                    self.node.one('p').set('innerHTML', '<h1>I JUST SET SOME COOKIES!!!</h1>' +
                            '<p>Go <a href="/server/catch">here</a> to see them.</p>');
                }
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

}, '0.0.1', {requires: []});
