/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Simple: part4");

         suite.add(new Y.Test.Case({
         
             "test part4": function() {
                 Y.Assert.areEqual("Mojito is Working.", Y.one('body').get('innerHTML').match(/Mojito is Working./gi));
             }
         }));    

         Y.Test.Runner.add(suite);
});

