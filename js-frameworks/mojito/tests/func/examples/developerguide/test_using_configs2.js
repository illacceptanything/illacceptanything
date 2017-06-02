/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: usingconfigs2");

         suite.add(new Y.Test.Case({
         
             "test usingconfigs2": function() {
                 Y.Assert.areEqual("BooYah Inc. (a WooHah company)", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("Woohah", Y.all('a').item(0).get('innerHTML'));
                 Y.Assert.areEqual("BooYah", Y.all('a').item(1).get('innerHTML'));
                 Y.Assert.areEqual("Engineering", Y.all('li').item(0).get('innerHTML').match(/Engineering/gi));
                 Y.Assert.areEqual("Tommy 'Tacoma' Bridge 555.555.1212 x121 hr@woohoo.com", Y.all('li').item(0).get('innerHTML').match(/Tommy 'Tacoma' Bridge 555.555.1212 x121 hr@woohoo.com/gi));
                 Y.Assert.areEqual("Customer Service", Y.all('li').item(1).get('innerHTML').match(/Customer Service/gi));
                 Y.Assert.areEqual("Willy 'Windy' Wilson 555.555.1212 x784 help@woohoo.com", Y.all('li').item(1).get('innerHTML').match(/Willy 'Windy' Wilson 555.555.1212 x784 help@woohoo.com/gi));
                 Y.Assert.areEqual("Copyright BooYah Inc. 2012 (a WooHah subsidiary)", Y.all('p').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

