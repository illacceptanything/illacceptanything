YUI.add('github-tests', function (Y) {

    var suite = new YUITest.TestSuite('github-tests'),
        controller = null,
        A = YUITest.Assert,
        config_def = null,
        model;

    suite.add(new YUITest.TestCase({

        name: 'github user tests',

        setUp: function () {
            controller = Y.mojito.controllers["github"];
            model = Y.mojito.models["stats-model-yql"];
            config_def = {
                "yui": {
                    "title" : "YUI GitHub Activity",
                    "id": "yui",
                    "repo": "yui3"
                },
                "mojito": {
                    "title" : "Mojito GitHub Activity",
                    "id": "yahoo",
                    "repo": "mojito"
                }
             };
        },
        tearDown: function () {
            controller = null;
        },
        'test mojit': function () {
            var ac,
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
                        return config_def[key];
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
                        return {
                            getData: function (params, tablePath, id, repo, cb) {
                                return { 
                                    onDataReturn: function (cb, data) {
                                        cb(data);
                                    }
                                }; 
                            }
                        };
                    }
                },
                done: function (data) {
                    console.log(data);
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
