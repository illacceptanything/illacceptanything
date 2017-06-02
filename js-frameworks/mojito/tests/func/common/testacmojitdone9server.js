/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitdone9server");

      suite.add(new Y.Test.Case({


        "test acmojitdone9server": function() {
	        Y.Assert.areEqual("<h4></h4>", Y.one('#ACMojitTest').get('innerHTML').match(/<h4><\/h4>/gi));
        }

     }));

     Y.Test.Runner.add(suite);

});
