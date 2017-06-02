/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: compositemojit2server");

    suite.add(new Y.Test.Case({

        "test compositemojit2server": function() {
            Y.Assert.areEqual('news heard a click from nav \(type\: CM_Nav\) with the data\:', Y.one('#click1').get('innerHTML').match(/news heard a click from nav \(type\: CM_Nav\) with the data\:/gi));
            Y.Assert.areEqual('ALERT - Run Run !!', Y.one('#click1').get('innerHTML').match(/ALERT - Run Run !!/gi));
        }

   }));

   Y.Test.Runner.add(suite);

});