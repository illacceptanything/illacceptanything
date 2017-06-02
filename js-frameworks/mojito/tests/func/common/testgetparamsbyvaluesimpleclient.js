/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: getparamsbyvaluesimpleclient");

    suite.add(new Y.Test.Case({
        "test getparamsbyvaluesimpleclient": function() {
            Y.Assert.areEqual('Input Parameters Testing', Y.one('#iptitle').get('innerHTML').match(/Input Parameters Testing/gi));
            Y.Assert.areEqual('GET Parameters', Y.one('#gptitle').get('innerHTML').match(/GET Parameters/gi));
            Y.Assert.areEqual('Params by key', Y.one('#desc').get('innerHTML').match(/Params by key/gi));
            Y.Assert.areEqual('Does the \"foo\" param exist\?', Y.one('#question').get('innerHTML').match(/Does the \"foo\" param exist\?/gi));
            Y.Assert.areEqual('YES', Y.one('#answer').get('innerHTML').match(/YES/gi));
        }

    }));

    Y.Test.Runner.add(suite);
});