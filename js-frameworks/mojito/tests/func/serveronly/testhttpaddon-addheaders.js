/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: addHeaders");

     suite.add(new Y.Test.Case({

         "test addHeaders": function(){
               var headers = getMyResponseHeader("/httpParent/testAddSetHeaders");
               Y.Assert.areEqual('foo: bar, bar_one_more', headers.match(/foo: bar, bar_one_more/gi));
               Y.Assert.areEqual('foo1: bar1, bar1_one_more', headers.match(/foo1: bar1, bar1_one_more/gi));
               Y.Assert.areEqual('foo2: bar2', headers.match(/foo2: bar2/gi));
               Y.Assert.areEqual('I am done...Please check for the headers.', Y.one('body').get('innerHTML').match(/I am done...Please check for the headers./gi));
          }
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
