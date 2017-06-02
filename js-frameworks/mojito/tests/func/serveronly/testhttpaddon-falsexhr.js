/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
   var suite = new Y.Test.Suite("ServerOnly: falseXhr");

   suite.add(new Y.Test.Case({
  
	     "test falseXhr": function(){
              Y.Assert.areEqual('This is the Xhr value: false', Y.one('#xhrValue').get('innerHTML'));
         }
   }));    

   Y.Test.Runner.add(suite);

});
