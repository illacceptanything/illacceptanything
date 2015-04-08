/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-deletewithparamsclient-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: DELETEWithParamsClient");
    suite.add(new Y.Test.Case({
        "test DELETEWithParamsClient": function() {
            Y.Assert.areEqual('(METHOD: DELETE)', Y.one('#output').get('innerHTML'));
        }
    }));
    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});
