/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */

YUI.add('StatefulModel-tests', function(Y) {
    
    var suite = new YUITest.TestSuite('StatefulModel-tests'),
        model = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'Stateful model user tests',
        
        setUp: function() {
            model = Y.mojito.models.Stateful;
        },
        tearDown: function() {
            model = null;
        },
        
        'test mojit model': function() {
            A.isNotNull(model);
            A.isFunction(model.getData);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'StatefulModel']});
