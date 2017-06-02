/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ContainerBinderLazy', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
            var me = this;
            this.mp = mojitProxy;
            this.mp.listen('lazy-load', function() {
                Y.Node.one('#lazyResult').set('innerHTML', 'LAZY LOAD COMPLETE');
                me.mp.broadcast('update-style', {
                    color: 'red',
                    border: 'dashed white 30px',
                    padding: '20px'
                });
            });
            this.mp.listen('lazy-load-complete', function(evt) {
                Y.log('lazy-load-complete**************** ');
                Y.Node.one('#finalLazyResult').set('innerHTML', Y.JSON.stringify(evt.data, null, 2));
            });

        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
        },

        onChildDestroyed: function(evt) {
            Y.log('my child was destroyed: ' + evt.id, 'info', NAME);
        }

    };

}, '0.0.1', {requires: ['mojito-client', 'json']});
