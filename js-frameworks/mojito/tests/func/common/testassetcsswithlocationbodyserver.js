/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: csswithlocationbodyserver");

    suite.add(new Y.Test.Case({

        "test csswithlocationbodyserver": function() {
	        var pat1 = /\/static\/AssetsMojit\/assets\/css\/css_not_present.css/gi;
            Y.Assert.areEqual('/static/AssetsMojit/assets/css/css_not_present.css', checkscript(Y.one('body'), 'link', 'href', pat1));
        }

     }));

     Y.Test.Runner.add(suite);

     function checkscript(mynode, assetLoc, assetTag, assetPat){
         var mystring;
         mynode.all(assetLoc).each(function (taskNode){
             var mysrc = taskNode.get(assetTag).match(assetPat);
             if(mysrc!=null){ mystring=mysrc; }   
         });
         return mystring;
     }
});