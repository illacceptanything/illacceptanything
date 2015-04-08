/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: inspectErrorServer");

     suite.add(new Y.Test.Case({
	    "test inspectErrorServer": function(){
            Y.Assert.areEqual('Error: 404', Y.one('h1').get('innerHTML'));
        }
     }));

     Y.Test.Runner.add(suite);
});
