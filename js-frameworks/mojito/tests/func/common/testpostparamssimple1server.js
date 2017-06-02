/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: postparamssimple1server");

    suite.add(new Y.Test.Case({

        "test postparamssimple1server": function() {
	            Y.Assert.areEqual('Input Parameters Testing', Y.one('#iptitle').get('innerHTML').match(/Input Parameters Testing/gi));
	            Y.Assert.areEqual('POST Parameters', Y.one('#pptitle').get('innerHTML').match(/POST Parameters/gi));
	            Y.Assert.areEqual('Here\'s the POST data!', Y.one('#desc').get('innerHTML').match(/Here\'s the POST data!/gi));
	            Y.Assert.areEqual('Everyone likes ice cream!', Y.one('#ouput').get('innerHTML').match(/Everyone likes ice cream!/gi));
        }

    }));

    Y.Test.Runner.add(suite);
});