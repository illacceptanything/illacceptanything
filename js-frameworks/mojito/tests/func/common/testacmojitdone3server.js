/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: ACMojitDone3Server");

      suite.add(new Y.Test.Case({

          "test ACMojitDone3Server": function() {
	          Y.Assert.areEqual('Hello Action Context Testing', Y.one('#ACMojitTest').get('innerHTML').match(/Hello Action Context Testing/gi));
          }

      }));

      Y.Test.Runner.add(suite);

});