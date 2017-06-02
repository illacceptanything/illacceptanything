/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: PostParamsServer");

    suite.add(new Y.Test.Case({

        "test PostParamsServer": function() {
	        Y.one('#name').set('value', "Everyone");
            Y.one('#likes > option[value="ice cream"]').set('selected','selected'); 
        }

    }));

    Y.Test.Runner.add(suite);
});