YUI.add('LazyLoadRPCBinderIndex', function(Y, NAME) {

/**
 * The LazyLoadRPCBinderIndex module.
 *
 * @module LazyLoadRPCBinderIndex
 */

    /**
     * Constructor for the LazyLoadRPCBinderIndex class.
     *
     * @class LazyLoadRPCBinderIndex
     * @constructor
     */
    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            var self = this,
                count = 0;
            self.node = node;
            self.node.one('#lazyloadrpcButton').on('click', function() {
                self.mojitProxy.data.on('change', function(){
                     count++;
                });
                self.mojitProxy.data.set('foo', "foo-value set by binder");
                self.mojitProxy.invoke('lazyloadrpc', {}, 
                    function(err, result) {
                        if (err || result.indexOf('Error') >= 0) {
                            Y.Node.one('#LazyLoadRPCResult').set('innerHTML', '<div id="LazyLoadRPCTitle">Lazy load failed:' + err + "/div>");
                        } else {
                            Y.Node.one('#LazyLoadRPCResult').set('innerHTML', '<div id="LazyLoadRPCtitle">Lazy load succeeded:</div>' 
                               + '<div id="LazyLoadRPCfoo">'+ self.mojitProxy.data.get('foo') + '</div>'
                               + '<div id="LazyLoadRPCbar">'+ self.mojitProxy.data.get('bar') + '</div>'
                               + '<div id="LazyLoadRPCbaz">'+ self.mojitProxy.data.get('baz') + '</div>'
                               + '<div id="LazyLoadRPCcount">'+ 'Data has changed: '+count+' times</div>');
                        }
                });
            }, this);
        }

    };

}, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});
