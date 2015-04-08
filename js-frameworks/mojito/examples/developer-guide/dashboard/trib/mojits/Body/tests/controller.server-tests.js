
YUI.add('body-tests', function (Y) {

    var suite = new YUITest.TestSuite('body-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({

        name: 'body user tests',

        setUp: function () {
            controller = Y.mojito.controllers.body;
        },
        tearDown: function () {
            controller = null;
        },

        'test body mojit ': function () {
            var ac,
                modelData,
                assetsResults,
                doneResults,
                route_param,
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
                        return 'mojito';
                    }
                },
                models: {
                    get: function (modelName) {
                        return model;
                    }
                },
                composite: {
                    done: function (data) {
                        doneResults = data;
                    }
                }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isNotNull(route_param);


        } ,

        'test body mojit 002': function () {
            var ac,
                modelData,
                assetsResults,
                doneResults,
                route_param,
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
                        return 'yui';
                    }
                },
                models: {
                    get: function (modelName) {
                        return model;
                    }
                },
                composite: {
                    done: function (data) {
                        doneResults = data;
                    }
                }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isNotNull(route_param);
            A.isObject(doneResults);

        }

    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'body']});
