/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: configinappfilecompserver");

    suite.add(new Y.Test.Case({

        "test configinappfilecompserver": function() {
            Y.Assert.areEqual('ac.config.get\(\) -', Y.one('#completeConfig').get('innerHTML').match(/ac.config.get\(\) -/gi));
            var matches = Y.one('#completeConfig').get('innerHTML').match(/({.+})/);
            var json;
            try {
                json = JSON.parse(matches[1]);
            } catch(e) {
            }
            Y.Assert.isObject(json);
            Y.Assert.areEqual(8, Y.Object.keys(json).length);
            Y.Assert.areSame('Value of commonKey1 in application.yaml', json.commonKey1);
            Y.Assert.areSame('This is the config for config1 in application.yaml', json.config1);
            Y.Assert.areSame('This is the value from the default.yaml for key1', json.key1);
            Y.Assert.areSame('This is the value from the default.yaml for key2', json.key2);
            Y.Assert.areSame('["configArray1Value1","configArray1Value2","configArray1Value3"]', JSON.stringify(json.configArray1))
            Y.Assert.areSame('["defaultArrayValue1","defaultArrayValue2"]', JSON.stringify(json.defaultArray));
            Y.Assert.areSame('{"config2Key1":"config2Key1 value from application.yaml","config2Key2":{"config2Key2Key1":"It gets complicated here- config2Key2Key1 value in application.yaml","config2Key2Key2":"config2Key2Key2 value in application.yaml"},"config2Key3Array1":["config2Key3Array1Value1","config2Key3Array1Value2","config2Key3Array1Value3"]}', JSON.stringify(json.config2));
            Y.Assert.areSame('{"subConfig1":"SubConfig from defaults.yaml","subConfig2":{"subsubConfig1":"SubSubConfig1 from defaults.yaml","subsubConfig2":"SubSubConfig2 from defaults.yaml"}}', JSON.stringify(json.nestedConfig));
        }

    }));
    Y.Test.Runner.add(suite);
});
