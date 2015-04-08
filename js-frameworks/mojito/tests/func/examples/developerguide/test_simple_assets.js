/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

    var suite = new Y.Test.Suite("DeveloperGuide: simpleassets");

    suite.add(new Y.Test.Case({

        "test simpleassets": function() {
            if (ARROW.testParams["testName"] === "first") {
                Y.Assert.areEqual('Simple Assets', Y.one('h2').get('innerHTML'));
                Y.Assert.areEqual('green', Y.all('a').item(0).get('innerHTML'));
                Y.Assert.areEqual('brown', Y.all('a').item(1).get('innerHTML'));
                Y.Assert.areEqual('grey', Y.all('a').item(2).get('innerHTML'));
                Y.Assert.areEqual('blue', Y.all('a').item(3).get('innerHTML'));
                Y.Assert.areEqual('red', Y.all('a').item(4).get('innerHTML'));
                Y.Assert.areEqual('background-color: rgb(97, 101, 54);', Y.one('h2').getAttribute('style').match(/background-color: rgb\(97, 101, 54\);/gi));
            } else if (ARROW.testParams["testName"] === "second") {
                Y.Assert.areEqual('background-color: rgb(89, 62, 26);', Y.one('h2').getAttribute('style').match(/background-color: rgb\(89, 62, 26\);/gi));
            } else {
                Y.Assert.areEqual('background-color: rgb(119, 123, 136);', Y.one('h2').getAttribute('style').match(/background-color: rgb\(119, 123, 136\);/gi));
            }
        }
    }));

    Y.Test.Runner.add(suite);
});

