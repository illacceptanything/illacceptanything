/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: mojitproxyrender");

    suite.add(new Y.Test.Case({

        "test mojitproxyrender": function() {
            Y.log("************"+Y.one('#thisdata').get('innerHTML'));
            Y.Assert.areEqual('this is my data: abc', Y.one('#thisdata').get('innerHTML').match(/this is my data: abc/gi));
        }

    }));

    Y.Test.Runner.add(suite);
});