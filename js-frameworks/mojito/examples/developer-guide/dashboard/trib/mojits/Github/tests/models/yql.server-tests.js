YUI.add('stats-model-yql-tests', function (Y, NAME) {
    var suite = new YUITest.TestSuite(NAME),
        model = null,
        A = YUITest.Assert;

suite.add(new YUITest.TestCase({
        name: "StatsModelYQL user tests",

        setUp: function () {
            model = Y.mojito.models["stats-model-yql"];
        },

        tearDown: function () {
            model = null;
        },

        'test mojit model 001': function () {
            var called = false,
                called2 = false,
                cfg = { color: 'red' };

            Y.log("github: yql.server-tests.js", "info", NAME);

            A.isNotNull(model);
            A.isFunction(model.init);
            model.init(cfg);
            A.areSame(cfg, model.config);

            // check getData function is there
            A.isFunction(model.getData, 'getData not a function');


            model.getData({}, '', '', '',function (data) {
                called = true;
                //A.isTrue(!err);
                //A.isObject(data);
                //A.areSame('data', data.some);
            });
            //A.isTrue(called);

            // check onDataReturn function is there
            A.isFunction(model.onDataReturn, 'onDataReturn not a function');

            model.onDataReturn(function(data){
                called2 = true;
            },'', {
                query:{
                    results:{
                        json:{
                            json: [{"foo":"bar"}, {"baz":"bat"}]
                        }
                    }
                }
            });
            A.isTrue(called2);

        },

       'test mojit model 002': function () {
            var called = false,
                called2 = false,
                cfg = { color: 'red' };

            Y.log("github: yql.server-tests.js", "info", NAME);

            A.isNotNull(model);
            A.isFunction(model.init);
            model.init(cfg);
            A.areSame(cfg, model.config);

            // check getData function is there
            A.isFunction(model.getData, 'getData not a function');


            model.getData({}, '', '', '',function (data) {
                called = true;
                //A.isTrue(!err);
                //A.isObject(data);
                //A.areSame('data', data.some);
            });
            //A.isTrue(called);

            // check onDataReturn function is there
            A.isFunction(model.onDataReturn, 'onDataReturn not a function');

            model.onDataReturn(function(data){
                called2 = true;
            },'', {
                error: "some error",
                query:{
                    results:{
                        json:{
                            json: [{"foo":"bar"}, {"baz":"bat"}]
                        }
                    }
                }
            });
            A.isTrue(called2);

        }



    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'stats-model-yql']});
