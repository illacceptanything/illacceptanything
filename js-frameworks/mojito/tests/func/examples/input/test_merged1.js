/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("Input: merged1");

         suite.add(new Y.Test.Case({
         
             "test merged": function() {
                 Y.Assert.areEqual("Merged Parameters Example", Y.one('h1').get('innerHTML'));
                 Y.Assert.areEqual("POST Parameters", Y.one('h2').get('innerHTML'));
                 Y.Assert.areEqual("Submit for for example of POST processing.", Y.one('p').get('innerHTML'));
                 Y.one('#name').set('value', "Everyone");
                 Y.one('#likes > option[value="ice cream"]').set('selected','selected');
             }
         }));    

         Y.Test.Runner.add(suite);
});

