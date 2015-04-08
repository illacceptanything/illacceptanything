/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: statefulclient");

    suite.add(new Y.Test.Case({

        "test statefulclient": function() {
            if (ARROW.testParams["testName"] === "part1") {
                Y.one('#inputbox').set('value', "baseball");
            } else if (ARROW.testParams["testName"] === "part2") {
                Y.Assert.areEqual('pitched: baseball', Y.one('#ControllerCachingResult').get('innerHTML').match(/pitched: baseball/gi));
            } else {
                Y.Assert.areEqual('ball: baseball', Y.one('#ControllerCachingResult').get('innerHTML').match(/ball: baseball/gi));
            };
        }

   }));

   Y.Test.Runner.add(suite);
});