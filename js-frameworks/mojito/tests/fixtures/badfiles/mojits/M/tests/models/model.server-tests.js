/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */

YUI.add('MModel-tests', function(Y) {
    
    var suite = new YUITest.TestSuite('MModel-tests'),
        model = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'M model user tests',
        
        setUp: function() {
            model = Y.mojito.models.M;
        },
        tearDown: function() {
            model = null;
        },
        
        'test mojit model': function() {
            A.isNotNull(model);
            A.isFunction(model.getMessage);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'MModel']});
