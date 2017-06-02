/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: bz5300267 multiaction4");

     suite.add(new Y.Test.Case({
	  "test bz5300267 multiaction4": function(){
          Y.Assert.areEqual('There are two children in this Container:', Y.all('#ctitle').get('innerHTML'));
          Y.Assert.areEqual('Index', Y.all('h3').item(0).get('innerHTML'));
          Y.Assert.areEqual('hello from index', Y.all('h3').item(1).get('innerHTML'));
          Y.Assert.areEqual('Inst1', Y.all('h3').item(2).get('innerHTML'));
          Y.Assert.areEqual('hello from inst1', Y.all('h3').item(3).get('innerHTML'));
      }
  }));

  Y.Test.Runner.add(suite);
});
