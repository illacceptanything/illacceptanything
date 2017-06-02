/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Simple: part1");

         suite.add(new Y.Test.Case({
         
             "test part1": function() {
                 Y.Assert.areEqual("Mojito is Working.", Y.one('pre').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

