/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
    var suite = new Y.Test.Suite("ServerOnly: getHeaders");
    suite.add(new Y.Test.Case({
      "test getHeaders": function(){
          Y.Assert.areEqual('All Headers match', Y.one('#output').get('innerHTML'));
      }
    }));    

   Y.Test.Runner.add(suite);

});
