/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
         var suite = new Y.Test.Suite("ServerOnly: trueXhr");

         suite.add(new Y.Test.Case({
  
	     "test trueXhr": function(){
              Y.Assert.areEqual('This is the Xhr value: true', Y.one('#xhrValue').get('innerHTML'));
         }

  }));    

  Y.Test.Runner.add(suite);

});
