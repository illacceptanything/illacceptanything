/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: compositemojitserver");

    suite.add(new Y.Test.Case({

        "test compositemojitserver": function() {
	        var that = this;
            Y.Assert.areEqual('This is the title from controller.js of layout', Y.one('#cmtitle').get('innerHTML').match(/This is the title from controller.js of layout/gi));
	        Y.Assert.areEqual('Info about all the children:', Y.one('#cmtitle3').get('innerHTML').match(/Info about all the children:/gi));

	        Y.Assert.areEqual('nav:', Y.all('#childrenname').item(0).get('innerHTML').match(/nav:/gi));
			Y.Assert.areEqual('news:', Y.all('#childrenname').item(1).get('innerHTML').match(/news:/gi));
			Y.Assert.areEqual('footer:', Y.all('#childrenname').item(2).get('innerHTML').match(/footer:/gi));

            var st1 = '\{\"type\":\"CM_Nav\",\"config\":{\"id\":\"nav\"\}\}';
			var st2 = '\{\"type\":\"CM_News\",\"config\":\{\"id\":\"news\"\}\}';
			var st3 = '\{\"base\":\"CM_Footer\",\"config\":{\"id\":\"footer_id\"}\}';
			
            Y.Assert.areEqual(st1, Y.all('#childrenconfig').item(0).get('innerHTML').match(/\{\"type\":\"CM_Nav\",\"config\":{\"id\":\"nav\"\}\}/gi));
			Y.Assert.areEqual(st2, Y.all('#childrenconfig').item(1).get('innerHTML').match(/\{\"type\":\"CM_News\",\"config\":\{\"id\":\"news\"\}\}/gi));
			Y.Assert.areEqual(st3, Y.all('#childrenconfig').item(2).get('innerHTML').match(/\{\"base\":\"CM_Footer\",\"config\":{\"id\":\"footer_id\"}\}/gi));

        }

   }));

   Y.Test.Runner.add(suite);

});