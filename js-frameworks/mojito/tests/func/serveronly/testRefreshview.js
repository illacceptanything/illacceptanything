/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-refreshview-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: refreshview");

    suite.add(new Y.Test.Case({
        "test refreshview": function(){
            if (ARROW.testParams['testName'] === "Page1") {
                Y.Assert.areEqual('Sports', Y.one('#titlesports').get('innerHTML'));
            } else {
                Y.Assert.areEqual('Sports', Y.one('#titlesports').get('innerHTML'));
            }
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});
