/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testpagedflickr-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: pagedflickr");

    suite.add(new Y.Test.Case({
        "test pagedflickr": function() {
            Y.Assert.areEqual('Enjoy your Flickr Images!',Y.one('h2').get('innerHTML').match(/Enjoy your Flickr Images!/gi));
            var imagelink = Y.all('a').item(1).get('href');
            Y.Assert.areEqual('http:',imagelink.match(/http:/gi));
            Y.Assert.areEqual('/static/usecase/assets',imagelink.match(/\/static\/usecase\/assets/gi));
            Y.Assert.areEqual('page=2', Y.one('#paginate a').get('href').match(/page=2/gi));
            Y.Assert.areEqual('next', Y.one('#paginate a').get('innerHTML'));
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
