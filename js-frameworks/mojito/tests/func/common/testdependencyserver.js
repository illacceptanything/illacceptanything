/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: dependencyserver");

    suite.add(new Y.Test.Case({

	  "test dependencyserver": function() {
	      Y.Assert.areEqual('0,1,Aardvark,attic,zebra,Zoo', Y.one('#myarray').get('innerHTML').match(/0,1,Aardvark,attic,zebra,Zoo/gi));
      }

     }));

     Y.Test.Runner.add(suite);

});
