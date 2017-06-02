/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('AccessGlobalMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        accessModel: function(ac) {
			ac.models.get('GlobalMojit').myGlobalModelFunction(function(data) {
				Y.log(data.some, "info");
			});
			ac.models.get('Binders').getTaco(function(data) {
				Y.log(data.message, "info");
			});
			
			Y.log("******************************" + ac.mytest.myAddonFunction(), "info");
            ac.done("I am done");
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-models-addon',
    'GlobalMojitModel',
    'mojito-mytest-addon',
    'BindersModel']});
