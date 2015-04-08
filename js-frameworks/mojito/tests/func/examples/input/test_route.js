/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Input: route");

         suite.add(new Y.Test.Case({
         
             "test route": function() {
                 Y.Assert.areEqual("Input Parameters Example", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("ROUTE Parameters", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("All route params", Y.one('p').get('innerHTML'));
                 Y.Assert.areEqual("foo ==&gt; fooval", Y.all('li').item(0).get('innerHTML'));
                 Y.Assert.areEqual("bar ==&gt; barval", Y.all('li').item(1).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

