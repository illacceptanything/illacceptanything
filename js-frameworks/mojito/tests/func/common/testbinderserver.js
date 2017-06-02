/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: BinderAllServer");

    suite.add(new Y.Test.Case({

        "test BinderAllServer": function() {
            if (ARROW.testParams["testName"] === "part1") {
                Y.Assert.areEqual('Hello, chicken.', Y.one('#bindertitle').get('innerHTML').match(/Hello, chicken./gi));
                Y.Assert.areEqual('This is extra text from the config file', Y.one('#myconfig').get('innerHTML').match(/This is extra text from the config file/gi));
                Y.Assert.areEqual('\/static\/Binders\/assets\/chicken.jpg', Y.one('#chickenimg').get('src').match(/\/static\/Binders\/assets\/chicken.jpg/gi));
                Y.Assert.areEqual('Brought to you by ChickenMojit v0.1.0', Y.one('#myfooter').get('innerHTML').match(/Brought to you by ChickenMojit v0.1.0/gi));
            } else if (ARROW.testParams["testName"] === "part2") {
                Y.Assert.areEqual('This is the data coming from the config for the id: myId', Y.one('#data').get('innerHTML').match(/This is the data coming from the config for the id: myId/gi));
                Y.Assert.areEqual('I clicked myself 1 time', Y.one('#para1').get('innerHTML').match(/I clicked myself 1 time/gi));
            } else {
                Y.Assert.areEqual('I clicked myself 2 times', Y.one('#para2').get('innerHTML').match(/I clicked myself 2 times/gi));
            };
        }
   }));

   Y.Test.Runner.add(suite);

});