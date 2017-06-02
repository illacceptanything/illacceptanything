/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone10server");

    suite.add(new Y.Test.Case({

      "test acmojitdone10server": function() {
	      Y.Assert.areEqual("1,2,,4", Y.one('#ACMojitTest').get('innerHTML').match(/1,2,,4/gi));
      }

    }));

    Y.Test.Runner.add(suite);

});