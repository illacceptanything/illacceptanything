YUI.add('MergeParams', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
      	  index: function(ac) {
	          ac.done();
	      },  
	      mergeParams: function(ac) {
	            var merged = ac.params.getFromMerged(),
	                get = ac.params.getFromUrl(),
	                post = ac.params.getFromBody(),
	                route = ac.params.getFromRoute(),
	                all = ac.params.getAll();

	                dataOut = {
	                    desc: "Merged Parameters Test",
	                    name: merged.name,
	                    thing: merged.likes,
	                    paramGroup: [
	                        {
	                            desc: "merged params",
	                            params: paramArrayBuilder(merged)
	                        },
	                        {
	                            desc: "get params",
	                            params: paramArrayBuilder(get)
	                        },
	                        {
	                            desc: "post params",
	                            params: paramArrayBuilder(post)
	                        },
	                        {
	                            desc: "route params",
	                            params: paramArrayBuilder(route)
	                        }
	                    ],
                        allparams: {
                            desc: "all params",
                            params: paramArrayBuilder1(all)
                        }
	                };

	            ac.done(dataOut);

	        },
	        mergeParamsSimple: function(ac) {
    	            var merged = ac.params.merged(),
    	                get = ac.params.url(),
    	                post = ac.params.body(),
    	                route = ac.params.route(),
    	                all = ac.params.all();

    	                dataOut = {
    	                    desc: "Merged Parameters Test",
    	                    name: merged.name,
    	                    thing: merged.likes,
    	                    paramGroup: [
    	                        {
    	                            desc: "merged params",
    	                            params: paramArrayBuilder(merged)
    	                        },
    	                        {
    	                            desc: "get params",
    	                            params: paramArrayBuilder(get)
    	                        },
    	                        {
    	                            desc: "post params",
    	                            params: paramArrayBuilder(post)
    	                        },
    	                        {
    	                            desc: "route params",
    	                            params: paramArrayBuilder(route)
    	                        }
    	                    ],
                            allparams: {
                                desc: "all params",
                                params: paramArrayBuilder1(all)
                            }
    	                };

    	            ac.done(dataOut);

    	        }
	        
    };

    function paramArrayBuilder(params) {
        var paramsArray = [];
        Y.Object.each(params, function(param, key) {
            paramsArray.push({key: key, value: param});
        });
        return paramsArray;
    }
    
    function paramArrayBuilder1(params) {
        var paramsArray = [];
        Y.Object.each(params, function(param, key) {
            //Y.log("***param***"+param+"***key***"+key);
            Y.Object.each(param, function(subparam, subkey) {
                //Y.log("---subparam---"+subparam+"---subkey---"+subkey);
                paramsArray.push({key: key, subkey: subkey, subparam: subparam});
            });
        });
        return paramsArray;
    }

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
