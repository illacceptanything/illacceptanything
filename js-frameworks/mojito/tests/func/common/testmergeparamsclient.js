/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: mergeparamsclient");

    suite.add(new Y.Test.Case({

        "test mergeparamsclient": function() {
            Y.Assert.areEqual('Merge Parameters Test', Y.one('#mpttitle').get('innerHTML').match(/Merge Parameters Test/gi));
            Y.Assert.areEqual('MERGED Parameters', Y.one('#mptitle').get('innerHTML').match(/MERGED Parameters/gi));
            Y.Assert.areEqual('Merged Parameters Test', Y.one('#desc').get('innerHTML').match(/Merged Parameters Test/gi));
            Y.Assert.areEqual('Hillary Clinton likes Beer\?', Y.one('#mpresult').get('innerHTML').match(/Hillary Clinton likes Beer\?/gi));
            Y.Assert.areEqual('Hey, is that what you entered in the form\? If not, here\'s why...', Y.one('#question').get('innerHTML').match(/Hey, is that what you entered in the form\? If not, here\'s why.../gi));
            Y.Assert.areEqual('name ==&gt; Hillary Clinton', Y.all('#keyandvalue').item(0).get('innerHTML').match(/name ==&gt; Hillary Clinton/gi));
            Y.Assert.areEqual('likes ==&gt; Beer', Y.all('#keyandvalue').item(1).get('innerHTML').match(/likes ==&gt; Beer/gi));
            Y.Assert.areEqual('name ==&gt; Hillary Clinton', Y.all('#keyandvalue').item(2).get('innerHTML').match(/name ==&gt; Hillary Clinton/gi));
            Y.Assert.areEqual('name ==&gt; Everyone', Y.all('#keyandvalue').item(3).get('innerHTML').match(/name ==&gt; Everyone/gi));  
            Y.Assert.areEqual('likes ==&gt; ice cream', Y.all('#keyandvalue').item(4).get('innerHTML').match(/likes ==&gt; ice cream/gi));
            Y.Assert.areEqual('likes ==&gt; Beer', Y.all('#keyandvalue').item(5).get('innerHTML').match(/likes ==&gt; Beer/gi));
            Y.Assert.areEqual('route ==&gt; likes ==&gt; Beer', Y.all('#allparams').item(0).get('innerHTML').match(/route ==&gt; likes ==&gt; Beer/gi));
            Y.Assert.areEqual('url ==&gt; name ==&gt; Hillary Clinton', Y.all('#allparams').item(1).get('innerHTML').match(/url ==&gt; name ==&gt; Hillary Clinton/gi));  
            Y.Assert.areEqual('body ==&gt; name ==&gt; Everyone', Y.all('#allparams').item(2).get('innerHTML').match(/body ==&gt; name ==&gt; Everyone/gi));
            Y.Assert.areEqual('body ==&gt; likes ==&gt; ice cream', Y.all('#allparams').item(3).get('innerHTML').match(/body ==&gt; likes ==&gt; ice cream/gi));        
        }
    }));

    Y.Test.Runner.add(suite);
});