/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickrBinder', function(Y, NAME) {
    

    function Binder(mojitProxy) {
        this.mojitProxy = mojitProxy;
    }
    
    Binder.prototype = {
        
        init: function() {},

        bind: function(node) {
            this.node = node;
            var paginator = function(evt) {
                var tgt = evt.target;
                var page = getPage(tgt.get('href'));
                
                var updateDOM = function(markup) {
                    this.node.set('innerHTML', markup);
                    this.node.all('#paginate a').on('click', paginator, this);
                };

                evt.halt();
                
                this.mojitProxy.invoke('index', {
                    params: {
                        url: {
                            page: page
                        }
                    },
                    callback: updateDOM,
                    scope: this
                });
                
            };
            this.node.all('#paginate a').on('click', paginator, this);
        }
        
    };

    function getPage(href) {
        var pathParts = href.split('/');
        return pathParts[pathParts.length-1];
    }

    Y.namespace('mojito.binders.PagedFlickr');
    Y.mojito.binders.PagedFlickr.index = Binder;
    
}, '0.0.1', {requires: ['mojito']});
