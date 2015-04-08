/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', function (Y) {
   
    var suite = new Y.Test.Suite("Routing: BasicRouting3");
    
    suite.add(new Y.Test.Case({
	     "test BasicRouting3": function(){   
             Y.Assert.areEqual('myAction output - This is another action', Y.one('#mytext').get('innerHTML'));
         }
   }));    

   Y.Test.Runner.add(suite);
});
