/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('PagedFlickr2Binder', function(Y, NAME) {
    

    function getPage(href) {
        return href.split('/').pop().split('=').pop()
    }

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
            var self = this;
            this.node = node;
            Y.log("inbinder-----"+self.mojitProxy.data.get('config1'));
            var paginator = function(evt) {

                var tgt = evt.target;
                var page = getPage(tgt.get('href'));

                evt.halt();

                self.mojitProxy.refreshView({
                    rpc: true,
                    params: {
                        route: {page: page}
                    }
                });

            };
            this.node.all('#paginate a').on('click', paginator, this);
        },

        onRefreshView: function(node) {
            node.one('#myconfig').set('innerHTML', Y.JSON.stringify(this.mojitProxy.data.get('config1')));
        }

    };

}, '0.0.1', {requires: ['mojito', 'lifecycle']});
