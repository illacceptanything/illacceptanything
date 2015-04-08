/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: adding_view_engine2");

         suite.add(new Y.Test.Case({
         
             "test adding_view_engine2": function() {
                  Y.Assert.areEqual(' EJS at work!', Y.one('h2').get('innerHTML'));
                  Y.Assert.areEqual('In addition to Handlebars and EJS, you can also use these rendering engines:', Y.one('h3').get('innerHTML'));
                  Y.Assert.areEqual('Jade', Y.all('li').item(0).get('innerHTML'));
                  Y.Assert.areEqual('Dust', Y.all('li').item(1).get('innerHTML'));
                  Y.Assert.areEqual('underscore', Y.all('li').item(2).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

