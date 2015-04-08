/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-getwithparamsclient-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: GETWithParamsClient");
    suite.add(new Y.Test.Case({
        "test GETWithParamsClient": function() {
            Y.Assert.areEqual('(METHOD: GET) This is sprint 4 for the project Mojito', Y.one('#output').get('innerHTML'));
       }
    }));
    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});
