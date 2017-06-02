/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: usingparameters");

         suite.add(new Y.Test.Case({
         
             "test usingparameters": function() {
                 Y.Assert.areEqual("Show all query string parameters", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("foo =&gt; bar", Y.all('li').item(0).get('innerHTML'));
                 Y.Assert.areEqual("bar =&gt; foo", Y.all('li').item(1).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

