/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone3client");

    suite.add(new Y.Test.Case({
         
       "test acmojitdone3client": function() {
          if (ARROW.testParams["testName"] === "part1") {
              Y.one('#testcase > option[value="done3"]').set('selected','selected'); 
          } else {
              Y.Assert.areEqual('Hello Action Context Testing', Y.one('#ACMojitTest').get('innerHTML').match(/Hello Action Context Testing/gi));
          };
      }
      }));

       Y.Test.Runner.add(suite);

});