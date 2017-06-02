/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: assetjswithdefaultlocationbodyserver");

    suite.add(new Y.Test.Case({

        "test assetjswithdefaultlocationbodyserver": function() {
	        var pat1 = /\/static\/AssetsMojit\/assets\/js\/js1.js/gi;
            var pat2 = /\/static\/AssetsMojit\/assets\/js\/js2.js/gi;
            Y.Assert.areEqual('/static/AssetsMojit/assets/js/js1.js', checkscript(Y.one('body'), 'script', 'src', pat1));
            Y.Assert.areEqual('/static/AssetsMojit/assets/js/js2.js', checkscript(Y.one('body'), 'script', 'src', pat2));
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
