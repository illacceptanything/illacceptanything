/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Routing: BasicRouting9-1");
    
    suite.add(new Y.Test.Case({
	     "test BasicRouting9-1": function(){   
	         Y.Assert.areEqual('Click to execute the action \'route-2\' for the mojit \'index\'', Y.one('#mylink').get('innerHTML'));
             Y.Assert.areEqual('/route-2/myPath', Y.one('#mylink').get('href').match(/\/route-2\/myPath/gi));
         }
   }));    

   Y.Test.Runner.add(suite);
});
