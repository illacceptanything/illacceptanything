/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

    var suite = new Y.Test.Suite("Common: configindefinitionfilenegclient");

    suite.add(new Y.Test.Case({

        "test configindefinitionfilenegclient": function() {
            Y.Assert.areEqual('ac.config.getDefinition\(\'something_unknown\', \'definition not found\'\) -', Y.one('#noMatchDefinition').get('innerHTML').match(/ac.config.getDefinition\(\'something_unknown\', \'definition not found\'\) -/gi));
            Y.Assert.areEqual('\"definition not found\"', Y.one('#noMatchDefinition').get('innerHTML').match(/\"definition not found\"/gi));
            Y.Assert.areEqual('ac.config.getDefinition\(\'something_unknown\'\, \'\[\"I\"\, \"am\"\, \"an\"\, \"array\"\]\'\) -', Y.one('#noMatchDefinitionArray').get('innerHTML').match(/ac.config.getDefinition\(\'something_unknown\'\, \'\[\"I\"\, \"am\"\, \"an\"\, \"array\"\]\'\) -/gi));
            Y.Assert.areEqual('ac.config.getDefinition\(\'something_unknown\', \'\{one\: \{two\: \"I am two\"\, three\: \"I am three\"\}\}\'\) -', Y.one('#noMatchDefinitionJson').get('innerHTML').match(/ac.config.getDefinition\(\'something_unknown\', \'\{one\: \{two\: \"I am two\"\, three\: \"I am three\"\}\}\'\) -/gi));
        }

    }));

    Y.Test.Runner.add(suite);

});