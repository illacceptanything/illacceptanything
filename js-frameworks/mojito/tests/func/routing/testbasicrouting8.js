/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Routing: BasicRouting8");
    
    suite.add(new Y.Test.Case({
	     "test BasicRouting8": function(){   
             Y.Assert.areEqual('Checking POST and GET with routing', Y.one('#title').get('innerHTML'));
         }
   }));    

   Y.Test.Runner.add(suite);
});
