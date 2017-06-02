YUI.add('github-tests', function (Y) {

    var suite = new YUITest.TestSuite('github-tests'),
        controller = null,
        A = YUITest.Assert,
        model;

    suite.add(new YUITest.TestCase({

        name: 'Github user tests',

        setUp: function () {
            controller = Y.mojito.controllers["github"];
            model = Y.mojito.models["stats-model-yql"];
        },
        tearDown: function () {
            controller = null;
        },
        'test mojit': function () {
            var ac,
                modelData,
                assetsResults,
                route_param,
                doneResults,
                def_value;
            modelData = { x: 'y' };
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
                        A.areEqual('yql', modelName, 'wrong model name');
                        return model;
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

}, '0.0.1', {requires: ['mojito-test', 'github', 'stats-model-yql']});
