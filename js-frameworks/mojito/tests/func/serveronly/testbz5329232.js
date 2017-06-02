/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: bz5329232");

     suite.add(new Y.Test.Case({
	  "test bz5329232": function(){
          Y.Assert.areEqual('Testing Mojito Features', Y.one('h2').get('innerHTML'));
          Y.Assert.areEqual('This is from index.mu.html', Y.one('p').get('innerHTML').match(/This is from index.mu.html/gi));
      }
  }));

  Y.Test.Runner.add(suite);
});
