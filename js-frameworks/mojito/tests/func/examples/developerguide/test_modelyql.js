/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("DeveloperGuide: modelyql");

        suite.add(new Y.Test.Case({

            "test modelyql": function() {
                if(Y.all('a').size() === 0){
                    Y.Assert.fail("No pic is shown on the page!");
                }
                for(i=0; i<Y.all('a').size(); i++){
                    Y.Assert.areEqual("/static/flickr/assets", Y.all('a').item(i).getAttribute('href').match(/\/static\/flickr\/assets/gi));
                };
             }
         }));

         Y.Test.Runner.add(suite);
});

