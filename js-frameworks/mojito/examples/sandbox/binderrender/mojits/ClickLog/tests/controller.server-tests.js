/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('clicklog-tests', function(Y) {

    var suite = new YUITest.TestSuite('clicklog-tests'),
        controller = null,
        A = YUITest.Assert;
    
    suite.add(new YUITest.TestCase({
        
        name: 'clicklog user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["clicklog"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test index': function() {
            var ac, gotDone, gotAddCss,
                expectedDone = {},
                expectedAddCss = './index.css';
            A.isNotNull(controller);
            A.isFunction(controller.index);
            ac = {
                assets: {
                    addCss: function(data) {
                        gotAddCss = data;
                    }
                },
                done: function(data) {
                    gotDone = data;
                }
            };
            controller.index(ac);
            A.areSame(Y.JSON.stringify(expectedDone), Y.JSON.stringify(gotDone));
            A.areSame(Y.JSON.stringify(expectedAddCss), Y.JSON.stringify(gotAddCss));
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'clicklog']});
