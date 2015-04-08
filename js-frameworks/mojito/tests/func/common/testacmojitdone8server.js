/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitdone8server");

      suite.add(new Y.Test.Case({


        "test acmojitdone8server": function() {
	        Y.Assert.areEqual('Hello, world!--from done', Y.one('#ACMojitTest').get('innerHTML').match(/Hello, world!--from done/gi));
        }

      }));

      Y.Test.Runner.add(suite);
});