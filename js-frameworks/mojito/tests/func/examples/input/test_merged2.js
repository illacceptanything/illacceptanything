/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Input: merged2");

         suite.add(new Y.Test.Case({
         
             "test merged": function() {
                 Y.Assert.areEqual("Input Parameters Example", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("MERGED Parameters", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("Merged Parameters Example", Y.all('p').item(0).get('innerHTML'));
                 Y.Assert.areEqual("Hillary Clinton likes beer?", Y.one('div').get('innerHTML'));
                 Y.Assert.areEqual("Hey, is that what you entered in the form? If not, here's why...", Y.all('p').item(1).get('innerHTML'));
                 Y.Assert.areEqual("name ==&gt; Hillary Clinton", Y.all('li').item(0).get('innerHTML'));
                 Y.Assert.areEqual("likes ==&gt; beer", Y.all('li').item(1).get('innerHTML'));
                 Y.Assert.areEqual("name ==&gt; Hillary Clinton", Y.all('li').item(2).get('innerHTML'));
                 Y.Assert.areEqual("name ==&gt; Everyone", Y.all('li').item(3).get('innerHTML'));
                 Y.Assert.areEqual("likes ==&gt; ice cream", Y.all('li').item(4).get('innerHTML'));
                 Y.Assert.areEqual("likes ==&gt; beer", Y.all('li').item(5).get('innerHTML'));
             }
         }));    

         Y.Test.Runner.add(suite);
});

