/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: inspectResponseHeaderServer");

     suite.add(new Y.Test.Case({
	     "test inspectResponseHeaderServer": function(){
             Y.Assert.areEqual('x-powered-by: Express', Y.one('#header1').get('innerHTML'));
		     Y.Assert.areEqual('content-type: text/html; charset=utf-8', Y.one('#header2').get('innerHTML'));
			 Y.Assert.areEqual('transfer-encoding: chunked', Y.one('#header3').get('innerHTML'));
             Y.Assert.areEqual('invalid header: ', Y.one('#header4').get('innerHTML'));
         }
     }));

     Y.Test.Runner.add(suite);
});
