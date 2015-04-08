/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: generatingurl2");

         suite.add(new Y.Test.Case({
         
             "test generatingurl2": function() {
                 Y.Assert.areEqual("This is the contact page last viewed on: ", Y.all('p').item(0).get('innerHTML').match(/This is the contact page last viewed on: /gi));
                 Y.Assert.areEqual("Yahoo Inc, 701 First Avenue, Sunnyvale CA 94089", Y.all('p').item(1).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

