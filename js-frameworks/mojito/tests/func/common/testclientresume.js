/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: clientresumefunc");

    suite.add(new Y.Test.Case({

        "test clientresumefunc": function() {
            if (ARROW.testParams["testName"] === "part1") {
                Y.Assert.areEqual('Testing ac.pause And ac.resume', Y.one('#MojitProxyMojitResult').get('innerHTML').match(/Testing ac.pause And ac.resume/gi));
            } else if (ARROW.testParams["testName"] === "part2") {
                Y.Assert.areEqual('Testing ac.pause And ac.resume', Y.one('#MojitProxyMojitResult').get('innerHTML').match(/Testing ac.pause And ac.resume/gi));
            } else {
                Y.Assert.areEqual('this is my data: abc', Y.one('#thisdata').get('innerHTML').match(/this is my data: abc/gi));
            };
        }
    }));

    Y.Test.Runner.add(suite);
});