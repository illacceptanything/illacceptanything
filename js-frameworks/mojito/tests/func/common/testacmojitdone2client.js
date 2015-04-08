/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitdone2client");

    suite.add(new Y.Test.Case({
         
       "test acmojitdone2client": function() {
           if (ARROW.testParams["testName"] === "part1") {
               Y.one('#testcase > option[value="done2"]').set('selected','selected'); 
           } else {
               Y.Assert.areEqual('\{\"greeting\"\:\"Hello Action Context Testing\"\}', Y.one('#ACMojitResult').get('innerHTML').match(/\{\"greeting\"\:\"Hello Action Context Testing\"\}/gi));
           };
        }

      }));

       Y.Test.Runner.add(suite);

});