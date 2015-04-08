/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: deviceviews");

         suite.add(new Y.Test.Case({
         
             "test deviceviews": function() {
                 var output = ARROW.testParams["testName"]+" View";
                 Y.Assert.areEqual(output, Y.one('h2').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

