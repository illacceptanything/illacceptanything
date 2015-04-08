/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: configinappfilecompclient");

    suite.add(new Y.Test.Case({

        "test configinappfilecompclient": function() {
            var have = Y.one('#completeConfig').get('innerHTML'),
                matches;
            Y.Assert.areEqual('ac.config.get\(\) -', have.match(/ac.config.get\(\) -/gi));
            matches = have.match(/(\{.+\})/);
            Y.Assert.isNotNull(matches);
            Y.Assert.isTrue(!!matches[0]);
            have = JSON.parse(matches[0]);
            Y.Assert.isObject(have);
            Y.Assert.areEqual('This is the value from the default.yaml for key1', have.key1);
            Y.Assert.areEqual('This is the value from the default.yaml for key2', have.key2);
            Y.Assert.isArray(have.defaultArray);
            Y.Assert.areEqual('defaultArrayValue1', have.defaultArray[0]);
            Y.Assert.areEqual('defaultArrayValue2', have.defaultArray[1]);
            Y.Assert.isObject(have.nestedConfig);
            Y.Assert.areEqual('SubConfig from defaults.yaml', have.nestedConfig.subConfig1);
            Y.Assert.isObject(have.nestedConfig.subConfig2);
            Y.Assert.areEqual('SubSubConfig1 from defaults.yaml', have.nestedConfig.subConfig2.subsubConfig1);
            Y.Assert.areEqual('SubSubConfig2 from defaults.yaml', have.nestedConfig.subConfig2.subsubConfig2);
        }

    }));

    Y.Test.Runner.add(suite);

});
