/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
    var suite = new Y.Test.Suite("ServerOnly: setHeaders");

    suite.add(new Y.Test.Case({
        "test setHeaders": function(){
              var that = this;
              var headers = getMyResponseHeader("/httpParent/testAddSetHeaders?header_option=set");
              Y.Assert.areEqual('foo: bar_final', headers.match(/foo: bar_final/gi));
              Y.Assert.areEqual('foo1: bar1_final', headers.match(/foo1: bar1_final/gi));
              Y.Assert.areEqual(null, headers.foo2);
              Y.Assert.areEqual('I am done...Please check for the headers.', Y.one('body').get('innerHTML').match(/I am done...Please check for the headers./gi));
          },
    }));    
    Y.Test.Runner.add(suite);
  
    function getMyResponseHeader(url){
	     var req = new XMLHttpRequest();
         req.open('HEAD', url, false);
         req.send(null);
         var headers = req.getAllResponseHeaders().toLowerCase();
    	 return headers;
    }

});
