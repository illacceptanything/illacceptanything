/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('pagedflickr-binder-index', function (Y, NAME) {

    "use strict";
    function getPage(href) {
        return href.split('/').pop().split('=').pop();
    }
    Y.namespace('mojito.binders')[NAME] = {

        init: function (mojitProxy) {
            this.mojitProxy = mojitProxy;
        },
        bind: function (node) {
            this.node = node;
            var self = this,
                paginator = function (evt) {
                    var tgt = evt.target,
                        page = getPage(tgt.get('href'));

                    evt.halt();
                    self.mojitProxy.refreshView({
                        rpc: true,
                        params: {
                            route: {page: page}
                        }
                    });
                };
            this.node.all('#paginate a').on('click', paginator, this);
        }
    };
}, '0.0.1', {requires: []});
