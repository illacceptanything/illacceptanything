YUI.add('log-binder-index', function(Y, NAME) {
    Y.namespace('mojito.binders')[NAME] = {
        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },
        bind: function(node) {
            Y.log("[BINDER]: Default Log level: " + Y.config.logLevel, null, NAME);
            Y.log('[BINDER]: Error log message.', "error", NAME);
            Y.one("#client_config").all("b").item(0).insert(Y.config.logLevel,"after");
            this.node = node;
        }
    };
}, '0.0.1', {requires: ['mojito-client']});
