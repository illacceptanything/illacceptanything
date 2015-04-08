/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitdone7server");

      suite.add(new Y.Test.Case({

        "test acmojitdone7server": function() {
	        Y.Assert.areEqual('data', Y.one('body').get('innerHTML').match(/data/gi));
            Y.Assert.areEqual('\"Hello, world!\"', Y.one('body').get('innerHTML').match(/\"Hello, world!\"/gi));
        }

     }));

     Y.Test.Runner.add(suite);

});