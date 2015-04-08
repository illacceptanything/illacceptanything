YUI.add('receiver-binder-index', function(Y, NAME) {
  Y.namespace('mojito.binders')[NAME] = {
    init: function(mojitProxy) {
      var self = this;
      this.mojitProxy = mojitProxy;
      this.mojitProxy.listen('broadcast-link', function(payload) {
        Y.log('Intercepted broadcast-link event: ' + payload.data.url, 'info', NAME);
        // Fire an event to the mojit to reload
        // with the correct URL
        var params = {
          url: {
            url: payload.data.url
          }
        };
        mojitProxy.invoke('show', { params: params }, function(err, markup) {
          self.node.setContent(markup);
        });
      });
    },
    /**
    * The binder method, invoked to allow the
    * mojit to attach DOM event handlers.
    * @param node {Node} The DOM node to which
    * this mojit is attached.
    **/
    bind: function(node) {
      this.node = node;
    }
  };
}, '0.0.1', {requires: ['mojito-client']});
