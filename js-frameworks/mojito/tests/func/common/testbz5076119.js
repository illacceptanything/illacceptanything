/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: bz5076119");

    suite.add(new Y.Test.Case({

        "test bz5076119": function() {
            Y.Assert.areEqual('List of all the children:', Y.one('h3').get('innerHTML').match(/List of all the children:/gi));
        }

    }));

    Y.Test.Runner.add(suite);

});
