/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', function (Y) {

    var suite = new Y.Test.Suite("Routing: BasicRoutingNeg3");

    suite.add(new Y.Test.Case({
        _should: {
            ignore: {
            }
        },
        "test BasicRoutingNeg3": function(){
            Y.Assert.areEqual("Click to execute the action 'route-2' for the mojit 'nothing'", Y.one('#mylink').get('innerHTML'));
                Y.Assert.areEqual("/route-2/nothing", Y.one('#mylink').getAttribute("href"));
        }
    }));

    Y.Test.Runner.add(suite);
});
