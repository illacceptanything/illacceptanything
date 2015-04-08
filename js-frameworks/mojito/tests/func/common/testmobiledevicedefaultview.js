/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: mobiledevicedefaultview");

    suite.add(new Y.Test.Case({

        "test mobiledevicedefaultview": function() {
            Y.Assert.areEqual('This is the default view.', Y.one('p').get('innerHTML').match(/This is the default view./gi));
        }

    }));

    Y.Test.Runner.add(suite);

});
