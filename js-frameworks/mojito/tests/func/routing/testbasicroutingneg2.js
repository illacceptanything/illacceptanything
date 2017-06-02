/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', function (Y) {

    var suite = new Y.Test.Suite("Routing: BasicRoutingNeg2");

    suite.add(new Y.Test.Case({
        "test BasicRoutingNeg2": function(){
            Y.Assert.areEqual("Cannot GET /complete/invalid/path", Y.one('body').get('innerHTML').match(/Cannot GET \/complete\/invalid\/path/gi));
        }
    }));

    Y.Test.Runner.add(suite);
});
