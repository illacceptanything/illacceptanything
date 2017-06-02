/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testhello-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: hello");

    suite.add(new Y.Test.Case({
        "test hello": function() {
            Y.Assert.areEqual('Mojito is working.', Y.one('pre').get('innerHTML'));
        }
    }));    

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});

