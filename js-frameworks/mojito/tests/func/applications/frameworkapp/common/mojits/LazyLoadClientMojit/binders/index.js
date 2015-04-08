YUI.add('LazyLoadClientMojitBinderIndex', function(Y, NAME) {

/**
 * The LazyLoadClientMojitBinderIndex module.
 *
 * @module LazyLoadClientMojitBinderIndex
 */

    /**
     * Constructor for the LazyLoadClientMojitBinderIndex class.
     *
     * @class LazyLoadClientMojitBinderIndex
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
            self.node.one('#lazyloadclientmojitButton').on('click', function() {
                self.mojitProxy.data.on('change', function(){
                     count++;
                });
                self.mojitProxy.data.set('fooc', "fooc-value set by binder");
                self.mojitProxy.invoke('lazyloadclientmojit', {}, 
                    function(err, result) {
                        if (err || result.indexOf('Error') >= 0) {
                            Y.Node.one('#LazyLoadClientMojitResult').set('innerHTML', '<div id="LazyLoadClientMojitTitle">Lazy load failed:' + err + "/div>");
                        } else {
                            Y.Node.one('#LazyLoadClientMojitResult').set('innerHTML', '<div id="LazyLoadClientMojittitle">Lazy load succeeded:</div>' 
                               + '<div id="LazyLoadClientMojitfoo">'+ self.mojitProxy.data.get('fooc') + '</div>'
                               + '<div id="LazyLoadClientMojitbar">'+ self.mojitProxy.data.get('barc') + '</div>'
                               + '<div id="LazyLoadClientMojitbaz">'+ self.mojitProxy.data.get('bazc') + '</div>'
                               + '<div id="LazyLoadClientMojitcount">'+ 'Data has changed: '+count+' times</div>');
                        }
                });
            }, this);
        }

    };

}, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});
