
YUI.add('footer-tests', function(Y) {

    var suite = new YUITest.TestSuite('footer-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({
        
        name: 'footer user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers.footer;
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac,
                doneResults;
            ac = {
                done: function(data) {
                    doneResults = data;
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isObject(doneResults);
            A.areSame('Copyright 2013 Yahoo! Inc.', doneResults.title);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'footer']});
