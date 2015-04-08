/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('parent-binder-index', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
            this.node = node;
        },
        
        handleClick: function() {
            var node = this.node;
            this.mojitProxy.invoke('index', function(err, data, meta) {
                
                node.one(".metastuff").setContent(meta.common.anal.colors.join(', '));
                
            });
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
