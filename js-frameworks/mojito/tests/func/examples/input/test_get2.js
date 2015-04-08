/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Input: get2");

         suite.add(new Y.Test.Case({
         
             "test get2": function() {
                 Y.Assert.areEqual("Input Parameters Example", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("GET Parameters", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("Params by key", Y.one('p').get('innerHTML'));
                 Y.Assert.areEqual("Does the \"foo\" param exist?", Y.one('h3').get('innerHTML'));
                 Y.Assert.areEqual("NO", Y.one('div').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

