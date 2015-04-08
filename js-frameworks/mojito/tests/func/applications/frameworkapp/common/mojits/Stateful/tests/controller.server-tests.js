/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */

YUI.add('Stateful-tests', function(Y) {

    var suite = new YUITest.TestSuite('Stateful-tests'),
        controller = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'Stateful user tests',
        
        setUp: function() {
            controller = Y.mojito.controller;
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac, results;
            A.isNotNull(controller);
            A.isFunction(controller.index);
            ac = {
                done: function(data) {
                    results = data;
                }
            };
            controller.index(ac);
            A.areSame('Mojito is working.', results);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'Stateful']});