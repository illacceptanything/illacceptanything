/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testflickr1-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: flickr");

    suite.add(new Y.Test.Case({
        "test flickr": function() {
            Y.Assert.areEqual('Hello, world!', Y.one('#flickrtitle').get('innerHTML'));
            var imagelink = Y.all('#image a').item(1).get('href');
            Y.Assert.areEqual('http:', imagelink.match(/http:/gi));
            Y.Assert.areEqual('/static/usecase/assets', imagelink.match(/\/static\/usecase\/assets/gi));
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
