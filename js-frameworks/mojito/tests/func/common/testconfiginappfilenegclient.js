/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: configinappfilenegclient");

    suite.add(new Y.Test.Case({

        "test configinappfilenegclient": function() {
            Y.Assert.areEqual('ac.config.get\(\'something_unknown\', \'\[\"I\", \"am\", \"an\", \"array\"\]\'\) -', Y.one('#noMatchConfigArray').get('innerHTML').match(/ac.config.get\(\'something_unknown\', \'\[\"I\", \"am\", \"an\", \"array\"\]\'\) -/gi));
            Y.Assert.areEqual('ac.config.get\(\'something_unknown\', \'Config not found\'\) -', Y.one('#noMatchConfig').get('innerHTML').match(/ac.config.get\(\'something_unknown\', \'Config not found\'\) -/gi));
            Y.Assert.areEqual('\"Config not found\"', Y.one('#noMatchConfig').get('innerHTML').match(/\"Config not found\"/gi));
        }

    }));

    Y.Test.Runner.add(suite);

});