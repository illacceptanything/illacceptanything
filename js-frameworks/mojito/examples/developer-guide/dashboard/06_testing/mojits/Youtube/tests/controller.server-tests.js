
YUI.add('youtube-tests', function(Y) {

    var suite = new YUITest.TestSuite('youtube-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({
        
        name: 'youtube user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["youtube"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac,
                modelData,
                assetsResults,
                doneResults;
            modelData = { x:'y' };
            ac = {
                models: {
                    get: function(modelName) {
                        A.areEqual('youtube', modelName, 'wrong model name');
                        return {
                            getData: function(config, cb) {
                                cb(null, modelData);
                            }
                        }
                    }
                },
                done: function(data) {
                    doneResults = data;
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isObject(doneResults);
        }
        
    }));
    
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojito-test', 'youtube']});
