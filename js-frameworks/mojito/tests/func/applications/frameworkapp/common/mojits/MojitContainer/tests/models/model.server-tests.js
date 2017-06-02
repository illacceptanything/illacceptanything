/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */

YUI.add('MojitContainerModel-tests', function(Y) {
    
    var suite = new YUITest.TestSuite('MojitContainerModel-tests'),
        model = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'MojitContainer model user tests',
        
        setUp: function() {
            model = Y.mojito.models.MojitContainer;
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
    
}, '0.0.1', {requires: ['mojito-test', 'MojitContainerModel']});
