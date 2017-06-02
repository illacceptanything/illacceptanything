/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: bindingevents2");

         suite.add(new Y.Test.Case({
         
             "test bindingevents2": function() {
                  Y.Assert.areEqual('Query Term: mojito', Y.one('h3').get('innerHTML'));
                  Y.Assert.areEqual('prev', Y.all('#nav a').item(0).get('innerHTML'));
                  Y.Assert.areEqual('next', Y.all('#nav a').item(1).get('innerHTML'));
                  var imagelink = Y.all('a').item(3).get('href');
                  Y.Assert.areEqual('/static/PagerMojit/assets/pic.com/1234',imagelink.match(/\/static\/PagerMojit\/assets\/pic.com\/1234/gi));
             }
         }));    

         Y.Test.Runner.add(suite);
});

