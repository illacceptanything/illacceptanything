/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('layout-tests', function(Y, NAME) {
    
    var suite = new YUITest.TestSuite(NAME),
        controller = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'layout user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["layout"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac, compCalled = false;
            A.isNotNull(controller);
            A.isFunction(controller.index);
            ac = {
                composite: {
                    done: function(data) {
                        compCalled = true;
                        A.isUndefined(data);
                    }
                }
            };

            console.log(ac);

            controller.index(ac);

            A.isTrue(compCalled);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'layout']});
