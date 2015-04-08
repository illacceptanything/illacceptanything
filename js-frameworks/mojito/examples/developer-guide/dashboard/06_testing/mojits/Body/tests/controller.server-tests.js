
YUI.add('body-tests', function(Y) {

    var suite = new YUITest.TestSuite('body-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({
        
        name: 'body user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["body"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac,
                doneResults;
            ac = {
                composite: {
                    done: function(data) {
                        doneResults = data;
                    }
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'body']});
