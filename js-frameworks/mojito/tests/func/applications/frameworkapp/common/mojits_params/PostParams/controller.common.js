YUI.add('PostParams', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
        index: function(ac) {
	          ac.done();
	    },
        postParams: function(ac) {
            var name = ac.params.getFromBody('name'),
                thing = ac.params.getFromBody('likes');
            ac.done({
                desc: "Here's the POST data!",
                name: name,
                thing: thing
            });
        },
        postParamsSimple: function(ac) {
            var name = ac.params.body('nameSimple'),
                thing = ac.params.body('likesSimple');
            ac.done({
                desc: "Here's the POST data!",
                name: name,
                thing: thing
            });
        }	
    };
    
}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
