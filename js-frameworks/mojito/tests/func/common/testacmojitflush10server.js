/*
 * This is a basic func test for a Common application.
 */
 YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
 }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: ACMojitFlush10Server");

      suite.add(new Y.Test.Case({
          
          "test ACMojitFlush10Server": function() {
	            Y.Assert.areEqual(Y.one('body').get('innerHTML').match(/Hello, world!--from flush,Hello, world!--from done/gi), 'Hello, world!--from flush,Hello, world!--from done');
          }

       }));

       Y.Test.Runner.add(suite);

});