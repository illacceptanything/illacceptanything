
YUI.add('gallery-tests', function (Y) {

    var suite = new YUITest.TestSuite('gallery-tests'),
        controller = null,
        model = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({

        name: 'gallery user tests',

        setUp: function () {
            controller = Y.mojito.controllers["gallery"];
            model = Y.mojito.models["gallery-model-yql"];
        },
        tearDown: function () {
            controller = null;
        },
        'test mojit gallery 001': function () {
            var ac,
                assetsResults,
                doneResults,
                def_value,
                route_param,
                modelData = { x: 'y' };
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = "joe";
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
                        return 'yui';
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('gallery', modelName, 'wrong model name');
                        return {
                            getData: function (params, tablePath, cb) {
                                cb(modelData);
                            }
                        };
                    }
                },
                done: function (data) {
                    doneResults = data;
                },
                error: function(data) {
                    doneResults = data;
                } 
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);

            A.areSame('joe', assetsResults);
            A.isObject(doneResults);
            A.isNotNull(route_param);
        },
        'test mojit gallery 002': function () {
            var ac,
                assetsResults,
                doneResults,
                def_value,
                route_param,
                modelData = { x: 'y' };
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = "joe";
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
                        return 'mojito';
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('gallery', modelName, 'wrong model name');
                        return {
                            getData: function (params, tablePath, cb) {
                                cb(modelData);
                            }
                        };
                    }
                },
                done: function (data) {
                    doneResults = data;
                },
                error: function (data) {
                    doneResults = data;
                },
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
        }
    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'gallery', 'gallery-model-yql']});
