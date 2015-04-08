/*
 * This is a basic func test for a Common application.
 */
YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitflush9server");

      suite.add(new Y.Test.Case({

        "test acmojitflush9server": function() {
	        Y.Assert.areEqual('Hello, world!--from flush,Hello, world!--from done', Y.one('body').get('innerHTML').match(/Hello, world!--from flush,Hello, world!--from done/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});
