/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: HeadersServer");

     suite.add(new Y.Test.Case({
	  "test HeadersServer": function(){
          Y.Assert.areEqual('\{\"myheader\":\"somevalue\",\"connection\":\"keep-alive\",\"keep-alive\":\"200\",\"host\":\"', Y.one('#something').get('innerHTML').match(/\{\"myheader\":\"somevalue\",\"connection\":\"keep-alive\",\"keep-alive\":\"200\",\"host\":\"/gi));
          Y.Assert.areEqual('somevalue', Y.one('#my_header').get('innerHTML'));
          Y.Assert.areEqual('keep-alive', Y.one('#connection').get('innerHTML'));
      }
  }));

  Y.Test.Runner.add(suite);
});
