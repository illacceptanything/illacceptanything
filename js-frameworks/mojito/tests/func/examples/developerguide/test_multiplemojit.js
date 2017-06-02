/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: multiplemojit");

         suite.add(new Y.Test.Case({
         
             "test multiplemojit": function() {
                 Y.Assert.areEqual("Parent Frame", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("Header", Y.all('h3').item(0).get('innerHTML'));
                 Y.Assert.areEqual("Body", Y.one('h4').get('innerHTML'));
                 Y.Assert.areEqual("Footer", Y.all('h3').item(1).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

