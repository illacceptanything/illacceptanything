/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('GlobalMojitModel-tests', function(Y) {
    
    var suite = new YUITest.TestSuite('GlobalMojitModel-tests'),
        model = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'GlobalMojit model user tests',
        
        setUp: function() {
            model = new Y.mojit.test.GlobalMojit.model();
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
    
}, '0.0.1', {requires: ['mojit-test', 'GlobalMojitModel']});
