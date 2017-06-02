/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: htmlframe");

         suite.add(new Y.Test.Case({
         
             "test htmlframe": function() {
                 Y.Assert.areEqual("Framed Mojit", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("90%", Y.one('h2').getStyle('width'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

