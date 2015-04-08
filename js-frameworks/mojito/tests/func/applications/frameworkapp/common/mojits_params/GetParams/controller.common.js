YUI.add('GetParams', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
      index: function(ac){
	     ac.done();
	  },  
      allParams: function(ac) {
            var params = ac.params.getFromUrl(),
                paramsArray = [];
            Y.Object.each(params, function(param, key) {
                paramsArray.push({key: key, value: param});
            });
            ac.done({
                desc: 'All params',
                get: paramsArray
            });
        },

        paramsByValue: function(ac) {
            var fooVal = ac.params.getFromUrl('foo'),
                existsString = fooVal ? "YES" : "NO";
            ac.done({
                desc: 'Params by key',
                exists: existsString
            });
        },
        
        allParamsSimple: function(ac) {
            var params = ac.params.url(),
                    paramsArray = [];
                Y.Object.each(params, function(param, key) {
                    paramsArray.push({key: key, value: param});
                });
                ac.done({
                    desc: 'All params',
                    get: paramsArray
                });
            },

        paramsByValueSimple: function(ac) {
            var fooVal = ac.params.url('foo'),
                    existsString = fooVal ? "YES" : "NO";
                ac.done({
                    desc: 'Params by key',
                    exists: existsString
                });
        }
    };   
}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
