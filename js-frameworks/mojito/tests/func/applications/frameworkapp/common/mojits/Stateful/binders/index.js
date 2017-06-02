/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('StatefulBinderIndex', function(Y, NAME) {

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
            self.node.one('.pitchButton').on('click', function() {
                var ball = self.node.one('input').get('value');
                self.mp.invoke('pitch', {
                    params: {
                        url: {
                            ball: ball
                        }
                    }
                }, function(err, data) {
                    if (err) {
                        Y.Node.one('#ControllerCachingResult').set('innerHTML', 'error');
                    } else {
                        Y.Node.one('#ControllerCachingResult').set('innerHTML', 'pitched: '+ball);
                    }
                });
            }, this);
            node.one('.catchButton').on('click', function() {
                self.mp.invoke('catch', {}, function(err, data) {
                    if (err) {
                         Y.Node.one('#ControllerCachingResult').set('innerHTML', 'error');
                    } else {
                        Y.Node.one('#ControllerCachingResult').set('innerHTML', 'ball: ' + data.ball);
                    }
                });
            }, this);

        },

        handleClick: function(evt) {
            var ball;
            if (evt.currentTarget.tagName === 'BUTTON') {
                ball = this.node.one('input').get('value');
                this.mp.invoke('pitch', {
                    params: {
                        url: {
                            ball: ball
                        }
                    }
                }, function(err, data) {

                });
            }
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
