/*
 * This is a basic func test for a Common application.
 */
YUI.add("common-testsimplemodelclient", function(Y) {

    var suite = new Y.Test.Suite("Common: simplemodelclient");

    suite.add(new Y.Test.Case({

        "test simplemodelclient": function() {
            Y.Assert.areEqual('Turkey Viewer 1.0', Y.one('#SimpleModelTitle').get('innerHTML').match(/Turkey Viewer 1.0/gi));
            var imglist = Y.all('img');
            imglist.each(function (taskNode) {
                Y.Assert.areEqual('/static/SimpleModel/assets', taskNode.get('src').match(/\/static\/SimpleModel\/assets/gi));
            });
        }

    }));

    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console'
]});
