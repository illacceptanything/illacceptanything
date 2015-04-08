/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-bz366776-tests', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: bz5366776");

     suite.add(new Y.Test.Case({
	  "test bz5366776": function(){
          var pagesource= window.document.documentElement.innerHTML;
          Y.Assert.areNotEqual('-1', pagesource.search(".css"));
      }
  }));

  Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
  'node', 'node-event-simulate', 'test'
]});
