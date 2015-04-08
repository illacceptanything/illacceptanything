
YUI.add('github-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite('github-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({
        
        name: 'github user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["github"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac,
                modelData,
                assetsResults,
                qs_params,
                doneResults;
            modelData = { x:'y' };
            ac = {
                assets: {
                    addCss: function(css) {
                        assetsResults = css;
                    }
                },
                models: {
                    get: function(modelName) {
                        A.areEqual('model', modelName, 'wrong model name');
                        return {
                            getData: function(cb) {
                                cb(null, modelData);
                            }
                        }
                    }
                },
                done: function(data) {
                    doneResults = data;
                },
                params: {
                    getFromUrl: function(qs) {
                        qs_params = qs;
                    }
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isObject(doneResults);
            Y.log(doneResults.github.x, "info", NAME);
            A.isTrue(doneResults.github.hasOwnProperty('x'));
            A.areSame("y", doneResults.github.x);
            A.isObject(doneResults);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'github']});
