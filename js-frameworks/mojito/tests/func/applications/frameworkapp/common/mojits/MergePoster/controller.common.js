YUI.add('MergePoster', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
        
        index: function(ac) {
            ac.done({
                desc: "Submit for for POST processing.",
                desc1:"Submit for POST simple processing"});
        }
        
    };
    
}, '0.0.1', {requires: ['mojito']});
