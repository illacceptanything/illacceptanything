YUI.add('LazyLoadRPC', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        
        index: function(ac) {
            ac.done();
        },

        lazyloadrpc: function(ac) {
            var foo = ac.data.get('foo');
            ac.data.set('bar', "bar-value set by controller");
            ac.data.set('baz', 'From controller: ' + foo);
            ac.done();
        }
    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-data-addon']});
