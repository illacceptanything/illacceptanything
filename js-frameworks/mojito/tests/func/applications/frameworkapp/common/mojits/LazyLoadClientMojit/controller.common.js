YUI.add('LazyLoadClientMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        
        index: function(ac) {
            ac.done();
        },

        lazyloadclientmojit: function(ac) {
            var foo = ac.data.get('fooc');
            ac.data.set('barc', "barc-value set by controller");
            ac.data.set('bazc', 'From controller: ' + foo);
            ac.done();
        }
    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-data-addon']});
