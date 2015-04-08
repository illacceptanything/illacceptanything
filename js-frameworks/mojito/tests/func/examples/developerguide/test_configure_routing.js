/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: configurerouting");

         suite.add(new Y.Test.Case({
         
             "test configurerouting": function() {
                  Y.Assert.areEqual('Route Path:', Y.one('body').get('innerHTML').match(/Route Path:/gi));
                  Y.Assert.areEqual('HTTP Methods:', Y.one('body').get('innerHTML').match(/HTTP Methods:/gi));
                  // Y.Assert.areEqual('GET, POST, PUT', Y.one('body').get('innerHTML').match(/GET, POST, PUT/gi));
                  Y.Assert.areEqual('GET', Y.one('body').get('innerHTML').match(/GET/gi));
                  Y.Assert.areEqual('Route Name:', Y.one('body').get('innerHTML').match(/Route Name:/gi));
                  Y.Assert.areEqual('root_route', Y.one('body').get('innerHTML').match(/root_route/gi));
             }
         }));    

         Y.Test.Runner.add(suite);
});

