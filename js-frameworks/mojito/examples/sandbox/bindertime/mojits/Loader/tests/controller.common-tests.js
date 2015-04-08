/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('loader-tests', function(Y, NAME) {
    
    var suite = new YUITest.TestSuite(NAME),
        controller = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'loader user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["loader"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit foo': function() {

            var ac, results;

            A.isNotNull(controller);
            A.isFunction(controller.index);

            ac = {
                done: function(data) {
                    results = data;
                }
            };

            controller.foo(ac);

            A.isObject(results, 'done was not passed an object');
            A.areSame('foo', results.name, 'wrong name value');
            A.isNumber(results.time, 'time was not a number');

        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'loader']});
