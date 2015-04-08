/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testpreinit-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: preinit");

    suite.add(new Y.Test.Case({
        "test preinit": function() {
            if (ARROW.testParams["testName"] === "Page1") {
                Y.Assert.areEqual('Enjoy your Flickr Images!',Y.one('h2').get('innerHTML').match(/Enjoy your Flickr Images!/gi));
                var imagelink = Y.all('a').item(1).get('href');
                Y.Assert.areEqual('http:',imagelink.match(/http:/gi));
                Y.Assert.areEqual('/static/usecase/assets',imagelink.match(/\/static\/usecase\/assets/gi));
                Y.Assert.areEqual('page=2', Y.one('#paginate a').get('href').match(/page=2/gi));
                Y.Assert.areEqual('next', Y.one('#paginate a').get('innerHTML'));
            } else {
                Y.Assert.areEqual('Hallo!',Y.one('h2').get('innerHTML').match(/Hallo!/gi));
                Y.Assert.areEqual('zur',Y.one('#paginate a').get('innerHTML').match(/zur/gi));
                Y.Assert.areEqual('weiter',Y.all('#paginate a').item(1).get('innerHTML').match(/weiter/gi));
            }
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
