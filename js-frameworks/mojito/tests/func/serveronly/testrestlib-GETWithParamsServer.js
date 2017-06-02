/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: GETWithParamsServer");

     suite.add(new Y.Test.Case({
	    "test GETWithParamsServer": function(){
            Y.Assert.areEqual('(METHOD: GET) This is sprint 4 for the project Mojito', Y.one('#output').get('innerHTML'));
        }
     }));

     Y.Test.Runner.add(suite);
});
