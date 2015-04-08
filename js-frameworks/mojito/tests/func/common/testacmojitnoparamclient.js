/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: acmojitnoparamclient");

    suite.add(new Y.Test.Case({
         
      "test acmojitnoparamclient": function() {
          if (ARROW.testParams["testName"] === "part1") {
              Y.one('#testcase > option[value="noparam"]').set('selected','selected'); 
          } else {
              Y.Assert.areEqual('Hello, world!--from done', Y.one('#ACMojitResult').get('innerHTML').match(/Hello, world!--from done/gi));
          };
      }

      }));

       Y.Test.Runner.add(suite);

});