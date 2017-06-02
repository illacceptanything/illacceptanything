YUI.add('log', function(Y, NAME) {
    Y.namespace('mojito.controllers')[NAME] = {   
        index: function(ac) {
            Y.log('[CONTROLLER]: Default log-level message with date: ' + new Date(), null, NAME);
            Y.log('[CONTROLLER]: Warn message.','warn', NAME);
            var data = {
                log_config: Y.config.logLevel,
            };
            ac.done(data);
        }
    };
}, '0.0.1', { requires: ['mojito','mojito-config-addon']});
