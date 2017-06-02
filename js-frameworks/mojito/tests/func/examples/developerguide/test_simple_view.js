/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: simpleview");

         suite.add(new Y.Test.Case({
         
             "test simpleview": function() {
                 Y.Assert.areEqual("Simple View", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("type: simple", Y.all('div').item(1).get('innerHTML'));
                 Y.Assert.areEqual("show: simple", Y.all('div').item(3).get('innerHTML'),
                                  'div[3] test failed');
                 Y.Assert.areEqual("hide: ", Y.all('div').item(4).get('innerHTML'),
                                  'div[4] test failed');
                 Y.Assert.areEqual("no show: ", Y.all('div').item(5).get('innerHTML'),
                                  'div[5] test failed');
                 Y.Assert.areEqual("no hide: simple", Y.all('div').item(6).get('innerHTML'),
                                  'div[6] test failed');
                 Y.Assert.areEqual("list: 213", Y.all('div').item(7).get('innerHTML'),
                                  'div[7] test failed');
                 Y.Assert.areEqual("hole: no list", Y.all('div').item(8).get('innerHTML'),
                                  'div[8] test failed');
             }
         }));    

         Y.Test.Runner.add(suite);
});

