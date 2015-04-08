/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-inspecterrorclient-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: inspectErrorClient");
    suite.add(new Y.Test.Case({
        "test inspectErrorClient": function() {
            Y.Assert.areEqual('Cannot GET \/invalidURL', Y.one('#td_inspectErr').get('innerHTML').match(/Cannot GET \/invalidURL/gi));
        }
    }));
    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});
