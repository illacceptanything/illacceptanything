/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', function (Y) {

    var suite = new Y.Test.Suite("Routing: BasicRoutingNeg1");

    suite.add(new Y.Test.Case({
        "test BasicRoutingNeg1": function(){
            Y.Assert.areEqual("Error: 404", Y.one('h1').get("innerHTML"));
            Y.Assert.areEqual("Error details are not available.", Y.one('p').get("innerHTML"));
        }
    }));

    Y.Test.Runner.add(suite);
});
