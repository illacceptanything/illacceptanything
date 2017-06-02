/*
 * This is a basic func test for a Common application.
 */
YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitflush8server");

      suite.add(new Y.Test.Case({

        "test acmojitflush8server": function() {
	        Y.Assert.areEqual('Hello, world!--from flush', Y.all('#ACMojitTest').item(0).get('innerHTML').match(/Hello, world!--from flush/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});