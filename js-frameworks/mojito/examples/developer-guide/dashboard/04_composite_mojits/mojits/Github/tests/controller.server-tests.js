YUI.add('github-tests', function (Y) {

    var suite = new YUITest.TestSuite('github-tests'),
        controller = null,
        A = YUITest.Assert,
        model;

    suite.add(new YUITest.TestCase({

        name: 'github user tests',

        setUp: function () {
            controller = Y.mojito.controllers["github"];
            model = Y.mojito.models["github-model"];
        },
        tearDown: function () {
            controller = null;
        },
        'test mojit': function () {
            var ac,
                modelData = { watchers: 1, forks: 1},
                assetsResults,
                route_param,
                doneResults,
                def_value;
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = css;
                    }
                },
                config: {
                    getDefinition: function (key) {
                        def_value = key;
                    }
                },
                params: {
                    getFromRoute: function (param) {
                        route_param = param;
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('model', modelName, 'wrong model name');
                        return {
                            getData: function(cb) {
                                cb(modelData);
                            }
                        }
                    }
                },
                done: function (data) {
                    doneResults = data;
                }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
        }
    }));
    YUITest.TestRunner.add(suite);
}, '0.0.1', {requires: ['mojito-test', 'github', 'github-model']});
