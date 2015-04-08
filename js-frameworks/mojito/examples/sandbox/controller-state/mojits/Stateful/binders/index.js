/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('stateful-binder-index', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mp = mojitProxy;
        },

        bind: function(node) {
            var self = this;
            var resultNode = node.one('div');
            this.node = node;

            node.one('.pitch').on('click', function() {
                var ball = this.node.one('input').get('value');
                this.mp.invoke('pitch', {
                    params: {
                        url: {
                            ball: ball
                        }
                    }
                }, function(err, data) {
                    if (err) {
                        resultNode.setContent('error');
                    } else {
                        resultNode.setContent('pitched');
                    }
                });
            }, this);

            node.one('.catch').on('click', function() {
                this.mp.invoke('catch', {}, function(err, data) {
                    if (err) {
                        resultNode.setContent('error');
                    } else {
                        resultNode.setContent('ball: ' + data.ball + ' (id: ' + data.time + ')' + ' (model: ' + data.model + ')');
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
