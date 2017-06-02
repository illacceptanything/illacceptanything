/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: adding_view_engine");

         suite.add(new Y.Test.Case({
         
             "test adding_view_engine": function() {
                  Y.Assert.areEqual('Handlebars at work!', Y.one('h2').get('innerHTML'));
                  Y.Assert.areEqual('Here are some of the other available rendering engines:', Y.one('h3').get('innerHTML').match(/Here are some of the other available rendering engines:/gi));
                  Y.Assert.areEqual('EJS', Y.all('li').item(0).get('innerHTML'));
                  Y.Assert.areEqual('Jade', Y.all('li').item(1).get('innerHTML'));
                  Y.Assert.areEqual('dust', Y.all('li').item(2).get('innerHTML'));
                  Y.Assert.areEqual('underscore', Y.all('li').item(3).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

