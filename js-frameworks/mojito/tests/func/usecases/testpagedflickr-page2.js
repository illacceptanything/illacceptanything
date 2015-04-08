/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testpagedflickr-page2-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: pagedflickr-page2");

    suite.add(new Y.Test.Case({
        "test pagedflickr-page2": function() {
            Y.Assert.areEqual('Enjoy your Flickr Images!', Y.one('h2').get('innerHTML').match(/Enjoy your Flickr Images!/gi));
            Y.Assert.areEqual("previous", Y.all('#paginate a').item(0).get('innerHTML'));
            Y.Assert.areEqual("next", Y.all('#paginate a').item(1).get('innerHTML'));
            Y.Assert.areEqual("page=1", Y.all('#paginate a').item(0).get('href').match(/page=1/gi));
            Y.Assert.areEqual("page=3", Y.all('#paginate a').item(1).get('href').match(/page=3/gi));
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
