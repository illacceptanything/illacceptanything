/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Simple: part2");

         suite.add(new Y.Test.Case({
         
             "test part2": function() {
                 Y.Assert.areEqual("Mojito is Working.", Y.one('div').get('innerHTML').match(/Mojito is Working./gi));
             }
         }));    

         Y.Test.Runner.add(suite);
});

