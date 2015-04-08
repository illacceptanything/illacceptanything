/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('i18nMojitModel-tests', function(Y, NAME) {
    
    var suite = new YUITest.TestSuite(NAME),
        model = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'i18nMojit model user tests',
        
        setUp: function() {
            model = new Y.mojit.test.i18nMojit.model();
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
    
}, '0.0.1', {requires: ['mojit-test', 'i18nMojitModel']});
