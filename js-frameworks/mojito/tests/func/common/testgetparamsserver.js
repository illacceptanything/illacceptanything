/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: getparamsserver");

    suite.add(new Y.Test.Case({

        "test getparamsserver": function() {
            Y.Assert.areEqual('Input Parameters Testing', Y.one('#iptitle').get('innerHTML').match(/Input Parameters Testing/gi));
            Y.Assert.areEqual('GET Parameters', Y.one('#gptitle').get('innerHTML').match(/GET Parameters/gi));
            Y.Assert.areEqual('All params', Y.one('#desc').get('innerHTML').match(/All params/gi));
            Y.Assert.areEqual('foo ==&gt; 123', Y.all('#gpresult').item(0).get('innerHTML').match(/foo ==&gt; 123/gi));
            Y.Assert.areEqual('bar ==&gt; 2', Y.all('#gpresult').item(1).get('innerHTML').match(/bar ==&gt; 2/gi));
            Y.Assert.areEqual('test ==&gt; no', Y.all('#gpresult').item(2).get('innerHTML').match(/test ==&gt; no/gi));
        }

    }));

    Y.Test.Runner.add(suite);
});