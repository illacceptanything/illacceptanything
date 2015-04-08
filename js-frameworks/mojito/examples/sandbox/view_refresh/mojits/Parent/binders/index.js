/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('ParentBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
            this.myid = Y.guid();
        },

        bind: function(node) {
            var mp = this.mojitProxy;
            var id = this.myid;
            this.node = node;
            this.buttonClickHandler = node.one('#' + mp._viewId + '_ParentRefresh').on('click', function() {
//                mp.refreshView();
                mp.refreshView(function(data, meta) {
                    Y.log('refresh complete', 'warn', NAME);
                });
            });
            this.destroyHandler = node.one('#' + mp._viewId + '_destroyButton').on('click', function() {
                var childId = this.node.one('#' + mp._viewId + '_destroyInput').get('value');
                mp.destroyChild(childId);
            }, this);
            this.moHandler = node.one('h3').on('mouseover', function() {
                Y.log('parent: ' + id, 'info', NAME);
            });
        },

        onRefreshView: function(node, element) {
            Y.log(this.myid + ' refreshed', 'info', NAME);
            this.buttonClickHandler.detach();
            this.destroyHandler.detach();
            this.moHandler.detach();
            this.bind(node, element);
        },

        destroy: function() {
            console.error(this.myid + ' destroyed!');
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
