/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('MetaChildBinderIndex', function(Y, NAME) {

/**
 * The StatefulBinderIndex module.
 *
 * @module StatefulBinderIndex
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
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var self = this;
            //var resultNode = node.one('div');
            self.node = node;
            self.node.one('.storeButton').on('click', function() {
                var metainput = self.node.one('input').get('value');
                self.mp.invoke('store', {
                    params: {
                        url: {
                            metainput: metainput
                        }
                    }
                }, function(err, data) {
                    if (err) {
                        Y.Node.one('#ControllerCachingResult').set('innerHTML', 'error');
                    } else {
                        Y.Node.one('#ControllerCachingResult').set('innerHTML', 'stored: '+ metainput);
                    }
                });
            }, this);
        }
    };

}, '0.0.1', {requires: ['mojito-client']});
