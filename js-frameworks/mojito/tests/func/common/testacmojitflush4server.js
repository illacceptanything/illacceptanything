/*
 * This is a basic func test for a Common application.
 */
YUI({
      useConsoleOutput: true,
      useBrowserConsole: true,
      logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

      var suite = new Y.Test.Suite("Common: acmojitflush4server");

      suite.add(new Y.Test.Case({


        "test acmojitflush4server": function() {
            Y.Assert.areEqual('Saab,Volvo,BMW', Y.all('#ACMojitTest').item(0).get('innerHTML').match(/Saab,Volvo,BMW/gi));
        }

      }));

      Y.Test.Runner.add(suite);

});