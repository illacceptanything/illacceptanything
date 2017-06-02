/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitdone5server");

      suite.add(new Y.Test.Case({

        "test acmojitdone5server": function() {
	        Y.Assert.areEqual('\"Saab\"', Y.one('body').get('innerHTML').match(/\"Saab\"/gi));
            Y.Assert.areEqual('\"Volvo\"', Y.one('body').get('innerHTML').match(/\"Volvo\"/gi));
            Y.Assert.areEqual('"BMW"', Y.one('body').get('innerHTML').match(/\"BMW\"/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});