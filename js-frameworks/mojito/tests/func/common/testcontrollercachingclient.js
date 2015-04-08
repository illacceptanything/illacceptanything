/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: testcontrollercachingclient");

    suite.add(new Y.Test.Case({

        "test testcontrollercachingclient": function() {
            if (ARROW.testParams["testName"] === "part1") {
                Y.one('#inputbox.ballinput').set('value', "basketball");
            } else if (ARROW.testParams["testName"] === "part2") {
                Y.Assert.areEqual('pitched: basketball', Y.one('#ControllerCachingResult').get('innerHTML').match(/pitched: basketball/gi));
            } else if (ARROW.testParams["testName"] === "part3") {
                Y.Assert.areEqual('ball: basketball', Y.one('#ControllerCachingResult').get('innerHTML').match(/ball: basketball/gi));
                Y.one('#inputbox.ballinput').set('value', "softball");
            } else if (ARROW.testParams["testName"] === "part4") {
                Y.Assert.areEqual('pitched: softball', Y.one('#ControllerCachingResult').get('innerHTML').match(/pitched: softball/gi));
            } else {
                Y.Assert.areEqual('ball: softball', Y.one('#ControllerCachingResult').get('innerHTML').match(/ball: softball/gi));
            };
       }

    }));

    Y.Test.Runner.add(suite);
});