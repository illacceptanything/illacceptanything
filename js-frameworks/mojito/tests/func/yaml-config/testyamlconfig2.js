/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("yaml-config");

         suite.add(new Y.Test.Case({
         
             "test yamlconfig2": function() {
                  Y.Assert.areEqual('Using YAML configs with comments!', Y.one('pre').get('innerHTML'));
             }
         }));    
         
         Y.Test.Runner.add(suite);
});

