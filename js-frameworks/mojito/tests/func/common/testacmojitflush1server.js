/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitflush1server");

      suite.add(new Y.Test.Case({


        "test acmojitflush1server": function() {
	        Y.Assert.areEqual('Hello Action Context Testing', Y.all('#ACMojitTest').item(0).get('innerHTML').match(/Hello Action Context Testing/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});