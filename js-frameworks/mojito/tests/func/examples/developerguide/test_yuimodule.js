/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

         var suite = new Y.Test.Suite("DeveloperGuide: yuimodule");

         suite.add(new Y.Test.Case({

             "test yuimodule": function() {
                 Y.Assert.areEqual("Storage Lite", Y.one('a').get('innerHTML'));
                 Y.Assert.isTrue(Y.one('#notes').hasClass('ready'), 'the classname [ready] should be applied if Y.StorageLite works');
             }
         }));

         Y.Test.Runner.add(suite);
});

