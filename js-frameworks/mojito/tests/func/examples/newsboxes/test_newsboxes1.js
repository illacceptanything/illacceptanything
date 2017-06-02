/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('newsbox-tests', function (Y) {
   
    var suite = new Y.Test.Suite("Newsboxes: newsboxes1");

    suite.add(new Y.Test.Case({
         
        "test newsboxes1": function() {
            Y.Assert.areEqual("News", Y.one('h1').get('innerHTML').match(/News/gi));
            Y.Assert.areEqual("Boxes", Y.one('h1').get('innerHTML').match(/Boxes/gi));
            Y.Assert.areEqual(12, Y.all('#toc a').size());
         }
     }));    

     Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
     'node', 'node-event-simulate', 'test'
]});

