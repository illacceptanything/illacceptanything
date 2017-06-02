/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

    var suite = new Y.Test.Suite("Common: mojitproxygetfromurl");

    suite.add(new Y.Test.Case({
 
        "test mojitproxygetfromurl": function() {
            Y.Assert.areEqual('abc', Y.one('#thisvalue').get('innerHTML').match(/abc/gi));
        }

    }));

    Y.Test.Runner.add(suite);
});