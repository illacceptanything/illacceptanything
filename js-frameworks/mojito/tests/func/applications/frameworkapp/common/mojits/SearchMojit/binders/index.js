/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SearchMojitBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
            this.node = node;
            this.submitHandler = node.one('form').on('submit', this._onSubmit, this);
        },
        
        _onSubmit: function (e) {
            e.preventDefault();
            
            var node = this.node,
                args = { 
                    params: { 
                        url: { 
                            query: node.one("input[name='query']").get('value'),
                            zip:   node.one("input[name='zip']").get('value')
                        }
                    }
                };
            
            node.one('.results').replace('<div class="results">Searching...</div>');
            
            this.mojitProxy.refreshView(args, function () {
                Y.log('*****************The view has been refreshed!');
            });
        },
        
        onRefreshView: function (node) {
            this.submitHandler.detach();
            this.bind(node);
        }

    };

}, '0.0.1', {requires: ['mojito-client', 'node']});
