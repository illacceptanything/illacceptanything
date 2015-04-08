/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone9client");

    suite.add(new Y.Test.Case({

      "test acmojitdone9client": function() {
          if (ARROW.testParams["testName"] === "part1") {
              Y.one('#testcase > option[value="done9"]').set('selected','selected'); 
          } else {
              Y.Assert.areEqual("<h4></h4>", Y.one('#ACMojitTest').get('innerHTML').match(/<h4><\/h4>/gi));
          };
      }

      }));

      Y.Test.Runner.add(suite);

});