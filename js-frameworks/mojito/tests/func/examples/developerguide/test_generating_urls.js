/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: generatingurls");

         suite.add(new Y.Test.Case({
         
             "test generatingurls": function() {
                 Y.Assert.areEqual("This is the default page that is visible on the root path.", Y.all('p').item(0).get('innerHTML'));
                 Y.Assert.areEqual("here", Y.one('a').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

