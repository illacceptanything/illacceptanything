
YUI.add('youtube-tests', function (Y, NAME) {

    var suite = new YUITest.TestSuite('youtube-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({

        name: 'Youtube user tests',

        setUp: function () {
            controller = Y.mojito.controllers["youtube"];
            //Y.log("controllers", "info", NAME);
            //Y.log(Y.mojito.controllers, "info", NAME);
        },
        tearDown: function () {
            controller = null;
        },

        'test mojit 001': function () {
            var ac,
                assetsResults,
                doneResults,
                errMessage;
                modelData = [
                    {
                        title: 'sometitle',
                        id: "http://gdata.youtube.com/feeds/base/videos/1234"
                    }
                ];
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
                                Y.log("youtube: modelData", "info", NAME);
                                Y.log(modelData, "info", NAME);

                                cb(null, modelData);
                            }
                        };
                    }
                },
                done: function (data) {
                    doneResults = data;
                },
                error: function(err) {
                    errMessage = err;
                } 
            };

            A.isNotNull(controller);
            A.isFunction(controller.index, "not a function");
            controller.index(ac);
            A.isObject(doneResults);
        }
    }));
    YUITest.TestRunner.add(suite);
}, '0.0.1', {requires: ['mojito-test', 'youtube']});
