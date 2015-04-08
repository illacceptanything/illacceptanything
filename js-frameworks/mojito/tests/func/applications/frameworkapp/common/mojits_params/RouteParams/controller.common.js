YUI.add('RouteParams', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
        index: function(ac){
		     ac.done();
		},
        routeParams: function(ac) {
            var params = ac.params.getFromRoute(),
                fookey = ac.params.getFromRoute('foo'),
                paramsArray = [];
            Y.Object.each(params, function(param, key) {
                paramsArray.push({key: key, value: param});
            });
            ac.done({
                desc: 'All route params',
                rte: paramsArray,
                fooparams: fookey
            });
        },
        routeParamsSimple: function(ac) {
            var params = ac.params.route(),
                fookey = ac.params.route('foo'),
                paramsArray = [];
            Y.Object.each(params, function(param, key) {
                paramsArray.push({key: key, value: param});
            });
            ac.done({
                desc: 'All route params',
                rte: paramsArray,
                fooparams: fookey
            });
        }
        
    };
    
}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
