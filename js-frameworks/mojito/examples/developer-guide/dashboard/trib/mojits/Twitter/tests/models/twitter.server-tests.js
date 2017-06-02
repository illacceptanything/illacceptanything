YUI.add('twittersearch-model-tests', function (Y, NAME) {
    var suite = new YUITest.TestSuite(NAME),
        model = null,
        A = YUITest.Assert;

suite.add(new YUITest.TestCase({
        name: "twittersearch-model user tests",

        setUp: function () {
            model = Y.mojito.models["twittersearch-model"];
        },

        tearDown: function () {
            model = null;
        },

        'test mojit model 001': function () {
            var called = false,
                called2 = false,
                cfg = { color: 'red' };

            Y.log("twitter.server-tests.js");

            A.isNotNull(model);
            A.isFunction(model.init);
            model.init(cfg);
            A.areSame(cfg, model.config);

            // check getData function is there
            A.isFunction(model.getData, 'getData not a function');


            model.getData(0,'@yuilibrary',null,function (data) {
                called = true;
                //A.isTrue(!err);
                //A.isObject(data);
                //A.areSame('data', data.some);
            });
            A.isTrue(called);



        },


    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'twittersearch-model']});
