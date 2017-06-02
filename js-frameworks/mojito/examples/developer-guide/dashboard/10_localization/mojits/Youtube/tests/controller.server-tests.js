
YUI.add('youtube-tests', function (Y) {

    var suite = new YUITest.TestSuite('youtube-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({

        name: 'youtube user tests',

        setUp: function () {
            controller = Y.mojito.controllers.youtube;
            //Y.log(Y.mojito.controllers);
        },
        tearDown: function () {
            controller = null;
        },

        'test mojit 001': function () {
            var ac,
                modelData,
                assetsResults,
                doneResults;
            modelData = { x: 'y' };
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = css;
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('youtube', modelName, 'wrong model name');
                        return {
                            getData: function (params, cb) {
                                cb(null, modelData);
                            }
                        };
                    }
                },
                done: function (data) {
                    doneResults = data;
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index, "not a function");
            controller.index(ac);
        }
    }));
    YUITest.TestRunner.add(suite);
}, '0.0.1', {requires: ['mojito-test', 'youtube']});
