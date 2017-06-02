/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: inspectResponseServer");

     suite.add(new Y.Test.Case({
	     "test inspectResponseServer": function(){
              Y.Assert.areEqual('Status Code: 200', Y.one('#code').get('innerHTML'));
			  Y.Assert.areEqual('Status Message: OK', Y.one('#msg').get('innerHTML'));
			  Y.Assert.areEqual('This is a very simple web service', Y.one('#output').get('innerHTML'));
         }
     }));

     Y.Test.Runner.add(suite);
});
