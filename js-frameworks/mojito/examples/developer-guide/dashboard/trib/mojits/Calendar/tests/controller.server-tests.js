
YUI.add('calendar-tests', function (Y, NAME) {

    var suite = new YUITest.TestSuite('calendar-tests'),
        controller = null,
        A = YUITest.Assert;

    suite.add(new YUITest.TestCase({

        name: 'calendar user tests',

        setUp: function () {
            controller = Y.mojito.controllers["calendar"];
        },
        tearDown: function () {
            controller = null;
        },

        'test mojit': function () {
            var ac,
                modelData,
                assetsResults,
                doneResults;
            modelData = { x: 'y', error: { "description": "Error" }};
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = css;
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('calendar', modelName, 'wrong model name');
                        return {
                            getData: function (params, cb) {
                                Y.log("controller test for calendar", "info", NAME)
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
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.areSame('./index.css', assetsResults);
            A.isObject(doneResults);
            //A.areSame('Mojito is working.', doneResults.status);
            //A.isObject(doneResults.data);
            //A.isTrue(doneResults.data.hasOwnProperty('x'));
            //A.areEqual('y', doneResults.data['x']);
        }
    }));
    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'calendar']});
