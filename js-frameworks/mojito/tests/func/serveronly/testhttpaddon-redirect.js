/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
         var suite = new Y.Test.Suite("ServerOnly: redirect");

         suite.add(new Y.Test.Case({
  
	     "test redirect": function(){
              Y.Assert.areEqual('This is a very simple web service', Y.one('#output').get('innerHTML'));
          }
  }));    

  Y.Test.Runner.add(suite);

});
