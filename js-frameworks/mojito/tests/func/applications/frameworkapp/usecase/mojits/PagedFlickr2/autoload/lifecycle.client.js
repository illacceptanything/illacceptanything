YUI.add('lifecycle', function(Y) {
    Y.mojito.Client.subscribe('pre-init', function(data) {
        Y.log("********** pre-init***********");
        data.config.context.lang = 'de';
    });
    Y.mojito.Client.subscribe('post-init', function(data) {
        Y.log("********** In post-init***********");
    });
    Y.mojito.Client.subscribe('post-attach-binders', function(data) {
        Y.log("********** In post-attach-binders***********");
    });
    Y.mojito.Client.subscribe('pre-attach-binders', function(data) {
        Y.Object.each(data.binderMap, function(key) {
            Y.log("**********key[viewId]***********"+ key['viewId']);
            data.binderMap[key.viewId].data = data.binderMap[key.viewId].data || {};
            data.binderMap[key.viewId].data.config1 = "mynewconfig";
            Y.log("**********k2***********"+ data.binderMap[key.viewId].data.config1);
        });
    });
}, '0.0.1', {requires: ['mojito-client']});

