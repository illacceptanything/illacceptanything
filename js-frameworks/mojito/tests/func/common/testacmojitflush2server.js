/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitflush2server");

      suite.add(new Y.Test.Case({


        "test acmojitflush2server": function() {
	        Y.Assert.areEqual('\{\"greeting\"\:\"Hello Action Context Testing\"\}', Y.one('body').get('innerHTML').match(/\{\"greeting\"\:\"Hello Action Context Testing\"\}/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});