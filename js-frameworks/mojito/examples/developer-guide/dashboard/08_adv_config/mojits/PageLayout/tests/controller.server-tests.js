YUI.add('pagelayout-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite('pagelayout-tests'),
        controller = null,
        route_params = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({
        
        name: 'pagelayout user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["pagelayout"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac,
                doneResults;
            ac = {
                params: {
                    getFromRoute: function (param) {
                        route_param = param;
                    }
                }, 
                composite: {
                    done: function(data) {
                        doneResults = data;
                    }
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isObject(doneResults);
            A.areSame("Trib - YUI Developer Dashboard", doneResults.title);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'pagelayout']});
