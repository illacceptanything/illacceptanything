/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Input: post2");

         suite.add(new Y.Test.Case({
         
             "test post2": function() {
                 Y.Assert.areEqual("Input Parameters Example", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("POST Parameters", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("Here's the POST data!", Y.one('p').get('innerHTML'));
                 Y.Assert.areEqual("Everyone likes ice cream!", Y.one('div').get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

