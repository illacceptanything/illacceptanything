/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: configindefinitionfileclient");

    suite.add(new Y.Test.Case({

        "test configindefinitionfileclient": function() {
            Y.Assert.areEqual('ac.config.getDefinition\(\'def1\'\) -', Y.one('#definitionValue').get('innerHTML').match(/ac.config.getDefinition\(\'def1\'\) -/gi));
            Y.Assert.areEqual('\"This is the definition for def1\"', Y.one('#definitionValue').get('innerHTML').match(/\"This is the definition for def1\"/gi));
            Y.Assert.areEqual('ac.config.getDefinition\(\'defArray1\'\) -', Y.one('#defArrayValue').get('innerHTML').match(/ac.config.getDefinition\(\'defArray1\'\) -/gi));
            Y.Assert.areEqual('\[\"defArray1Value1\"\,\"defArray1Value2\"\,\"defArray1Value3\"\]', Y.one('#defArrayValue').get('innerHTML').match(/\[\"defArray1Value1\"\,\"defArray1Value2\"\,\"defArray1Value3\"\]/gi));
            Y.Assert.areEqual('ac.config.getDefinition\(\'nested.subset2\'\) -', Y.one('#defNested1').get('innerHTML').match(/ac.config.getDefinition\(\'nested.subset2\'\) -/gi));
            Y.Assert.areEqual('\{\"subsubset1\"\:\"inner subset from def\"\,\"subsubsetArray\"\:\[\"innerArrayValue1\"\,\"innerArrayValue2\"\]\}', Y.one('#defNested1').get('innerHTML').match(/\{\"subsubset1\"\:\"inner subset from def\"\,\"subsubsetArray\"\:\[\"innerArrayValue1\"\,\"innerArrayValue2\"\]\}/gi));
            Y.Assert.areEqual('ac.config.getDefinition\(\'nested.subset2.subsubsetArray\'\) -', Y.one('#defNestedArray').get('innerHTML').match(/ac.config.getDefinition\(\'nested.subset2.subsubsetArray\'\) -/gi));
            Y.Assert.areEqual('\"innerArrayValue1\"', Y.one('#defNestedArray').get('innerHTML').match(/\"innerArrayValue1\"/gi));
        }

    }));

    Y.Test.Runner.add(suite);

});