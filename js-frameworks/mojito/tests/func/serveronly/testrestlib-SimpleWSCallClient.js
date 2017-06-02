/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-restlibclient-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: RestLibClient");
    suite.add(new Y.Test.Case({
        "test RestLibClient": function() {
            Y.Assert.areEqual(Y.one('#output').get('innerHTML'), 'This is a very simple web service');
        }
    }));
    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});
