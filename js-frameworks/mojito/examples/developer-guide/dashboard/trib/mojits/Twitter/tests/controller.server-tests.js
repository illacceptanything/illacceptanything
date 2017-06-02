
YUI.add('twitter-tests', function (Y, NAME) {

    var suite = new YUITest.TestSuite('twitter-tests'),
        controller = null,
        A = YUITest.Assert,
        model;

    suite.add(new YUITest.TestCase({

        name: 'twitter user tests',
        setUp: function () {
            controller = Y.mojito.controllers["twitter"];
            model = Y.mojito.models["twittersearch-model"];
        },
        tearDown: function () {
            controller = null;
        },
        'test mojit twitter 001': function () {
            var ac,
                assetsResults,
                def_value,
                route_param,
                doneResults,
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
                        return "yui";
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('twitter', modelName, 'wrong model name');
                        return model;
                    }
                },
                done: function (data) {
                    doneResults = data;
                }
            };
            console.log(assetsResults);
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);

            A.areSame('./index.css', assetsResults);
            A.isObject(doneResults);
        },
        'test mojit twitter 002': function () {
            var ac,
                assetsResults,
                def_value,
                route_param,
                doneResults,
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
                        return "mojito";
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('twitter', modelName, 'wrong model name');
                        return model;
                    }
                },
                done: function (data) {
                    doneResults = data;
                }
            };
            console.log(assetsResults);
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);

            A.areSame('./index.css', assetsResults);
            A.isObject(doneResults);
        }
    }));
    YUITest.TestRunner.add(suite);
}, '0.0.1', {requires: ['mojito-test', 'twitter', 'twittersearch-model']});
