/*
 * This is a basic func test for a Routing application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Routing: BasicRouting9-2");
    suite.add(new Y.Test.Case({
	     "test BasicRouting9-2": function(){   
             Y.Assert.areEqual('Click to execute the action \'route-1\' for the mojit \'myAction\'', Y.one('#mylink').get('innerHTML'));
             Y.Assert.areEqual('/myAction', Y.one('#mylink').get('href').match(/\/myAction/gi));
         }
   }));    

   Y.Test.Runner.add(suite);
});
