/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: mojitproxyrefreshview");

    suite.add(new Y.Test.Case({

        "test mojitproxyrefreshview": function() {
            if (ARROW.testParams["testName"] === "part1") {
                Y.Assert.areEqual('Testing ac.refreshView', Y.one('#MojitProxyMojitResult').get('innerHTML').match(/Testing ac.refreshView/gi));
            } else {
                Y.Assert.areEqual('Testing ac.refreshView', Y.one('#MojitProxyMojitResult').get('innerHTML').match(/Testing ac.refreshView/gi));
            };
        }

    }));

    Y.Test.Runner.add(suite);
});