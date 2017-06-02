/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone10client");

    suite.add(new Y.Test.Case({

      "test acmojitdone10client": function() {
          if (ARROW.testParams["testName"] === "part1") {
              Y.one('#testcase > option[value="done10"]').set('selected','selected'); 
          } else {
              Y.Assert.areEqual("1,2,,4", Y.one('#ACMojitTest').get('innerHTML').match(/1,2,,4/gi));
          };
      }

      }));

      Y.Test.Runner.add(suite);

});