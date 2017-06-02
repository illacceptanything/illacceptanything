/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
    var suite = new Y.Test.Suite("Common: assetcsswithlocationheaderserver");

    suite.add(new Y.Test.Case({

        "test assetcsswithlocationheaderserver": function() {
	        var pat1 = /\/static\/AssetsMojit\/assets\/css\/css1.css/gi;
            var pat2 = /\/static\/Binders\/assets\/css\/css_in_binders.css/gi;
            Y.Assert.areEqual('/static/AssetsMojit/assets/css/css1.css', checkscript(Y.one('head'), 'link', 'href', pat1));
            Y.Assert.areEqual('/static/Binders/assets/css/css_in_binders.css', checkscript(Y.one('head'), 'link', 'href', pat2));
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