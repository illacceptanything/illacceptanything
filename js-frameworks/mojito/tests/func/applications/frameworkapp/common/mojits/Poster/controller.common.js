YUI.add('Poster', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
        
        index: function(ac) {
            ac.done({
                desc: "Submit for for example of POST processing.",
                desc1: "Submit for for example of filtered POST processing.",
                desc2: "Submit for for example of POST simple processing."
            });
        }
        
    };
    
}, '0.0.1', {requires: ['mojito']});
