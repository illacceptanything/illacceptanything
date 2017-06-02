/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: ACMojitServer");

    suite.add(new Y.Test.Case({

        "test ACMojitServer": function() {
            Y.Assert.areEqual('Hello, world!--from flush,Hello, world!--from done', Y.one('pre').get('innerHTML').match(/Hello, world!--from flush,Hello, world!--from done/gi));
        }

    }));

    Y.Test.Runner.add(suite);

});
