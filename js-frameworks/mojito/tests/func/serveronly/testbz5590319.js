/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-bz5590319-tests', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: bz5590319");

     suite.add(new Y.Test.Case({
	  "test bz5590319": function(){
          var pagesource= window.document.documentElement.innerHTML;
          Y.Assert.areNotEqual('-1', pagesource.search("testing xss"));
      }
  }));

  Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
  'node', 'node-event-simulate', 'test'
]});