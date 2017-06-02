/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone5client");

    suite.add(new Y.Test.Case({
         
       "test acmojitdone5client": function() {
          if (ARROW.testParams["testName"] === "part1") {
              Y.one('#testcase > option[value="done5"]').set('selected','selected'); 
          } else {
              Y.Assert.areEqual('\{\"mycars\":[\"Saab\",\"Volvo\",\"BMW\"\]\}', Y.one('#ACMojitResult').get('innerHTML').match(/\{\"mycars\":\[\"Saab\",\"Volvo\",\"BMW\"\]\}/gi));
          };
      }

      }));

       Y.Test.Runner.add(suite);

});