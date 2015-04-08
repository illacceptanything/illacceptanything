/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: mobiledeviceiphoneview");

    suite.add(new Y.Test.Case({

        "test mobiledeviceiphoneview": function() {
            Y.Assert.areEqual('This is the view for the IPhone.', Y.one('p').get('innerHTML').match(/This is the view for the IPhone./gi));
        }

    }));

    Y.Test.Runner.add(suite);

});
