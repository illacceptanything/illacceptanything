/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
         var suite = new Y.Test.Suite("ServerOnly: responseObject");

         suite.add(new Y.Test.Case({
  
	     "test responseObject": function(){
              Y.Assert.areEqual('Headers: {\"x-powered-by\":\"Express\"}', Y.one('#headers').get('innerHTML'));
		      Y.Assert.areEqual('shouldKeepAlive: true', Y.one('#shouldKeepAlive').get('innerHTML'));
			  Y.Assert.areEqual('hasBody: true', Y.one('#hasBody').get('innerHTML'));
         }       
  }));    

  Y.Test.Runner.add(suite);

});
