/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-bz5332780-tests', function (Y) {
   
     var suite = new Y.Test.Suite("ServerOnly: bz5332780");

     suite.add(new Y.Test.Case({
	  "test bz5332780": function(){
          var pagesource= window.document.documentElement.innerHTML;
          Y.Assert.isTrue((pagesource.search("\"testingyuiconfig\":\"myyuiconfig\"")!=-1)||(pagesource.search("\"testingyuiconfig\": \"myyuiconfig\"")!=-1));
      }
  }));

  Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
  'node', 'node-event-simulate', 'test'
]});
