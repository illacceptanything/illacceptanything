/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: postparamssimpleserver");

    suite.add(new Y.Test.Case({

        "test postparamssimpleserver": function() {
	        var that = this;
            Y.one('#namesimple').set('value', "Everyone");
	        Y.one('#likessimple > option[value="ice cream"]').set('selected','selected'); 
        }
    }));

    Y.Test.Runner.add(suite);
});