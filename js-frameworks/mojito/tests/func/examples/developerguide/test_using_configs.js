/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: usingconfigs");

         suite.add(new Y.Test.Case({
         
             "test usingconfigs": function() {
                 Y.Assert.areEqual("WooHah Inc.", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("Woohah", Y.all('a').item(0).get('innerHTML'));
                 Y.Assert.areEqual("BooYah", Y.all('a').item(1).get('innerHTML'));
                 Y.Assert.areEqual("Human Resources", Y.all('li').item(0).get('innerHTML').match(/Human Resources/gi));
                 Y.Assert.areEqual("Bobby 'Soylent' Green 555.555.1212 x123 hr@woohoo.com", Y.all('li').item(0).get('innerHTML').match(/Bobby 'Soylent' Green 555.555.1212 x123 hr@woohoo.com/gi));
                 Y.Assert.areEqual("<b>Sales</b>", Y.all('li').item(1).get('innerHTML').match(/<b>Sales<\/b>/gi));
                 Y.Assert.areEqual("Biff 'Whitey' Shoemaker 555.555.1212 x124 sales@woohoo.com", Y.all('li').item(1).get('innerHTML').match(/Biff 'Whitey' Shoemaker 555.555.1212 x124 sales@woohoo.com/gi));
                 Y.Assert.areEqual("Customer Service", Y.all('li').item(2).get('innerHTML').match(/Customer Service/gi));
                 Y.Assert.areEqual("Earl 'Ringer' Scheib 555.555.1212 x234 help@woohoo.com", Y.all('li').item(2).get('innerHTML').match(/Earl 'Ringer' Scheib 555.555.1212 x234 help@woohoo.com/gi));
                 Y.Assert.areEqual("Copyright WooHah Inc. 2012", Y.all('p').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

