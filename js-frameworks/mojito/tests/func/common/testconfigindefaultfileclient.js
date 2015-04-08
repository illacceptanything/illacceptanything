/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: configindefaultfileclient");

    suite.add(new Y.Test.Case({

        "test configindefaultfileclient": function() {
            Y.Assert.areEqual('ac.config.get\(\'key1\'\) -', Y.one('#defaultFileValue').get('innerHTML').match(/ac.config.get\(\'key1\'\) -/gi));
            Y.Assert.areEqual('\"This is the value from the default.yaml for key1\"', Y.one('#defaultFileValue').get('innerHTML').match(/\"This is the value from the default.yaml for key1\"/gi));
            Y.Assert.areEqual('ac.config.get\(\'commonKey1\'\) -', Y.one('#commonKeyValue').get('innerHTML').match(/ac.config.get\(\'commonKey1\'\) -/gi));
            Y.Assert.areEqual('\"Value of commonKey1 in application.yaml\"', Y.one('#commonKeyValue').get('innerHTML').match(/\"Value of commonKey1 in application.yaml\"/gi));
            Y.Assert.areEqual('ac.config.get\(\'defaultArray\'\) -', Y.one('#defaultArray').get('innerHTML').match(/ac.config.get\(\'defaultArray\'\) -/gi));
            Y.Assert.areEqual('\[\"defaultArrayValue1\",\"defaultArrayValue2\"\]', Y.one('#defaultArray').get('innerHTML').match(/\[\"defaultArrayValue1\",\"defaultArrayValue2\"\]/gi));
            Y.Assert.areEqual('ac.config.get\(\'nestedConfig.subConfig2\'\) -', Y.one('#defaultNested1').get('innerHTML').match(/ac.config.get\(\'nestedConfig.subConfig2\'\) -/gi));
            Y.Assert.areEqual('\{\"subsubConfig1\":\"SubSubConfig1 from defaults.yaml\",\"subsubConfig2\":\"SubSubConfig2 from defaults.yaml\"\}', Y.one('#defaultNested1').get('innerHTML').match(/\{\"subsubConfig1\":\"SubSubConfig1 from defaults.yaml\",\"subsubConfig2\":\"SubSubConfig2 from defaults.yaml\"\}/gi));          
        }

    }));

    Y.Test.Runner.add(suite);

});