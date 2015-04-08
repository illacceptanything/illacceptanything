/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */

YUI.add('M-tests', function(Y) {

    var suite = new YUITest.TestSuite('M-tests'),
        controller = null,
        A = YUITest.Assert,
        OA = YUITest.ObjectAssert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'M user tests',
        
        setUp: function() {
            controller = Y.mojito.controller;
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac, expected, results;
            A.isNotNull(controller);
            A.isFunction(controller.index);
            ac = {
                models: {
                    'M': {
                        getMessage: function(cb) {
                            cb(null, 'Mojito is working');
                        }
                    }
                },
                done: function(data) {
                    results = data;
                }
            };
            controller.index(ac);
            expected = {
                title: 'Congrats!',
                message: 'Mojito is working.'
            };
            OA.areEqual(expected, results);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'M']});
