/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: configinappfileserver");

    suite.add(new Y.Test.Case({

        "test configinappfileserver": function() {
	        Y.Assert.areEqual('ac.config.get\(\'config1\'\) -', Y.one('#configValue').get('innerHTML').match(/ac.config.get\(\'config1\'\) -/gi));
	        Y.Assert.areEqual('\"This is the config for config1 in application.yaml\"', Y.one('#configValue').get('innerHTML').match(/\"This is the config for config1 in application.yaml\"/gi));

            Y.Assert.areEqual('ac.config.get\(\'configArray1\'\) -', Y.one('#configArrayValue').get('innerHTML').match(/ac.config.get\(\'configArray1\'\) -/gi));
            Y.Assert.areEqual('\[\"configArray1Value1\",\"configArray1Value2\",\"configArray1Value3\"\]', Y.one('#configArrayValue').get('innerHTML').match(/\[\"configArray1Value1\",\"configArray1Value2\",\"configArray1Value3\"\]/gi));

            Y.Assert.areEqual('ac.config.get\(\'config2.config2Key1\'\) -', Y.one('#configNested1').get('innerHTML').match(/ac.config.get\(\'config2.config2Key1\'\) -/gi));
            Y.Assert.areEqual('\"config2Key1 value from application.yaml\"', Y.one('#configNested1').get('innerHTML').match(/\"config2Key1 value from application.yaml\"/gi));

            Y.Assert.areEqual('ac.config.get\(\'config2.config2Key2\'\) -', Y.one('#configNested2').get('innerHTML').match(/ac.config.get\(\'config2.config2Key2\'\) -/gi));
            Y.Assert.areEqual('\{\"config2Key2Key1\":\"It gets complicated here- config2Key2Key1 value in application.yaml\",\"config2Key2Key2\":\"config2Key2Key2 value in application.yaml\"\}', Y.one('#configNested2').get('innerHTML').match(/\{\"config2Key2Key1\":\"It gets complicated here- config2Key2Key1 value in application.yaml\",\"config2Key2Key2\":\"config2Key2Key2 value in application.yaml\"\}/gi));

            Y.Assert.areEqual('ac.config.get\(\'config2.config2Key2.config2Key2Key1\'\) -', Y.one('#configNested3').get('innerHTML').match(/ac.config.get\(\'config2.config2Key2.config2Key2Key1\'\) -/gi));
            Y.Assert.areEqual('\"It gets complicated here- config2Key2Key1 value in application.yaml\"', Y.one('#configNested3').get('innerHTML').match(/\"It gets complicated here- config2Key2Key1 value in application.yaml\"/gi));
           }

    }));

    Y.Test.Runner.add(suite);
});
