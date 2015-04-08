/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: mojitproxygetid");

    suite.add(new Y.Test.Case({
 
        "test mojitproxygetid": function() {
            Y.log("************"+Y.one('#thisid').get('innerHTML'));
            Y.Assert.areEqual('yui_', Y.one('#thisid').get('innerHTML').match(/yui_/gi));
        }
    }));

    Y.Test.Runner.add(suite);
});