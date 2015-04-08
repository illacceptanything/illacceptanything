/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: withdefaultlocationserver");

    suite.add(new Y.Test.Case({

        "test withdefaultlocationserver": function() {
            if (ARROW.testParams["testName"] === "part1") {
                Y.Assert.areEqual('I was appended by the recently added javascript file - js1.js.', Y.one('#para_node').get('innerHTML').match(/I was appended by the recently added javascript file - js1.js./gi));
            } else {
                Y.Assert.areEqual('I was appended by the recently added javascript file - js2.js.', Y.one('#para_node').get('innerHTML').match(/I was appended by the recently added javascript file - js2.js./gi));
            };
        }

     }));

     Y.Test.Runner.add(suite);

     function checkscript(mynode, assetLoc, assetTag, assetPat){
         var mystring;
         mynode.all(assetLoc).each(function (taskNode){
             var mysrc = taskNode.get(assetTag).match(assetPat);
             if(mysrc!=null){ mystring=mysrc; }   
         });
         return mystring;
     }
});
