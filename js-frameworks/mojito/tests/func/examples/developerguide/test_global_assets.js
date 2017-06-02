/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: globalassets");

         suite.add(new Y.Test.Case({
         
             "test globalassets": function() { 
                 Y.Assert.areEqual("/static/global_assets/assets/ohhai.css", Y.one('link').getAttribute('href'));
                 Y.Assert.areEqual("/static/global_assets/assets/sadwalrus.jpeg", Y.one('img').getAttribute('src'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

