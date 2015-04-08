YUI.add('usecases-testpreattachedbinder-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: preattachedbinder");

    suite.add(new Y.Test.Case({
        "test preattachedbinder": function() {
            if(ARROW.testParams["testName"] === "Page1") {
                Y.Assert.areEqual('Enjoy your Flickr Images!',Y.one('h2').get('innerHTML').match(/Enjoy your Flickr Images!/gi));
                Y.Assert.areEqual('\"This is the config for config1 in application.json\"', Y.one('#myconfig').get('innerHTML'));
            } else {
                Y.Assert.areEqual('Hallo!',Y.one('h2').get('innerHTML').match(/Hallo!/gi));
                Y.Assert.areEqual('\"mynewconfig\"', Y.one('#myconfig').get('innerHTML'));
            }
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
