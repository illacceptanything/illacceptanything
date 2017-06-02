
YUI.add('blog-tests', function (Y, NAME) {

    var suite = new YUITest.TestSuite('blog-tests'),
        controller = null,
        A = YUITest.Assert,
        model;

    suite.add(new YUITest.TestCase({

        name: 'blog user tests',

        setUp: function () {
            controller = Y.mojito.controllers["blog"];
            model = Y.mojito.models["blog-model-yql"];
            //Y.log("controllers", "info", NAME);
            //Y.log(Y.mojito.controllers, "info", NAME);
        },
        tearDown: function () {
            controller = null;
        },

        'test mojit yui 001': function () {
            var ac,
                modelData,
                assetsResults,
                doneResults,
                route_param,
                def_value;
            ac = {
                assets: {
                    addCss: function (css) {
                        Y.log("addCss called", "info", NAME);
                        Y.log(css, "info", NAME);
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
                        Y.log("getFromRoute called", "info", NAME)
                        route_param = param;
                        return 'yui';
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('blog', modelName, 'wrong model name');
                        Y.log("blog: ac.models.get called", "info", NAME);
                        return {
                            getData: function (params, feedURL, cb) {
                                Y.log("blog: models get getData called.", "info", NAME);
                                return {
                                    onDataReturn: function (cb, result) {
                                        Y.log("blog: in onDataReturn", "info", NAME);
                                       cb(result);
                                    }
                                }
                            }
                        };
                    }
                },
                done: function (data) {
                    doneResults = data;
                }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isNotNull(route_param);


        },

        'test mojit mojito 002' : function (){
            var ac,
                modelData,
                assetsResults,
                doneResults,
                route_param,
                def_value;

            ac = {
                assets: {
                    addCss: function (css) {
                        Y.log("addCss called", "info", NAME);
                        Y.log(css, "info", NAME);
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
                        Y.log("getFromRoute called", "info", NAME)
                        route_param = param;
                        return 'mojito';
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('blog', modelName, 'wrong model name');
                        Y.log("ac.models.get called", "info", NAME);
                        return {
                            getData: function (params, feedURL, cb) {
                                return {
                                    onDataReturn: function (cb, result) {
                                       cb(result);
                                    }
                                }
                            }
                        };
                    }
                },
                done: function (data) {
                    doneResults = data;
                }
            };
            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isNotNull(route_param);


        }

    }));
    YUITest.TestRunner.add(suite);
}, '0.0.1', {requires: ['mojito-test', 'blog', 'blog-model-yql']});
