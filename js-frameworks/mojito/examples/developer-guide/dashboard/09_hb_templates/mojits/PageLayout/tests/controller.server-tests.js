
YUI.add('pagelayout-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite('pagelayout-tests'),
        controller = null,
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
                assets: {
                    addCss: function (css) {
                        assetsResults = css;
                    }
                },
                composite: {
                    done: function(data) {
                        doneResults = data;
                    }

                },
                config: {
                    getDefinition: function (key) {
                        def_value = key;
                    }
                },
                helpers: {
                    expose: function(func_name, func) {
                        func.apply(func_name);
                    }
                }, 
                params: {
                    getFromRoute: function (param) {
                        route_param = param;
                    }
                },
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
