/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone7client");

    suite.add(new Y.Test.Case({
         
       "test acmojitdone7client": function() {
          if (ARROW.testParams["testName"] === "part1") {
              Y.one('#testcase > option[value="done7"]').set('selected','selected'); 
          } else {
              Y.Assert.areEqual('\{\"data\":\"Hello, world!\"\}', Y.one('#ACMojitResult').get('innerHTML').match(/\{\"data\":\"Hello, world!\"\}/gi));
          };
      }

      }));

       Y.Test.Runner.add(suite);

});