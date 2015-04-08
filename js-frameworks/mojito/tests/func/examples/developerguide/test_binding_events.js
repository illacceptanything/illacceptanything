/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: bindingevents");

         suite.add(new Y.Test.Case({
         
             "test bindingevents": function() {
                  Y.Assert.areEqual('Query Term: mojito', Y.one('h3').get('innerHTML'));
                  Y.Assert.areEqual('prev', Y.one('#nav').get('innerHTML').match(/prev/gi));
                  Y.Assert.areEqual('next', Y.one('#nav a').get('innerHTML'));
                  var imagelink = Y.all('a').item(1).get('href');
                  Y.Assert.areEqual('/static/PagerMojit/assets/pic.com/1234',imagelink.match(/\/static\/PagerMojit\/assets\/pic.com\/1234/gi));
             }
         }));    

         Y.Test.Runner.add(suite);
});

