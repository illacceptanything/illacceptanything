/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: HEADParamsServer");

     suite.add(new Y.Test.Case({
	    "test HEADParamsServer": function(){
            Y.Assert.areEqual('200', Y.one('#status').get('innerHTML'));
            Y.Assert.areEqual('new_header = dummy_value', Y.one('#header').get('innerHTML'));
        }
     }));

     Y.Test.Runner.add(suite);
});
