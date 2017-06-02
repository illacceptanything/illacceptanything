/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-bz5249892-tests', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: bz5249892");

     suite.add(new Y.Test.Case({
	  "test bz5249892": function(){
          var pagesource= window.document.documentElement.innerHTML;
          var sub = pagesource.match(/http:\/\/yui.yahooapis.com\/combo?/gi);
          Y.log("*******"+sub.length);
          if(sub.length < 3){
              Y.fail("the js src is not broken to parts.");
          }     
      }
  }));

  Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
