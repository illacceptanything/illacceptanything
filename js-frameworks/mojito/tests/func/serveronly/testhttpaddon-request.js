/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
         var suite = new Y.Test.Suite("ServerOnly: requestObject");

         suite.add(new Y.Test.Case({
  
	     "test requestObject": function(){
              Y.Assert.areEqual('Method: GET', Y.one('#method').get('innerHTML'));
              Y.Assert.areEqual('URL: /httpParent/testRequestObj', Y.one('#url').get('innerHTML'));
              Y.Assert.areEqual('Trailers: [object Object]', Y.one('#trailers').get('innerHTML'));
              Y.Assert.areEqual('httpVersion: 1.1', Y.one('#httpVersion').get('innerHTML'));
              Y.Assert.areEqual('Headers: {', Y.one('#headers').get('innerHTML').match(/Headers\: {/gi));
              Y.Assert.areEqual('\"host\":', Y.one('#headers').get('innerHTML').match(/\"host\":/gi));
         }
  }));    

  Y.Test.Runner.add(suite);

});
