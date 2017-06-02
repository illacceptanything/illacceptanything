/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('coloredchild-binder-index', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mp) {
            this.mp = mp;
            this.myid = Y.guid();
            this.handlers = [];
            this.valueDiv;
            this._savedData;
        },

        bind: function(node) {
            this.node = node;
            var id = this.myid,
                inputEl = this.node.one('input');
                
            this.valueDiv = this.node.one('.cacheValue span');
                
            node.setStyle('color', this.mp.config.text);
            
            this.handlers.push(node.on('mouseover', function() {
                Y.log('ouch! ' + id, 'debug', NAME);
            }));
            
            this.handlers.push(node.one('.ChildRefresh').on('click', function() {
                this.mp.refreshView({
                        params: { 
                            url: {
                                background: 'olive' 
                            } 
                        } 
                    }, function(data, meta) {
                        Y.log(id + ' refreshed!', 'warn', NAME);
                    }
                );
            }, this));
            
            this.handlers.push(node.one('.cacheButton').on('click', function() {
                this._savedData = inputEl.get('value');
                this._update();
            }, this));
            
            if (this._savedData) {
                this._update();
            }
        },

        onRefreshView: function(node) {
            Y.log(this.myid + ' refreshed', 'info', NAME);
            Y.Array.each(this.handlers, function(handler) {
                handler.detach();
            });
            this.bind(node);
        },

        onPause: function() {
            Y.log(this.myid + ' paused', 'info', NAME);
        },

        onResume: function() {
            Y.log(this.myid + ' resumed', 'info', NAME);
        },
        
        destroy: function() {
            console.error(this.myid + ' destroyed!');
        },
        
        _update: function() {
            this.valueDiv.setContent(this._savedData);
        }

    };

}, '0.0.1', {requires: ['node', 'mojito-client']});
