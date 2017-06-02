YUI.add('github-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite('github-tests'),
        controller = null,
        model = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({
        
        name: 'Github user tests',
        
        setUp: function() {
            controller = Y.mojito.controllers["github"];
            model = Y.mojito.models["github-model"];
        },
        tearDown: function() {
            controller = null;
        },
        
        'test mojit': function() {
            var ac,
                modelData,
                assetsResults,
                doneResults;
            ac = {
                assets: {
                    addCss: function(css) {
                        assetsResults = css;
                    }
                },
                config: {
                    get: function(key) {
                       def_value = key;
                    },
                    getDefinition: function (key) {
                        def_value = key;
                    }
                },
                models: {
                    get: function(modelName) {
                        A.areEqual('model', modelName, 'wrong model name');
                        return {
                            getData: function(err, cb) {
                                cb(null, modelData);
                            }
                        }
                    }
                },
                done: function(data) {
                    console.log(data);
                    doneResults = data;
                }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.areSame('./index.css', assetsResults);
            A.isObject(doneResults);
        }
    }));
    YUITest.TestRunner.add(suite);
}, '0.0.1', {requires: ['mojito-test', 'github', 'github-model']});
