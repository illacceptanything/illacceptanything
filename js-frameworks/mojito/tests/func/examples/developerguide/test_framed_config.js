/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: framedconfig");

         suite.add(new Y.Test.Case({
         
             "test framedconfig": function() {
                 Y.Assert.areEqual("HTML Frame Configuration Example", Y.one('h3').get('innerHTML').match(/HTML Frame Configuration Example/gi));
                 Y.Assert.areEqual("This mojit should be framed with proper &lt;HTML&gt; tags.", Y.all('div').item(1).get('innerHTML').match(/This mojit should be framed with proper &lt;HTML&gt; tags./gi));
             }
         }));    

         Y.Test.Runner.add(suite);
});

