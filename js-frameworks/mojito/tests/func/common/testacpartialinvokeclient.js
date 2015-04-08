/*
 * This is a basic func test for a Common application.
 */
YUI.add('common-testacpartialrenderclient-tests', function (Y) {

    var suite = new Y.Test.Suite("Common: ACPartailRenderClient");

    suite.add(new Y.Test.Case({

        "test ACPartailRenderClient": function() {
            Y.Assert.areEqual('This is mytest1', Y.one('#partialrenderlabel1').get('innerHTML').match(/This is mytest1/gi));
            Y.Assert.areEqual('data from url', Y.one('#thisdata').get('innerHTML').match(/data from url/gi));
        }   

    }));

    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console'
]});

