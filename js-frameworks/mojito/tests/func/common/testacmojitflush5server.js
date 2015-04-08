/*
 * This is a basic func test for a Common application.
 */
YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitflush5server");

      suite.add(new Y.Test.Case({


        "test acmojitflush5server": function() {
	        Y.Assert.areEqual('\{\"mycars\":\[\"Saab\",\"Volvo\",\"BMW\"\]\}', Y.one('body').get('innerHTML').match(/\{\"mycars\":\[\"Saab\",\"Volvo\",\"BMW\"\]\}/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});